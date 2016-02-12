/*
 * Requires Arduino IDE 1.6.7 or later for running OTA updates. 
 * OTA update requires Python 2.7, OTA IDE plugin, ...  http://esp8266.github.io/Arduino/versions/2.1.0-rc2/doc/ota_updates/ota_updates.html#arduino-ide
 * Adafruit Huzzah ESP8266-E12, 4MB flash, uploads with 3MB SPIFFS (3MB filesystem of total 4MB)
 * Use Termite serial terminal software for debugging
 */

 /* NOTE  
  *  ESP8266 stores last set SSID and PWD in reserved flash area 
  *  connect to SSID huzzah, and call http://192.168.4.1/api?ssid=<ssid>&pwd=<password> to set new SSID/PASSWORD 
  *  TODO: WebUI doesnt load properly when connected in AP mode
  *        maybe because of "#define MAX_SOCK_NUM 4" in ethernet code?
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

#define DOTSTAR // if defined, DotStar LED strip code is enabled
#define OTA // if defined, Arduino IDE based OTA is enabled

#define FIRMWARE_VERSION "2016.02.09 experimental"

boolean debug = true;
const char *host = "huzzah";

// ESP8266 Huzzah Pin usage
#define PIN_NEOPIXEL 5
#define PIN_ONBOARDLED 13
#define PIN_FACTORYRESET 2

Adafruit_NeoPixel neopixel = Adafruit_NeoPixel(7, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
boolean centerpixel = false;
boolean centerpixelred = false;
boolean centerpixelgreen = false;
byte whirlr = 255;
byte whirlg = 255;
byte whirlb = 255;
boolean whirl = false;


//****************************** DOTSTAR **************************************
#include <Adafruit_DotStar.h>
#include <SPI.h>         // COMMENT OUT THIS LINE FOR GEMMA OR TRINKET
#define DOTSTAR_NUMPIXELS 150 // Number of LEDs in strip
#define DOTSTAR_DATAPIN    15
#define DOTSTAR_CLOCKPIN   4
#if defined(DOTSTAR)
  Adafruit_DotStar strip = Adafruit_DotStar(
    DOTSTAR_NUMPIXELS, DOTSTAR_DATAPIN, DOTSTAR_CLOCKPIN, DOTSTAR_BRG); 
    // note: using Hardware SPI it is a little faster
    // Adafruit_DotStar strip = Adafruit_DotStar(DOTSTAR_NUMPIXELS, DOTSTAR_BRG);
#endif

ESP8266WebServer server ( 80 );


// calculate rainbow colors
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
   return Adafruit_NeoPixel::Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return Adafruit_NeoPixel::Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
   WheelPos -= 170;
   return Adafruit_NeoPixel::Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

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



#if defined(DOTSTAR)
  // Runs 3 LEDs at a time along strip, cycling through red, green and blue.
  // This requires about 200 mA for all the 'on' pixels + 1 mA per 'off' pixel.
  int      head  = 0, tail = -3; // Index of first 'on' and 'off' pixels
  uint32_t color = 0xFF0000;      // 'On' color (starts red)

  void runDotStar() {
    /**strip.setPixelColor(head, color); // 'On' pixel at head
    strip.setPixelColor(tail, 0);     // 'Off' pixel at tail
    strip.show();                     // Refresh strip
    delay(20);                        // Pause 20 milliseconds (~50 FPS)
  
    if(++head >= DOTSTAR_NUMPIXELS) {         // Increment head index.  Off end of strip?
      head = 0;                       //  Yes, reset head index to start
      if((color >>= 8) == 0)          //  Next color (R->G->B) ... past blue now?
        color = 0xFF0000;             //   Yes, reset to red
    }
    if(++tail >= DOTSTAR_NUMPIXELS) tail = 0; // Increment, reset tail index**/
    for (int i = 0; i < DOTSTAR_NUMPIXELS; i++) {
      strip.setPixelColor(0, 0x000000);     // 'Off' pixel at tail
    }
    strip.setPixelColor(0, 0xFF0000);     
    strip.setPixelColor(10, 0x00FF00);     
    strip.show();                     // Refresh strip
  }
#endif

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
      centerpixel = true;
    } else if (server.arg("led").equals("red")) {
      Serial.println("lets turn led on");
      centerpixelred = true;
    } else if (server.arg("led").equals("green")) {
      Serial.println("lets turn led on");
      centerpixelgreen = true;
    } else {
      Serial.println("lets turn led off");
      centerpixel = false;
      centerpixelgreen = false;
      centerpixelred = false;
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

// initialization routines
void setup ( void ) {
  // setup neopixel
  neopixel.begin();
  neopixel.clear();
  neopixel.show();

  // Dotstar strip.begin();
  #if defined(DOTSTAR)
    strip.begin(); // Initialize pins for output
    strip.show();  // Turn all LEDs off ASAP
  #endif

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
 <
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

  // adjust centerpixel
  if (centerpixel) {
    if (tick % 50 == 0) {
      wheelcolor++;
    }
    if (centerpixelred)  neopixel.setPixelColor(0, 255, 0, 0); 
    else if (centerpixelgreen) neopixel.setPixelColor(0, 0, 255, 0); 
    else neopixel.setPixelColor(0, Wheel(wheelcolor));
  } else {
    neopixel.setPixelColor(0, 0, 0, 0);
  }

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
      neopixel.setPixelColor((p % 6)+1, r, g, b);
    }
    if (tick % 200 == 0) {
      whirlpos++;
    }
  } else {
    for (byte i = 1; i<7; i++) {
      neopixel.setPixelColor(i, 0, 0, 0);
    }
  }

  neopixel.show();

  #if defined(DOTSTAR)
    runDotStar();
  #endif  

}



