#include <WiFi.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#ifndef _RADIOLIB_EX_LORAWAN_CONFIG_H
  #define _RADIOLIB_EX_LORAWAN_CONFIG_H
#endif
#ifndef LORA_MISO
  #define LORA_MISO 8
#endif

#ifndef LORA_SCK
  #define LORA_SCK 7
#endif

#ifndef LORA_MOSI
  #define LORA_MOSI 9
#endif

#ifndef LORA_CS
  #define LORA_CS 41
#endif

#ifndef LORA_DIO1
  #define LORA_DIO1 39
#endif

#ifndef LORA_DIO2
  #define LORA_DIO2 38
#endif

#ifndef LORA_BUSY
  #define LORA_BUSY 40
#endif

#ifndef LORA_RESET
  #define LORA_RESET 42
#endif

//make sure this matches what is in frontend
uint16_t version = 3;

//#define LED_PIN 21
const int ONE_WIRE_BUS = D0;
//THESE TWO LINES MUST BE CHANGED IF THE NODE IS MOVED TO A DIFFERENT YARD OR COMPANY. IF NOT UPDATES WON'T WORK
const String COMPANY_ID = "CV_AG_GRIND";
const String YARD_ID = "Oakdale_1";

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

Preferences prefs;
int failures;

void triggerWiFiUpdate(const char* ssid, const char* pass, const char* url);
void logPrint(const String &message);
void saveSession();
bool loadSession();
void setLED(bool state);
void resetBatteryState();
void processDownlink(uint8_t *data, size_t len, uint8_t port);
void createPayload(uint8_t *payload, float temp, float batt);
void getSensorData(float &temp);
bool performNetworkJoin();
void setupLoRaWAN();
void setupHardware();
void transmission();
void goToSleep(uint64_t seconds);
float updateAndGetBatteryPercentage();

#include <RadioLib.h>

// first you have to set your radio model and pin configuration
// this is provided just as a default example
//SX1278 radio = new Module(41, 39, 42, 40);

// if you have RadioBoards (https://github.com/radiolib-org/RadioBoards)
// and are using one of the supported boards, you can do the following:

#define RADIO_BOARD_AUTO
#include <RadioBoards.h>

Radio radio = new RadioModule();

// how often to send an uplink - consider legal & FUP constraints - see notes
//const uint32_t uplinkIntervalSeconds = 5UL * 60UL;    // minutes x seconds
const uint32_t uplinkIntervalSeconds = 60UL * 60UL * 8UL;    // minutes x seconds x hours

// joinEUI - previous versions of LoRaWAN called this AppEUI
// for development purposes you can use all zeros - see wiki for details
#define RADIOLIB_LORAWAN_JOIN_EUI  0x0000000000000000ULL;

// the Device EUI & two keys can be generated on the TTN console 
#ifndef RADIOLIB_LORAWAN_DEV_EUI   // Replace with your Device EUI
#define RADIOLIB_LORAWAN_DEV_EUI   0x1096630fc0013d99ULL;
#endif
#ifndef RADIOLIB_LORAWAN_APP_KEY   // Replace with your App Key 
#define RADIOLIB_LORAWAN_APP_KEY   0xF8, 0x66, 0x2F, 0x22, 0x9E, 0x6F, 0x38, 0x3C, 0x2C, 0x61, 0x70, 0xCE, 0x11, 0x95, 0x8E, 0x0B
#endif
#ifndef RADIOLIB_LORAWAN_NWK_KEY   // Put your Nwk Key here
#define RADIOLIB_LORAWAN_NWK_KEY   0xF8, 0x66, 0x2F, 0x22, 0x9E, 0x6F, 0x38, 0x3C, 0x2C, 0x61, 0x70, 0xCE, 0x11, 0x95, 0x8E, 0x0B
#endif

// for the curious, the #ifndef blocks allow for automated testing &/or you can
// put your EUI & keys in to your platformio.ini - see wiki for more tips

// regional choices: EU868, US915, AU915, AS923, AS923_2, AS923_3, AS923_4, IN865, KR920, CN470
const LoRaWANBand_t Region = US915;

// subband choice: for US915/AU915 set to 2, for CN470 set to 1, otherwise leave on 0
const uint8_t subBand = 2;

