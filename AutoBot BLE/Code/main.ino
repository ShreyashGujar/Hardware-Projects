/*
  AutoBot BLE (ESP32)

  - BLE writable characteristic receives single-character commands:
      'F' : Forward
      'B' : Backward
      'L' : Left
      'R' : Right
      'S' : Stop
      'I' : Forward Right
      'J' : Backward Right
      'G' : Forward Left
      'H' : Backward Left

  - Obstacle avoidance: if distance <= 15 cm, motors are stopped.
  - Change pin mappings below if required by hardware.

  Dependencies:
    - Install "ESP32 BLE Arduino" (uses BLEDevice.h, BLEServer, BLECharacteristic)
*/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

//
// --- Pin configuration (ESP32-safe defaults) ---
//
const int m1a = 14;   // Motor 1 control A
const int m1b = 27;   // Motor 1 control B
const int m2a = 26;   // Motor 2 control A
const int m2b = 25;   // Motor 2 control B

const int trigPin = 32; // HC-SR04 TRIG (output)
const int echoPin = 33; // HC-SR04 ECHO (input)

volatile char val = 0;   // Latest command (updated by BLE callback)
long duration_us = 0;
int distance_cm = 0;

//
// --- BLE parameters ---
const char* bleDeviceName = "AutoBot-BLE";
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // example (Nordic UART style)
#define CHAR_UUID_RX        "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // write to this characteristic

//
// --- BLE write callback ---
class CommandCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) override {
    std::string rx = pChar->getValue();
    if (rx.length() > 0) {
      // take the first byte as command
      char c = rx[0];
      // normalize newline or carriage returns by ignoring them
      if (c == '\n' || c == '\r') return;
      val = c;
      Serial.print("BLE -> command: ");
      Serial.println(val);
    }
  }
};

//
// --- Setup and helpers ---
void stopMotors() {
  digitalWrite(m1a, LOW);
  digitalWrite(m1b, LOW);
  digitalWrite(m2a, LOW);
  digitalWrite(m2b, LOW);
}

void setupPins() {
  pinMode(m1a, OUTPUT);
  pinMode(m1b, OUTPUT);
  pinMode(m2a, OUTPUT);
  pinMode(m2b, OUTPUT);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  stopMotors();
}

void setupBLE() {
  BLEDevice::init(bleDeviceName);
  BLEServer *pServer = BLEDevice::createServer();

  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHAR_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new CommandCallback());

  // optional: add descriptor so many mobile apps discover characteristic correctly
  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();

  Serial.print("BLE advertising as: ");
  Serial.println(bleDeviceName);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  setupPins();
  setupBLE();
}

//
// --- Distance measurement (HC-SR04) ---
int measureDistanceCm() {
  // Send trig pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read echo pulse duration in microseconds
  // pulseIn is available on ESP32 but can block; use as before
  duration_us = pulseIn(echoPin, HIGH, 30000); // timeout 30ms => ~5m
  if (duration_us == 0) {
    // timeout: assume out of range
    return 999;
  }
  int dist = (int)((duration_us * 0.0343) / 2.0); // cm
  return dist;
}

void distSafetyCheck() {
  distance_cm = measureDistanceCm();
  Serial.print("Distance (cm): ");
  Serial.println(distance_cm);

  if (distance_cm <= 15) {
    Serial.println("Obstacle detected: stopping motors");
    stopMotors();
    delay(300);
  }
}

//
// --- Motion commands ---
void cmdForward() {
  digitalWrite(m1a, LOW);
  digitalWrite(m1b, HIGH);
  digitalWrite(m2a, LOW);
  digitalWrite(m2b, HIGH);
  distSafetyCheck();
}

void cmdBackward() {
  digitalWrite(m1a, HIGH);
  digitalWrite(m1b, LOW);
  digitalWrite(m2a, HIGH);
  digitalWrite(m2b, LOW);
}

void cmdLeft() {
  // left turn: stop left motor, run right forward
  digitalWrite(m1a, LOW);
  digitalWrite(m1b, LOW);
  digitalWrite(m2a, LOW);
  digitalWrite(m2b, HIGH);
  distSafetyCheck();
}

void cmdRight() {
  // right turn: run left forward, stop right motor
  digitalWrite(m1a, LOW);
  digitalWrite(m1b, HIGH);
  digitalWrite(m2a, LOW);
  digitalWrite(m2b, LOW);
  distSafetyCheck();
}

void cmdStop() {
  stopMotors();
}

void cmdForwardRight() {
  // small pivot / differential drive
  digitalWrite(m1a, HIGH);
  digitalWrite(m1b, LOW);
  digitalWrite(m2a, LOW);
  digitalWrite(m2b, LOW);
}

void cmdBackwardRight() {
  digitalWrite(m1a, LOW);
  digitalWrite(m1b, HIGH);
  digitalWrite(m2a, LOW);
  digitalWrite(m2b, LOW);
}

void cmdForwardLeft() {
  digitalWrite(m1a, LOW);
  digitalWrite(m1b, LOW);
  digitalWrite(m2a, HIGH);
  digitalWrite(m2b, LOW);
}

void cmdBackwardLeft() {
  digitalWrite(m1a, LOW);
  digitalWrite(m1b, LOW);
  digitalWrite(m2a, LOW);
  digitalWrite(m2b, HIGH);
}

void processCommand(char c) {
  switch (c) {
    case 'F': cmdForward(); break;
    case 'B': cmdBackward(); break;
    case 'L': cmdLeft(); break;
    case 'R': cmdRight(); break;
    case 'S': cmdStop(); break;
    case 'I': cmdForwardRight(); break;
    case 'J': cmdBackwardRight(); break;
    case 'G': cmdForwardLeft(); break;
    case 'H': cmdBackwardLeft(); break;
    default:
      // ignore unknown chars or handle as needed
      Serial.print("Unknown command: ");
      Serial.println(c);
      break;
  }
}

void loop() {
  // Also allow serial input from USB for debugging (single char commands)
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c != '\n' && c != '\r') {
      val = c;
      Serial.print("Serial -> command: ");
      Serial.println(val);
    }
  }

  // If a BLE or Serial command arrived, process it
  if (val != 0) {
    char cmd = val;
    val = 0; // reset command (avoids repeated execution unless sent again)
    processCommand(cmd);
  }

  // small delay to yield to BLE stack and not hog CPU
  delay(10);
}
