#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define BUTTON_PIN1 D1
#define BUTTON_PIN2 D2
#define LED_PIN D4
#define FORCE_SENSOR_PIN A0  // Define the pin for the force sensor

#define SERVICE_UUID        "b515d81b-3a96-483e-bee0-e0cef8fc33b0"
#define CHARACTERISTIC_UUID "d0821380-005b-498e-a88e-445b34384456"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Define the proportionality constant for the force sensor
const float k = 0.01801;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_PIN1, INPUT_PULLUP);
    pinMode(BUTTON_PIN2, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    pinMode(FORCE_SENSOR_PIN, INPUT);  // Initialize the force sensor pin as an input

    BLEDevice::init("GameDevice");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    BLEDevice::startAdvertising();
    Serial.println("Ready to play the game!");
}

void loop() {
    if (deviceConnected) {
        Serial.println("Ready... Wait for the signal!");

        long waitTime = random(5000, 8000);
        delay(waitTime);

        digitalWrite(LED_PIN, HIGH);
        Serial.println("Go!");

        bool button1Pressed = false;
        bool button2Pressed = false;
        int analogReading = 0 - 0.14;
        float massInGrams = 0.0;

        while (!button1Pressed && !button2Pressed) {
            button1Pressed = digitalRead(BUTTON_PIN1) == LOW;
            button2Pressed = digitalRead(BUTTON_PIN2) == LOW;

            if (button1Pressed || button2Pressed) {
                analogReading = analogRead(FORCE_SENSOR_PIN);
                massInGrams = k * analogReading;  // Convert the sensor value to grams
            }
        }

        digitalWrite(LED_PIN, LOW);
        String message;

        if (button1Pressed) {
            message = "Player 1 wins, force: " + String(massInGrams) + " g.";
        } else if (button2Pressed) {
            message = "Player 2 wins, force: " + String(massInGrams) + " g.";
        }

        pCharacteristic->setValue(message.c_str());
        pCharacteristic->notify();
        Serial.println(message);

        delay(3000); // Delay before next round starts
    }

    if (!deviceConnected && oldDeviceConnected) {
        delay(500);
        pServer->startAdvertising();
        oldDeviceConnected = deviceConnected;
    }

    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
}
