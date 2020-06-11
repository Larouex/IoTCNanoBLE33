/* ==========================================================================
    File:     main.cpp
    Author:   Larry W Jordan Jr (larouex@gmail.com)
    Purpose:  Arduino Nano BLE 33 example for Bluetooth Connectivity
              to IoT Gateway Device working with Azure IoT Central
    Online:   www.hackinmakin.com

    (c) 2020 Larouex Software Design LLC
    This code is licensed under MIT license (see LICENSE.txt for details)    
  ==========================================================================*/
#include <ArduinoBLE.h>
 
/* --------------------------------------------------------------------------
    Frequency of Simulation on Battery Level
   -------------------------------------------------------------------------- */
int TELEMETRY_FREQUENCY = 500;

/* --------------------------------------------------------------------------
    Leds we manipulate for Status, etc.
   -------------------------------------------------------------------------- */
#define ONBOARD_LED     13
#define RED_LIGHT_PIN   22
#define GREEN_LIGHT_PIN 23
#define BLUE_LIGHT_PIN  24

/* --------------------------------------------------------------------------
    Previous Battery Level Monitors
   -------------------------------------------------------------------------- */
int OLD_BATTERY_LEVEL = 0;  // last battery level reading from analog input

/* --------------------------------------------------------------------------
    Broadcast Version for BLE Device
   -------------------------------------------------------------------------- */
const int VERSION = 0x00000001;

/* --------------------------------------------------------------------------
    BLE Service Definition
   -------------------------------------------------------------------------- */
#define LAROUEX_BLE_SERVICE_UUID(val) ("6F165338-" val "-43B9-837B-41B1A3C86EC1")
BLEService blePeripheral(LAROUEX_BLE_SERVICE_UUID("0000")); 

/* --------------------------------------------------------------------------
    BLE Service Characteristics - Readable by the Gateway
   -------------------------------------------------------------------------- */
BLEUnsignedIntCharacteristic  versionCharacteristic(LAROUEX_BLE_SERVICE_UUID("1001"), BLERead);
BLEFloatCharacteristic        batteryChargedCharacteristic(LAROUEX_BLE_SERVICE_UUID("2001"), BLERead | BLENotify | BLEIndicate);
BLEIntCharacteristic          telemetryFrequencyCharacteristic(LAROUEX_BLE_SERVICE_UUID("3001"), BLERead | BLEWrite| BLENotify | BLEIndicate);

BLEDescriptor versionCharacteristicDesc (LAROUEX_BLE_SERVICE_UUID("1002"), "Version");
BLEDescriptor batteryChargedCharacteristicDesc (LAROUEX_BLE_SERVICE_UUID("2002"), "Battery Charged");
BLEDescriptor telemetryFrequencyCharacteristicDesc (LAROUEX_BLE_SERVICE_UUID("3002"), "Telemetry Frequency");

/* --------------------------------------------------------------------------
    Function to set the onboard Nano RGB LED
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

/* --------------------------------------------------------------------------
    Function to set the RGB LED to the color of the battery charge
      * Green >=50%
      * Yellow <= 49% && >=20%
      * Red <=19%
   -------------------------------------------------------------------------- */
void BatteryCheck(int level) {
  if (level >=5 )
  {
    Serial.println("BatteryCheck Set Green");
    SetBuiltInRGB(HIGH, HIGH, LOW);
  }
  else if (level >=2 and level <= 4 )
  {
    Serial.println("BatteryCheck Set Yellow");
    SetBuiltInRGB(LOW, HIGH, LOW);
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
  
  int batteryLevel = 1 + rand() % 10;

  // only if the battery level has changed
  if (batteryLevel != OLD_BATTERY_LEVEL) {      
    Serial.print("Battery Level % is now: ");
    Serial.println(batteryLevel);
    // and update the battery level characteristic to BLE
    batteryChargedCharacteristic.writeValue(batteryLevel);
    OLD_BATTERY_LEVEL = batteryLevel;
    BatteryCheck(batteryLevel);
  }

  delay(TELEMETRY_FREQUENCY);
}

/* --------------------------------------------------------------------------
    Event Handler for Telemetery Frequency upadated from Central
   -------------------------------------------------------------------------- */
void telemetryFrequencyCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic) {
  SetBuiltInRGB(HIGH, LOW, LOW); // blue
  delay(1000);
  Serial.print("Characteristic event - Telemetery Frequencey:");
  Serial.println(TELEMETRY_FREQUENCY);
  TELEMETRY_FREQUENCY = telemetryFrequencyCharacteristic.value();
}

/* --------------------------------------------------------------------------
    Standard Sketch Setup
   -------------------------------------------------------------------------- */
void setup() {
  
  // Setup our Pins
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
  BLE.setLocalName("LarouexBLE");
  BLE.setDeviceName("Larouex BLE Device 001");
  BLE.setAdvertisedService(blePeripheral);

  // add the services
  blePeripheral.addCharacteristic(versionCharacteristic); 
  versionCharacteristic.addDescriptor(versionCharacteristicDesc);
  versionCharacteristic.setValue(VERSION);
    
  blePeripheral.addCharacteristic(batteryChargedCharacteristic); 
  batteryChargedCharacteristic.addDescriptor(batteryChargedCharacteristicDesc);
  batteryChargedCharacteristic.writeValue(OLD_BATTERY_LEVEL);
  batteryChargedCharacteristic.broadcast();

  blePeripheral.addCharacteristic(telemetryFrequencyCharacteristic); 
  telemetryFrequencyCharacteristic.addDescriptor(telemetryFrequencyCharacteristicDesc);
  telemetryFrequencyCharacteristic.setValue(TELEMETRY_FREQUENCY);
  
  BLE.addService(blePeripheral);

  // assign event handler for Frequency Updates
  telemetryFrequencyCharacteristic.setEventHandler(BLEWritten, telemetryFrequencyCharacteristicWritten);

  /* Start advertising BLE.  It will start continuously transmitting BLE
     advertising packets and will be visible to remote BLE central devices
     until it receives a new connection */
  BLE.advertise();

  // Set leds to idle
  digitalWrite(LED_BUILTIN, LOW);
  SetBuiltInRGB(LOW, LOW, LOW);

  Serial.println("Bluetooth device active, waiting for connections...");
}

/* --------------------------------------------------------------------------
    Standard Sketch Loop
   -------------------------------------------------------------------------- */
void loop() {

  // listen for BLE peripherals to connect:
  BLEDevice central = BLE.central();

  // if a central is connected to peripheral:
  if (central) {
    
    digitalWrite(ONBOARD_LED, HIGH);
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());

    // while the central is still connected to peripheral:
    while (central.connected()) {
      UpdateBatteryLevel();
    }

    // when the central disconnects, print it out:
    digitalWrite(ONBOARD_LED, LOW);
    Serial.print(F("Disconnected from central: "));
    Serial.println(central.address());
  }
}

