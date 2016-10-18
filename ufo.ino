/*
   Requires Arduino IDE 1.6.8 or later
   ESP8266 Arduino library v2.2.0 or later
   Adafruit Huzzah ESP8266-E12, 4MB flash, uploads with 3MB SPIFFS (3MB filesystem of total 4MB) -- note that SPIFFS upload packages up everything from the "data" folder and uploads it via serial (same procedure as uploading sketch) or OTA. however, OTA is disabled by default
   Use Termite serial terminal software for debugging

   NOTE
    ESP8266 stores last set SSID and PWD in reserved flash area
    connect to SSID huzzah, and call http://192.168.4.1/api?ssid=<ssid>&pwd=<password> to set new SSID/PASSWORD



   TODO
       Web contents should provide
       1) DONE: get quickly started by configuring WIFI
       2) activate some demo scenarios (showcase fancy illumination patterns)
       3) DONE - see homepage (index.html): providing help about available API calls
       4) DONE: web based firmware upload; firmware updates (load from github or upload to device? or both?)
*/

boolean debug = true;
//#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <FS.h>
//#include <Adafruit_DotStar.h>

//#include <math.h>
#include <StreamString.h>

#define FIRMWARE_VERSION F(__DATE__ " " __TIME__)

#define DEFAULT_HOSTNAME F("ufo")
#define DEFAULT_APSSID F("ufo")
//#define DEFAULT_SSID "ufo"
//#define DEFAULT_PWD "ufo"

// ESP8266 Huzzah Pin usage
#define PIN_DOTSTAR_LOGO 2
#define PIN_DOTSTAR_LOWERRING 4
#define PIN_DOTSTAR_UPPERRING 5
#define PIN_DOTSTAR_CLOCK 14
#define PIN_FACTORYRESET 15

#include "DisplayCharter.h"

Adafruit_DotStar ledstrip_logo = Adafruit_DotStar(4, PIN_DOTSTAR_LOGO, PIN_DOTSTAR_CLOCK, DOTSTAR_BGR); //NOTE: in case the colors dont match, check out the last color order parameter
Adafruit_DotStar ledstrip_lowerring = Adafruit_DotStar(RING_LEDCOUNT, PIN_DOTSTAR_LOWERRING, PIN_DOTSTAR_CLOCK, DOTSTAR_BRG);
Adafruit_DotStar ledstrip_upperring = Adafruit_DotStar(RING_LEDCOUNT, PIN_DOTSTAR_UPPERRING, PIN_DOTSTAR_CLOCK, DOTSTAR_BRG);

DisplayCharter displayCharter_lowerring;
DisplayCharter displayCharter_upperring;

IPDisplay ipDisplay;

boolean logo = true;

ESP8266WebServer        httpServer ( 80 );
HTTPClient              httpClient;

boolean wifiStationOK = false;
boolean wifiAPisConnected = false;
boolean wifiConfigMode = true;
boolean httpClientOnlyMode = false;
long uploadSize = 0;

int applicationProblems = -1;
int serviceProblems = -1;
int infrastructureProblems = -1;

String dynatraceEnvironmentID; // needed for Config
String dynatraceApiKey; // needed for Config

//format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

// Handle Factory reset and enable Access Point mode to enable configuration
// *********TODO ********************************** DIFFERENTIATE LONG PRESS (reset all!) VS SHORT PRESS (exit client only mode to enable back website)
void handleFactoryReset() {
  if (digitalRead(PIN_FACTORYRESET)) {
    if (millis() < 10000) {  // ignore reset button during first 10 seconds of reboot to avoid looping resets due to fast booting of ESP8266
      return;
    }

    if (debug) {
      Serial.println(F("*********FACTORYRESET***********"));
      //WiFi.printDiag(Serial);
    }

    //delete config file to make sure we exit client only mode that disables the webserver
    configDelete();
    
    WiFi.disconnect(false); //disconnect and disable station mode; delete old config
    // default IP address for Access Point is 192.168.4.1
    //WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0)); // IP, gateway, netmask -- NOT STORED TO FLASH!!
    WiFi.softAP(String(DEFAULT_APSSID).c_str()); // default is open Wifi
    //WiFi.mode(WIFI_AP);
    if (debug) {
      Serial.println(String(F("Wifi reset to SSID: ")) + WiFi.SSID() + String(F(" pass: ")) + WiFi.psk());
      Serial.print(F("Wifi config mode enabled: Access point enabled at open Wifi SSID: "));
      Serial.println(DEFAULT_APSSID);
      Serial.println(F("Restarting...."));
      //WiFi.printDiag(Serial);
      Serial.flush();
    }
    ESP.restart();

  }
}



