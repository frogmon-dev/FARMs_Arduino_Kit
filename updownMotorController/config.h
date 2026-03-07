
const char*   MQTT_HOST       = "frogmon.synology.me";  // 원격컨트롤 서버 주소 - 변경 불필요
const int     MQTT_PORT       = 8359;                   // 원격컨트롤 서버 포트 - 변경 불필요
const char*   MQTT_USERID     = "local";                 // *앱에 등록한뚝딱이ID
const char*   MQTT_DEVICEID   = "FM-PUMP01";               // *앱에 등록한장비ID
const int     MQTT_RETRY_WAIT = 5000;                   // 원격컨트롤 서버 대기시간 - 변경 불필요
const char*   MQTT_PUB        = "FARMs/Status/";        // 원격컨트롤 서버 상태정보용 주소 - 변경 불필요
const char*   MQTT_SUB        = "FARMs/Control/";       // 원격컨트롤 서버 제어용 주소 - 변경 불필요
const char*   MQTT_ALARM_SUB        = "FARMs/Alarm/";       // 원격컨트롤 서버 제어용 주소 - 변경 불필요

// WIFI 설정
const char*   ssid        = "frogmon";                  // * WIFI ID
const char*   password    = "1234567890";               // * WIFI 비번

#define BUTTON_PIN  0
#define BUILTIN_LED 2
#define WATER_PIN   15
#define SWITCH_PIN  13 
