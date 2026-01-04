#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

//
// ---- CONFIG ----
//
const char* TARGET_UUID_STR = "12345678-1234-1234-1234-1234567890ab";  // must match your phone beacon UUID
const int LED_PIN = 2;        // change if needed

// RSSI → Distance tuning (unchanged)
float TX_POWER_AT_1M = -59;   // RSSI at 1 meter (calibrate later)
float PATH_LOSS_EXP  = 2.0;   // 2.0 = free space, 2.5–3.0 = indoor walls

// Presence timeout (used for marking "recently seen")
unsigned long SEEN_TIMEOUT = 5000;  // ms; no beacon for 5 sec → consider not recently seen

// RSSI trigger to allow LED ON (unchanged semantics)
// smoothedRSSI must be > RSSI_TRIGGER to be considered "strong"
int RSSI_TRIGGER = -79; // tune this to control range

// How long RSSI must remain WEAK continuously to confirm shutdown (your requested 5s)
const unsigned long SHUTDOWN_CONFIRM_MS = 5000;

//
// ---- GLOBALS ----
//
BLEScan* pBLEScan;
BLEUUID targetUUID(TARGET_UUID_STR);

unsigned long lastSeen = 0;
float smoothRSSI = -90.0;   // start with bad RSSI so it smooths in

bool ledState = false;           // current LED output state
unsigned long shutdownStart = 0; // 0 = no shutdown countdown running; otherwise millis() when weak RSSI first seen

//
// ---- FUNCTIONS ----
//
float calculateDistance(float rssi) {
  float exponent = (TX_POWER_AT_1M - rssi) / (10 * PATH_LOSS_EXP);
  return pow(10, exponent);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("Init BLE (smoothed RSSI with shutdown-confirmation)...");
  BLEDevice::init("");

  pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(80);
}

void loop() {
  // --------------------------
  // 1) Perform the scan (unchanged)
  // --------------------------
  BLEScanResults* results = pBLEScan->start(1, false);
  int count = results->getCount();

  bool foundThisScan = false;
  float rssiThisScan = -100.0;

  for (int i = 0; i < count; i++) {
    BLEAdvertisedDevice dev = results->getDevice(i);

    if (dev.haveServiceUUID() && dev.isAdvertisingService(targetUUID)) {
      foundThisScan = true;
      rssiThisScan = dev.getRSSI();
      lastSeen = millis(); // same as your original behavior

      Serial.print("Detected beacon | Raw RSSI: ");
      Serial.println(rssiThisScan);
    }
  }

  pBLEScan->stop();
  pBLEScan->clearResults();

  // --------------------------
  // 2) Smooth RSSI (unchanged)
  // --------------------------
  if (foundThisScan) {
    smoothRSSI = (smoothRSSI * 0.7f) + (rssiThisScan * 0.3f);
  } else {
    smoothRSSI = (smoothRSSI * 0.9f) + (-100.0f * 0.1f);
  }

  // --------------------------
  // 3) LED ON decision (same as before)
  //    LED turns ON only when:
  //      - device seen recently (within SEEN_TIMEOUT)
  //      - smoothed RSSI strong: smoothRSSI > RSSI_TRIGGER
  // --------------------------
  unsigned long now = millis();
  bool recentlySeen = (now - lastSeen) < SEEN_TIMEOUT;
  bool rssiStrong = (smoothRSSI > RSSI_TRIGGER); // true when close/strong

  if (!ledState) {
    // LED is currently OFF -> normal ON condition
    if (recentlySeen && rssiStrong) {
      ledState = true;
      // cancel any pending shutdown (shouldn't be running while LED is OFF but safe)
      shutdownStart = 0;
      Serial.println(">>> LED TURNED ON (smoothed RSSI OK)");
    }
  } else {
    // LED is currently ON -> we implement the behavior you requested:
    // - Continuously monitor smoothed RSSI.
    // - When smoothedRSSI becomes WEAK (i.e. NOT > RSSI_TRIGGER), start shutdown timer.
    // - If it remains WEAK for SHUTDOWN_CONFIRM_MS continuously, turn LED OFF.
    // - If at any point during the countdown smoothedRSSI returns STRONG, cancel countdown.
    if (!rssiStrong) {
      // RSSI went weak -> start or continue the shutdown countdown
      if (shutdownStart == 0) {
        shutdownStart = now; // begin countdown
        Serial.println(">>> Shutdown countdown START (RSSI weak)");
      } else {
        // countdown already running -> check if confirmed
        if (now - shutdownStart >= SHUTDOWN_CONFIRM_MS) {
          // confirmed weak for full confirmation window -> turn off
          ledState = false;
          shutdownStart = 0;
          Serial.println("<<< LED TURNED OFF (RSSI weak confirmed for 5s)");
        } else {
          // still counting down
          Serial.print("...Shutdown counting (ms elapsed): ");
          Serial.println(now - shutdownStart);
        }
      }
    } else {
      // RSSI strong again -> cancel any shutdown countdown
      if (shutdownStart != 0) {
        shutdownStart = 0;
        Serial.println(">>> Shutdown CANCELLED (RSSI strong again)");
      }
      // Also refresh lastSeen when RSSI strong, to maintain presence (already done in scan)
    }
  }

  // Apply LED output
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);

  // --------------------------
  // 4) Debug prints (compact)
  // --------------------------
  Serial.print("Smoothed RSSI: ");
  Serial.println(smoothRSSI);
  Serial.print("RSSI_TRIGGER: ");
  Serial.println(RSSI_TRIGGER);
  Serial.print("LED State: ");
  Serial.println(ledState ? "ON" : "OFF");
  if (shutdownStart != 0) {
    Serial.print("Shutdown ms elapsed: ");
    Serial.println(now - shutdownStart);
  }
  Serial.println();

  delay(100); // keep same timing as your original loop
}
