
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "config.h"

//** DOIT ESP32 DEVKIT V1 */

String mPubAddr = String(MQTT_PUB) + String(MQTT_USERID)+"/"+String(MQTT_DEVICEID);
String mSubAddr = String(MQTT_SUB) + String(MQTT_USERID)+"/"+String(MQTT_DEVICEID);

int  mRemote = 0;
int  mPumpStat = 0;
int  lstSwitchState;
bool wifiConnected = false;
unsigned long lastAttemptTime = 0;
const long attemptInterval = 60000; 
unsigned long pumpStartTime = 0; // 펌프가 켜진 시간을 기록할 변수
unsigned long pumpTimeout = 1800000; // 기본 30분(1800초) 시간 제한


WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

String getPubString(int remote, int stat) {
  // Create a DynamicJsonDocument
  DynamicJsonDocument doc(100);

  String strStatus = stat == 1 ? "on" : "off";  
  
  doc["remote"] = remote;
  doc["pump"] = strStatus;  
  
  // Serialize the document to a JSON string
  String jsonString;
  serializeJson(doc, jsonString);
  
  return jsonString;
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    wifiConnected = true; // 연결 성공
  } else {
    Serial.println("Failed to connect to WiFi. Please check your settings.");
    wifiConnected = false; // 연결 실패
  }
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
    if (doc.containsKey("pump")) {
      const char* pumpStatus = doc["pump"];
      if (strcmp(pumpStatus, "on") == 0) {
        Serial.println("Pump is ON");
        digitalWrite(WATER_PIN, HIGH);
        mRemote = 1;
        mPumpStat = 1;
        pumpStartTime = millis();  // 펌프가 켜진 시간을 기록합니다.
        client.publish(mPubAddr.c_str(), getPubString(mRemote, mPumpStat).c_str());
      } else if (strcmp(pumpStatus, "off") == 0) {
        Serial.println("Pump is OFF");          
        digitalWrite(WATER_PIN, LOW);
        mRemote = 1;
        mPumpStat = 0;
        client.publish(mPubAddr.c_str(), getPubString(mRemote, mPumpStat).c_str());
      } else {
        Serial.println("Invalid pump status");
      }
    } else if (doc.containsKey("timer")) {
      int timerValue = doc["timer"];
      if (timerValue > 0) {
        Serial.print("Pump is ON for ");
        Serial.print(timerValue);
        Serial.println(" minutes.");
        digitalWrite(WATER_PIN, HIGH);
        mRemote = 1;
        mPumpStat = 1;
        pumpStartTime = millis();  // 펌프가 켜진 시간을 기록합니다.
        pumpTimeout = timerValue * 60000;  // 타이머 값 설정 (분 단위)
        client.publish(mPubAddr.c_str(), getPubString(mRemote, mPumpStat).c_str());
      }
    } else if (doc.containsKey("status")) {
      int numStatus = doc["status"];
      if (numStatus == 1) {
        Serial.println("Status request");
        client.publish(mPubAddr.c_str(), getPubString(mRemote, mPumpStat).c_str());
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

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(WATER_PIN, OUTPUT);
  
  digitalWrite(WATER_PIN, LOW);  

  Serial.begin(115200);
  setup_wifi();
  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(callback);
}

void loop() {
  unsigned long currentMillis = millis();

  int switchState = digitalRead(SWITCH_PIN);
  if (lstSwitchState != switchState) {
    Serial.print("Switch Status:");
    Serial.println(switchState);
    lstSwitchState = switchState;
    mRemote = 0;    
    if (switchState == 0){
      mPumpStat = 1;
      digitalWrite(WATER_PIN, HIGH);
      pumpStartTime = millis();  // 펌프가 켜진 시간을 기록합니다.
      pumpTimeout = 1800000;  // 스위치로 켜질 때는 기본 타임아웃 30분 설정
    } else {
      mPumpStat = 0;
      digitalWrite(WATER_PIN, LOW);
    }
    if (client.connected()) {
      client.publish(mPubAddr.c_str(), getPubString(mRemote, mPumpStat).c_str());    
    }
  }

  // 펌프가 켜진 상태에서 설정된 타이머가 경과했는지 확인
  if (mPumpStat == 1 && (currentMillis - pumpStartTime >= pumpTimeout)) {
    Serial.println("Pump automatically turned OFF after the set time.");
    mPumpStat = 0;
    digitalWrite(WATER_PIN, LOW);
    if (client.connected()) {
      client.publish(mPubAddr.c_str(), getPubString(mRemote, mPumpStat).c_str());
    }
  }

  if (!wifiConnected || WiFi.status() != WL_CONNECTED) {
    if (currentMillis - lastAttemptTime >= attemptInterval) {
      Serial.println("Attempting to reconnect to WiFi...");
      digitalWrite(BUILTIN_LED, HIGH);
      setup_wifi();
      digitalWrite(BUILTIN_LED, LOW);
      lastAttemptTime = currentMillis;
    }
  } else {
    if (!client.connected()) {
      reconnect();
    } else {
      digitalWrite(BUILTIN_LED, LOW);
      delay(500);
      digitalWrite(BUILTIN_LED, HIGH);
      delay(500);
      client.loop();

      unsigned long now = millis();
      if (now - lastMsg > 60000) {
        lastMsg = now;
        ++value;
        client.publish(mPubAddr.c_str(), getPubString(mRemote, mPumpStat).c_str());
      }      
    }
  }
}