// ============================================================================
// Below is to support the sketch - only make changes if the notes say so ...

// copy over the EUI's & keys in to the something that will not compile if incorrectly formatted
uint64_t joinEUI =   RADIOLIB_LORAWAN_JOIN_EUI;
uint64_t devEUI  =   RADIOLIB_LORAWAN_DEV_EUI;
uint8_t appKey[] = { RADIOLIB_LORAWAN_APP_KEY };
uint8_t nwkKey[] = { RADIOLIB_LORAWAN_NWK_KEY };

// create the LoRaWAN node
LoRaWANNode node(&radio, &Region, subBand);

// result code to text - these are error codes that can be raised when using LoRaWAN
// however, RadioLib has many more - see https://jgromes.github.io/RadioLib/group__status__codes.html for a complete list
String stateDecode(const int16_t result) {
  switch (result) {
  case RADIOLIB_ERR_NONE:
    return "ERR_NONE";
  case RADIOLIB_ERR_CHIP_NOT_FOUND:
    return "ERR_CHIP_NOT_FOUND";
  case RADIOLIB_ERR_PACKET_TOO_LONG:
    return "ERR_PACKET_TOO_LONG";
  case RADIOLIB_ERR_RX_TIMEOUT:
    return "ERR_RX_TIMEOUT";
  case RADIOLIB_ERR_MIC_MISMATCH:
    return "ERR_MIC_MISMATCH";
  case RADIOLIB_ERR_INVALID_BANDWIDTH:
    return "ERR_INVALID_BANDWIDTH";
  case RADIOLIB_ERR_INVALID_SPREADING_FACTOR:
    return "ERR_INVALID_SPREADING_FACTOR";
  case RADIOLIB_ERR_INVALID_CODING_RATE:
    return "ERR_INVALID_CODING_RATE";
  case RADIOLIB_ERR_INVALID_FREQUENCY:
    return "ERR_INVALID_FREQUENCY";
  case RADIOLIB_ERR_INVALID_OUTPUT_POWER:
    return "ERR_INVALID_OUTPUT_POWER";
  case RADIOLIB_ERR_NETWORK_NOT_JOINED:
	  return "RADIOLIB_ERR_NETWORK_NOT_JOINED";
  case RADIOLIB_ERR_DOWNLINK_MALFORMED:
    return "RADIOLIB_ERR_DOWNLINK_MALFORMED";
  case RADIOLIB_ERR_INVALID_REVISION:
    return "RADIOLIB_ERR_INVALID_REVISION";
  case RADIOLIB_ERR_INVALID_PORT:
    return "RADIOLIB_ERR_INVALID_PORT";
  case RADIOLIB_ERR_NO_RX_WINDOW:
    return "RADIOLIB_ERR_NO_RX_WINDOW";
  case RADIOLIB_ERR_INVALID_CID:
    return "RADIOLIB_ERR_INVALID_CID";
  case RADIOLIB_ERR_UPLINK_UNAVAILABLE:
    return "RADIOLIB_ERR_UPLINK_UNAVAILABLE";
  case RADIOLIB_ERR_COMMAND_QUEUE_FULL:
    return "RADIOLIB_ERR_COMMAND_QUEUE_FULL";
  case RADIOLIB_ERR_COMMAND_QUEUE_ITEM_NOT_FOUND:
    return "RADIOLIB_ERR_COMMAND_QUEUE_ITEM_NOT_FOUND";
  case RADIOLIB_ERR_JOIN_NONCE_INVALID:
    return "RADIOLIB_ERR_JOIN_NONCE_INVALID";
  case RADIOLIB_ERR_DWELL_TIME_EXCEEDED:
    return "RADIOLIB_ERR_DWELL_TIME_EXCEEDED";
  case RADIOLIB_ERR_CHECKSUM_MISMATCH:
    return "RADIOLIB_ERR_CHECKSUM_MISMATCH";
  case RADIOLIB_ERR_NO_JOIN_ACCEPT:
    return "RADIOLIB_ERR_NO_JOIN_ACCEPT";
  case RADIOLIB_LORAWAN_SESSION_RESTORED:
    return "RADIOLIB_LORAWAN_SESSION_RESTORED";
  case RADIOLIB_LORAWAN_NEW_SESSION:
    return "RADIOLIB_LORAWAN_NEW_SESSION";
  case RADIOLIB_ERR_NONCES_DISCARDED:
    return "RADIOLIB_ERR_NONCES_DISCARDED";
  case RADIOLIB_ERR_SESSION_DISCARDED:
    return "RADIOLIB_ERR_SESSION_DISCARDED";
  }
  return "See https://jgromes.github.io/RadioLib/group__status__codes.html";
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
  char euiStr[17];
  sprintf(euiStr, "%016llX", devEUI);
  String euiString = String(euiStr);
  euiString.toLowerCase();
  String fullUrl = String(url) + COMPANY_ID + ":" + YARD_ID + ":" + euiString;
  logPrint(fullUrl);
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
    t_httpUpdate_return ret = httpUpdate.update(client, fullUrl);
    if (ret == HTTP_UPDATE_FAILED) {
      Serial.printf("OTA failed: %s\n", httpUpdate.getLastErrorString().c_str());
    }
  } else {
    logPrint("WiFi failed.");
  }
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

