#include <RadioLib.h>
#include <WiFi.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define LED_PIN 21

const int ONE_WIRE_BUS = D0;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ------------------------------------------------------------------
// CHIRPSTACK CREDENTIALS
// ------------------------------------------------------------------
uint64_t devEUI  = 0x1096630FC0013D99ULL;
uint64_t joinEUI = 0x0000000000000000ULL;

uint8_t appKey[] = {
  0xF8, 0x66, 0x2F, 0x22, 0x9E, 0x6F, 0x38, 0x3C,
  0x2C, 0x61, 0x70, 0xCE, 0x11, 0x95, 0x8E, 0x0B
};

SX1262 radio = new Module(41, 39, 42, 40);
LoRaWANNode node(&radio, &US915, 2);
Preferences prefs;

void triggerWiFiUpdate(const char* ssid, const char* pass, const char* url);
void logPrint(const String &message);
void saveSession();
bool loadSession();
void setLED(bool state);

void setup() {
  Serial.begin(115200);
  sensors.begin();
  randomSeed(analogRead(0));
  //while (!Serial) { delay(10); }
  //delay(1000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  logPrint("--- XIAO ESP32-S3 Node Awake ---");

  // Init radio
  SPI.begin(7, 8, 9, 41);
  //SPI.begin();
  int state = radio.begin();
  if (state != RADIOLIB_ERR_NONE) {
    logPrint("Radio init failed: " + String(state));
    while (true) { delay(1000); }
  }

  // TCXO
  radio.setDio2AsRfSwitch(true);
  //radio.setRfSwitchPins(38, RADIOLIB_NC);
  //success at 1.7,2.2,
  state = radio.setTCXO(1.7);
  
  node.setADR(true);
  //node.setDatarate(2);
  //node.setTxPower(22);
  //radio.setOutputPower(-9);

  delay(500);
  if (state != RADIOLIB_ERR_NONE) {
    logPrint("TCXO failed: " + String(state));
    while (true) { delay(1000); }
  }
  
  // Stage keys
  node.beginOTAA(joinEUI, devEUI, appKey, appKey);

  // Try to restore saved session from NVS
  //if (loadSession()) {
  //  logPrint("Session restored from NVS, skipping join...");
  //  return;
  //}

  // Fresh join
  logPrint("No saved session, joining...");
  unsigned long joinStart = millis();
  state = RADIOLIB_ERR_UNKNOWN;
  for (int attempt = 1; attempt <= 100; attempt++) {
    logPrint("Join attempt " + String(attempt) + "...");
    state = node.activateOTAA();
    logPrint("Result: " + String(state));
    if (state == RADIOLIB_LORAWAN_NEW_SESSION) break;
  }
  unsigned long joinDuration = millis() - joinStart;

  if (state == RADIOLIB_LORAWAN_NEW_SESSION) {
    logPrint("Join SUCCESSFUL!");
    saveSession();
  } else {
    logPrint("Join FAILED");
    while (true) { delay(1000); }
  }

}

