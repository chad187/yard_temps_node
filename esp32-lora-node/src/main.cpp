#include "config.h"

void setup() {
  setupHardware();
  setupLoRaWAN();
  //make function to initialize this and power
  prefs.begin("lorawan", false);
  failures = prefs.getInt("failures", 0);
  prefs.end();
  const float battery = calculateBatteryPercentage();


  //deal with battery stuff and return the batter percentage, we will pass that to transmission()
  if (failures >= 6) {
    logPrint("No confirmation in 24hrs, forcing rejoin...");
    prefs.begin("lorawan", false);
    prefs.clear();
    prefs.end();
    ESP.restart();
  }

  if (performNetworkJoin()) {
    //setLED(true);
    transmission(battery);
  } else {
    logPrint("Join FAILED");
  }

  //setLED(false);
  goToSleep(uplinkIntervalSeconds);
}

void loop() {

  //const float battery = calculateBatteryPercentage();

  //transmission(battery);

  //delay(uplinkIntervalSeconds * 1000UL);
}

