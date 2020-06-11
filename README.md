# Arduino Nana BLE 33 for Azure IoT Central

## Overview
This repository is part of a training and project series for Azure IoT Central. The name of the series is "Raspberry Pi Gateway and Arduino Nano BLE Devices for Azure Iot Central" and is located at...

[LINK: Training & Project Site for Raspberry Pi Gateway and Arduino Nano BLE Devices for Azure Iot Central](http://www.hackinmakin.com/Raspberry%20Pi%20Gateway%20and%20BLE/index.html)


### Arduino Nano 33 BLE
![alt text](./Assets/nano-ble-33.jpg "Arduino Nano 33 BLE") 

The Arduino Nano 33 BLE is an evolution of the traditional Arduino Nano, but featuring a lot more powerful processor, the 
nRF52840 from Nordic Semiconductors, a 32-bit ARM® Cortex™-M4 CPU running at 64 MHz. This will allow you to make larger programs 
than with the Arduino Uno (it has 1MB of program memory, 32 times bigger), and with a lot more variables (the RAM is 128 times bigger). 
The main processor includes other amazing features like Bluetooth® pairing via NFC and ultra low power consumption modes. The Nano 
33 BLE comes with a 9 axis inertial measurement unit (IMU) which means that it includes an accelerometer, a gyroscope, and a 
magnetometer with 3-axis resolution each. This makes the Nano 33 BLE the perfect choice for more advanced robotics experiments, exercise trackers, digital compasses, etc.

The communications chipset on the Nano 33 BLE can be both a BLE and Bluetooth® client and host device. Something pretty unique in the world of 
microcontroller platforms. If you want to see how easy it is to create a Bluetooth® central or a peripheral device.

### Arduino Nano BLE 33 - PINOUT
![alt text](./Assets/nano33blepinout.png "Arduino Nano 33 BLE Pinout") 

## Project Details and Settign up Your Development Toolchain
The code in this repository depends on Ardunio, Visual Studio Code and PlatformIO.

### Your Local Machine
The development "toolchain" refers to all of the various tools, SDK's and bits we need to install on your machine to facilitate a smooth experience 
developing our BLE devices and the Raspberry Pi Gateway device. Our main development tool will be Visual Studio code. It has dependencies on tools 
from Arduino and other open source projects, but it will be the central place where all our development will occur making it easy to follow along 
regardless of which operating system you are working on.

![alt text](./Assets/nano33blepinout.png "Arduino Nano 33 BLE Pinout") 

![Some Title](./Assets/vs-code-icon.png){:style="float: right;margin-right: 7px;margin-top: 7px;"}
#### Install Visual Studio Code
[LINK: Visual Studio Code Installation Page](https://code.visualstudio.com/download)
Visual Studio Code is a lightweight but powerful source code editor which runs on your desktop and is available for Windows, macOS and Linux. This is the IDE we will use to write code and deploy to the our BLE Devices and the Raspberry Pi Gateway. 