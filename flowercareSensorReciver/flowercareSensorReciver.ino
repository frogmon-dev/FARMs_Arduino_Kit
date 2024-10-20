#include <BLEDevice.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"
#include "Arduino.h"
#include "heltec.h"
#include <ArduinoJson.h>

//** Heltec Wireless Stick */

// boot count used to check if battery status should be read
RTC_DATA_ATTR int bootCount = 0;

// device count
static int deviceCount = sizeof FLORA_DEVICES / sizeof FLORA_DEVICES[0];

// the remote service we wish to connect to
static BLEUUID serviceUUID("00001204-0000-1000-8000-00805f9b34fb");

// the characteristic of the remote service we are interested in
static BLEUUID uuid_version_battery("00001a02-0000-1000-8000-00805f9b34fb");
static BLEUUID uuid_sensor_data("00001a01-0000-1000-8000-00805f9b34fb");
static BLEUUID uuid_write_mode("00001a00-0000-1000-8000-00805f9b34fb");

TaskHandle_t hibernateTaskHandle = NULL;


WiFiClient espClient;
PubSubClient client(espClient);

void connectWifi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("");
}

void disconnectWifi() {
  WiFi.disconnect(true);
  Serial.println("WiFi disonnected");
}

void connectMqtt() {
  Serial.println("Connecting to MQTT...");
  client.setServer(MQTT_HOST, MQTT_PORT);
  
  while (!client.connected()) {
    String clientId = String(MQTT_USERID) + "_" + String(MQTT_DEVICEID);
    if (!client.connect(clientId.c_str())) {
      Serial.print("MQTT connection failed:");
      Serial.print(client.state());
      Serial.println("Retrying...");
      delay(MQTT_RETRY_WAIT);
    }
  }
  Serial.println("MQTT connected");
  Serial.println("");
}

void disconnectMqtt() {
  client.disconnect();
  Serial.println("MQTT disconnected");
}

BLEClient* getFloraClient(BLEAddress floraAddress) {
  BLEClient* floraClient = BLEDevice::createClient();

  if (!floraClient->connect(floraAddress)) {
    Serial.println("- Connection failed, skipping");
    dspPrintln("Sensor Error.");
    return nullptr;
  }

  Serial.println("- Connection successful");
  return floraClient;
}

BLERemoteService* getFloraService(BLEClient* floraClient) {
  BLERemoteService* floraService = nullptr;

  try {
    floraService = floraClient->getService(serviceUUID);
  }
  catch (...) {
    // something went wrong
  }
  if (floraService == nullptr) {
    Serial.println("- Failed to find data service");
    dspPrintln("Sensor Error.");
  }
  else {
    Serial.println("- Found data service");
  }

  return floraService;
}

bool forceFloraServiceDataMode(BLERemoteService* floraService) {
  BLERemoteCharacteristic* floraCharacteristic;

  // get device mode characteristic, needs to be changed to read data
  Serial.println("- Force device in data mode");
  floraCharacteristic = nullptr;
  try {
    floraCharacteristic = floraService->getCharacteristic(uuid_write_mode);
  }
  catch (...) {
    // something went wrong
  }
  if (floraCharacteristic == nullptr) {
    Serial.println("-- Failed, skipping device");
    dspPrintln("Sensor Error.");
    return false;
  }

  // write the magic data
  uint8_t buf[2] = {0xA0, 0x1F};
  floraCharacteristic->writeValue(buf, 2, true);

  delay(500);
  return true;
}

bool readFloraDataCharacteristic(BLERemoteService* floraService, String baseTopic) {
  BLERemoteCharacteristic* floraCharacteristic = nullptr;

  // get the main device data characteristic
  Serial.println("- Access characteristic from device");
  try {
    floraCharacteristic = floraService->getCharacteristic(uuid_sensor_data);
  }
  catch (...) {
    // something went wrong
  }
  if (floraCharacteristic == nullptr) {
    Serial.println("-- Failed, skipping device");
    dspPrintln("Sensor Error.");
    return false;
  }

  // read characteristic value
  Serial.println("- Read value from characteristic");
  String value;
  try {
    value = String(floraCharacteristic->readValue().c_str());
  }
  catch (...) {
    // something went wrong
    Serial.println("-- Failed, skipping device");
    dspPrintln("Sensor Error.");
    return false;
  }
  const char *val = value.c_str();

  Serial.print("Hex: ");
  for (int i = 0; i < 16; i++) {
    Serial.print((int)val[i], HEX);
    Serial.print(" ");
  }
  Serial.println(" ");

  int16_t* temp_raw = (int16_t*)val;
  float temperature = (*temp_raw) / ((float)10.0);
  Serial.print("-- Temperature: ");
  Serial.println(temperature);

  int moisture = val[7];
  Serial.print("-- Moisture: ");
  Serial.println(moisture);

  int light = val[3] + val[4] * 256;
  Serial.print("-- Light: ");
  Serial.println(light);

  int conductivity = val[8] + val[9] * 256;
  Serial.print("-- Conductivity: ");
  Serial.println(conductivity);

  if (temperature > 200) {
    Serial.println("-- Unreasonable values received, skip publish");
    dspPrintln("Sensor Error.");
    return false;
  }

  char buffer[256];
  StaticJsonDocument<256> doc; // JSON 문서 생성

  doc["temperature"] = temperature;
  doc["moisture"] = moisture;
  doc["light"] = light;
  doc["moisture"] = moisture;
  doc["conductivity"] = conductivity;
  doc["bootCount"] = bootCount;
  int battery = readFloraBatteryCharacteristic(floraService);
  doc["battery"] = battery;
  
  // JSON 문서를 문자열로 직렬화  
  serializeJson(doc, buffer);

  // MQTT를 통해 전체 JSON 문자열 발행
  client.publish((baseTopic).c_str(), buffer);

  return true;
}

