#include "config.h"
#include <FS.h>
#include <ArduinoJson.h>


// Storing JSON configuration file in flash file system
// Uses ArduinoJson library by Benoit Blanchon.
// https://github.com/bblanchon/ArduinoJson
// note: wifi credentials are stored in ESP8266 special area flash memory and NOT in this config
#define MAX_CONFIGFILESIZE 512
#define CONFIG_FILE F("/config.json")


Config::Config(bool debug){
  mDebug = debug;
  mChanged = true;
}

bool Config::Read() {
  File configFile = SPIFFS.open(CONFIG_FILE, "r");
  if (!configFile) {
    if (mDebug) Serial.println(F("Failed to open config file"));
    return false;
  }

  size_t size = configFile.size();
  if (size > (MAX_CONFIGFILESIZE)) {
    if (mDebug) Serial.println(F("Config file size is too large"));
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
    if (mDebug) Serial.println(F("Failed to parse config file"));
    return false;
  }
  enabled = json[F("dynatrace-enabled")];
  dynatraceEnvironmentID = (const char*)json[F("dynatrace-environmentid")];
  dynatraceApiKey = (const char*)json[F("dynatrace-apikey")];
  pollingIntervalS = json[F("dynatrace-interval")];
  if (pollingIntervalS < 30){
    pollingIntervalS = 30;
  }
  
  return true;
}


// note that writing to the SPIFFS wears the flash memory; so make sure to only use it when saving is really required.
bool Config::Write() {
  StaticJsonBuffer < MAX_CONFIGFILESIZE + 1 > jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json[F("dynatrace-enabled")] = enabled;
  json[F("dynatrace-environmentid")] = dynatraceEnvironmentID;
  json[F("dynatrace-apikey")] = dynatraceApiKey;
  json[F("dynatrace-interval")] = pollingIntervalS;

  File configFile = SPIFFS.open(CONFIG_FILE, "w");
  if (!configFile) {
    if (mDebug) Serial.println(F("Failed to open config file for writing"));
    return false;
  }

  json.printTo(configFile);
  if (mDebug) json.printTo(Serial);
  return true;
}

bool Config::Delete() {
  SPIFFS.begin(); // just make sure its already open
  SPIFFS.remove(CONFIG_FILE);
 
}

bool Config::Enabled(){
  return enabled && (dynatraceEnvironmentID.length() > 0) && (dynatraceApiKey.length() > 0);
}

bool Config::Changed(){
  if (mChanged){
    mChanged = false;
    return true;
  }
  return false;
}

