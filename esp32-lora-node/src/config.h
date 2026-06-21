#include <WiFi.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#ifndef _RADIOLIB_EX_LORAWAN_CONFIG_H
#define _RADIOLIB_EX_LORAWAN_CONFIG_H

uint16_t version = 2;

#define LED_PIN 21
const int ONE_WIRE_BUS = D0;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

Preferences prefs;
int failures;

void triggerWiFiUpdate(const char* ssid, const char* pass, const char* url);
void logPrint(const String &message);
void saveSession();
bool loadSession();
void setLED(bool state);
void processDownlink(uint8_t *data, size_t len, uint8_t port);
void createPayload(uint8_t *payload, float temp, float batt);
void getSensorData(float &temp, float &batt);
bool performNetworkJoin();
void setupLoRaWAN();
void setupHardware();

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
const uint32_t uplinkIntervalSeconds = 6UL;    // minutes x seconds

// joinEUI - previous versions of LoRaWAN called this AppEUI
// for development purposes you can use all zeros - see wiki for details
#define RADIOLIB_LORAWAN_JOIN_EUI  0x0000000000000000ULL;

// the Device EUI & two keys can be generated on the TTN console 
#ifndef RADIOLIB_LORAWAN_DEV_EUI   // Replace with your Device EUI
#define RADIOLIB_LORAWAN_DEV_EUI   0x1096630FC0013D99ULL;
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

void getSensorData(float &temp, float &batt) {
  sensors.requestTemperatures();
  delay(750);
  temp = sensors.getTempFByIndex(0);
  batt = 9.99; // Replace with actual battery logic
  
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

void processDownlink(uint8_t *data, size_t len, uint8_t port) {
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
  randomSeed(analogRead(0));
  Serial.begin(115200);
  sensors.begin();
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  delay(10000); // Allow time for Serial monitor to connect, remove for prod it is just to allow upload new firmware.
  logPrint("--- XIAO ESP32-S3 Node Awake ---");
}

void setupLoRaWAN() {
  ConfigLoRa_t config;
  config.frequency = 868;
  int state = radio.begin(config);
  debug(state != RADIOLIB_ERR_NONE, F("Initialise radio failed"), state, true);

  state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  debug(state != RADIOLIB_ERR_NONE, F("Initialise node failed"), state, true);
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

#endif