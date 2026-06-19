#include "config.h"

void setup() {
  Serial.begin(115200);
  sensors.begin();
  randomSeed(analogRead(0));
  delay(5000);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  logPrint("--- XIAO ESP32-S3 Node Awake ---");

  ConfigLoRa_t config;
  config.frequency = 868;
  int state = radio.begin(config);
  debug(state != RADIOLIB_ERR_NONE, F("Initialise radio failed"), state, true);

  state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  debug(state != RADIOLIB_ERR_NONE, F("Initialise node failed"), state, true);

  logPrint("Join ('login') the LoRaWAN Network");

  // Try to restore saved session from NVS
  if (loadSession()) {
    logPrint("Session restored from NVS, skipping join...");
    return;
  }

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

  int16_t  tempInt = (int16_t)(currentTemp * 100);
  uint16_t battInt = (uint16_t)(currentBatt * 100);

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
  
  int16_t state = node.sendReceive(payload, sizeof(payload), 1, downlinkData, &downlinkLen, false, NULL, &eventDown);
  debug(state < RADIOLIB_ERR_NONE, F("Error in sendReceive"), state, false);
  setLED(false);

  if (state >= 0) {
    logPrint("Uplink complete (window " + String(state) + ")");
    saveSession();

    if (state > 0) {
      uint8_t receivedPort = eventDown.fPort;
      if (downlinkLen == 0) {
          logPrint("Received MAC-only downlink (ACK and/or MAC commands).");
      } else {
          logPrint("Downlink received on fPort: " + String(receivedPort) + ", length: " + String(downlinkLen));
      }
      
      Serial.print("Downlink payload: ");
      for (size_t i = 0; i < downlinkLen; i++) {
        if (i > 0) {
          Serial.print(' ');
        }
        if (downlinkData[i] < 0x10) {
          Serial.print('0');
        }
        Serial.print(downlinkData[i], HEX);
      }
      Serial.println();

      if (receivedPort == 10) {


        logPrint("Raw downlink hex:");

        for (size_t i = 0; i < downlinkLen; i++) {
          Serial.printf("%02X ", downlinkData[i]);
        }
        Serial.println();


        logPrint("Raw ASCII (safe):");

        for (size_t i = 0; i < downlinkLen; i++) {
          char c = (downlinkData[i] >= 32 && downlinkData[i] <= 126)
                  ? downlinkData[i]
                  : '.';
          Serial.print(c);
        }
        Serial.println();


        StaticJsonDocument<256> doc;
        DeserializationError err = deserializeJson(doc, downlinkData, downlinkLen);
        if (!err) {
          logPrint("Valid JSON firmware command received!");
          const char* ssid = doc["s"];
          const char* pass = doc["p"];
          const char* url  = doc["u"];
          if (ssid && pass && url) {
            triggerWiFiUpdate(ssid, pass, url);
          } else {
            logPrint("Downlink JSON missing required fields.");
          }
        } else {
          logPrint("Downlink payload is not valid JSON: " + String(err.c_str()));
        }
      }
    } else {
      logPrint("No downlink payload received.");
    }
  } else {
    logPrint("Uplink failed, code " + String(state) + " after " + String(millis() - startTime) + " ms");
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
  delay(uplinkIntervalSeconds * 1000UL);

  //esp_sleep_enable_timer_wakeup(15 * 1000000); // Sleep for 15 seconds (5 mins)
  //esp_deep_sleep_start();
}
