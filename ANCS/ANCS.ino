// @author James Hudson bugs.feedback.whatever@gmail.com
// @todo license text and header - please see github project for license

// A simple example program to retrieve notifications from your device, and output them to the Serial console.


// Header for this library, from https://www.github.com/Smartphone-Companions/ESP32-ANCS-Notifications.git
#include "esp32notifications.h"
#include <FastLED.h>

// For LED display
#define MATRIX_WIDTH  16
#define MATRIX_HEIGHT 16
#define NUM_LEDS      (MATRIX_WIDTH * MATRIX_HEIGHT)
#define DATA_PIN      5      // change to whichever GPIO your LED data line is on
CRGB leds[NUM_LEDS];

// Wave parameters
static float wavePhase = 0;

// Forward declarations
void ledWaveTask(void* pvParameters);
void bleTask(void* pvParameters);

// ——— Definition of the LED-wave task ———
void ledWaveTask(void* pvParameters) {
  (void) pvParameters;

  while (true) {
    // For each pixel in the matrix, compute a sine-based hue wave
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
      for (int x = 0; x < MATRIX_WIDTH; x++) {
        // Compute an index into leds[] (row-major)
        int idx = y * MATRIX_WIDTH + x;
        // Create a wave that moves in X over time and pulses per row
        float value = sinf((x * 0.3f) + wavePhase) * 0.5f + 0.5f; 
        uint8_t hue   = uint8_t((value * 255.0f + y * 16)) & 0xFF;
        uint8_t bright= uint8_t(value * 192.0f + 64.0f);
        leds[idx] = CHSV(hue, 200, bright);
      }
    }
    FastLED.show();

    // Advance phase for animation
    wavePhase += 0.1f;
    if (wavePhase > 2 * PI) wavePhase -= 2 * PI;

    // ~50 Hz refresh
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// Create an interface to the BLE notification library
BLENotifications notifications;

// Holds the incoming call's ID number, or zero if no notification
uint32_t incomingCallNotificationUUID;

// This callback will be called when a Bluetooth LE connection is made or broken.
// You can update the ESP 32's UI or take other action here.
void onBLEStateChanged(BLENotifications::State state) {
  switch(state) {
      case BLENotifications::StateConnected:
          Serial.println("StateConnected - connected to a phone or tablet"); 
          break;

      case BLENotifications::StateDisconnected:
          Serial.println("StateDisconnected - disconnected from a phone or tablet"); 
          /* We need to startAdvertising on disconnection, otherwise the ESP 32 will now be invisible.
          IMO it would make sense to put this in the library to happen automatically, but some people in the Espressif forums
          were requesting that they would like manual control over re-advertising.*/
          notifications.startAdvertising(); 
          break; 
  }
}

// A notification arrived from the mobile device, ie a social media notification or incoming call.
// parameters:
//  - notification: an Arduino-friendly structure containing notification information. Do not keep a
//                  pointer to this data - it will be destroyed after this function.
//  - rawNotificationData: a pointer to the underlying data. It contains the same information, but is
//                         not beginner-friendly. For advanced use-cases.
void onNotificationArrived(const ArduinoNotification * notification, const Notification * rawNotificationData) {
    Serial.print("Got notification: ");   
    Serial.println(notification->title); // The title, ie name of who sent the message
    Serial.println(notification->message); // The detail, ie "be home for dinner at 7".
    Serial.println(notification->type);  // Which app sent it
    Serial.println(notifications.getNotificationCategoryDescription(notification->category));  // ie "social media"
    Serial.println(notification->categoryCount); // How may other notifications are there from this app (ie badge number)
    if (notification->category == CategoryIDIncomingCall) {
		// If this is an incoming call, store it so that we can later send a user action.
        incomingCallNotificationUUID = notification->uuid;
        Serial.println("--- INCOMING CALL: PRESS A TO ACCEPT, C TO REJECT ---"); 
    }
    else {
        incomingCallNotificationUUID = 0; // Make invalid - no incoming call
    }
}

// A notification was cleared
void onNotificationRemoved(const ArduinoNotification * notification, const Notification * rawNotificationData) {
     Serial.print("Removed notification: ");   
     Serial.println(notification->title);
     Serial.println(notification->message);
     Serial.println(notification->type);  
}

// BLE ANCS Task - runs on core 0
void bleTask(void* pvParameters) {
  // Initialize BLE ANCS
  Serial.println("Starting BLE ANCS on core 0...");
  notifications.begin("Jovan's ESP32");
  notifications.setConnectionStateChangedCallback(onBLEStateChanged);
  notifications.setNotificationCallback(onNotificationArrived);
  notifications.setRemovedCallback(onNotificationRemoved);

  // Optional: if library requires a loop call
  while (true) {
    // Uncomment if required by esp32notifications:
    // notifications.loop();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize LED matrix (FastLED)
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(64);

  // Create BLE task pinned to core 0
  xTaskCreatePinnedToCore(
    bleTask,        // task function
    "BLE ANCS Task", // task name
    8192,           // stack size (bytes)
    nullptr,        // parameters
    2,              // priority (higher than LED)
    nullptr,        // task handle
    0               // run on core 0
  );

  // Create LED wave task pinned to core 1
  xTaskCreatePinnedToCore(
    ledWaveTask,
    "LED Wave Task",
    4096,
    nullptr,
    1,
    nullptr,
    1
  );
}


// Standard Arduino function that is called in an endless loop after setup
void loop() {   
}