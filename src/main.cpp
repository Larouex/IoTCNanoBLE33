/* ==========================================================================
    File:     main.cpp
    Author:   Larry W Jordan Jr (larouex@gmail.com)
    Purpose:  Arduino Nano BLE  33 example for Bluetooth Connectivity
              to IoT Gateway Device working with Azure IoT Central
    Online:   www.hackinmakin.com

    (c) 2020 Larouex Software Design LLC
    This code is licensed under MIT license (see LICENSE.txt for details)    
  ==========================================================================*/
#include <ArduinoBLE.h>
 
/* --------------------------------------------------------------------------
    Leds we manipulate for Status, etc.
   -------------------------------------------------------------------------- */
#define ONBOARD_LED 13
#define RED_LIGHT_PIN 22
#define GREEN_LIGHT_PIN 23
#define BLUE_LIGHT_PIN 24

/* --------------------------------------------------------------------------
    Previous Basttery Level Monitors
   -------------------------------------------------------------------------- */
int oldBatteryLevel = 0;  // last battery level reading from analog input
long previousMillis =  0;  // last time the battery level was checked, in ms



/* --------------------------------------------------------------------------
    BLE Service Characteristic - readable by the Gateway
   -------------------------------------------------------------------------- */
#define larouexServiceId "6F165338-BF34-43B9-837B-41B1A3C86EC1"
BLEService bleService(larouexServiceId); 
BLEFloatCharacteristic batteryChargedCharacteristic(larouexServiceId, BLERead);
BLEIntCharacteristic telemetryFrequencyCharacteristic(larouexServiceId, BLERead | BLEWrite);
int telemetryFrequency = 500;
bool isConnected = false;

/* --------------------------------------------------------------------------
    Function to set the RGB LED to the color of the battery charge
      * Green >=50%
      * Yellow <= 49% && >=10%
      * Red <=9%
   -------------------------------------------------------------------------- */
void SetBuiltInRGB(
  PinStatus RED_LIGHT_PIN_VALUE,
  PinStatus BLUE_LIGHT_PIN_VALUE,
  PinStatus GREEN_LIGHT_PIN_VALUE)
{
     digitalWrite(RED_LIGHT_PIN, RED_LIGHT_PIN_VALUE);
     digitalWrite(BLUE_LIGHT_PIN, BLUE_LIGHT_PIN_VALUE);
     digitalWrite(GREEN_LIGHT_PIN, GREEN_LIGHT_PIN_VALUE);    
    return;
}

void BatteryCheck(int level) {
  if (level >=7 )
  {
    Serial.println("BatteryCheck Set Green");
    SetBuiltInRGB(HIGH, HIGH, LOW);
  }
  else if (level >=3 and level <= 6 )
  {
    Serial.println("BatteryCheck Set Yellow");
    SetBuiltInRGB(HIGH, LOW, HIGH);
  }
  else
  {
    Serial.println("BatteryCheck Set Red");
    SetBuiltInRGB(LOW, HIGH, HIGH);
  }
  return;
}

/* --------------------------------------------------------------------------
    Read the current voltage level on the A0 analog input pin.
    This is used here to simulate the charge level of a battery.
   -------------------------------------------------------------------------- */
void UpdateBatteryLevel() {
  int battery = analogRead(A0);
  int batteryLevel = map(battery, 0, 1023, 0, 100);

  if (batteryLevel != oldBatteryLevel) {      // if the battery level has changed
    Serial.print("Battery Level % is now: "); // print it
    Serial.println(batteryLevel);
    batteryChargedCharacteristic.writeValue(batteryLevel);  // and update the battery level characteristic
    oldBatteryLevel = batteryLevel;           // save the level for next comparison
    BatteryCheck(batteryLevel);
  }
}

/* --------------------------------------------------------------------------
    Event Handler for Telemtery Frequency upadated from Central
   -------------------------------------------------------------------------- */
void telemetryFrequencyCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic) {
  SetBuiltInRGB(HIGH, LOW, LOW); // blue
  // central wrote new value to characteristic, update LED
  Serial.print("Characteristic event, written: ");
  Serial.printf("Telemetery Frequencey %d", telemetryFrequencyCharacteristic.value());
}

/* --------------------------------------------------------------------------
    Standard Sketch Setup
   -------------------------------------------------------------------------- */
void setup() {
  
  // Setup out battery LED
  pinMode(RED_LIGHT_PIN, OUTPUT);
  pinMode(GREEN_LIGHT_PIN, OUTPUT);
  pinMode(BLUE_LIGHT_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);


  Serial.begin(9600);    // initialize serial communication
  while (!Serial);

  // begin initialization
  if (!BLE.begin()) {
    Serial.println("Starting BLE Failed!");
    while (1);
  }
  
  /* 
    Set a local name for the BLE device
    This name will appear in advertising packets
    and can be used by remote devices to identify this BLE device
    The name can be changed but maybe be truncated based on space left in advertisement packet
  */
  BLE.setLocalName("larouexble33");
  BLE.setDeviceName("larouexiotcble33");
  BLE.setAdvertisedService(bleService);

  // add the services
  bleService.addCharacteristic(batteryChargedCharacteristic); 
  bleService.addCharacteristic(telemetryFrequencyCharacteristic); 
  BLE.addService(bleService);

  // set defaults
  batteryChargedCharacteristic.writeValue(oldBatteryLevel);
  telemetryFrequencyCharacteristic.writeValue(telemetryFrequency);

  // assign event handler for Frequency Updates
  telemetryFrequencyCharacteristic.setEventHandler(BLEWritten, telemetryFrequencyCharacteristicWritten);

  SetBuiltInRGB(HIGH, HIGH, HIGH);
  digitalWrite(LED_BUILTIN, LOW);

  /* Start advertising BLE.  It will start continuously transmitting BLE
     advertising packets and will be visible to remote BLE central devices
     until it receives a new connection */
  BLE.advertise();

  Serial.println("Bluetooth device active, waiting for connections...");

}

/* --------------------------------------------------------------------------
    Standard Sketch Loop
   -------------------------------------------------------------------------- */
void loop() {

  // listen for BLE peripherals to connect:
  BLEDevice central = BLE.central();
  SetBuiltInRGB(HIGH, LOW, HIGH); // blue

  // if a central is connected to peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());

    // while the central is still connected to peripheral:
    while (central.connected()) {
      long currentMillis = millis();
      if (currentMillis - previousMillis >= 500) {
        previousMillis = currentMillis;
        UpdateBatteryLevel();
      }
    }

    // when the central disconnects, print it out:
    Serial.print(F("Disconnected from central: "));
    Serial.println(central.address());
  }
}

