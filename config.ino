// Storing JSON configuration file in flash file system
// Uses ArduinoJson library by Benoit Blanchon.
// https://github.com/bblanchon/ArduinoJson
//
// note: wifi credentials are stored in ESP8266 special area flash memory and NOT in this config

#include <ArduinoJson.h>
#include "FS.h"

#define MAX_CONFIGFILESIZE 512
#define CONFIG_FILE "/config.json"

class UfoConfig {
  StaticJsonBuffer<MAX_CONFIGFILESIZE+1> _jsonBuffer;
  JsonObject& _json;
  bool load();
  bool save();
  JsonObject& json();
  String getValue(String key);
  void setValue(String key, String value);

};

bool UfoConfig::load() {
  File configFile = SPIFFS.open(CONFIG_FILE, "r");
  if (!configFile) {
    if (debug) Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > (MAX_CONFIGFILESIZE)) {
    if (debug) Serial.println("Config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  JsonObject& json = _jsonBuffer.parseObject(buf.get());

  if (!_json.success()) {
    if (debug) Serial.println("Failed to parse config file");
    return false;
  }

  const char* ruxitEnvironmentID = _json["ruxitEnvironmentID"];
  const char* ruxitApiKey = _json["ruxitAPIKey"];
  const char* ruxitApplicationID = _json["ruxitApplicationID"];

  return true;
}
// note that writing to the SPIFFS wears the flash memory; so make sure to only use it when saving is really required.
bool UfoConfig::save() {
  //_json = (JsonObject&)jsonBuffer.createObject();
  //_json["dynatraceRuxitWebHookUrl"] = "api.example.com";
  

  File configFile = SPIFFS.open(CONFIG_FILE, "w");
  if (!configFile) {
    if (debug) Serial.println("Failed to open config file for writing");
    return false;
  }

  _json.printTo(configFile);
  return true;
}

JsonObject& UfoConfig::json() {
  return _json;
}

String UfoConfig::getValue(String key) {
  return _json[key.c_str()];
}

void UfoConfig::setValue(String key, String value) {
  if (_json.containsKey(key.c_str())) {
    _json[key.c_str()] = value.c_str();
  } else {
    JsonObject& object = _jsonBuffer.createObject();
    object[key.c_str()] = value.c_str();
  }
}

