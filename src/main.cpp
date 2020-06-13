/* ==========================================================================
    File:     main.cpp
    Author:   Larry W Jordan Jr (larouex@gmail.com)
    Purpose:  Arduino Nano BLE 33 example for Bluetooth Connectivity
              to IoT Gateway Device working with Azure IoT Central
    Online:   www.hackinmakin.com

    (c) 2020 Larouex Software Design LLC
    This code is licensed under MIT license (see LICENSE.txt for details)    
  ==========================================================================*/
#include <Arduino_LSM9DS1.h>
#include <MadgwickAHRS.h>
#include <string>
#include <ArduinoBLE.h>

// MACROS for Reading Build Flags
#define XSTR(x) #x
#define STR(x) XSTR(x)

/* --------------------------------------------------------------------------
    We use this to delay the start when the BLE Central application/gateway
    is interogating the device...
   -------------------------------------------------------------------------- */
unsigned long bleStartDelay   = 0;
bool          bleDelayActive  = false;

/* --------------------------------------------------------------------------
    Frequency of Simulation on Battery Level
   -------------------------------------------------------------------------- */
unsigned long   telemetryStartDelay     = 0;
bool            telemetryDelayActive    = false;
unsigned long   telemetryFrequency       = 1500;

/* --------------------------------------------------------------------------
    Configure IMU
   -------------------------------------------------------------------------- */
const int IMU_HZ = 119;
Madgwick filter;
unsigned long msecsPerReading, msecsPrevious;

/* --------------------------------------------------------------------------
    Leds we manipulate for Status, etc.
   -------------------------------------------------------------------------- */
#define ONBOARD_LED     13
#define RED_LIGHT_PIN   22
#define GREEN_LIGHT_PIN 23
#define BLUE_LIGHT_PIN  24

/* --------------------------------------------------------------------------
    Characteristic Mappers
   -------------------------------------------------------------------------- */
const int VERSION_CHARACTERISTIC            = 1;
const int BATTERYCHARGED_CHARACTERISTIC     = 2;
const int TELEMETRYFREQUENCY_CHARACTERISTIC = 3;
const int ACCELEROMETER_CHARACTERISTIC      = 4;
const int GYROSCOPE_CHARACTERISTIC          = 5;
const int MAGNETOMETER_CHARACTERISTIC       = 6;
const int ORIENTATION_CHARACTERISTIC        = 7;
const int RGBLED_CHARACTERISTIC             = 8;

/* --------------------------------------------------------------------------
    Previous Battery Level Monitors
   -------------------------------------------------------------------------- */
int oldBatteryLevel = 0;

/* --------------------------------------------------------------------------
    Broadcast Version for BLE Device
   -------------------------------------------------------------------------- */
const unsigned char SEMANTIC_VERSION[14] = STR(VERSION);

/* --------------------------------------------------------------------------
    BLE Service Definition
   -------------------------------------------------------------------------- */
#define LAROUEX_BLE_SERVICE_UUID(val) ("6F165338-" val "-43B9-837B-41B1A3C86EC1")
BLEService blePeripheral(LAROUEX_BLE_SERVICE_UUID("0000")); 

// ************************* BEGIN CHARACTERISTICS **************************

/* --------------------------------------------------------------------------
    BLE Peripheral Characteristics - Readable by Central/Gateway
   -------------------------------------------------------------------------- */
// Version
BLECharacteristic       versionCharacteristic("1001", BLERead | BLEWrite, sizeof(SEMANTIC_VERSION));
BLEDescriptor           versionCharacteristicDesc ("1002", "Version");

// Battery Charged
BLEFloatCharacteristic  batteryChargedCharacteristic("2001", BLERead | BLENotify | BLEIndicate);
BLEDescriptor           batteryChargedCharacteristicDesc ("2002", "Battery Charged");

// Telemetry Frequency
BLEIntCharacteristic    telemetryFrequencyCharacteristic("3001", BLERead | BLEWrite | BLENotify | BLEIndicate);
BLEDescriptor           telemetryFrequencyCharacteristicDesc ("3002", "Telemetry Frequency");

// Accelerometer
BLECharacteristic       accelerometerCharacteristic("4001", BLENotify, 3 * sizeof(float));
BLEDescriptor           accelerometerCharacteristicDesc ("4002", "Accelerometer");