int readFloraBatteryCharacteristic(BLERemoteService* floraService) {
  BLERemoteCharacteristic* floraCharacteristic = nullptr;

  // get the device battery characteristic
  Serial.println("- Access battery characteristic from device");
  try {
    floraCharacteristic = floraService->getCharacteristic(uuid_version_battery);
  }
  catch (...) {
    // something went wrong
  }
  if (floraCharacteristic == nullptr) {    
    Serial.println("-- Failed, skipping battery level");
    return 0;
  }

  // read characteristic value
  Serial.println("- Read value from characteristic");
  String value;
  try {
    value = String(floraCharacteristic->readValue().c_str());
  }
  catch (...) {
    // something went wrong
    Serial.println("-- Failed, skipping battery level");
    return 0;
  }
  const char *val2 = value.c_str();
  int battery = val2[0];
  
  return battery;
}

bool processFloraService(BLERemoteService* floraService, char* deviceMacAddress, bool readBattery) {
  // set device in data mode
  if (!forceFloraServiceDataMode(floraService)) {
    return false;
  }

  //String baseTopic = MQTT_BASE_TOPIC + "/" + deviceMacAddress + "/";
  String baseTopic =  MQTT_PUB + String(MQTT_USERID) + "/" + MQTT_DEVICEID;  
  bool dataSuccess = readFloraDataCharacteristic(floraService, baseTopic);

  return dataSuccess;
}

bool processFloraDevice(BLEAddress floraAddress, char* deviceMacAddress, bool getBattery, int tryCount) {
  Serial.print("Processing Flora device at ");
  Serial.print(floraAddress.toString().c_str());
  Serial.print(" (try ");
  Serial.print(tryCount);
  Serial.println(")");

  // connect to flora ble server
  BLEClient* floraClient = getFloraClient(floraAddress);
  if (floraClient == nullptr) {
    return false;
  }

  // connect data service
  BLERemoteService* floraService = getFloraService(floraClient);
  if (floraService == nullptr) {
    floraClient->disconnect();
    return false;
  }

  // process devices data
  bool success = processFloraService(floraService, deviceMacAddress, getBattery);

  // disconnect from device
  floraClient->disconnect();

  return success;
}

void hibernate() {
  esp_sleep_enable_timer_wakeup(SLEEP_DURATION * 1000000ll);
  Serial.println("Going to sleep now.");
  esp_deep_sleep_start();
}

void delayedHibernate(void *parameter) {
  delay(EMERGENCY_HIBERNATE*1000); // delay for five minutes
  Serial.println("Something got stuck, entering emergency hibernate...");
  hibernate();
}

void setup() {
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
  Heltec.display->setContrast(255);
  dspPrintln("Flower Care Start!");
  
  // all action is done when device is woken up
  Serial.begin(115200);
  delay(1000);

  // increase boot count
  bootCount++;

  // create a hibernate task in case something gets stuck
  xTaskCreate(delayedHibernate, "hibernate", 4096, NULL, 1, &hibernateTaskHandle);

  Serial.println("Initialize BLE client...");
  dspPrintln("Initialize BLE client...");
  BLEDevice::init("");
  BLEDevice::setPower(ESP_PWR_LVL_P7);

  // connecting wifi and mqtt server
  dspPrintln("Wifi Connect.");
  connectWifi();
  
  dspPrintln("Mqtt Connect.");
  connectMqtt();
  
  // check if battery status should be read - based on boot count
  bool readBattery = ((bootCount % BATTERY_INTERVAL) == 0);

  // process devices
  for (int i=0; i<deviceCount; i++) {
    int tryCount = 0;
    char* deviceMacAddress = FLORA_DEVICES[i];
    BLEAddress floraAddress(deviceMacAddress);

    while (tryCount < RETRY) {
      tryCount++;
      if (processFloraDevice(floraAddress, deviceMacAddress, readBattery, tryCount)) {
        break;
      }
      delay(1000);
    }
    delay(1500);
  }

  // disconnect wifi and mqtt
  disconnectWifi();
  disconnectMqtt();

  // delete emergency hibernate task
  vTaskDelete(hibernateTaskHandle);

  dspPrintln("goto Sleep");
  // go to sleep now
  hibernate();
}

// OLED display
void dspPrintln(String str) {
  Heltec.display->setLogBuffer(5, 30);
  Heltec.display->clear();
  Heltec.display->println(str);
  Heltec.display->drawLogBuffer(0, 0);
  Heltec.display->display();
  delay(500);
}

void loop() {
  /// we're not doing anything in the loop, only on device wakeup
  delay(10000);
}