//void setLED(bool state) {
//  // XIAO ESP32-S3 LEDs are usually active-low (LOW = ON, HIGH = OFF)
//  // Check your specific board, but this is standard for the XIAO S3:
//  digitalWrite(LED_PIN, state ? LOW : HIGH);
//}

// helper function to display any issues
void debug(bool failed, const __FlashStringHelper* message, int state, bool halt) {
  if(failed) {
    Serial.print(message);
    Serial.print(" - ");
    Serial.print(stateDecode(state));
    Serial.print(" (");
    Serial.print(state);
    Serial.println(")");
    while(halt) { delay(1); }
  }
}

// helper function to display a byte array
void arrayDump(uint8_t *buffer, uint16_t len) {
  for(uint16_t c = 0; c < len; c++) {
    char b = buffer[c];
    if(b < 0x10) { Serial.print('0'); }
    Serial.print(b, HEX);
  }
  Serial.println();
}

void getSensorData(float &temp) {
  sensors.requestTemperatures();
  delay(750);
  temp = sensors.getTempFByIndex(0);
  
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.println(" °F");
}

void createPayload(uint8_t *payload, float temp, float batt) {
  int16_t tempInt = (int16_t)(temp * 100);
  uint16_t battInt = (uint16_t)(batt * 100);

  payload[0] = (tempInt >> 8) & 0xFF;
  payload[1] = tempInt & 0xFF;
  payload[2] = (battInt >> 8) & 0xFF;
  payload[3] = battInt & 0xFF;
  payload[4] = (version >> 8) & 0xFF;
  payload[5] = version & 0xFF;
}

void resetBatteryState() {
  logPrint("Resetting battery state...");
  prefs.begin("power", false);
  prefs.putFloat("curCap", 5400.0);
  prefs.end();
}

void processDownlink(uint8_t *data, size_t len, uint8_t port) {
  if (port == 9) {
    resetBatteryState();
    return;
  }

  if (port != 10) return;

  logPrint("Processing FPort 10 Command...");
  
  // Handle JSON
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, data, len);
  if (!err) {
    const char* ssid = doc["s"];
    const char* pass = doc["p"];
    const char* url  = doc["u"];
    if (ssid && pass && url) triggerWiFiUpdate(ssid, pass, url);
    else logPrint("JSON missing fields.");
  } else {
    logPrint("Invalid JSON: " + String(err.c_str()));
  }
}

void setupHardware() {
  Serial.begin(115200);
  //while(!Serial) { delay(10); } //remove in production

  sensors.begin();
  //don't use in production
  //pinMode(LED_PIN, OUTPUT);
  //digitalWrite(LED_PIN, HIGH);
  logPrint("--- XIAO ESP32-S3 Node Awake ---");
}

void setupLoRaWAN() {
  ConfigLoRa_t config;
  config.frequency = 868;
  int state = radio.begin(config);
  if (state != RADIOLIB_ERR_NONE) {
      logPrint("Radio failed to init, sleeping and trying again...");
      goToSleep(uplinkIntervalSeconds);
  }

  state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  if (state != RADIOLIB_ERR_NONE) {
      logPrint("Radio failed to init, sleeping and trying again...");
      goToSleep(uplinkIntervalSeconds);
  }
}

