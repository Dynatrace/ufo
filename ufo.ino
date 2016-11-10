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
       4) DONE: web based firmware upload; firmware s (load from github or upload to device? or both?)
*/
//-------------------------------------------------------------------------------------------------------------------------

boolean debug = true;

//-------------------------------------------------------------------------------------------------------------------------

//#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <FS.h>
#include <Adafruit_DotStar.h>
#include <StreamString.h>
#include "DisplayCharter.h"
#include "MyRequestHandler.h"
#include "DataPolling.h"
#include "Config.h"
#include "defines.h"

//-------------------------------------------------------------------------------------------------------------------------

Adafruit_DotStar ledstrip_logo = Adafruit_DotStar(4, PIN_DOTSTAR_LOGO, PIN_DOTSTAR_CLOCK, DOTSTAR_BGR); //NOTE: in case the colors dont match, check out the last color order parameter
Adafruit_DotStar ledstrip_lowerring = Adafruit_DotStar(RING_LEDCOUNT, PIN_DOTSTAR_LOWERRING, PIN_DOTSTAR_CLOCK, DOTSTAR_BRG);
Adafruit_DotStar ledstrip_upperring = Adafruit_DotStar(RING_LEDCOUNT, PIN_DOTSTAR_UPPERRING, PIN_DOTSTAR_CLOCK, DOTSTAR_BRG);

DisplayCharter displayCharter_lowerring;
DisplayCharter displayCharter_upperring;

IPDisplay ipDisplay;
Config dTConfig(debug);
DataPolling dataPolling(&displayCharter_lowerring, &displayCharter_upperring, &ipDisplay, &dTConfig, debug);

boolean logo = true;

ESP8266WebServer   httpServer(80);

boolean wifiStationOK = false;
boolean wifiAPisConnected = false;
boolean wifiConfigMode = true;
boolean httpClientMode = false;

//-------------------------------------------------------------------------------------------------------------------------

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

    //disable dynatrace polling mode - otherwise the IP display wont be visible
    dTConfig.enabled = false;
    dTConfig.Write();
    
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
      //case WIFI_STAMODE_DHCP_TIMEOUT:                             THIS IS A NEW CONSTANT ENABLE WITH D SDK
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
  if (dTConfig.Read()) {
    if (debug) Serial.println(String(F("Dynatrace envid: ")) + dTConfig.dynatraceEnvironmentID + String(F(" apikey: ")) + dTConfig.dynatraceApiKey);
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

  httpServer.addHandler(new MyFunctionRequestHandler(&displayCharter_lowerring, &displayCharter_upperring, &ipDisplay, &ledstrip_logo, &dTConfig, debug));
  httpServer.begin();

}

unsigned int tick = 0;
unsigned long startMillis = millis();

void loop ( void ) {

  tick++;

  handleFactoryReset();

  if (!wifiConfigMode && wifiStationOK){
    unsigned long m = millis();
    if (m - startMillis > dTConfig.pollingIntervalS * 1000){
      startMillis = m;
      dataPolling.Poll();
    }
    ipDisplay.ProcessTick();   
  }
  
  httpServer.handleClient();

 
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
  ledstrip_logo.show();
  ledstrip_upperring.show();
  ledstrip_lowerring.show();
  
}



