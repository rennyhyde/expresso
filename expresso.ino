#include <Bounce2.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <iostream>
using namespace std;

#define DEBUG 0

#define VRX_PIN  A1
#define VRY_PIN  A2
#define BUTTON_PIN A3

// #if DEBUG
#define STATUS_PIN A0   // Connected to LED indicating mode of QT PY (boot vs running)
// #endif


int xVal = 0;
int yVal = 0;
const int xMax = 4095;
const int yMax = 4095;

int xImpulse = 0; //Left-Right impulse (1: +x, 0: 0, -1: -x)
int yImpulse = 0; //Up-Down impulse (1: +y, 0: 0, -1: -y)
int buttonPress = 0;  // 1 when button is pressed, 0 otherwise. In other words, bang when button is pressed (not released)
int buttonHold = 0;  // 1 while button is held down, 0 otherwise

string prevData = "";

#if DEBUG
int count = 0;    // For counting button clicks
#endif

Bounce2::Button button = Bounce2::Button();

// Bluetooth setup
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID        "29b26bc8-182d-4998-9b85-c70217b582da"
#define CHARACTERISTIC_UUID "2f57bc23-d2e5-4e33-910f-cba6f96ed89b"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void setup() {
  Serial.begin(9600);
  button.attach( BUTTON_PIN ,  INPUT_PULLUP );
  button.interval(5);
  button.setPressedState(LOW);
  

  //#if DEBUG
  pinMode(STATUS_PIN, OUTPUT);
 // #endif

  // Bluetooth setup

  BLEDevice::init("Expresso");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");

}

void loop() {
  // #if DEBUG
  digitalWrite(STATUS_PIN, HIGH); //Turn LED on while QT PY is running and off when in boot mode
  // #endif


  ///////// READ INPUT FROM JOYSTICK /////////


  // Get joystick status
  xVal = analogRead(VRX_PIN);
  yVal = analogRead(VRY_PIN);
  button.update();

  if  (button.pressed()) {
    buttonPress = 1;
  } else {
    buttonPress = 0;
  }
  if  (button.isPressed()) {
    buttonHold = 1;
  } else {
    buttonHold = 0;
  }

  if (xVal > xMax * 0.9) {
    xImpulse = 1;
  } else if (xVal < xMax * 0.1) {
    xImpulse = -1;
  } else {
    xImpulse = 0;
  }

  if (yVal > yMax * 0.9) {
    yImpulse = 1;
  } else if (yVal < yMax * 0.1) {
    yImpulse = -1;
  } else {
    yImpulse = 0;
  }

  #if DEBUG
  // Test joystick directions:
  Serial.printf("%d %d\n", xImpulse, yImpulse);

  // Test button hold:
  // Serial.printf("%d %d\n", buttonPress, buttonHold);

  // Test button press:
  // if (buttonPress) {
  //   count += 1;
  //   Serial.println(count);
  // }
  #endif


  ///////// SEND DATA VIA BLUETOOTH /////////

  string x(to_string(xImpulse + 1));  // Added 1 to the directional impulses to avoid negative sign -- remember to subtract 1 on client side
  string y(to_string(yImpulse + 1));
  string p(to_string(buttonPress));
  string h(to_string(buttonHold));
  // string data = "x" + x + "y" + y + "p" + p + "h" + h; 
  string data = x + y + p + h + "~";
  
  pCharacteristic->setValue(data);

  // notify changed value
  if (deviceConnected && data != prevData) {
      pCharacteristic->setValue(data);
      pCharacteristic->notify();
      delay(3); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
      delay(500); // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // restart advertising
      Serial.println("start advertising");
      oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
      // do stuff here on connecting
      oldDeviceConnected = deviceConnected;
  }

  prevData = data;
}
