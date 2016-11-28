#define FIRMWARE_VERSION F(__DATE__ " " __TIME__)

#define DEFAULT_HOSTNAME F("ufo")
#define DEFAULT_APSSID F("ufo")

// ESP8266 Huzzah Pin usage
#define PIN_DOTSTAR_LOGO 2
#define PIN_DOTSTAR_LOWERRING 4
#define PIN_DOTSTAR_UPPERRING 5
#define PIN_DOTSTAR_CLOCK 14
#define PIN_FACTORYRESET 15

#define EEPROM_MAXSIZE 64 // max number of bytes to store in eeprom (can be up to 4k; its using one sector after the spiffs) 

