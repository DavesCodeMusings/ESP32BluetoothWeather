# ESP32BluetoothWeather
Periodically poll a BME280 temperature / humidity / pressure sensor and make the reading available over Bluetooth Low Energy.

## What is it?
This is a sketch for reading temperature, humidity, and barometric pressure and making the data available over Bluetooth Low Energy. It uses a Bosch BME280 sensor attached to an ESP32 board via I2C. My plan is to stick it outside in a sheltered location and feed data about outside weather into my home automation system.

## Why should I care?
My first attempt at an outdoor weather sensor involved an ESP8266 and the same BME280 sensor. It connected to my home's WiFi and transmitted data to the home automation system via an MQTT server. It works great, but there's a security flaw. A device with my network's WiFi credentials is sitting outside where anyone could grab it. If the Bluetooth device gets nabbed, there's only the loss of hardware, and not a free pass into my network.

If that's not enough, the sketch is an introduction to Bluetooth on the ESP32 that goes a little beyond, "Look Ma! I can read a GATT characteristic on my smart phone." It also offers some insight into how to deal with sensor self-heating. (And it's not by subtracting 4 degrees Celsius.)

## How does it work?
The `setup()` function configures a Bluetooth Environmental Sensing service with characteristics for pressure, temperature, and humidity, with notifications. It also sets the BME280 for a forced read mode. This is what keeps the sensor from self-heating so much, mitigating the need to factor in a few degrees of error correction in the temperature.

The `loop()` function reads the BME280 data every so often and updates the BLE GATT characteristics for pressure, temperature, and humidity. It also takes care of notifying if there is a client connected.

It looks like this on the serial debug:
```
Reading BME280.
  Pressure: 98249.46Pa
  Temperature: 24.75C
  Humidity: 58.00%
Updating GATT characteristics.
  0x2A6D: 982495
  0x2A6E: 2475
  0x2A6F: 5800
```

Anything that can understand Bluetooth Low Energy GATT charcteristics can read the sensor data.
