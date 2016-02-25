/*
 * Requires Arduino IDE 1.6.7 or later for running OTA updates. 
 * OTA update requires Python 2.7, OTA IDE plugin, ...  http://esp8266.github.io/Arduino/versions/2.1.0-rc2/doc/ota_updates/ota_updates.html#arduino-ide
 * Adafruit Huzzah ESP8266-E12, 4MB flash, uploads with 3MB SPIFFS (3MB filesystem of total 4MB)
 * Use Termite serial terminal software for debugging
 */

 /* NOTE  
  *  ESP8266 stores last set SSID and PWD in reserved flash area 
  *  connect to SSID huzzah, and call http://192.168.4.1/api?ssid=<ssid>&pwd=<password> to set new SSID/PASSWORD 
  *        
  *        
  * NOTE       
  *   WHEN Development OTA is enabled then the access point doesnt work properly HTTPserver accessing the SPIFFS fails/stucks
  *        
  *        
  * TODO       
  *     Web contents should provide   
  *     1) get quickly started by configuring WIFI
  *     2) activate some demo scenarios (showcase fancy illumination patterns)
  *     3) providing help about available API calls
  *     4) firmware updates (load from github or upload to device? or both?)
  */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <FS.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

//#define OTA // if defined, Arduino IDE based OTA is enabled

#define FIRMWARE_VERSION "2016.02.25 experimental"

boolean debug = true;
const char *host = "huzzah";

// ESP8266 Huzzah Pin usage
#define PIN_NEOPIXEL_LOGO 2
#define PIN_NEOPIXEL_LOWERRING 4
#define PIN_NEOPIXEL_UPPERRING 5
#define PIN_ONBOARDLED 13
#define PIN_FACTORYRESET 15


