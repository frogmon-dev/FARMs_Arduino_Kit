#include <SoftwareSerial.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "config.h"

#define RE 8
#define DE 7

#define TX 17
#define RX 16

#define MSG_BUFFER_SIZE	(50)

WiFiClient espClient;
PubSubClient client(espClient);
char msg[MSG_BUFFER_SIZE];
unsigned long lastMsg = 0;

const byte p7in1[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x07, 0x04, 0x08};

byte values[19];
SoftwareSerial mod(TX, RX);

void setup() {
  Serial.begin(115200);
  mod.begin(4800);
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  setup_wifi();
  client.setServer(MQTT_HOST, 8359);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    digitalWrite(BUILTIN_LED, LOW);
    reconnect();
  } else {    
    digitalWrite(BUILTIN_LED, LOW);
    delay(500);
    digitalWrite(BUILTIN_LED, HIGH);
    delay(500);
  }
  client.loop();

  
  unsigned long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;

    // get 7in1 soil sensor data
    digitalWrite(DE, HIGH);
    digitalWrite(RE, HIGH);

    delay(10);

    if (mod.write(p7in1, sizeof(p7in1)) == 8) {
      digitalWrite(DE, LOW);
      digitalWrite(RE, LOW);
      delay(200);
      for (byte i = 0; i < 19; i++) {
        values[i] = mod.read();
      }
      if (values[0] == 0x01 && values[1] == 0x03) {
        Serial.println(
          getPubString(getHumidity(), getTemperature(), getConductivity(), getPH(), getNitrogen(), getPhosphorus(), getPotassium())
        );
        client.publish(mPubAddr.c_str(), getPubString(getHumidity(), getTemperature(), getConductivity(), getPH(), getNitrogen(), getPhosphorus(), getPotassium()).c_str());
      }
    }    
  } 
}

float getHumidity() {
  int humidity = (values[3] << 8) + values[4];
  float moisture = humidity / 10.0;
  return moisture;
}

float getTemperature() {
  int temperature = (values[5] << 8) + values[6];
  float celsius = temperature / 10.0;
  return celsius;
}

int getConductivity() {
  int conductivity = (values[7] << 8) + values[8];
  return conductivity;
}

float getPH() {
  int iPH = (values[9] << 8) + values[10];
  float pH = iPH / 10.0;
  return pH;
}

int getNitrogen() {
  int nitrogen = (values[11] << 8) + values[12];
  return nitrogen;
}

int getPhosphorus() {
  int phosphorus = (values[13] << 8) + values[14];
  return phosphorus;
}

int getPotassium() {
  int potassium = (values[15] << 8) + values[16];
  return potassium;
}

String getPubString(float humidity, float temperature, int conductivity, float PH, int nitrogen, int phosphorus, int potassium) {
  // Create a DynamicJsonDocument
  DynamicJsonDocument doc(100);
  
  doc["humidity"] = humidity;
  doc["temperature"] = temperature;  
  doc["conductivity"] = conductivity;  
  doc["PH"] = PH;  
  doc["nitrogen"] = nitrogen;  
  doc["phosphorus"] = phosphorus;  
  doc["potassium"] = potassium;  

  // Serialize the document to a JSON string
  String jsonString;
  serializeJson(doc, jsonString);
  
  return jsonString;
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  if (payload[0] == '{') {  
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print("Failed to parse JSON: ");
      Serial.println(error.c_str());
    } else {
      if (doc.containsKey("status")) {
        int numStatus = doc["status"];
        if (numStatus == 1) {
          Serial.println("Status request");
          client.publish(mPubAddr.c_str(), getPubString(getHumidity(), getTemperature(), getConductivity(), getPH(), getNitrogen(), getPhosphorus(), getPotassium()).c_str());          
        }
      }      
    }
  } else {
    // Print the received message as a string to the serial monitor
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);      
    }    
    Serial.println();    
  }  
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.publish(mPubAddr.c_str(), "connected");
      // ... and resubscribe
      client.subscribe(mSubAddr.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}