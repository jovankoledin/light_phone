// @author Jovan Koledin
// A Program that runs a LED matrix driver and a BLE ANCS stack on my ESP32

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "esp32notifications.h"
#include <FastLED.h>
#include <math.h>
#include <string.h>

// For LED display
#define MATRIX_WIDTH  16
#define MATRIX_HEIGHT 16
#define NUM_LEDS      (MATRIX_WIDTH * MATRIX_HEIGHT)
#define DATA_PIN      5
CRGB leds[NUM_LEDS];

// Global state
volatile bool chosenNotificationActive = false;
volatile unsigned long chosenNotificationTimestamp = 0;
const char* matching_string1 = "Claire";
const char* matching_string2 = "Mom";
const char* matching_string3 = "Dad";
const char* ssid = "Toa_the_Queen";
const char* password = "browndog!!@@2001";

// Forward declarations
void ledWaveTask(void* pvParameters);
void bleTask(void* pvParameters);
void startOTA();

// --- LED Task ---
void ledWaveTask(void* pvParameters) {
  (void) pvParameters;
  uint32_t time = 0;
  uint16_t scale = 30; 

  while (true) {
    if (chosenNotificationActive && (millis() - chosenNotificationTimestamp > 30000)) {
      chosenNotificationActive = false;
      Serial.println("Chosen notification alert has timed out.");
    }

    if (chosenNotificationActive) {
      uint8_t pulse = (sin(millis() / 400.0f) + 1) / 2.0f * 120 + 30;
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CHSV(0, 255, pulse);
      }
    } else {
      for (int y = 0; y < MATRIX_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {
          int idx = y * MATRIX_WIDTH + x;
          uint8_t noise_val = inoise8(x * scale, y * scale, time);
          uint8_t hue = noise_val + (time / 20);
          leds[idx] = CHSV(hue, 220, 80);
        }
      }
    }

    FastLED.show();
    time += 10;
    vTaskDelay(pdMS_TO_TICKS(33));
  }
}

// BLE interface
BLENotifications notifications;
uint32_t incomingCallNotificationUUID;

// --- OTA trigger ---
void startOTA() {
  Serial.println("Entering OTA Mode via Dad’s text...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    if (millis() - start > 10000) {
      Serial.println("WiFi connection failed, skipping OTA.");
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      return;
    }
    delay(500);
  }

  ArduinoOTA.setHostname("LightPhone-ESP32");
  ArduinoOTA.onStart([]() {
    Serial.println("Start OTA update...");
  }).onEnd([]() {
    Serial.println("\nEnd OTA update");
  }).onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  }).onError([](ota_error_t error) {
    Serial.printf("Error[%u]\n", error);
  });

  ArduinoOTA.begin();
  Serial.println("OTA Ready - upload via IDE now");

  // Block here until OTA completes
  while (true) {
    ArduinoOTA.handle();
    delay(10);
  }
}

// BLE state changes
void onBLEStateChanged(BLENotifications::State state) {
  switch(state) {
      case BLENotifications::StateConnected:
          Serial.println("StateConnected - connected to a phone or tablet"); 
          break;
      case BLENotifications::StateDisconnected:
          Serial.println("StateDisconnected - disconnected from a phone or tablet"); 
          notifications.startAdvertising(); 
          break; 
  }
}

bool anyMatches(const String& title, const char* check1, const char* check2, const char* check3){
  return ((strstr(title.c_str(), check1) != NULL) 
        || (strstr(title.c_str(), check2) != NULL) 
        ||  (strstr(title.c_str(), check3) != NULL));
}

// --- Main notification handler ---
void onNotificationArrived(const ArduinoNotification * notification, const Notification * rawNotificationData) {
    Serial.print("Got notification: ");   
    Serial.println(notification->title);
    Serial.println(notification->message);

    // Check for Dad’s "OTA" text
    if (strstr(notification->title.c_str(), "Dad") != NULL &&
        strstr(notification->message.c_str(), "OTA") != NULL) {
        Serial.println("OTA Triggered by Dad!");
        notifications.stop();
        startOTA();
    }

    // Normal chosen-notification logic
    if (anyMatches(notification->title, matching_string1, matching_string2, matching_string3)) {     
        chosenNotificationActive = true;
        Serial.println("Chosen notification activated");
        chosenNotificationTimestamp = millis();
    }

    if (notification->category == CategoryIDIncomingCall) {
        incomingCallNotificationUUID = notification->uuid;
        Serial.println("--- INCOMING CALL ---"); 
    } else {
        incomingCallNotificationUUID = 0;
    }
}

void onNotificationRemoved(const ArduinoNotification * notification, const Notification * rawNotificationData) {
     Serial.print("Removed notification: ");   
     Serial.println(notification->title);
     Serial.println(notification->message);
}

// BLE Task
void bleTask(void* pvParameters) {
  Serial.println("Starting BLE ANCS on core 0...");
  notifications.begin("LightPhone");
  notifications.setConnectionStateChangedCallback(onBLEStateChanged);
  notifications.setNotificationCallback(onNotificationArrived);
  notifications.setRemovedCallback(onNotificationRemoved);
  Serial.println("Done");

  while (true) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void setup() {
  Serial.begin(115200);

  // No WiFi/OTA at boot → BLE stays stable

  // LED init
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(80); 

  // BLE task
  xTaskCreatePinnedToCore(bleTask, "BLE ANCS Task", 8192, nullptr, 3, nullptr, 0);

  // LED task
  xTaskCreatePinnedToCore(ledWaveTask, "LED Wave Task", 4096, nullptr, 2, nullptr, 1);
}

void loop() {
  // empty → OTA only when Dad texts
}
