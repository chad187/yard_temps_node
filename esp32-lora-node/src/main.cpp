#include "config.h"

void setup() {
  setupHardware();
  setupLoRaWAN();
  prefs.begin("lorawan", false);
  failures = prefs.getInt("failures", 0);
  prefs.end();

  if (failures >= 6) {
    logPrint("No confirmation in 24hrs, forcing rejoin...");
    prefs.begin("lorawan", false);
    prefs.clear();
    prefs.end();
    ESP.restart();
  }

  if (!performNetworkJoin()) {
    logPrint("Join FAILED");
    esp_sleep_enable_timer_wakeup(6 * 1000000); // Sleep for 6 seconds
    esp_deep_sleep_start();
  }


  //old loop data start
  //float currentTemp, currentBatt;
  //uint8_t payload[6];
  
  //getSensorData(currentTemp, currentBatt);
  //createPayload(payload, currentTemp, currentBatt);

  //uint8_t downlinkData[256];
  //size_t downlinkLen = 0;
  //LoRaWANEvent_t eventDown;

  
  //int16_t state = node.sendReceive(payload, sizeof(payload), 1, downlinkData, &downlinkLen, true, NULL, &eventDown);

  //if (state == 0) {
  //  // TX success but no ACK — confirmed uplink failure
  //  saveSession();
  //  prefs.begin("lorawan", false);
  //  prefs.putInt("failures", failures + 1);
  //  prefs.end();
  //  logPrint("No ACK. Failure count: " + String(failures + 1) + "/6");
  //} else if (state > 0) {
  //    // Downlink received in RX1 or RX2 — ACK confirmed
  //    saveSession();
  //    prefs.begin("lorawan", false);
  //    prefs.putInt("failures", 0);
  //    prefs.end();
  //    logPrint("ACK received in window " + String(state));
  //    if (downlinkLen > 0) {
  //        processDownlink(downlinkData, downlinkLen, eventDown.fPort);
  //    }
  //} else if (state == RADIOLIB_ERR_NETWORK_NOT_JOINED 
  //      || state == RADIOLIB_ERR_SESSION_DISCARDED
  //      || state == RADIOLIB_ERR_NONCES_DISCARDED) {
  //    logPrint("Session invalid, rejoining...");
  //    prefs.begin("lorawan", false);
  //    prefs.clear();
  //    prefs.end();
  //    ESP.restart();
  //} else {
  //    // Radio/hardware error - don't penalise
  //    logPrint("Radio error: " + stateDecode(state));
  //}
  //old loop data end

  //esp_sleep_enable_timer_wakeup(6 * 1000000); // Sleep for 6 seconds
  //esp_deep_sleep_start();
}

void loop() {

  float currentTemp, currentBatt;
  uint8_t payload[6];
  
  getSensorData(currentTemp, currentBatt);
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

  delay(uplinkIntervalSeconds * 1000UL);
}

