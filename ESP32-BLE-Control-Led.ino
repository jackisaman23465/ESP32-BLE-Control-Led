#include <analogWrite.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#define SERVICE_UUID                   "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define Switch_Characteristic_UUID     "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define Brightness_Characteristic_UUID "58761b6e-1575-11ee-be56-0242ac120002"
#define Mode_Characteristic_UUID       "b9fde1bc-1586-11ee-be56-0242ac120002"

BLEServer* pServer = NULL;

BLECharacteristic* pSwitchCharacteristic = NULL;
BLECharacteristic* pBrightnessCharacteristic = NULL;
BLECharacteristic* pModeCharacteristic = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

uint8_t state = LOW;
uint8_t brightness = 0x88;
uint8_t mode = 0x01;
uint8_t breathBrightness = 0x00;

bool lightUp = true;
bool firstTime = true;

bool switchValueChanged = false;
bool brightnessValueChanged = false;
bool modeValueChanged = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class SwitchCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    Serial.println("onWrite");
    switchValueChanged = true;
  }
};

class BrightnessCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    brightnessValueChanged = true;
  }
};

class ModeCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    modeValueChanged = true;
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  // attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, CHANGE); 
  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create LED Switch Characteristic
  pSwitchCharacteristic = pService->createCharacteristic(
                      Switch_Characteristic_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE
                    );

  // Create LED Brightness Characteristic
  pBrightnessCharacteristic = pService->createCharacteristic(
                      Brightness_Characteristic_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE
                    );

  // Create LED Mode Characteristic
  pModeCharacteristic = pService->createCharacteristic(
                      Mode_Characteristic_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE
                    );

  // Add characteristic callback
  pSwitchCharacteristic->setCallbacks(new SwitchCallbacks());
  pBrightnessCharacteristic->setCallbacks(new BrightnessCallbacks());
  pModeCharacteristic->setCallbacks(new ModeCallbacks());
  // Init characteristic value
  // uint8_t value = 0x55;
  pSwitchCharacteristic->setValue(&state,1);
  pBrightnessCharacteristic->setValue(&brightness,1);
  pModeCharacteristic->setValue(&mode,1);

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

String convertToHexString(uint8_t* data, size_t size) {
  String hexString;
  for (size_t i = 0; i < size; i++) {
    if (data[i] < 16) {
      hexString += '0'; 
    }
    hexString += String(data[i], HEX); 
  }
  return hexString;
}

void loop() {
    if (deviceConnected) {
      if(switchValueChanged){
        Serial.println("switchValueChanged");

        uint8_t* data = pSwitchCharacteristic->getData();

        String hexString = convertToHexString(data, sizeof(data));
        Serial.println(hexString);

        state = data[0];

        switchValueChanged = false;
      }

      if(brightnessValueChanged){
        Serial.println("brightnessValueChanged");

        uint8_t* data = pBrightnessCharacteristic->getData();

        String hexString = convertToHexString(data, sizeof(data));
        Serial.println(hexString);
    
        brightness = data[0];

        brightnessValueChanged = false;
      }

      if(modeValueChanged){
        Serial.println("modeValueChanged");

        uint8_t* data = pModeCharacteristic->getData();

        String hexString = convertToHexString(data, sizeof(data));
        Serial.println(hexString);
    
        mode = data[0];

        modeValueChanged = false;
      }

      delay(3);
    }
    if(state == 0x01){
      switch (mode) {
        case 0x01:
          firstTime = true;
          analogWrite(LED_BUILTIN,brightness);
          break;
        case 0x02:
          if(firstTime){
            firstTime = false;
            breathBrightness = brightness;
          }
          if(breathBrightness == 0x00){
            lightUp = true;
          }else if(breathBrightness == 0xFF){
            lightUp = false;
          }
          if(lightUp){
            breathBrightness++;
          }else{
            breathBrightness--;
          }
          analogWrite(LED_BUILTIN,breathBrightness);
          break;
        default:
          break;
      }
    }else{
      digitalWrite(LED_BUILTIN, LOW);
      analogWrite(LED_BUILTIN,0);
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
}