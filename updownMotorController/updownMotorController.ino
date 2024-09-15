#define BUTTON_PIN  0
#define BUILTIN_LED 2

//** DOIT ESP32 DEVKIT V1 */

#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "config.h"

String mPubAddr = String(MQTT_PUB) + String(MQTT_USERID)+"/"+String(MQTT_DEVICEID);
String mSubAddr = String(MQTT_SUB) + String(MQTT_USERID)+"/"+String(MQTT_DEVICEID);

int  mRemote = 0;
int  mMotorStat = 0;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

String getPubString(int remote, int stat) {
  // Create a DynamicJsonDocument
  DynamicJsonDocument doc(100);
  String strStatus = "stop";
  
  if (stat == 1){
    strStatus = "up";
  } else if (stat == 2) {
    strStatus = "down";
  } else {
    strStatus = "stop";    
  }

  doc["remote"] = remote;
  doc["motor"] = strStatus;  
  
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

  Serial.print("Received payload: ");
  for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
  }
  Serial.println(); 

  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());
  } else {
    if (doc.containsKey("motor")) {
      const char* Status = doc["motor"];
      if (strcmp(Status, "stop") == 0) {
        Serial.println("motor is STOP");
        onStop();          
        mRemote = 1;
        mMotorStat = 0;
        client.publish(mPubAddr.c_str(), getPubString(mRemote, mMotorStat).c_str());
      } else if (strcmp(Status, "up") == 0) {
        Serial.println("motor is up");          
        onForward();
        mRemote = 1;
        mMotorStat = 1;
        client.publish(mPubAddr.c_str(), getPubString(mRemote, mMotorStat).c_str());
      } else if (strcmp(Status, "down") == 0) {
        Serial.println("motor is down");          
        onBackward();
        mRemote = 1;
        mMotorStat = 2;
        client.publish(mPubAddr.c_str(), getPubString(mRemote, mMotorStat).c_str());
      } else {
        Serial.println("Invalid Motor status");
      }
    } else if (doc.containsKey("status")) {
      int numStatus = doc["status"];
      if (numStatus == 1) {
        Serial.println("Status request");
        client.publish(mPubAddr.c_str(), getPubString(mRemote, mMotorStat).c_str());
      }
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = String(MQTT_USERID) + "_" + String(MQTT_DEVICEID);
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

void onForward() {
  digitalWrite(DIRECT_PIN, HIGH);
  digitalWrite(BREAK_PIN, LOW);
  Serial.println("Forward!");
}

void onBackward() {
  digitalWrite(DIRECT_PIN, LOW);
  digitalWrite(BREAK_PIN, LOW);
  Serial.println("Backward");
}

void onStop() {
  digitalWrite(DIRECT_PIN, LOW);
  digitalWrite(BREAK_PIN, HIGH);
  Serial.println("Stop");
}


void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  pinMode(DIRECT_PIN, OUTPUT);
  pinMode(BREAK_PIN, OUTPUT);
  
  Serial.begin(115200);
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
  if (now - lastMsg > 60000) {
    lastMsg = now;
    ++value;
    client.publish(mPubAddr.c_str(), getPubString(mRemote, mMotorStat).c_str());
  }

}