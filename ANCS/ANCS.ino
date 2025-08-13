// @author Jovan Koledin
// @modified by Gemini

// A Program that runs a LED matrix driver and a BLE ANCS stack on my ESP32

// Header for this library, from https://www.github.com/Smartphone-Companions/ESP32-ANCS-Notifications.git
#include "esp32notifications.h"
#include <FastLED.h>
#include <math.h>
#include <string.h>

// For LED display
#define MATRIX_WIDTH  16
#define MATRIX_HEIGHT 16
#define NUM_LEDS      (MATRIX_WIDTH * MATRIX_HEIGHT)
#define DATA_PIN      5      // change to whichever GPIO your LED data line is on
CRGB leds[NUM_LEDS];

// Global state for chosen notification
// 'volatile' is used because these are accessed by the main loop and a callback function (interrupt)
volatile bool chosenNotificationActive = false; // Set to false by default
volatile unsigned long chosenNotificationTimestamp = 0; // To time out the notification display
const char* matching_string1 = "Claire";
const char* matching_string2 = "Mom";
const char* matching_string3 = "Dad";



// Forward declarations
void ledWaveTask(void* pvParameters);
void bleTask(void* pvParameters);

// ——— Definition of the LED-wave task ———
// This function has been significantly updated for new visual effects.
void ledWaveTask(void* pvParameters) {
  (void) pvParameters;

  // --- New animation variables ---
  uint32_t time = 0; // Use a 32-bit integer for time to prevent overflow issues
  // The scale determines the "zoom" level of the noise pattern.
  // Larger values result in smaller, more detailed patterns.
  // Smaller values result in larger, smoother patterns.
  uint16_t scale = 30; 

  while (true) {
    // --- Check if the "mom" notification alert should be turned off ---
    // It will be active for 30 seconds.
    if (chosenNotificationActive && (millis() - chosenNotificationTimestamp > 30000)) {
      chosenNotificationActive = false;
      Serial.println("Chosen notification alert has timed out.");
    }

    // --- Main Rendering Logic ---
    if (chosenNotificationActive) {
      // --- "Mom" Notification Pattern: Pulsing Red ---
      // This pattern is shown when a notification from "mom" is active.
      // A sine wave is used to create a smooth pulsing effect for the brightness.
      uint8_t pulse = (sin(millis() / 400.0f) + 1) / 2.0f * 120 + 30; // Varies brightness between 30 and 150

      for (int i = 0; i < NUM_LEDS; i++) {
        // Fill the entire matrix with a red hue (0) and max saturation (255)
        leds[i] = CHSV(0, 255, pulse);
      }
    } else {
      // --- New Default Pattern: "Calm Aurora Noise" ---
      // This pattern uses Perlin noise to create a slow, gentle, and ever-shifting
      // field of color, much like an aurora.
      
      // The time variable is used as the z-axis in the 3D noise field,
      // which makes the pattern animate over time.
      for (int y = 0; y < MATRIX_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {
          int idx = y * MATRIX_WIDTH + x;

          // FastLED's inoise8 function is used to get a noise value for each pixel.
          // We provide the x, y, and a time (z) coordinate.
          // The result is a smooth, continuous value between 0 and 255.
          uint8_t noise_val = inoise8(x * scale, y * scale, time);

          // We use the noise value to set the hue of the pixel.
          // A small offset based on time is added to make the colors shift slowly.
          uint8_t hue = noise_val + (time / 20);

          // The saturation is kept high for rich colors.
          uint8_t saturation = 220;

          // The brightness is set to a consistent, medium-low value.
          // This prevents any LEDs from turning off and keeps the visual calm.
          uint8_t brightness = 80;

          leds[idx] = CHSV(hue, saturation, brightness);
        }
      }
    }

    FastLED.show();

    // Advance animation time. A larger increment means a faster animation.
    time += 10;

    // ~30 Hz update rate is plenty for this slow animation
    vTaskDelay(pdMS_TO_TICKS(33));
  }
}

// Create an interface to the BLE notification library
BLENotifications notifications;

// Holds the incoming call's ID number, or zero if no notification
uint32_t incomingCallNotificationUUID;

// This callback will be called when a Bluetooth LE connection is made or broken.
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

bool anyMatches(String title, const char* check1, const char* check2, const char* check3){
  return ((strstr(title.c_str(), check1) != NULL) 
        || (strstr(title.c_str(), check2) != NULL) 
        ||  (strstr(title.c_str(), check3) != NULL));
}

// A notification arrived from the mobile device.
// This function is modified to check for the sender's name.
void onNotificationArrived(const ArduinoNotification * notification, const Notification * rawNotificationData) {
    Serial.print("Got notification: ");   
    Serial.println(notification->title);
    Serial.println(notification->message);
    Serial.println(notification->type);
    Serial.println(notifications.getNotificationCategoryDescription(notification->category));
    Serial.println(notification->categoryCount);

    // --- MODIFICATION START ---
    // Check if the notification title contains the matching string (e.g., "mom")
    // strstr is used to find a substring, making it more flexible.
    if (notification->title && anyMatches(notification->title, matching_string1, matching_string2, matching_string3)) {     
        chosenNotificationActive = true;
        Serial.println("Chosen notification activated");
        chosenNotificationTimestamp = millis(); // Record the time the notification arrived
    }
    // --- MODIFICATION END ---

    if (notification->category == CategoryIDIncomingCall) {
        incomingCallNotificationUUID = notification->uuid;
        Serial.println("--- INCOMING CALL ---"); 
    }
    else {
        incomingCallNotificationUUID = 0;
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
  Serial.println("Starting BLE ANCS on core 0...");
  notifications.begin("LightPhone");
  notifications.setConnectionStateChangedCallback(onBLEStateChanged);
  notifications.setNotificationCallback(onNotificationArrived);
  notifications.setRemovedCallback(onNotificationRemoved);

  while (true) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize LED matrix (FastLED)
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  // The overall brightness is set here. The animation can still control individual LED brightness.
  FastLED.setBrightness(80); 

  // Create BLE task pinned to core 0
  xTaskCreatePinnedToCore(
    bleTask,
    "BLE ANCS Task",
    8192,
    nullptr,
    1,
    nullptr,
    0
  );

  // Create LED wave task pinned to core 1
  xTaskCreatePinnedToCore(
    ledWaveTask,
    "LED Wave Task",
    4096,
    nullptr,
    2,
    nullptr,
    1
  );
}

void loop() {
  // The main loop is empty because all work is done in the FreeRTOS tasks.
}