void loop() {
  sensors.requestTemperatures();
  delay(750);
  float currentTemp = sensors.getTempFByIndex(0);
  Serial.print("Temperature: ");
  Serial.print(currentTemp);
  Serial.println(" °F");
  float currentBatt = 9.99;
  uint16_t version = 2; // Your version number

  int16_t  tempInt = (int16_t)(currentTemp * 100);
  uint16_t battInt = (uint16_t)(currentBatt * 100);

  // 1. Increase array size to 6 bytes
  uint8_t payload[6]; 
  
  // 2. Pack the data (Big Endian)
  payload[0] = (tempInt >> 8) & 0xFF; // Temp High
  payload[1] = tempInt & 0xFF;        // Temp Low
  payload[2] = (battInt >> 8) & 0xFF; // Batt High
  payload[3] = battInt & 0xFF;        // Batt Low
  payload[4] = (version >> 8) & 0xFF; // Version High
  payload[5] = version & 0xFF;        // Version Low

  uint8_t downlinkData[256];
  size_t downlinkLen = 0;

  logPrint("[LoRaWAN] Sending uplink...");
  setLED(true);
  unsigned long startTime = millis();
  LoRaWANEvent_t eventDown;
  
  // 3. IMPORTANT: Change the length from 4 to 6 here!
  //logPrint("TX Power = " + String(radio.getOutputPower()));
  int state = node.sendReceive(payload, 6, 1, downlinkData, &downlinkLen, true, NULL, &eventDown);
  setLED(false);
  logPrint("Uplink state: " + String(state));
  unsigned long duration = millis() - startTime;

  if (state == RADIOLIB_ERR_NONE) {
    logPrint("Network ACK received! Delivery confirmed.");
    saveSession(); // persist updated frame counters

    if (downlinkLen > 0) {
      // 1. Get the fPort of the received downlink
      uint8_t receivedPort = eventDown.fPort;
      logPrint("Downlink received on fPort: " + String(receivedPort));

      // 2. Check if it's the port we care about (fPort 10)
      if (receivedPort == 10) {
          logPrint("Firmware update command detected on fPort 10!");
          StaticJsonDocument<256> doc;
          if (!deserializeJson(doc, downlinkData, downlinkLen)) {
              const char* ssid = doc["s"];
              const char* pass = doc["p"];
              const char* url  = doc["u"];
              if (ssid && pass && url) {
                  triggerWiFiUpdate(ssid, pass, url);
              }
          }
      } else {
          logPrint("Ignoring downlink on unexpected fPort.");
      }
    } else {
      logPrint("No downlink received.");
    }
  } else if (state == RADIOLIB_LORAWAN_NO_DOWNLINK) { 
    logPrint("Transmitted but NO ACK received - network did not confirm delivery.");
  } else {
    logPrint("Uplink failed, code " + String(state) + " after " + String(duration) + " ms");
    // If session is broken, clear NVS and reboot to rejoin
    if (state == RADIOLIB_ERR_NETWORK_NOT_JOINED) {
      logPrint("Session invalid, clearing NVS and rebooting...");
      prefs.begin("lorawan", false);
      prefs.clear();
      prefs.end();
      ESP.restart();
    }
  }

  logPrint("Sleeping 5 seconds...");
  Serial.flush();
  delay(5000);

  //esp_sleep_enable_timer_wakeup(15 * 1000000); // Sleep for 15 seconds (5 mins)
  //esp_deep_sleep_start();
}

void saveSession() {
  uint8_t* nonces  = node.getBufferNonces();
  uint8_t* session = node.getBufferSession();
  prefs.begin("lorawan", false);
  prefs.putBytes("nonces",  nonces,  RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
  prefs.putBytes("session", session, RADIOLIB_LORAWAN_SESSION_BUF_SIZE);
  prefs.end();
  logPrint("Session saved to NVS.");
}

bool loadSession() {
  uint8_t nonces[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
  uint8_t session[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];
  prefs.begin("lorawan", true);
  size_t n = prefs.getBytes("nonces",  nonces,  sizeof(nonces));
  size_t s = prefs.getBytes("session", session, sizeof(session));
  prefs.end();

  if (n == sizeof(nonces) && s == sizeof(session)) {
    node.setBufferNonces(nonces);
    node.setBufferSession(session);
    int state = node.activateOTAA();
    if (state == RADIOLIB_LORAWAN_SESSION_RESTORED) {
      return true;
    }
  }
  return false;
}

void logPrint(const String &message) {
  Serial.print("[");
  Serial.print(millis());
  Serial.print(" ms] ");
  Serial.println(message);
}

void triggerWiFiUpdate(const char* ssid, const char* pass, const char* url) {
  logPrint("Connecting to WiFi for OTA...");
  WiFi.begin(ssid, pass);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    logPrint("WiFi Connected. Starting OTA...");
    WiFiClient client;
    t_httpUpdate_return ret = httpUpdate.update(client, String(url));
    if (ret == HTTP_UPDATE_FAILED) {
      Serial.printf("OTA failed: %s\n", httpUpdate.getLastErrorString().c_str());
    }
  } else {
    logPrint("WiFi failed.");
  }
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void setLED(bool state) {
  // XIAO ESP32-S3 LEDs are usually active-low (LOW = ON, HIGH = OFF)
  // Check your specific board, but this is standard for the XIAO S3:
  digitalWrite(LED_PIN, state ? LOW : HIGH);
}
