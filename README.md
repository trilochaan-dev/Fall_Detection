# Fall Detection System 
## Overview
A system that detects falls in real time and sends alerts for rapid emergency response.
Designed to improve safety and reduce response time in critical situations.
## Features
- Real time Fall Detection using accelerometer, gyroscope and pulse sensor.
- Reduced fall detection with the use of pulse sensor.
- customizable threshold.
## Hardware Requirement
- ESP8266
- MPU6050 (accelerometer and gyroscope)
- Pulse sensor
- pumper wires
## Software Requirement
- Arduino IDE
- Embedded C / Arduino Programming Language
  - Wire Library (I2C communication)
  - MPU6050 Library
  - Pulse Sensor Library
  - ESP Mail Client
## Installation
1. Open the project code in Arduino IDE / PlatformIO
2. Install required libraries (if not already installed):
   - MPU6050
   - Pulse Sensor
   - Blynk (if used)
3. Connect the microcontroller (Arduino / ESP8266) via USB
4. Select the correct board from Tools → Board
5. Select the correct port from Tools → Port
6. Upload the code to the device
7. (Optional) Configure IoT settings (SMPT)
## 