/*
void generateHandler() {
  if (httpServer.hasArg("size")) {
    Serial.println("size arg found" + httpServer.arg("size"));
    long bytes = httpServer.arg("size").toInt();
    String top = "<html><header>Generator</header><body>sending " + String(bytes) + " bytes of additional payload.<p>";
    String end = F("</body></html>");
    httpServer.setContentLength(bytes + top.length() + end.length());
    httpServer.sendHeader(F("cache-control"), F("private, max-age=0, no-cache, no-store"));
    httpServer.send(200);
    String chunk = "";
    httpServer.sendContent(top);
    String a = String("a");
    while (bytes > 0) {
      chunk = String("");
      long chunklen = bytes < 4096 ? bytes : 4096;
      while (chunk.length() <= chunklen) {
        chunk += a;
      }
      httpServer.sendContent(chunk);
      bytes -= chunklen;
    }
    httpServer.sendContent(end);
  }
}*/



void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
      case WIFI_EVENT_STAMODE_GOT_IP:
        if (debug) Serial.println(String(F("WiFi connected. IP address: ")) + String(WiFi.localIP().toString()) + String(F(" hostname: ")) + WiFi.hostname() + String(F("  SSID: ")) + WiFi.SSID());
        wifiStationOK = true;
        ipDisplay.ShowIp(WiFi.localIP().toString(), &displayCharter_lowerring);
        break;
      case WIFI_EVENT_STAMODE_DISCONNECTED:
        if (debug) Serial.println(F("WiFi client lost connection"));
        wifiStationOK = false;
        break;
      case WIFI_EVENT_STAMODE_CONNECTED:
        if (debug) Serial.println(F("WiFi client connected"));
        break;
      case WIFI_EVENT_STAMODE_AUTHMODE_CHANGE:
        if (debug) Serial.println(F("WiFi client authentication mode changed."));
        break;
      //case WIFI_STAMODE_DHCP_TIMEOUT:                             THIS IS A NEW CONSTANT ENABLE WITH UPDATED SDK
      //  Serial.println("WiFi client DHCP timeout reached.");
      //break;
      case WIFI_EVENT_SOFTAPMODE_STACONNECTED:
        if (debug) Serial.println(String(F("WiFi accesspoint: new client connected. Clients: "))  + String(WiFi.softAPgetStationNum()));
        if (WiFi.softAPgetStationNum() > 0) {
          wifiAPisConnected = true;
        }
        break;
      case WIFI_EVENT_SOFTAPMODE_STADISCONNECTED:
        if (debug) Serial.println(String(F("WiFi accesspoint: client disconnected. Clients: ")) + String(WiFi.softAPgetStationNum()));
        if (WiFi.softAPgetStationNum() > 0) {
          wifiAPisConnected = true;
        } else {
          wifiAPisConnected = false;
        }
        break;
      case WIFI_EVENT_SOFTAPMODE_PROBEREQRECVED:
        //Serial.println("WiFi accesspoint: probe request received.");
        break;
    }
  
}

