// Storing JSON configuration file in flash file system
// Uses ArduinoJson library by Benoit Blanchon.
// https://github.com/bblanchon/ArduinoJson
// note: wifi credentials are stored in ESP8266 special area flash memory and NOT in this config
#define MAX_CONFIGFILESIZE 512
#define CONFIG_FILE F("/config.json")


/** TODO
#include <EEPROM.h>

#define EEPROM_CONFIGBUF 256

void configRead() {
  EEPROM.begin(EEPROM_CONFIGBUF); // Note: we can go up to 4kB. however, a smaller value reduces RAM consumption
  if ((EEPROM.read(0) != 0xAA) || (EEPROM.read(1) != 0xBB) || (EEPROM.read(2) != 0xCC) || (EEPROM.read(3) != 0x01)) {
    // Magic mismatch! EEPROM has not been initialized yet!!

    
  }
  //Serial.print("Reading EEPROM address(0): ");
  //Serial.println(EEPROM.read(0));
  EEPROM.end();
}

void configWrite() {
  EEPROM.begin(EEPROM_CONFIGBUF); // Note: we can go up to 4kB. however, a smaller value reduces RAM consumption
  if ((EEPROM.read(0) != 0xAA) || (EEPROM.read(1) != 0xBB) || (EEPROM.read(2) != 0xCC) || (EEPROM.read(3) != 0x01)) {

      // initialize EEPROM
    for (int addr = 0; addr < EEPROM_CONFIGBUF; addr++) {
      EEPROM.write(addr, 0);
    }
}

int newaddr configWriteString(int addr, String & s) {
  int len = s.length();

  for (int i = 0; i < len; i++) {
    EEPROM.write(addr + i, s.charAt(i));
  }
  EEPROM.write(addr + len, 0); // 0 termination
  return addr+len+1;
}

**/



bool configRead() {
  File configFile = SPIFFS.open(CONFIG_FILE, "r");
  if (!configFile) {
    if (debug) Serial.println(F("Failed to open config file"));
    return false;
  }

  size_t size = configFile.size();
  if (size > (MAX_CONFIGFILESIZE)) {
    if (debug) Serial.println(F("Config file size is too large"));
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  StaticJsonBuffer < MAX_CONFIGFILESIZE + 1 > jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    if (debug) Serial.println(F("Failed to parse config file"));
    return false;
  }

  dynatraceEnvironmentID = (const char*)json[F("dynatrace-environmentid")];
  dynatraceApiKey = (const char*)json[F("dynatrace-apikey")];
  return true;
}


// note that writing to the SPIFFS wears the flash memory; so make sure to only use it when saving is really required.
bool configWrite() {
  StaticJsonBuffer < MAX_CONFIGFILESIZE + 1 > jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json[F("dynatrace-environmentid")] = dynatraceEnvironmentID;
  json[F("dynatrace-apikey")] = dynatraceApiKey;

  File configFile = SPIFFS.open(CONFIG_FILE, "w");
  if (!configFile) {
    if (debug) Serial.println(F("Failed to open config file for writing"));
    return false;
  }

  json.printTo(configFile);
  if (debug) json.printTo(Serial);
  return true;
}

bool configDelete() {
  SPIFFS.begin(); // just make sure its already open
  SPIFFS.remove(CONFIG_FILE);
 
}

