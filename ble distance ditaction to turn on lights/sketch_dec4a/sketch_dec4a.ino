#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

const char* TARGET_UUID_STR = "12345678-1234-1234-1234-1234567890ab";  // must match phone beacon
const int LED_PIN = 2;   // change if needed

BLEScan* pBLEScan;
BLEUUID targetUUID(TARGET_UUID_STR);

unsigned long lastSeen = 0;
const unsigned long SEEN_TIMEOUT = 5000;  // 5s no beacon -> LED OFF

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("Init BLE (presence by UUID)...");
  BLEDevice::init("");

  pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);  // needed to get full data
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(80);
}

void loop() {
  // Scan for 1 second
  BLEScanResults* results = pBLEScan->start(1, false);
  int count = results->getCount();

  for (int i = 0; i < count; i++) {
    BLEAdvertisedDevice dev = results->getDevice(i);

    // Log address just for debugging
    String addr = dev.getAddress().toString().c_str();
    Serial.print("Saw addr: ");
    Serial.println(addr);

    // Check if this device is advertising our service UUID
    if (dev.haveServiceUUID() && dev.isAdvertisingService(targetUUID)) {
      Serial.println(">>> OUR UUID DETECTED (phone beacon)");
      lastSeen = millis();
    }
  }

  pBLEScan->stop();
  pBLEScan->clearResults();

  // LED logic
  unsigned long now = millis();
  if (now - lastSeen < SEEN_TIMEOUT) {
    digitalWrite(LED_PIN, HIGH);   // beacon recently seen -> LED ON
  } else {
    digitalWrite(LED_PIN, LOW);    // not seen -> LED OFF
  }

  delay(100);
}
