  
// array of different xiaomi flora MAC addresses
char* FLORA_DEVICES[] = {
    "C4:7C:8D:6D:6E:A2"    
};

// sleep between to runs in seconds
//#define SLEEP_DURATION 30 * 60
#define SLEEP_DURATION 1 * 60
// emergency hibernate countdown in seconds
#define EMERGENCY_HIBERNATE 3 * 60
// how often should the battery be read - in run count
#define BATTERY_INTERVAL 6
// how often should a device be retried in a run when something fails
#define RETRY 3

const char*   WIFI_SSID       = "frogmon";
const char*   WIFI_PASSWORD   = "1234567890";

// MQTT topic gets defined by "<MQTT_DEVICEID>/<MAC_ADDRESS>/<property>"
// where MAC_ADDRESS is one of the values from FLORA_DEVICES array
// property is either temperature, moisture, conductivity, light or battery

const char*   MQTT_HOST       = "frogmon.synology.me";
const int     MQTT_PORT       = 8359;
const char*   MQTT_CLIENTID   = "test";
const char*   MQTT_USERNAME   = "root";
const char*   MQTT_PASSWORD   = "PASSWORD";
const char*   MQTT_DEVICEID   = "flower02";
const int     MQTT_RETRY_WAIT = 5000;
const char*   MQTT_PUB        = "FARMs/Status/";        // 원격컨트롤 서버 상태정보용 주소 - 변경 불필요
const char*   MQTT_SUB        = "FARMs/Control/";       // 원격컨트롤 서버 제어용 주소 - 변경 불필요