void printSpiffsContents() {
  if (debug)
  {
    Dir dir = SPIFFS.openDir(F("/"));
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf(String(F("FS File: %s, size: %s\n")).c_str(), fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }
}

void setupSerial() {
  // checking availability of serial connection
  int serialtimeout = 5000; //ms
  Serial.begin ( 115200 );
  while (!Serial) {
    if (serialtimeout > 0) {
      //serialtimeout -= 50;
    } else {
      debug = false;
      break;
    }
    delay(50);
  }

  if (debug) {
    Serial.println(F(""));
    Serial.println(F(""));
    Serial.println(F(""));
    Serial.println(F("Welcome to Dynatrace UFO!"));
    Serial.print(F("UFO Firmware Version: "));
    Serial.println(FIRMWARE_VERSION);
    Serial.println(String(F("ESP8266 Bootversion: ")) + String(ESP.getBootVersion()));
    Serial.println(String(F("ESP8266 SDK Version: ")) + String(ESP.getSdkVersion()));
    Serial.println(String(F("Resetinfo: ")) + ESP.getResetInfo());
  }
}

// initialization routines
void setup ( void ) {
  setupSerial();

  pinMode(PIN_FACTORYRESET, INPUT); //, INPUT_PULLUP); use INPUT_PULLUP in case we put reset to ground; currently reset is doing a 3V signal


  ledsSetup();


  // initialize ESP8266 file system
  SPIFFS.begin();
  printSpiffsContents();

  // load non-wifi config from SPIFFS config.json file
  if (configRead()) {
    if (debug) Serial.println(String(F("Dynatrace envid: ")) + dynatraceEnvironmentID + String(F(" apikey: ")) + dynatraceApiKey);
  } else {
    if (debug) Serial.println(F("Loading Dynatrace config failed!"));
  }

  // initialize Wifi based on stored config
  WiFi.onEvent(WiFiEvent);
  WiFi.hostname(DEFAULT_HOSTNAME); // note, the hostname is not persisted in the ESP config like the SSID. so it needs to be set every time WiFi is started
  wifiConfigMode = WiFi.getMode() & WIFI_AP;

  if (debug) {
    Serial.println(String(F("Connecting to Wifi SSID: ")) + WiFi.SSID() + String(F(" as host ")) + WiFi.hostname());
    if (wifiConfigMode) {
      Serial.println(String(F("WiFi Configuration Mode - Access Point IP address: ")) + WiFi.softAPIP().toString());
    }
    //WiFi.printDiag(Serial);
  }

  
  
  // start webserver in Access Point mode ALWAYS and in Station mode only when not using HTTPS polling (which requires HTTP server turned off to conserve memory)
  if (wifiConfigMode || (dynatraceEnvironmentID.length() == 0) || (dynatraceApiKey.length() == 0)) {
    httpClientOnlyMode = false;

    // setup all web server routes; make sure to use / last
    #define STATICFILES_CACHECONTROL F("private, max-age=0, no-cache, no-store")
    String s =  F("/api");
    httpServer.on(s.c_str(), apiHandler );
    s = F("/dynatrace");
    httpServer.on(s.c_str(), HTTP_POST, dynatracePostHandler); // webhook URL for Dynatrace SaaS/Managed problem notifications
    s = F("/info");
    httpServer.on(s.c_str(), infoHandler );
    //httpServer.on ( "/gen", generateHandler );
    //httpServer.serveStatic("/index.html", SPIFFS, "/index.html", STATICFILES_CACHECONTROL);
    //httpServer.serveStatic("/app.js", SPIFFS, "/app.js", STATICFILES_CACHECONTROL);
    
    s = F("/phonon.min.css");
    httpServer.serveStatic(s.c_str(), SPIFFS, s.c_str());
    s = F("/phonon.min.js");
    httpServer.serveStatic(s.c_str(), SPIFFS, s.c_str());
    s = F("/phonon.min.js");
    httpServer.serveStatic(s.c_str(), SPIFFS, s.c_str());
    s = F("/forms.min.css");
    httpServer.serveStatic(s.c_str(), SPIFFS, s.c_str());
    s = F("/forms.min.js");
    httpServer.serveStatic(s.c_str(), SPIFFS, s.c_str());
    s = F("/icons.min.css");
    httpServer.serveStatic(s.c_str(), SPIFFS, s.c_str());
    s = F("/lists.min.css");
    httpServer.serveStatic(s.c_str(), SPIFFS, s.c_str());
    s = F("/phonon-base.min.css");
    httpServer.serveStatic(s.c_str(), SPIFFS, s.c_str());
    s = F("/phonon-core.min.js");
    httpServer.serveStatic(s.c_str(), SPIFFS, s.c_str());
    s = F("/font.woff");
    httpServer.serveStatic(s.c_str(), SPIFFS, s.c_str());
    s = F("/font.eot");
    httpServer.serveStatic(s.c_str(), SPIFFS, s.c_str());
    s = F("/font.svg");
    httpServer.serveStatic(s.c_str(), SPIFFS, s.c_str());
    s = F("/font.ttf");
    httpServer.serveStatic(s.c_str(), SPIFFS, s.c_str());
   
    //if (wifiConfigMode) {
    //    httpServer.serveStatic("/", SPIFFS, "/wifisettings.html", STATICFILES_CACHECONTROL);
    //    httpServer.serveStatic("/index.html", SPIFFS, "/wifisettings.html", STATICFILES_CACHECONTROL);
    //} else {
    s = F("/index.html");
    httpServer.serveStatic(String(F("/")).c_str(), SPIFFS, s.c_str(), String(STATICFILES_CACHECONTROL).c_str());
    httpServer.serveStatic(s.c_str(), SPIFFS, s.c_str(),  String(STATICFILES_CACHECONTROL).c_str());
    //}
    //httpServer.on ( "/", HTTP_GET, handleRoot );
    httpServer.onNotFound ( handleNotFound );
  
    // register firmware update HTTP server:
    //    To upload through terminal you can use: curl -F "image=@ufo.ino.bin" ufo.local/update
    //    Windows power shell use DOESNT WORK YET: wget -Method POST -InFile ufo.ino.bin -Uri ufo.local/update
    //httpServer.on("/", indexHtmlHandler);
    //httpServer.on("/update", HTTP_GET, indexHtmlHandler);
  
    s = F("/update");
    httpServer.on(s.c_str(), HTTP_GET, updateHandler);
    httpServer.on(s.c_str(), HTTP_POST, updatePostHandler, updatePostUploadHandler);
    
    httpServer.begin();
  } else {
    httpClientOnlyMode = true;
    urlSetup();
    if (debug) Serial.println(F("Entering HTTP Client only mode. Click the WIFI Reset button to reconfigure the UFO."));
  }

}

unsigned int tick = 0;
unsigned long trigger = 0;

void loop ( void ) {

  tick++;

  handleFactoryReset();

  if (httpClientOnlyMode) { // HTTP Server is turned off for less RAM usage!!
    // poll the problem status from Dynatrace SaaS/Managed
    if (dynatraceEnvironmentID.length() > 0) {
      ipDisplay.StopShowingIp();
      unsigned long m = millis();
      if (trigger < m) {
        pollDynatraceProblemAPI();
        trigger = m + 30 * 1000; //60*1000; // poll every minute 60*1000ms
      }
    }
  } else {
    httpServer.handleClient();
    ipDisplay.ProcessTick();
  }


  yield();
  ledstrip_logo.show();

 
  displayCharter_lowerring.Display(ledstrip_lowerring);
  displayCharter_upperring.Display(ledstrip_upperring);
  
  yield();

  // show AP mode in blue to tell user to configure WIFI; especially after RESET
  // blinking alternatively in blue when no client is connected to AP;
  // binking both rings in blue when at least one client is connected to AP
  if (wifiConfigMode) {
    if (wifiAPisConnected && (tick % 500 > 200)) {
      dotstarSetColor(ledstrip_upperring, 0, 0, 255);
      dotstarSetColor(ledstrip_lowerring, 0, 0, 255);
    } else {
      if (tick % 1000 > 500) {
        dotstarSetColor(ledstrip_upperring, 0, 0, 255);
        dotstarSetColor(ledstrip_lowerring, 0, 0, 0);
      } else {
        dotstarSetColor(ledstrip_lowerring, 0, 0, 255);
        dotstarSetColor(ledstrip_upperring, 0, 0, 0);
      }
    }
  } else { // blink yellow while not connected to wifi when it should
    if (!wifiStationOK && (tick % 500 > 375)) {
      dotstarSetColor(ledstrip_upperring, 255, 200, 0);
      dotstarSetColor(ledstrip_lowerring, 255, 200, 0);
    }
  }


  yield();
  ledstrip_upperring.show();
  ledstrip_lowerring.show();
}