// Gyroscope
BLECharacteristic       gyroscopeCharacteristic("5001", BLENotify, 3 * sizeof(float));
BLEDescriptor           gyroscopeCharacteristicDesc ("5002", "Gyroscope");

// Magnetometer
BLECharacteristic       magnetometerCharacteristic("6001", BLENotify, 3 * sizeof(float));
BLEDescriptor           magnetometerCharacteristicDesc ("6002", "Magnetometer");

// Orientation
BLECharacteristic       orientationCharacteristic("7001", BLENotify, 3 * sizeof(float));
BLEDescriptor           orientationCharacteristicDesc ("7002", "Orientation");

// RGB Led
BLECharacteristic       rgbLedCharacteristic("8001", BLERead | BLEWrite, 3 * sizeof(byte));
BLEDescriptor           rgbLedCharacteristicDesc ("8002", "RGB Led");

// ************************** END CHARACTERISTICS ***************************

/* --------------------------------------------------------------------------
    Function to set the onboard Nano RGB LED
   -------------------------------------------------------------------------- */
void SetBuiltInRGB(
  PinStatus redLightPinValue,
  PinStatus blueLightPinValue,
  PinStatus greenLightPinValue)
{
     digitalWrite(RED_LIGHT_PIN, redLightPinValue);
     digitalWrite(BLUE_LIGHT_PIN, blueLightPinValue);
     digitalWrite(GREEN_LIGHT_PIN, greenLightPinValue);    
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
    Function to read the following IMU capabilities
      Accelerometer
      Gyroscope
      MagneticField
      Orientation
   -------------------------------------------------------------------------- */
void UpdateIMU() {
  
  float acceleration[3];
  float gyroDPS[3];
  float magneticField[3];

  if ((orientationCharacteristic.subscribed() || accelerometerCharacteristic.subscribed()) && IMU.accelerationAvailable()) {
    
    // read the Accelerometer
    float x, y, z;
    IMU.readAcceleration(x, y, z);
    acceleration[0] = x;
    acceleration[1] = y;
    acceleration[2] = z;

    if (accelerometerCharacteristic.subscribed()) {
      accelerometerCharacteristic.writeValue(acceleration, sizeof(acceleration));
      
      Serial.print("[IMU] Acceleration(X): ");
      Serial.println(acceleration[0]);
      Serial.print("[IMU] Acceleration(Y): ");
      Serial.println(acceleration[1]);
      Serial.print("[IMU] Acceleration(Z): ");
      Serial.println(acceleration[2]);
    }
  }

  #ifdef DEBUG
    if (!orientationCharacteristic.subscribed())
    {
      Serial.println("[IMU] Please Subscribe to Orientation & Acceleration for Notifications");
    }
  #endif


  if ((orientationCharacteristic.subscribed() || gyroscopeCharacteristic.subscribed()) && IMU.gyroscopeAvailable()) {
    
    // read the Gyro
    float x, y, z;
    IMU.readGyroscope(x, y, z);
    gyroDPS[0] = x;
    gyroDPS[1] = y;
    gyroDPS[2] = z;

    if (gyroscopeCharacteristic.subscribed()) {
      gyroscopeCharacteristic.writeValue(gyroDPS, sizeof(gyroDPS));

      Serial.print("[IMU] Gyroscope(X): ");
      Serial.println(gyroDPS[0]);
      Serial.print("[IMU] Gyroscope(Y): ");
      Serial.println(gyroDPS[1]);
      Serial.print("[IMU] Gyroscope(Z): ");
      Serial.println(gyroDPS[2]);
    }
  }

  #ifdef DEBUG
    if (!orientationCharacteristic.subscribed())
    {
      Serial.println("[IMU] Please Subscribe to Orientation & Gyroscope for Notifications");
    }
  #endif

  if ((orientationCharacteristic.subscribed() || magnetometerCharacteristic.subscribed()) && IMU.magneticFieldAvailable()) {
    
    // read the Mag
    float x, y, z;
    IMU.readMagneticField(x, y, z);
    magneticField[0] = x;
    magneticField[1] = y;
    magneticField[2] = z;

    if (magnetometerCharacteristic.subscribed()) {
      magnetometerCharacteristic.writeValue(magneticField, sizeof(magneticField));
      
      Serial.print("[IMU] Magnetometer(X): ");
      Serial.println(magneticField[0]);
      Serial.print("[IMU] Magnetometer(Y): ");
      Serial.println(magneticField[1]);
      Serial.print("[IMU] Magnetometer(Z): ");
      Serial.println(magneticField[2]);
    }
  }

  #ifdef DEBUG
    if (!orientationCharacteristic.subscribed() && !magnetometerCharacteristic.subscribed())
    {
      Serial.println("[IMU] Please Subscribe to Magnetometer & Orientation for Notifications");
    }
  #endif

  if (orientationCharacteristic.subscribed() && (micros() - msecsPrevious >= msecsPerReading)) {
    
    float heading, pitch, roll;
    
    // Update and compute orientation
    filter.update(
      gyroDPS[0], gyroDPS[1], gyroDPS[2],
      acceleration[0], acceleration[1], acceleration[2],
      magneticField[0], magneticField[1], magneticField[2]
    );

    heading = filter.getYawRadians();
    pitch = filter.getPitchRadians();
    roll = filter.getRollRadians();
    
    float orientation[3] = { heading, pitch, roll };
    orientationCharacteristic.writeValue(orientation, sizeof(orientation));

    Serial.print("[IMU] Orientation(heading): ");
    Serial.println(heading);
    Serial.print("[IMU] Orientation(pitch): ");
    Serial.println(pitch);
    Serial.print("[IMU] Orientation(roll): ");
    Serial.println(roll);

    msecsPrevious = msecsPrevious + msecsPerReading;
  }

  #ifdef DEBUG
    if (!orientationCharacteristic.subscribed())
    {
      Serial.println("[IMU] Please Subscribe to Orientation for Notifications");
    }
  #endif

}

/* --------------------------------------------------------------------------
    Read the current voltage level on the A0 analog input pin.
    This is used here to simulate the charge level of a battery.
   -------------------------------------------------------------------------- */
void UpdateBatteryLevel() {
  
  int batteryLevel = 1 + rand() % 10;

  // only if the battery level has changed
  if (batteryLevel != oldBatteryLevel) {      
    Serial.print("Battery Level % is now: ");
    Serial.println(batteryLevel);
    // and update the battery level characteristic to BLE
    batteryChargedCharacteristic.writeValue(batteryLevel);
    oldBatteryLevel = batteryLevel;
    BatteryCheck(batteryLevel);
  }

  // Enable a Pause to Show this Write
  telemetryStartDelay = millis();
  telemetryDelayActive = true;
}

// ************************* BEGIN EVENT HANDLERS ***************************

/* --------------------------------------------------------------------------
    TELEMETRY_FREQUENCY (write) Event Handler from Central/Gateway
   -------------------------------------------------------------------------- */
void onTelemetryFrequencyCharacteristicWrite(BLEDevice central, BLECharacteristic characteristic) {
  
  unsigned long val = telemetryFrequencyCharacteristic.value();
  
  if (val >= 500 && val <= 30000) {
    telemetryFrequency = val;
    Serial.print("[EVENT] telemetryFrequency: ");
    Serial.println(telemetryFrequency);
  } else {
    Serial.print("[EVENT] telemetryFrequency MUST BE BETWEEN 500 AND 30000: ");
    Serial.println(telemetryFrequency);
  }

}

/* --------------------------------------------------------------------------
    RGBLED_CHARACTERISTIC Event Handler from Central/Gateway
   -------------------------------------------------------------------------- */
void onRgbLedCharacteristicWrite(BLEDevice central, BLECharacteristic characteristic) {
  
  // Enable a Pause to Show this Write
  bleStartDelay = millis();
  bleDelayActive = true;

  // parse the array for LED Set
  PinStatus r = rgbLedCharacteristic[0] == 0 ? LOW : HIGH;
  PinStatus g = rgbLedCharacteristic[1] == 0 ? LOW : HIGH;
  PinStatus b = rgbLedCharacteristic[2] == 0 ? LOW : HIGH;
  
  SetBuiltInRGB(r, g, b);
  
  // update the serial port with the values captured
  Serial.print("[EVENT] RGBLED_CHARACTERISTIC (RED): ");
  Serial.println(r);
  Serial.print("[EVENT] RGBLED_CHARACTERISTIC (GREEN): ");
  Serial.println(g);
  Serial.print("[EVENT] RGBLED_CHARACTERISTIC (BLUE): ");
  Serial.println(b);

}

// ************************** END EVENT HANDLERS ****************************

/* --------------------------------------------------------------------------
    This Function acts as a positive indicater (true) when the 
    characteristic is setup properly
   -------------------------------------------------------------------------- */
bool SetUpCharacteristic(int whichCharacteristic)
{
  bool result = false;
  switch (whichCharacteristic) {

    // add the Characteristic - VERSION
    case VERSION_CHARACTERISTIC:
      blePeripheral.addCharacteristic(versionCharacteristic); 
      versionCharacteristic.addDescriptor(versionCharacteristicDesc);
      versionCharacteristic.setValue(SEMANTIC_VERSION, 14);
      result = true;
      break;

    // add the Characteristic - BATTERY CHARGED
    case BATTERYCHARGED_CHARACTERISTIC:
      blePeripheral.addCharacteristic(batteryChargedCharacteristic); 
      batteryChargedCharacteristic.addDescriptor(batteryChargedCharacteristicDesc);
      batteryChargedCharacteristic.writeValue(oldBatteryLevel);
      batteryChargedCharacteristic.broadcast();
      result = true;
      break;

    // add the Characteristic - TELEMETRY FREQUENCY
    case TELEMETRYFREQUENCY_CHARACTERISTIC:
      blePeripheral.addCharacteristic(telemetryFrequencyCharacteristic); 
      telemetryFrequencyCharacteristic.addDescriptor(telemetryFrequencyCharacteristicDesc);
      telemetryFrequencyCharacteristic.setValue(telemetryFrequency);
      telemetryFrequencyCharacteristic.setEventHandler(BLEWritten, onTelemetryFrequencyCharacteristicWrite);
      result = true;
      break;
  
    // add the Characteristic - ACCELEROMETER
    case ACCELEROMETER_CHARACTERISTIC:
      blePeripheral.addCharacteristic(accelerometerCharacteristic); 
      accelerometerCharacteristic.addDescriptor(accelerometerCharacteristicDesc);
      result = true;
      break;

    // add the Characteristic - GYROSCOPE
    case GYROSCOPE_CHARACTERISTIC:
      blePeripheral.addCharacteristic(gyroscopeCharacteristic); 
      gyroscopeCharacteristic.addDescriptor(gyroscopeCharacteristicDesc);
      result = true;
      break;

    // add the Characteristic - MAGNETOMETER
    case MAGNETOMETER_CHARACTERISTIC:
      blePeripheral.addCharacteristic(magnetometerCharacteristic); 
      magnetometerCharacteristic.addDescriptor(magnetometerCharacteristicDesc);
      result = true;
      break;

    // add the Characteristic - ORIENTATION
    case ORIENTATION_CHARACTERISTIC:
      blePeripheral.addCharacteristic(orientationCharacteristic); 
      orientationCharacteristic.addDescriptor(orientationCharacteristicDesc);
      result = true;
      break;

    // add the Characteristic - RGB LED
    case RGBLED_CHARACTERISTIC:
      blePeripheral.addCharacteristic(rgbLedCharacteristic); 
      rgbLedCharacteristic.addDescriptor(rgbLedCharacteristicDesc);
      rgbLedCharacteristic.setEventHandler(BLEWritten, onRgbLedCharacteristicWrite);
      result = true;
      break;

    default:
      break;
  }
  return result;

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

  // initialize serial communication
  Serial.begin(9600);    
  while (!Serial);
    
  // begin IMU initialization
  if (!IMU.begin()) {
    Serial.println("[FAILED] Starting IMU");
    while (1);
  } else {
    Serial.println("[SUCCESS] Starting IMU");
    
    // start the MadgwickAHRS filter to run at the IMU sample rate
    filter.begin(IMU_HZ);
    msecsPerReading = 1000000 / IMU_HZ;
    msecsPrevious = micros();
  }

  // begin BLE initialization
  if (!BLE.begin()) {
    Serial.println("[FAILED] Starting BLE");
    while (1);
  } else {
    Serial.println("[SUCCESS] Starting BLE");
  }
  
  // Setup the Characteristics
  if (!SetUpCharacteristic(VERSION_CHARACTERISTIC)) {
    Serial.println("[FAILED] SetUpCharacteristic->VERSION_CHARACTERISTIC");
  } else {
    Serial.println("[SUCCESS] SetUpCharacteristic->VERSION_CHARACTERISTIC");
  }
  
  if (!SetUpCharacteristic(BATTERYCHARGED_CHARACTERISTIC)) {
    Serial.println("[FAILED] SetUpCharacteristic->BATTERYCHARGED_CHARACTERISTIC");
  } else {
    Serial.println("[SUCCESS] SetUpCharacteristic->BATTERYCHARGED_CHARACTERISTIC");
  }

  if (!SetUpCharacteristic(TELEMETRYFREQUENCY_CHARACTERISTIC)) {
    Serial.println("[FAILED] SetUpCharacteristic->TELEMETRYFREQUENCY_CHARACTERISTIC");
  } else {
    Serial.println("[SUCCESS] SetUpCharacteristic->TELEMETRYFREQUENCY_CHARACTERISTIC");
  }

  if (!SetUpCharacteristic(ACCELEROMETER_CHARACTERISTIC)) {
    Serial.println("[FAILED] SetUpCharacteristic->ACCELEROMETER_CHARACTERISTIC");
  } else {
    Serial.println("[SUCCESS] SetUpCharacteristic->ACCELEROMETER_CHARACTERISTIC");
  }

  if (!SetUpCharacteristic(GYROSCOPE_CHARACTERISTIC)) {
    Serial.println("[FAILED] SetUpCharacteristic->GYROSCOPE_CHARACTERISTIC");
  } else {
    Serial.println("[SUCCESS] SetUpCharacteristic->GYROSCOPE_CHARACTERISTIC");
  }

  if (!SetUpCharacteristic(MAGNETOMETER_CHARACTERISTIC)) {
    Serial.println("[FAILED] SetUpCharacteristic->MAGNETOMETER_CHARACTERISTIC");
  } else {
    Serial.println("[SUCCESS] SetUpCharacteristic->MAGNETOMETER_CHARACTERISTIC");
  }

  if (!SetUpCharacteristic(ORIENTATION_CHARACTERISTIC)) {
    Serial.println("[FAILED] SetUpCharacteristic->ORIENTATION_CHARACTERISTIC");
  } else {
    Serial.println("[SUCCESS] SetUpCharacteristic->ORIENTATION_CHARACTERISTIC");
  }

  if (!SetUpCharacteristic(RGBLED_CHARACTERISTIC)) {
    Serial.println("[FAILED] SetUpCharacteristic->RGBLED_CHARACTERISTIC");
  } else {
    Serial.println("[SUCCESS] SetUpCharacteristic->RGBLED_CHARACTERISTIC");
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
  BLE.addService(blePeripheral);

    /* Start advertising BLE.  It will start continuously transmitting BLE
     advertising packets and will be visible to remote BLE central devices
     until it receives a new connection */
  BLE.advertise();

  // Set leds to idle
  digitalWrite(LED_BUILTIN, LOW);
  SetBuiltInRGB(LOW, LOW, LOW);

  Serial.println("[READY] Bluetooth device active, waiting for connections...");
}

/* --------------------------------------------------------------------------
    Standard Sketch Loop
   -------------------------------------------------------------------------- */
void loop() {

  // listen for BLE peripherals to connect:
  BLEDevice central = BLE.central();

  // if a central is connected to peripheral:
  if (central) {

    bleStartDelay = millis();
    bleDelayActive = true;
    telemetryStartDelay = millis();
    telemetryDelayActive = true;

    digitalWrite(ONBOARD_LED, HIGH);

    // print the central's MAC address:
    Serial.print("[STARTED] Connected to central: ");
    Serial.println(central.address());

    // while the central is still connected to peripheral:
    while (central.connected()) {
      
      if (bleDelayActive && ((millis() - telemetryStartDelay) >= 10000)) {
        bleDelayActive = false;
      }

      if (!bleDelayActive) {
        if (telemetryDelayActive && ((millis() - telemetryStartDelay) >= telemetryFrequency)) {
          UpdateBatteryLevel();
          UpdateIMU();
        }
      }

    }

    // when the central disconnects, print it out:
    digitalWrite(ONBOARD_LED, LOW);
    SetBuiltInRGB(LOW, LOW, LOW);

    Serial.print(F("[STOPPED] Disconnected from central: "));
    Serial.println(central.address());
  }
}

