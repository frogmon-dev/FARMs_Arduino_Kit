
// Internet Setup
const char*   ssid        = "frogmon";    // WIFI SSID
const char*   password    = "1234567890"; // WIFI Password

// MQTT Server Setup
const char*   MQTT_HOST       = "frogmon.synology.me"; // 원격컨트롤 서버 주소
const int     MQTT_PORT       = 8359;                  // 원격컨트롤 서버 포트
const char*   MQTT_USERID     = "frogmon";         // 등록한뚝딱이ID
const char*   MQTT_DEVICEID   = "sensor01";          // 등록한장비ID
const int     MQTT_RETRY_WAIT = 5000;
const char*   MQTT_PUB        = "FARMs/Status/";      // 변경 불필요
const char*   MQTT_SUB        = "FARMs/Control/";     // 뱐걍 븚필요

#define BUTTON_PIN  0   // Pin number for the button
#define BUILTIN_LED 2

String mPubAddr = String(MQTT_PUB) + String(MQTT_USERID)+"/"+String(MQTT_DEVICEID);
String mSubAddr = String(MQTT_SUB) + String(MQTT_USERID)+"/"+String(MQTT_DEVICEID);