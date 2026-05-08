#include <RadioLib.h>
#include <WiFi.h>
#include <HTTPUpdate.h>

// --- Hardware Pinout for Wio SX1262 + XIAO ESP32-S3 ---
// NSS pin: D5 (GPIO5)
// DIO1 pin: D1 (GPIO1)
// NRST pin: D4 (GPIO4)
// BUSY pin: D3 (GPIO3)
SX1262 radio = new Module(D5, D1, D4, D3);

// --- LoRaWAN Credentials (Get these from ChirpStack) ---
uint64_t joinEUI = 0x0000000000000000;
uint64_t devEUI  = 0x0123456789ABCDEF; // Unique for each node
uint8_t appKey[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };

// --- WiFi Update Settings ---
const char* WIFI_SSID = "YARD_WIFI";
const char* WIFI_PASS = "PASSWORD";
const char* UPDATE_URL = "http://your-global-server.com/firmware/v2.bin";

// Create LoRaWAN node instance
LoRaWANNode node(&radio, &LoRaWAN_Regions::US915); // Adjust region if not in US

void setup() {
  Serial.begin(115200);
  
  // Initialize radio
  Serial.print(F("[Radio] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // Join the network
  Serial.print(F("[LoRaWAN] Attempting join ... "));
  state = node.beginOTAA(joinEUI, devEUI, appKey);
  if (state >= RADIOLIB_ERR_NONE) {
    Serial.println(F("Joined!"));
  } else {
    Serial.println(F("Join failed."));
  }
}

void loop() {
  // 1. Send sensor data
  uint8_t payload[4] = {24, 50, 36, 10}; // Example data, this may still be trying to send status but I killed that everywhere but here
  int state = node.sendReceive(payload, 4);

  if (state >= RADIOLIB_ERR_NONE) {
    Serial.println(F("Uplink successful"));

    // 2. Check for Downlink (Command from Server)

    if (node.available()) {
        uint8_t buffer[256];
        size_t size = node.read(buffer, 256);

        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, buffer, size);

        if (!error) {
            const char* ssid = doc["s"];
            const char* pass = doc["p"];
            const char* url  = doc["u"];
            
            if (ssid && pass && url) {
            triggerWiFiUpdate(ssid, pass, url);
            }
        }
    }
  }

  // 3. Deep Sleep for 15 minutes to save battery
  // The XIAO ESP32-S3 is great for this.
  Serial.println(F("Sleeping..."));
//  ESP.deepSleep(900e6);
  ESP.deepSleep(10800000000ULL); // 3 hours sleep if you want even longer
}

void triggerWiFiUpdate(const char* ssid, const char* pass, const char* url) {
  Serial.println(F("Connecting to WiFi for OTA..."));
  WiFi.begin(ssid, pass);
  
  int attempts = 0;
  // Give it 15 seconds to find the yard WiFi
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("WiFi Connected. Starting Download..."));
    // This handles the download and the flash writing automatically
    t_httpUpdate_return ret = httpUpdate.update(url);
    
    if (ret == HTTP_UPDATE_FAILED) {
        Serial.printf("Update failed: %s\n", httpUpdate.getLastErrorString().c_str());
    }
  } else {
    Serial.println(F("WiFi failed. Staying on current version."));
  }

  // Clean up and go back to LoRa mode
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}