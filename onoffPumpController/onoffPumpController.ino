
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
int leftTime = 0;
int sendTime = 0;


WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

String getFormattedRemainingTime() {
  if (mPumpStat == 1) {
    unsigned long currentMillis = millis();
    unsigned long elapsedTime = currentMillis - pumpStartTime;
    int remainingTime = (pumpTimeout - elapsedTime) / 1000; // 남은 시간을 초 단위로 계산
    
    if (remainingTime > 0) {
      int minutes = remainingTime / 60;  // 남은 시간을 분으로 변환
      int seconds = remainingTime % 60;  // 남은 시간을 초로 변환
      
      // 남은 시간 포맷팅
      String formattedTime = "";
      if (minutes > 0) {
        formattedTime += String(minutes) + "min ";
      }
      formattedTime += String(seconds) + "sec";
      
      return formattedTime;
    }
  }
  return "0sec";  // 펌프가 꺼져있거나 시간이 0 이하일 때
}

String getPubString(int remote, int stat) {
  // Create a DynamicJsonDocument
  DynamicJsonDocument doc(100);

  String strStatus = stat == 1 ? "on" : "off";  
  String remainingTime = getFormattedRemainingTime(); 
  
  doc["remote"] = remote;
  doc["pump"] = strStatus;  
  doc["timer"] = remainingTime;
  
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
        pumpTimeout = 1800000;     // 초기값 30분
        client.publish(mPubAddr.c_str(), getPubString(mRemote, mPumpStat).c_str());
      } else if (strcmp(pumpStatus, "off") == 0) {
        Serial.println("Pump is OFF");
        digitalWrite(WATER_PIN, LOW);
        mRemote = 1;
        mPumpStat = 0;
        pumpTimeout = 0;  // 펌프가 꺼질 때는 타이머도 0으로 설정합니다.
        client.publish(mPubAddr.c_str(), getPubString(mRemote, mPumpStat).c_str());
      } else {
        Serial.println("Invalid pump status");
      }
    }

    if (doc.containsKey("timer") && mPumpStat == 1) {  // 펌프가 켜져 있을 때만 타이머 값을 적용
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
    }

    if (doc.containsKey("status")) {
      int numStatus = doc["status"];
      if (numStatus == 1) {
        Serial.println("Status request");
        client.publish(mPubAddr.c_str(), getPubString(mRemote, mPumpStat).c_str());
      }
    }
  }

}

int getRemainingTime() {
  if (mPumpStat == 1) {
    unsigned long currentMillis = millis();
    unsigned long elapsedTime = currentMillis - pumpStartTime;
    int remainingTime = (pumpTimeout - elapsedTime) / 1000 / 60; // 남은 시간을 분 단위로 계산
    return remainingTime > 0 ? remainingTime : 0;
  }
  return 0;
}


void reconnect() {
  static unsigned long lastReconnectAttempt = 0;  // 마지막 시도 시간 기록

  if (!client.connected()) {
    unsigned long now = millis();
    // 5초마다 재연결 시도
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      Serial.print("Attempting MQTT connection...");
      String clientId = String(MQTT_USERID) + "_" + String(MQTT_DEVICEID);
      if (client.connect(clientId.c_str())) {
        Serial.println("connected");
        client.publish(mPubAddr.c_str(), "connected");
        client.subscribe(mSubAddr.c_str());
      } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
      }
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

  // 스위치 상태 확인 및 펌프 상태 업데이트
  int switchState = digitalRead(SWITCH_PIN);
  if (lstSwitchState != switchState) {
    Serial.print("Switch Status:");
    Serial.println(switchState);
    lstSwitchState = switchState;
    mRemote = 0;    
    if (switchState == 0) {
      mPumpStat = 1;
      digitalWrite(WATER_PIN, HIGH);
      pumpStartTime = millis();  // 펌프가 켜진 시간을 기록합니다.
      pumpTimeout = 1800000;  // 스위치로 켜질 때는 기본 타임아웃 30분 설정
    } else {
      mPumpStat = 0;
      pumpTimeout = 0;
      digitalWrite(WATER_PIN, LOW);
    }
    if (client.connected()) {
      client.publish(mPubAddr.c_str(), getPubString(mRemote, mPumpStat).c_str());
    }
  }

  // 펌프 타이머가 경과했는지 확인
  if (mPumpStat == 1 && (currentMillis - pumpStartTime >= pumpTimeout)) {
    Serial.println("Pump automatically turned OFF after the set time.");
    mPumpStat = 0;
    digitalWrite(WATER_PIN, LOW);
    if (client.connected()) {
      client.publish(mPubAddr.c_str(), getPubString(mRemote, mPumpStat).c_str());
    }
  }

  // Wi-Fi 연결 상태를 확인하고 필요 시 재연결
  if (!wifiConnected || WiFi.status() != WL_CONNECTED) {
    if (currentMillis - lastAttemptTime >= attemptInterval) {
      Serial.println("Attempting to reconnect to WiFi...");
      digitalWrite(BUILTIN_LED, LOW);
      setup_wifi();
      lastAttemptTime = currentMillis;
    }
  } else {
    // MQTT 클라이언트 연결 확인 및 재연결 시도
    reconnect();  // 기존 while 루프 대신 비동기 연결 시도
    if (client.connected()) {
      client.loop(); // 클라이언트가 연결되어 있을 때만 loop 실행

      // 1분마다 상태를 전송
      if (sendTime > 60) {
        sendTime = 0;
        Serial.print("Send Mqtt State! ");
        client.publish(mPubAddr.c_str(), getPubString(mRemote, mPumpStat).c_str());
      }

      // 1초마다 남은 시간을 체크하고 출력
      if (mPumpStat == 1 && currentMillis - lastMsg > 1000) {
        digitalWrite(BUILTIN_LED, HIGH);
        sendTime++;
        lastMsg = currentMillis;
        String remainingTime = getFormattedRemainingTime();
        Serial.print("Remaining Time: ");
        Serial.print(remainingTime);
        Serial.println(" minutes");
        client.publish(mPubAddr.c_str(), getPubString(mRemote, mPumpStat).c_str());
      }
    }
  }
}