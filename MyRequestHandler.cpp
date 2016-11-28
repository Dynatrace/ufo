#include "MyRequestHandler.h"
#include "DisplayCharter.h"
#include "Config.h"
#include "Defines.h"
#include "WebContent.h"
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <StreamString.h>

extern void ledsSetLogoDefault();

MyFunctionRequestHandler::MyFunctionRequestHandler(DisplayCharter* pDisplayLowerRing, DisplayCharter* pDisplayUpperRing, IPDisplay* pIpDisplay, Adafruit_DotStar* pLedstripLogo, Config* pConfig, bool debug){
  mpDisplayLowerRing = pDisplayLowerRing;
  mpDisplayUpperRing = pDisplayUpperRing;
  mpIpDisplay = pIpDisplay;
  mpLedstripLogo = pLedstripLogo;
  mpConfig = pConfig;
  mDebug = debug;
  miUploadSize = 0;
}
    
bool MyFunctionRequestHandler::handle(ESP8266WebServer& server, HTTPMethod requestMethod, String requestUri){
  
  server.client().setNoDelay(true);
  requestUri.toLowerCase();
  
  Serial.println(String(F("handle(")) + String(requestMethod) + String(F(", ")) + requestUri + String(F(")")));
   
  if (requestMethod == HTTP_GET){

    if (requestUri.equals(F("/")) || requestUri.equals(F("/index.html"))){
      server.sendHeader(F("cache-control"), F("private, max-age=0, no-cache, no-store"));
      server.sendHeader(F("Content-Encoding"), F("gzip"));
      server.send_P(200, String(F("text/html")).c_str(), index_html_gz, sizeof(index_html_gz));
      return true;
    }
    if (requestUri.equals(F("/font.woff"))){
      server.send_P(200, String(F("text/html")).c_str(), font_woff, sizeof(font_woff));
      return true;
    }
    /*if (handleStatic(server, requestUri, F("/"), F("/index_comb.html.gz"), F("text/html"), false)) return true;
    if (handleStatic(server, requestUri, F("/"), F("/index.html"), F("text/html"), false)) return true;
    if (handleStatic(server, requestUri, F("/index.html"), F("/index_comb.html.gz"), F("text/html"), false)) return true;
    if (handleStatic(server, requestUri, F("/index.html"), F("/index.html"), F("text/html"), false)) return true;
    if (handleStatic(server, requestUri, F("/phonon.min.css"), F("/phonon.min.css"), F("text/css"), true)) return true;
    if (handleStatic(server, requestUri, F("/phonon.min.js"), F("/phonon.min.js"), F("application/javascript"), true)) return true;
    if (handleStatic(server, requestUri, F("/forms.min.css"), F("/forms.min.css"), F("text/css"), true)) return true;
    if (handleStatic(server, requestUri, F("/forms.min.js"), F("/forms.min.js"), F("application/javascript"), true)) return true;
    if (handleStatic(server, requestUri, F("/icons.min.css"), F("/icons.min.css"), F("text/css"), true)) return true;
    if (handleStatic(server, requestUri, F("/lists.min.css"), F("/lists.min.css"), F("text/css"), true)) return true;
    if (handleStatic(server, requestUri, F("/phonon-base.min.css"), F("/phonon-base.min.css"), F("text/css"), true)) return true;
    if (handleStatic(server, requestUri, F("/phonon-core.min.js"), F("/phonon-core.min.js"), F("application/javascript"), true)) return true;
    if (handleStatic(server, requestUri, F("/font.woff"), F("/font.woff"), F("application/font.woff"), true)) return true;
    if (handleStatic(server, requestUri, F("/font.eot"), F("/font.eot"), F("application/font.eot"), true)) return true;
    if (handleStatic(server, requestUri, F("/font.svg"), F("/font.svg"), F("application/font.svg"), true)) return true;
    if (handleStatic(server, requestUri, F("/font.ttf"), F("/font.ttf"), F("application/font.ttf"), true)) return true;
    */
  
    if (requestUri.startsWith(F("/api"))){
      apiHandler(server);
      return true;
    }
    if (requestUri.startsWith(F("/info"))){
      infoHandler(server);
      return true;
    }
    if (requestUri.startsWith(F("/dtintinfo"))){
      dtIntegrationHandler(server);
      return true;
    }
  }
  else if (requestMethod == HTTP_POST){
    if (requestUri.startsWith(F("/update"))){

      StreamString error;
      if (Update.hasError()) {
        Update.printError(error);
      }
      if (mbRebootRequired)
        httpReboot(server, (Update.hasError()) ? error : String(F("Upload succeeded! ")) + String(miUploadSize) + String(F(" bytes transferred<p>")));
      else{
         server.sendHeader(F("location"), F("/"));
         server.send(302);
         return true;
      }
    }
    
    if (requestUri.equals(F("/dynatrace"))){
      dynatracePostHandler(server);
      return true;
    }
  }
  
  //Now we really need to respond with 404
  String message = F("File Not Found\n\n");
  message += F("URI: ");
  message += server.uri();
  message += F("\nMethod: ");
  message += (server.method() == HTTP_GET ) ? F("GET") : F("POST");
  message += F("\nArguments: ");
  message += server.args();
  message += F("\n");

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
  server.sendHeader(F("cache-control"), F("private, max-age=0, no-cache, no-store"));
  server.send (404, F("text/plain"), message);
  
  return true;
}