bool performNetworkJoin() {
  if (loadSession()) {
    logPrint("Session restored from NVS, skipping join...");
    return true;
  }

  logPrint("No saved session, joining...");
  for (int attempt = 1; attempt <= 50; attempt++) {
    logPrint("Join attempt " + String(attempt) + "...");
    int state = node.activateOTAA();
    if (state == RADIOLIB_LORAWAN_NEW_SESSION) {
      logPrint("Join SUCCESSFUL!");
      saveSession();
      return true;
    }
  }
  return false;
}

void transmission(float currentBatt) {
  float currentTemp;
  uint8_t payload[6];
  
  getSensorData(currentTemp);
  createPayload(payload, currentTemp, currentBatt);

  uint8_t downlinkData[256];
  size_t downlinkLen = 0;
  LoRaWANEvent_t eventDown;

  
  int16_t state = node.sendReceive(payload, sizeof(payload), 1, downlinkData, &downlinkLen, true, NULL, &eventDown);

  if (state == 0) {
    // TX success but no ACK — confirmed uplink failure
    saveSession();
    prefs.begin("lorawan", false);
    prefs.putInt("failures", failures + 1);
    prefs.end();
    logPrint("No ACK. Failure count: " + String(failures + 1) + "/6");
  } else if (state > 0) {
      // Downlink received in RX1 or RX2 — ACK confirmed
      saveSession();
      prefs.begin("lorawan", false);
      prefs.putInt("failures", 0);
      prefs.end();
      logPrint("ACK received in window " + String(state));
      if (downlinkLen > 0) {
          processDownlink(downlinkData, downlinkLen, eventDown.fPort);
      }
  } else if (state == RADIOLIB_ERR_NETWORK_NOT_JOINED 
        || state == RADIOLIB_ERR_SESSION_DISCARDED
        || state == RADIOLIB_ERR_NONCES_DISCARDED) {
      logPrint("Session invalid, rejoining...");
      prefs.begin("lorawan", false);
      prefs.clear();
      prefs.end();
      ESP.restart();
  } else {
      // Radio/hardware error - don't penalise
      logPrint("Radio error: " + stateDecode(state));
  }
}

void goToSleep(uint64_t seconds) {
  logPrint("Preparing for deep sleep...");

  radio.sleep(true);  // true = warm start, retains config

  pinMode(LORA_MISO,  INPUT);
  pinMode(LORA_MOSI,  INPUT);
  pinMode(LORA_SCK,   INPUT);
  pinMode(LORA_CS,    INPUT);
  pinMode(LORA_DIO1,  INPUT);
  pinMode(LORA_DIO2,  INPUT);
  pinMode(LORA_BUSY,  INPUT);
  pinMode(LORA_RESET, INPUT);
  pinMode(ONE_WIRE_BUS, INPUT);

  logPrint("Sleeping for " + String(seconds) + "s");
  esp_sleep_enable_timer_wakeup(seconds * 1000000ULL);
  esp_deep_sleep_start();
}

float calculateBatteryPercentage() {
    const float INITIAL_CAPACITY = 5400.0;
    
    // Per-transmission constants derived from your requirements:
    const float TX_COST = 0.120;             // Cost per transmission
    const float DRAIN_PER_TX = 1.6;          // 4.8 / 3 transmissions = 1.6 per TX
    const float SELF_DISCHARGE_PER_TX = 0.000166; // 0.015 / 90 transmissions

    prefs.begin("power", false);
    
    // Get the current stored capacity
    float curCap = prefs.getFloat("curCap", INITIAL_CAPACITY);
    
    // 1. Subtract the base drain for this 8-hour window
    curCap -= DRAIN_PER_TX;
    
    // 2. Subtract the cost of this transmission
    curCap -= TX_COST;
    
    // 3. Subtract the fractional self-discharge
    curCap -= (curCap * SELF_DISCHARGE_PER_TX);
    
    // 4. Bounds checking
    if (curCap < 0) curCap = 0;
    if (curCap > INITIAL_CAPACITY) curCap = INITIAL_CAPACITY;
    
    // 5. Persist the new capacity
    prefs.putFloat("curCap", curCap);
    prefs.end();

    return (curCap / INITIAL_CAPACITY) * 100.0;
}