Adafruit_NeoPixel neopixel_logo = Adafruit_NeoPixel(4, PIN_NEOPIXEL_LOGO, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel neopixel_lowerring = Adafruit_NeoPixel(15, PIN_NEOPIXEL_LOWERRING, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel neopixel_upperring = Adafruit_NeoPixel(15, PIN_NEOPIXEL_UPPERRING, NEO_GRB + NEO_KHZ800);
boolean logo = true;
boolean logored = false;
boolean logogreen = false;
byte whirlr = 255;
byte whirlg = 255;
byte whirlb = 255;
boolean whirl = false;

ESP8266WebServer server ( 80 );



//format bytes
String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}

// HTTP not found response
void handleNotFound() {
	digitalWrite ( PIN_ONBOARDLED, 0 );
	String message = "ESP8266 File Not Found\n\n";
	message += "URI: ";
	message += server.uri();
	message += "\nMethod: ";
	message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
	message += "\nArguments: ";
	message += server.args();
	message += "\n";

	for ( uint8_t i = 0; i < server.args(); i++ ) {
		message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
	}

	server.send ( 404, "text/plain", message );
	digitalWrite ( PIN_ONBOARDLED, 0 );
} 


// handle REST api call to set new WIFI credentials; note that URL encoding can be used
// api?ssid=<ssid>&pwd=<password>
boolean newWifi = false;;
String newWifiSSID;
String newWifiPwd;

void handleNewWifiSettings() {
   if (newWifi) {
       //wifi_station_disconnect(); // not sure this disconnect is needed
       WiFi.persistent(true);
       WiFi.mode(WIFI_AP_STA);
       WiFi.begin(newWifiSSID.c_str(), newWifiPwd.c_str() );
       Serial.println("New Wifi settings: " + newWifiSSID + " / " + newWifiPwd);
       newWifi = false;
       //newWifiPwd = String();
       //newWifiSSID = String();
       Serial.println("Restarting....");
       Serial.flush();
       ESP.restart();
   }
}



void infoHandler() {
  digitalWrite ( PIN_ONBOARDLED, 1 );
  
  String json = "{";
  json += "\"heap\":\""+String(ESP.getFreeHeap())+"\"";
  json += ", \"ssid\":\""+String(WiFi.SSID())+"\"";
  json += ", \"ipaddress\":\""+WiFi.localIP().toString()+"\"";
  json += ", \"ipgateway\":\""+WiFi.gatewayIP().toString()+"\"";
  json += ", \"ipdns\":\""+WiFi.dnsIP().toString()+"\"";
  json += ", \"ipsubnetmask\":\""+WiFi.subnetMask().toString()+"\"";
  json += ", \"macaddress\":\""+WiFi.macAddress()+"\"";
  json += ", \"hostname\":\""+WiFi.hostname()+"\"";
  json += ", \"apipaddress\":\""+WiFi.softAPIP().toString()+"\"";
  json += ", \"apconnectedstations\":\""+String(WiFi.softAPgetStationNum())+"\"";
  json += ", \"wifiautoconnect\":\""+String(WiFi.getAutoConnect())+"\"";
  json += ", \"firmwareversion\":\""+String(FIRMWARE_VERSION)+"\"";
  json += "}";
  server.send(200, "text/json", json);
  server.sendHeader("cache-control", "private, max-age=0, no-cache, no-store");
  json = String();
  digitalWrite ( PIN_ONBOARDLED, 0 );
}

void apiHandler() {
  digitalWrite ( PIN_ONBOARDLED, 1 );
  if (server.hasArg("led")) {
    Serial.println("LED arg found" + server.arg("led"));
    if (server.arg("led").equals("on")) {
      Serial.println("lets turn led on");
      logo = true;
    } else if (server.arg("led").equals("red")) {
      Serial.println("lets turn led on");
      logored = true;
    } else if (server.arg("led").equals("green")) {
      Serial.println("lets turn led on");
      logogreen = true;
    } else {
      Serial.println("lets turn led off");
      logo = false;
      logogreen = false;
      logored = false;
    }
  }
  if (server.hasArg("whirl")) {
    Serial.println("WHIRL arg found" + server.arg("whirl"));
    String ws = server.arg("whirl");
    if (ws.equals("r")) {
      whirlr = 255; whirlg=0; whirlb=0;
      whirl = true;
      Serial.println("whirl red");      
    } else if (ws.equals("g")) {
      whirlr = 0; whirlg=255; whirlb=0;
      whirl = true;
      Serial.println("whirl green");      
    } else if (ws.equals("b")) {
      whirlr = 0; whirlg=0; whirlb=255;
      whirl = true;
      Serial.println("whirl blue");      
    } else if (ws.equals("off")) {
      whirl = false;
      Serial.println("whirl off");      
    } else if (ws.equals("on")) {
      whirl = true;
      Serial.println("whirl on");      
    }

  }

  // note its required to provide both arguments SSID and PWD
  if (server.hasArg("ssid") && server.hasArg("pwd")) {
    newWifi = true;
    newWifiSSID = server.arg("ssid");
    newWifiPwd = server.arg("pwd");
  } 

  server.send(200);
  server.sendHeader("cache-control", "private, max-age=0, no-cache, no-store");
  digitalWrite ( PIN_ONBOARDLED, 0 );
}

void generateHandler() {
  digitalWrite ( PIN_ONBOARDLED, 1 );
  if (server.hasArg("size")) {
    Serial.println("size arg found" + server.arg("size"));
    long bytes = server.arg("size").toInt();
    String top = "<html><header>Generator</header><body>sending " + String(bytes) + " bytes of additional payload.<p>";
    String end = "</body></html>";
    server.setContentLength(bytes+top.length()+end.length());
    server.send(200);
    server.sendHeader("cache-control", "private, max-age=0, no-cache, no-store");
    String chunk = "";
    server.sendContent(top);
    String a = String("a");
    while (bytes > 0) {
      chunk = String("");
      long chunklen = bytes<4096?bytes:4096;
      while (chunk.length() <= chunklen) {
        chunk += a;
      }
      server.sendContent(chunk);
      bytes-=chunklen;
    }
    server.sendContent(end);
  }  
  
  digitalWrite ( PIN_ONBOARDLED, 0 );
}



// initialization routines
void setup ( void ) {
  // setup neopixel
  neopixel_logo.begin();
  neopixel_logo.clear();
  neopixel_logo.show();
  neopixel_lowerring.begin();
  neopixel_lowerring.clear();
  neopixel_lowerring.show();
  neopixel_upperring.begin();
  neopixel_upperring.clear();
  neopixel_upperring.show();

  // checking availability of serial connection
  int serialtimeout = 5000; //ms
  while(!Serial) {
    if (serialtimeout >0) {
      serialtimeout -= 50; 
    } else { 
      debug = false;
      break;
    }
    delay(50);
  }

  Serial.begin ( 115200 );

  if (debug) {
    Serial.print("GPIO4: "); Serial.println(digitalRead(PIN_FACTORYRESET));
  }
  
  pinMode(PIN_FACTORYRESET, INPUT_PULLUP);

  // Handle Factory reset and enable Access Point mode to enable configuration
  if (!digitalRead(PIN_FACTORYRESET)) {
    Serial.println("*********FACTORYRESET***********");
    //WiFi.persistent(true);
    //WiFi.mode(WIFI_AP_STA);
    WiFi.mode(WIFI_AP);
    //WiFi.hostname("huzzah");
    // default IP address for Access Point is 192.168.4.1
    //WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0)); // IP, gateway, netmask
  }
 
  WiFi.begin(); // load SSID and PWD from internal memory
  Serial.println ( "" );


  // Wait for connection
  int wifiretries = 30; // 30*500ms = 15 seconds
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
    if (!wifiretries) 
      break;
    wifiretries--;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println ( "" );
    Serial.print ( "Connected to " );
    Serial.println ( WiFi.SSID() );
    Serial.print ( "IP address: " );
    Serial.println ( WiFi.localIP() );
  
    if ( MDNS.begin ( "esp8266" ) ) { // Arduino IDE searches for that MDNS name
      Serial.println ( "MDNS responder started" );
    }
  } else {
    WiFi.disconnect();
    Serial.println("Could not connect to SSID: " +  String(WiFi.SSID()));
    Serial.println("Please connect your smartphone or wifi-enabled device to SSID: huzzah IP: 192.168.4.1");
  }

  // Launch Access Point
  
  WiFi.softAP("huzzah");
  Serial.println("Access Point IP Address: " + WiFi.softAPIP().toString());
  
  // initialize ESP8266 file system
  SPIFFS.begin();
  if (debug)
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {    
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }

  // setup all web server routes; make sure to use / last
  #define STATICFILES_CACHECONTROL "private, max-age=0, no-cache, no-store"
  server.on ( "/api", apiHandler );
  server.on ( "/info", infoHandler );
  server.on ( "/gen", generateHandler );
  server.serveStatic("/index.html", SPIFFS, "/index.html", STATICFILES_CACHECONTROL);
  server.serveStatic("/test.html", SPIFFS, "/test.html", STATICFILES_CACHECONTROL);
  server.serveStatic("/app.js", SPIFFS, "/app.js", STATICFILES_CACHECONTROL);
  server.serveStatic("/phonon.css", SPIFFS, "/phonon.min.css", STATICFILES_CACHECONTROL);
  server.serveStatic("/phonon.js", SPIFFS, "/phonon.min.js", STATICFILES_CACHECONTROL);
  server.serveStatic("/fonts/material-design-icons.eot", SPIFFS, "/fonts/mdi.eot", STATICFILES_CACHECONTROL);
  server.serveStatic("/fonts/material-design-icons.svg", SPIFFS, "/fonts/mdi.svg", STATICFILES_CACHECONTROL);
  server.serveStatic("/fonts/material-design-icons.ttf", SPIFFS, "/fonts/mdi.ttf", STATICFILES_CACHECONTROL);
  server.serveStatic("/fonts/material-design-icons.woff", SPIFFS, "/fonts/mdi.woff", STATICFILES_CACHECONTROL);
  server.serveStatic("/", SPIFFS, "/index.html", STATICFILES_CACHECONTROL);
  //server.on ( "/", HTTP_GET, handleRoot );
  /*server.on ( "/test.svg", drawGraph );
  server.on ( "/inline", []() {
    server.send ( 200, "text/plain", "this works as well" );
  } );*/
  server.onNotFound ( handleNotFound );
  server.begin();
  Serial.println ( "HTTP server started" );

  #if defined(OTA)
   // Port defaults to 8266
    // ArduinoOTA.setPort(8266);
  
    // Hostname defaults to esp8266-[ChipID]
    // ArduinoOTA.setHostname("myesp8266");
  
    // No authentication by default
    // ArduinoOTA.setPassword((const char *)"123");

    ArduinoOTA.onStart([]() {
      Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
  #endif //OTA
  
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
 
  pinMode ( PIN_ONBOARDLED, OUTPUT );
  digitalWrite ( PIN_ONBOARDLED, LOW );
}


unsigned int tick = 0;
byte whirlpos = 0;
byte wheelcolor = 0;

void loop ( void ) {
  tick++;

  handleNewWifiSettings();
  server.handleClient();
  #if defined(OTA)
    ArduinoOTA.handle();
  #endif

  // adjust logo colors
  if (logored) {
    neopixel_logo.setPixelColor(0, 255, 0, 0); 
    neopixel_logo.setPixelColor(1, 255, 0, 0); 
    neopixel_logo.setPixelColor(2, 255, 0, 0); 
    neopixel_logo.setPixelColor(3, 255, 0, 0); 
  } else if (logogreen) {
    neopixel_logo.setPixelColor(0, 0, 255, 0); 
    neopixel_logo.setPixelColor(1, 0, 255, 0); 
    neopixel_logo.setPixelColor(2, 0, 255, 0); 
    neopixel_logo.setPixelColor(3, 0, 255, 0); 
  } else {
    neopixel_logo.setPixelColor(0, 190, 223, 42); // north-lime BEDF2A
    neopixel_logo.setPixelColor(1, 125, 197, 64); // east-green 7DC540
    neopixel_logo.setPixelColor(2, 133, 68, 179); // south-purple 8544B3
    neopixel_logo.setPixelColor(3, 0, 166, 251); // west-blue 00A6FB
  }

  // adjust logo brightness (on/off right now)
  if (logo) {
    neopixel_logo.setBrightness(255);
  } else {
    neopixel_logo.setBrightness(0);
  }
  neopixel_logo.show();

  if (whirl) {
    byte p;
    byte r;
    byte g;
    byte b;
    for (byte i = 0; i<6; i++) {
      p = i + whirlpos;      
      if (i == 0 || i == 5) {
        r = whirlr/4;
        g = whirlg/4;
        b = whirlb/4;
      } else if (i == 1 || i == 4) {
        r = whirlr/2;
        g = whirlg/2;
        b = whirlb/2;
      } else {
        r = whirlr;
        g = whirlg;
        b = whirlb;
      }
      neopixel_upperring.setPixelColor((p % 15), r, g, b);
    }
    if (tick % 50 == 0) {
      whirlpos++;
    }
  } else {
    for (byte i = 0; i<15; i++) {
      neopixel_upperring.setPixelColor(i, 0, 0, 0);
    }
  }

  neopixel_upperring.show();
  neopixel_lowerring.show();
}



