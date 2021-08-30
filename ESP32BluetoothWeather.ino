/**
 * Periodically poll a BME280 temperature / humidity / pressure sensor and make the reading available over Bluetooth Low Energy.
 */

#include <Adafruit_BME280.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define BME_ADDRESS 0x76  // I2C address of BME280 sensor.
#define DEVICE_NAME "ESP32Dev"  // This determines what appears in a Bluetooth Low Energy (BLE) scan.

Adafruit_BME280 bme;
BLEServer *server;
BLEService *service;
BLECharacteristic *pressureCharacteristic;
BLECharacteristic *temperatureCharacteristic;
BLECharacteristic *humidityCharacteristic;

/** 
 * Handle client connect and disconnect events. 
 */
class ConnectionCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* server) {
    Serial.println("\nClient connected.");
  };

  void onDisconnect(BLEServer* server) {
    Serial.println("\nClient disconnected.");
    BLEDevice::startAdvertising();  // Restart advertising for the next client.
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println();

  if (!bme.begin(BME_ADDRESS)) {
    Serial.print("No BME280 found at I2C address: 0x");
    Serial.println(BME_ADDRESS, HEX);
  }
  else {
    // Set up BME280 for "weather station mode" (See data sheet for definition.)
    bme.setSampling(Adafruit_BME280::MODE_FORCED, Adafruit_BME280::SAMPLING_X1, Adafruit_BME280::SAMPLING_X1, Adafruit_BME280::SAMPLING_X1, Adafruit_BME280::FILTER_OFF);
  }

  // Set up Bluetooth Low Energy for an Environmental Sensing profile with pressure, temperature, and humidity characteristics.
  BLEDevice::init(DEVICE_NAME);
  server = BLEDevice::createServer();
  server->setCallbacks(new ConnectionCallbacks());
  service = server->createService("181A");  // 0x181A is Environmental Sensing (temperature, humdity, pressure, etc.)
  pressureCharacteristic = service->createCharacteristic("2A6D", BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pressureCharacteristic->addDescriptor(new BLE2902());  // This is required for notifications to work. 
  temperatureCharacteristic = service->createCharacteristic("2A6E", BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  temperatureCharacteristic->addDescriptor(new BLE2902());
  humidityCharacteristic = service->createCharacteristic("2A6F", BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  humidityCharacteristic->addDescriptor(new BLE2902());

  // Get BLE in a state so it can be discovered in scans.
  service->start();
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID("181A");  // Let's the client know this is an Environmental Sensing device.
  advertising->setScanResponse(true);
  Serial.print("BLE advertising started for: ");
  Serial.println(DEVICE_NAME);
  BLEDevice::startAdvertising();  // This is needed in setup() to ensure the device appears in scans.
}

void loop() {
  Serial.println();
  Serial.println("Reading BME280.");
  bme.takeForcedMeasurement();  // Take a reading and then go back to sleep. This mitigates self-heating that causes temperature readings to be too high.
  float pressure = bme.readPressure();  // Expressed in Pascals.
  float temperature = bme.readTemperature();  // Expressed in degrees Celsius.
  float humidity = bme.readHumidity();  // Expressed in percent.
  Serial.print("  Pressure: ");
  Serial.print(pressure);
  Serial.println("Pa");
  Serial.print("  Temperature: ");
  Serial.print(temperature);
  Serial.println("C");
  Serial.print("  Humidity: ");
  Serial.print(humidity);
  Serial.println("%");

  Serial.println("Updating GATT characteristics.");
  uint32_t gattPressure = (uint32_t) round(pressure * 10);  // Express Pascals as fixed-point with a single decimal place.
  uint16_t gattTemperature = (uint16_t) round(temperature * 100);  // Express degrees C as fixed-point with two decimal places.
  uint16_t gattHumidity = (uint16_t) round(humidity * 100);  // Express humidity percentage as fixed-point with two decimal places.
  pressureCharacteristic->setValue((uint32_t &)gattPressure);
  temperatureCharacteristic->setValue((uint16_t &)gattTemperature);
  humidityCharacteristic->setValue((uint16_t &)gattHumidity);
  Serial.print("  0x2A6D: ");
  Serial.println(gattPressure);
  Serial.print("  0x2A6E: ");
  Serial.println(gattTemperature);
  Serial.print("  0x2A6F: ");
  Serial.println(gattHumidity);

  if (server->getConnectedCount() > 0) {
    Serial.println("Notifying clients.");
    pressureCharacteristic->notify();
    temperatureCharacteristic->notify();
    humidityCharacteristic->notify();
  }

  delay(10 * 60e3);  // Don't make the delay too short or the BME temperature will be inaccurate due to self-heating.
}