void  MyFunctionRequestHandler::upload(ESP8266WebServer& server, String requestUri, HTTPUpload& upload){
  //Serial.println(String(F("upload(")) + requestUri + String(F(")")));

  String filename = parseFileName(upload.filename);

  if (filename.endsWith(".bin")) { // handle firmware upload
    mbRebootRequired = true;
    if (upload.status == UPLOAD_FILE_START) {
      if (mDebug) Serial.println("Update: " + upload.filename);
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) { //start with max available size
        if (mDebug) Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (mDebug) Serial.print(".");
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        if (mDebug) Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      miUploadSize = Update.size();
      if (Update.end(true)) { //true to set the size to the current progress
        if (mDebug) Serial.println(String(F("Update Success - uploaded: ")) + String(upload.totalSize) + String(F(".... rebooting now!")));
      } else {
        if (mDebug) Update.printError(Serial);
      }
      if (mDebug) Serial.setDebugOutput(false);
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
      miUploadSize = Update.size();
      Update.end();
      if (mDebug) Serial.println(F("Update was aborted"));
    }

  } else { // handle file upload
    mbRebootRequired = false;
    if (upload.status == UPLOAD_FILE_START) {
      if (mDebug) Serial.println(String(F("uploading to SPIFFS: /")) + filename);
      mUploadFile = SPIFFS.open("/" + filename, "w");
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (mDebug) Serial.print(".");
      if (mUploadFile.write(upload.buf, upload.currentSize) != upload.currentSize) {
        if (mDebug) Serial.println(String(F("ERROR writing file ")) + String(mUploadFile.name()) + String(F("to SPIFFS.")));
        mUploadFile.close();
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      miUploadSize = upload.totalSize;
      if (mUploadFile.size() == upload.totalSize) {
        if (mDebug) Serial.println(String(F("Upload to SPIFFS Succeeded - uploaded: ")) + String(upload.totalSize) + String(F(".... rebooting now!")));
      } else {
        if (mDebug) Serial.println(String(F("Upload to SPIFFS FAILED: ")) + String(mUploadFile.size()) + String(F(" bytes of ")) + String(upload.totalSize));
      }
      mUploadFile.close();
      if (!upload.totalSize){
        if (mDebug) Serial.println("Removing: " + filename);
        SPIFFS.remove("/" + filename);
      }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
      miUploadSize = upload.totalSize;
      mUploadFile.close();
      if (mDebug) Serial.println(F("Upload to SPIFFS was aborted"));
    }
  }
}

//--------------------------------------------------------------------------------------------------------------------------------

bool  MyFunctionRequestHandler::handleStatic(ESP8266WebServer& server, String& actualUrl, const __FlashStringHelper* url, const __FlashStringHelper* staticFile, const __FlashStringHelper* contentType, bool mayCache){
  if (actualUrl.equals(String(url))){
    File file = SPIFFS.open(String(staticFile), String(F("r")).c_str());
    if (file){
      if (!mayCache){
        server.sendHeader(F("Cache-Control"), F("private, max-age=0, no-cache, no-store"));
      }
      server.streamFile(file, contentType);
      //server.send(200, String(F("text/html")).c_str());
      //server.client().write(file, 100);
      file.close();
      return true;
    }
  }

  return false;
}

void MyFunctionRequestHandler::httpReboot(ESP8266WebServer& server, String message) {
  String response = F("<!DOCTYPE html>"
                    "<html>"
                    "<head>"
                    "<title>Dynatrace UFO configuration changed. Rebooting now... </title>"
                    "<META http-equiv='refresh' content='10;URL=/'>"
                    "</head>"
                    "<body>"
                    "<center>");
  response +=       message;
  response +=      F("Device is being rebooted. Redirecting to homepage in 10 seconds...</center>"
                   "</body>"
                   "</html>");

  server.sendHeader(F("cache-control"), F("private, max-age=0, no-cache, no-store"));
  server.send(200, F("text/html"), response);

  if (mDebug) Serial.println(F("ESP.restart()"));
  ESP.restart();
}

String MyFunctionRequestHandler::parseFileName(String& path) {
  String filename;
  int lastIndex = path.lastIndexOf('\\');
  if (lastIndex < 0) {
    lastIndex = path.lastIndexOf('/');
  }
  if (lastIndex > 0) {
    filename = path.substring(lastIndex + 1);
  } else {
    filename = path;
  }

  filename.toLowerCase();
  return filename;
}

void MyFunctionRequestHandler::infoHandler(ESP8266WebServer& server) {

  String json = F("{");
  json += '"'; json += F("heap"); json += '"';
  json += ':';
  json += '"'; json +=  String(ESP.getFreeHeap()); json += '"';
  json += F(", "); 
  json += '"'; json += F("ssid"); json += '"';
  json += ':';
  json += '"'; json +=  String(WiFi.SSID()); json += '"';
  json += F(", "); 
  json += '"'; json += F("ipaddress"); json += '"';
  json += ':';
  json += '"'; json +=  WiFi.localIP().toString(); json += '"';
  json += F(", "); 
  json += '"'; json += F("ipgateway"); json += '"';
  json += ':';
  json += '"'; json +=  WiFi.gatewayIP().toString(); json += '"';
  json += F(", "); 
  json += '"'; json += F("ipdns"); json += '"';
  json += ':';
  json += '"'; json += WiFi.dnsIP().toString(); json += '"';
  json += F(", "); 
  json += '"'; json += F("ipsubnetmask"); json += '"';
  json += ':';
  json += '"'; json +=  WiFi.subnetMask().toString(); json += '"';
  json += F(", "); 
  json += '"'; json += F("macaddress"); json += '"';
  json += ':';
  json += '"'; json +=  WiFi.macAddress(); json += '"';
  json += F(", "); 
  json += '"'; json += F("hostname"); json += '"';
  json += ':';
  json += '"'; json +=  WiFi.hostname(); json += '"';
  json += F(", "); 
  json += '"'; json += F("apipaddress"); json += '"';
  json += ':';
  json += '"'; json +=  WiFi.softAPIP().toString(); json += '"';
  json += F(", "); 
  json += '"'; json += F("apconnectedstations"); json += '"';
  json += ':';
  json += '"'; json += String(WiFi.softAPgetStationNum()); json += '"';
  json += F(", "); 
  json += '"'; json += F("wifiautoconnect"); json += '"';
  json += ':';
  json += '"'; json += String(WiFi.getAutoConnect()); json += '"';
  json += F(", "); 
  json += '"'; json += F("firmwareversion"); json += '"';
  json += ':';
  json += '"'; json += String(FIRMWARE_VERSION); json += '"';
  json += '}';
  
  server.sendHeader(F("cache-control"), F("private, max-age=0, no-cache, no-store"));
  server.send(200, F("text/json"), json);
}

void MyFunctionRequestHandler::dtIntegrationHandler(ESP8266WebServer& server) {

  String json = F("{");
  json += '"'; json += F("enabled"); json += '"';
  json += ':';
  json += mpConfig->enabled ? F("true") : F("false");
  json += F(", "); 
  json += '"'; json += F("environmentid"); json += '"';
  json += ':';
  json += '"'; json += mpConfig->dynatraceEnvironmentID; json += '"';
  json += F(", "); 
  json += '"'; json += F("apikey"); json += '"';
  json += ':';
  json += '"'; json += mpConfig->dynatraceApiKey; json += '"';
  json += F(", "); 
  json += '"'; json += F("interval"); json += '"';
  json += ':';
  json += '"'; json += mpConfig->pollingIntervalS; json += '"';
  json += '}';
  
  server.sendHeader(F("cache-control"), F("private, max-age=0, no-cache, no-store"));
  server.send(200, F("text/json"), json);
}

void eepromSet(String content);

void MyFunctionRequestHandler::apiHandler(ESP8266WebServer& server) {
  mpIpDisplay->StopShowingIp();
  //applicationProblems = -1;  //force repaint display in client mode

  // adjust logo brightness (on/off right now)
  if (server.hasArg(F("logo"))) {
    if (server.arg(F("logo")).equals(F("on"))) {
      mpLedstripLogo->setBrightness(255);
    } else if (server.arg(F("logo")).equals(F("default"))) {
       ledsSetLogoDefault();
    } else {
       mpLedstripLogo->setBrightness(0);
    }
  }

  // to quickly set the RGB colors of the logo remotely
  if (server.hasArg(F("logoled"))) {
    byte led = byte(server.arg(F("logoled")).toInt());
    byte r = byte(server.arg(F("r")).toInt());
    byte g = byte(server.arg(F("g")).toInt());
    byte b = byte(server.arg(F("b")).toInt());
    mpLedstripLogo->setPixelColor(led, r, g, b);
  }

  if (server.hasArg(F("top_init"))){
    mpDisplayUpperRing->Init();
  }
  if (server.hasArg(F("top"))){
    String argument = server.arg(F("top"));
    unsigned int i = 0;
    unsigned int ret = 0;
    
    while (i < argument.length())
      i = mpDisplayUpperRing->ParseLedArg(argument, i);
  }
  if (server.hasArg(F("top_bg"))){
    String argument = server.arg(F("top_bg"));
    if (argument.length() == 6)
      mpDisplayUpperRing->SetBackground(argument.substring(2, 4) + argument.substring(0, 2) + argument.substring(4, 6)); 
  }
  if (server.hasArg(F("top_whirl"))){
    String argument = server.arg(F("top_whirl"));
    mpDisplayUpperRing->ParseWhirlArg(argument);
  } 
  if (server.hasArg(F("top_morph"))){
    String argument = server.arg(F("top_morph"));
    mpDisplayUpperRing->ParseMorphArg(argument);
  } 
    
  if (server.hasArg(F("bottom_init"))){
    mpDisplayLowerRing->Init();
  }
  if (server.hasArg(F("bottom"))){
    String argument = server.arg(F("bottom"));
    unsigned int i = 0;
    unsigned int ret = 0;
    
    while (i < argument.length())
      i = mpDisplayLowerRing->ParseLedArg(argument, i);
  }
  if (server.hasArg(F("bottom_bg"))){
    String argument = server.arg(F("bottom_bg"));
    if (argument.length() == 6)
      mpDisplayLowerRing->SetBackground(argument.substring(2, 4) + argument.substring(0, 2) + argument.substring(4, 6)); 
  }
  if (server.hasArg(F("bottom_whirl"))){
    String argument = server.arg(F("bottom_whirl"));
    mpDisplayLowerRing->ParseWhirlArg(argument);
  } 
  if (server.hasArg(F("bottom_morph"))){
    String argument = server.arg(F("bottom_morph"));
    mpDisplayLowerRing->ParseMorphArg(argument);
  } 

  if (server.hasArg(F("dynatrace-environmentid")) && server.hasArg(F("dynatrace-apikey")) && server.hasArg(F("dynatrace-interval"))) {
    if (mDebug) Serial.println(F("Storing Dynatrace SaaS/Managed environment integration settings"));
    mpConfig->enabled = server.hasArg(F("dynatrace-on"));
    mpConfig->dynatraceEnvironmentID = server.arg(F("dynatrace-environmentid"));
    mpConfig->dynatraceApiKey = server.arg(F("dynatrace-apikey"));
    mpConfig->pollingIntervalS = server.arg(F("dynatrace-interval")).toInt();
    mpConfig->SignalChange();
    mpConfig->Write();
    server.sendHeader(F("location"), F("/"));
    server.send(302);
    return;
  }

  if (server.hasArg(F("hostname"))) {
    String newWifiHostname = server.arg(F("hostname"));
    eepromSet(newWifiHostname);
  }

  // note its required to provide both arguments SSID and PWD
  if (server.hasArg(F("ssid")) && server.hasArg(F("pwd"))) {
    String newWifiSSID = server.arg(F("ssid"));
    String newWifiPwd = server.arg(F("pwd"));

    // if SSID is given, also update wifi credentials
    if (newWifiSSID.length()) {
      WiFi.mode(WIFI_STA);
      WiFi.begin(newWifiSSID.c_str(), newWifiPwd.c_str() );
    }

    if (mDebug) {
      Serial.println(String(F("New Wifi settings: ")) + newWifiSSID + F(" / ") + newWifiPwd);
      Serial.println(String(F("Restarting....")));
      Serial.flush();
    }

    httpReboot(server, String(F("New WIFI settings accepted. Mac address: ")) + WiFi.macAddress() + String(F("<p/>")));

  }

 
  server.sendHeader(F("cache-control"), F("private, max-age=0, no-cache, no-store"));
  server.send(200);
}



void MyFunctionRequestHandler::dynatracePostHandler(ESP8266WebServer& server) {
  /*if (server.hasArg(F("plain"))) { // POST data is stored in the "plain" argument
    StaticJsonBuffer<512> jsonBuffer;
    if (mDebug) Serial.println(String(F("Dynatrace JSON POST data: ")) + server.arg(F("plain")));
    JsonObject& jsonObject = jsonBuffer.parseObject(server.arg(F("plain")));

    // TODO HANDLE Dynatrace PROBLEM DATA ################################################################################################

  } else {
    if (mDebug) Serial.println(String(F("Dynatrace JSON POST data MISSING!")));
  }

  server.sendHeader(F("cache-control"), F("private, max-age=0, no-cache, no-store"));
  server.send(200);*/
}


  
