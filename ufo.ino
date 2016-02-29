/*
 * Requires Arduino IDE 1.6.7 or later for running OTA updates. 
 * OTA update requires Python 2.7, OTA IDE plugin, ...  http://esp8266.github.io/Arduino/versions/2.1.0-rc2/doc/ota_updates/ota_updates.html#arduino-ide
 * Adafruit Huzzah ESP8266-E12, 4MB flash, uploads with 3MB SPIFFS (3MB filesystem of total 4MB) -- note that SPIFFS upload packages up everything from the "data" folder and uploads it via serial (same procedure as uploading sketch) or OTA. however, OTA is disabled by default
 * Use Termite serial terminal software for debugging
 */

 /* NOTE  
  *  ESP8266 stores last set SSID and PWD in reserved flash area 
  *  connect to SSID huzzah, and call http://192.168.4.1/api?ssid=<ssid>&pwd=<password> to set new SSID/PASSWORD 
  *        
  *        
  *        
  * TODO       
  *     Web contents should provide   
  *     1) get quickly started by configuring WIFI
  *     2) activate some demo scenarios (showcase fancy illumination patterns)
  *     3) providing help about available API calls
  *     4) firmware updates (load from github or upload to device? or both?)
  *     5) smartconfig https://tzapu.com/esp8266-smart-config-esp-touch-arduino-ide/
  *     6) see also https://github.com/tzapu/WiFiManager/blob/master/WiFiManager.cpp how others deal with Wifi config
  *     7) switch to fastlib for neopixels?
  */

boolean debug = true;
#define DEBUG_ESP_HTTP_SERVER

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
//#include <ESP8266mDNS.h>
//#include <WiFiUdp.h>
//#include <ArduinoOTA.h>
#include <Wire.h>
#include <FS.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>
#include <StreamString.h>

#define FIRMWARE_VERSION "2016.02.28 experimental"


#define DEFAULT_HOSTNAME "ufo"
#define DEFAULT_APSSID "ufo"
#define DEFAULT_SSID "ufo"
#define DEFAULT_PWD "ufo"

// ESP8266 Huzzah Pin usage
#define PIN_NEOPIXEL_LOGO 2
#define PIN_NEOPIXEL_LOWERRING 4
#define PIN_NEOPIXEL_UPPERRING 5
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

ESP8266WebServer        httpServer ( 80 );

/*
const char* indexHtml = R"indexhtml(<html>

    <head>
        <meta charset='utf-8' />
        <meta name='format-detection' content='telephone=no' />
        <meta name='viewport' content='user-scalable=no, initial-scale=1, maximum-scale=1, minimum-scale=1, width=device-width, height=device-height' />

        <link rel='stylesheet' href='phonon.min.css' />
        <title>Dynatrace UFO</title>
    </head>
    <body>
       <!-- Panel tags go here -->
      <div id="wifisettings" class="panel-full">
          <header class="header-bar">
              <button class="btn pull-right icon icon-close" data-panel-close="true"></button>
              <div class="center">
                  <h1 class="title">Wifi Settings</h1>
              </div>
          </header> 
          <div class="content">
            <!--<span> TODO: Please not this mac address <macaddress> for later lookup of IP address in DHCP table or for assigning a fixed IP address through DHCP </span>-->
            <input type="text" placeholder="SSID" id="ssid">
            <input type="password" placeholder="********" id="wifipwd">
            <button id="submitwifisettings" class="btn fit-parent primary">Apply WIFI setting</button>
         </div>
      </div>
  
      <!-- Side Panel tags go here -->
  
      <!-- Notification tags go here -->
  
      <!-- Dialog tags go here -->
  
  
      <!-- the home page is the default one. This page does not require to call its content because we define on its tag.-->
      <home data-page="true">
         <header class="header-bar">
            <div class="center">
                <h1 class="title">Huzzah Test</h1>
            </div>
            <button class="btn pull-right icon icon-settings" data-panel-id="wifisettings"></button>
         </header>
         <div class="content">
            <ul class="list">
            <li class="divider">IoT Actions</li>
            <li>
              <span class="padded-list">LED</span>
                <button class="btn btn-flat primary pull-right" data-api="led=off" id="led1off">Off</button>
              <button class="btn btn-flat primary pull-right" data-api="led=on" id="led1on">On</button>
            </li> 
            <li>
              <span class="padded-list">WHIRL</span>
                <button class="btn btn-flat primary pull-right" data-api="whirl=off" id="whirloff">Off</button>
              <button class="btn btn-flat primary pull-right" data-api="whirl=on" id="whirlon">On</button>
            </li>
            <li>
              <span class="padded-list">WHIRL COLOR</span>
                <button class="btn btn-flat negative pull-right" data-api="whirl=r" id="whirlred">R</button>
              <button class="btn btn-flat positive pull-right" data-api="whirl=g" id="whirlgreen">G</button>
            </li>
            <li>
              <a href="#!pageinfo" class="padded-list">Info</a>
            </li>
            <li>
              <a href="#!pagefirmwareupdate" class="padded-list">Firmware update</a>
            </li>
          </ul> 
        </div>
      </home>
  
      <!-- for the second page, Phonon will load its content. --> 
      <pageinfo data-page="true">
        <header class="header-bar">
          <button class="btn icon icon-arrow-back pull-left" data-navigation="$previous-page"></button>
          <div class="center">
            <h1 class="title">Info</h1>
          </div>
        </header>
        <div class="content">
          <ul class="list">
            <li class="divider">System</li>
            <!-- TODO class=pull-right doesnt work for wider strings -->
            <li class="padded-list">Heap: <span class="pull-right" id="infoheap">-</span></li>
            <li class="padded-list">Firmware: <span id="infofirmwareversion">-</span></li>
            <li class="divider">Network</li>
            <li class="padded-list">SSID: <span id="infossid">-</span></li>
            <li class="padded-list">IP: <span id="infoipaddress">-</span></li>
            <li class="padded-list">Subnetmask: <span id="infoipsubnetmask">-</span></li>
            <li class="padded-list">Gateway: <span id="infoipgateway">-</span></li>
            <li class="padded-list">DNS: <span id="infoipdns">-</span></li>
            <li class="padded-list">Hostname: <span id="infohostname">-</span></li>
            <li class="padded-list">Macaddress: <span id="infomacaddress">-</span></li>
            <li class="padded-list">Wifi autoconnect: <span id="infowifiautoconnect">-</span></li>          <li class="divider">Accesspoint</li>
            <li class="padded-list">IP: <span id="infoapipaddress">-</span></li>
            <li class="padded-list">Connected stations: <span id="infoapconnectedstations">-</span></li>
          </ul>
        </div> 
      </pageinfo>
  
      <pagefirmwareupdate data-page="true">
        <header class="header-bar">
          <button class="btn icon icon-arrow-back pull-left" data-navigation="$previous-page"></button>
          <div class="center">
            <h1 class="title">Firmware upload</h1>
          </div>
        </header>
        <div class="content">
          <form method='POST' action='/update' enctype='multipart/form-data'> 
             <input type='file' name='update'> 
             <input type='submit' value='Update'> 
          </form> 
          <div class="input-wrapper">
              <input class="with-label" type="file" name="update" id="firmwarefilename">
              <label class="floating-label" for="firmwarefilename">ufo.ino.bin</label>
              <input class="with-label" type="submit" value="Update">
          </div>          
        </div> 
      </pagefirmwareupdate>

        <!-- scripts -->
        <script src="phonon.min.js"></script>

        <!-- our app config -->
        <!-- <script src="app.js"></script> -->
    <script>
      phonon.options({
        navigator: {
          defaultPage: 'home',
          animatePages: true,
          enableBrowserBackButton: true
          //templateRootDirectory: ''
        },
        i18n: null // for this example, we do not use internationalization
      });


      var app = phonon.navigator();


      app.on({page: 'home', preventClose: false, content: null} , function(activity) {

        var onWifiSettingsSubmit = function(evt) {
          var target = evt.target;
          //action = 'ok';

          
          var req = phonon.ajax({
            method: 'GET',
            url: 'api?ssid=' + document.querySelector('#ssid').value + '&pwd=' + document.querySelector('#wifipwd').value,
            //crossDomain: false,
            dataType: 'json',
            //contentType: '',
            //data: {'ssid': document.querySelector('#ssid').value, 'pwd': document.querySelector('#wifipwd').value}, 
            //data: { 'key1': 'val1', 'key2': 'val2'},
            //timeout: 5000,
            success: function(res, xhr) {
              console.log(res);
            },
            
            error: function(res, flagError, xhr) {
              console.error(flagError);
              console.log(res);
            }
          }); 

        }; 
        
        
        var onApi = function(evt) {
          var target = evt.target;
          var apicall = target.getAttribute('data-api');
          console.log(apicall);

          var req = phonon.ajax({
            method: 'GET',
            url: 'api?'+apicall,
            dataType: 'json',
            //contentType: '',
            //data: {'ssid': document.querySelector('#ssid').value, 'pwd': document.querySelector('#wifipwd').value}, 
            //data: { 'key1': 'val1', 'key2': 'val2'},
            //timeout: 5000,
            success: function(res, xhr) {
              console.log(res);
            },
            
            error: function(res, flagError, xhr) {
              console.error(flagError);
              console.log(res);
            }
          }); 

        }; 

        activity.onCreate(function() {
          document.querySelector('#submitwifisettings').on('tap', onWifiSettingsSubmit);
          document.querySelector('#led1on').on('tap', onApi);
          document.querySelector('#led1off').on('tap', onApi);
          document.querySelector('#whirlon').on('tap', onApi);
          document.querySelector('#whirloff').on('tap', onApi);
          document.querySelector('#whirlred').on('tap', onApi);
          document.querySelector('#whirlgreen').on('tap', onApi);
        }) 
      });

      document.on('pageopened', function(evt) {
        console.log(evt.detail.page + ' is opened');
        if (evt.detail.page == "pageinfo") {
          
            var req = phonon.ajax({
            method: 'GET',
            url: 'info',
            //crossDomain: false,
            dataType: 'text/json',
            success: function(res, xhr) {
              console.log(xhr.response);
              var result = JSON.parse(xhr.response);
              document.getElementById('infossid').innerHTML = result.ssid;
              document.getElementById('infoipaddress').innerHTML = result.ipaddress;
              document.getElementById('infoheap').innerHTML = result.heap;
              document.getElementById('infoipgateway').innerHTML = result.ipgateway;
              document.getElementById('infoipdns').innerHTML = result.ipdns;
              document.getElementById('infoipsubnetmask').innerHTML = result.ipsubnetmask;
              document.getElementById('infomacaddress').innerHTML = result.macaddress;
              document.getElementById('infohostname').innerHTML = result.hostname;
              document.getElementById('infoapipaddress').innerHTML = result.apipaddress;
              document.getElementById('infoapconnectedstations').innerHTML = result.apconnectedstations;
              document.getElementById('infowifiautoconnect').innerHTML = result.wifiautoconnect;
              document.getElementById('infofirmwareversion').innerHTML = result.firmwareversion;
            },
            error: function(res, flagError, xhr) {
              console.error(flagError);
              console.log(res);
            }
          }); 
        }
      });

      app.on({page: 'pageinfo', preventClose: false, content: null} , function(activity) {
      });
    </script>


    <script>
    // Let's go!
    app.start();
          
    </script>
</body>
</html>)indexhtml"; */

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
	String message = "ESP8266 File Not Found\n\n";
	message += "URI: ";
	message += httpServer.uri();
	message += "\nMethod: ";
	message += ( httpServer.method() == HTTP_GET ) ? "GET" : "POST";
	message += "\nArguments: ";
	message += httpServer.args();
	message += "\n";

	for ( uint8_t i = 0; i < httpServer.args(); i++ ) {
		message += " " + httpServer.argName ( i ) + ": " + httpServer.arg ( i ) + "\n";
	}

	httpServer.send ( 404, "text/plain", message );
} 


// handle REST api call to set new WIFI credentials; note that URL encoding can be used
// api?ssid=<ssid>&pwd=<password>
boolean newWifi = false;;
String newWifiSSID;
String newWifiPwd;
String newWifiHostname;

void handleNewWifiSettings() {
   if (newWifi) {
       //wifi_station_disconnect(); // not sure this disconnect is needed
       WiFi.disconnect();
       WiFi.persistent(true);

       // if SSID is given, also update wifi credentials
       if (newWifiSSID.length()) {
          WiFi.mode(WIFI_STA);
          WiFi.begin(newWifiSSID.c_str(), newWifiPwd.c_str() );
       } 
       //newWifi = false;
       //newWifiPwd = String();
       //newWifiSSID = String();
       //newWifiHostname = String();
       if (debug) {
         Serial.println("New Wifi settings: " + newWifiSSID + " / " + newWifiPwd);
         Serial.println("Restarting....");
         Serial.flush();
       }
       ESP.restart();
   }
}

// Handle Factory reset and enable Access Point mode to enable configuration
void handleFactoryReset() {
  if (digitalRead(PIN_FACTORYRESET)) {
    if (millis() < 10000) {  // ignore reset button during first 10 seconds of reboot to avoid looping resets due to fast booting of ESP8266
      return;
    }
    if (debug) {
      Serial.println("*********FACTORYRESET***********");
      WiFi.printDiag(Serial); 
    }
    WiFi.disconnect(true); //delete old config
    WiFi.persistent(true);
    //WiFi.mode(WIFI_AP_STA);
    WiFi.mode(WIFI_AP);
    // default IP address for Access Point is 192.168.4.1
    WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0)); // IP, gateway, netmask

    WiFi.softAP(DEFAULT_APSSID); // default is open Wifi
    WiFi.begin(DEFAULT_SSID, DEFAULT_PWD);
    if (debug) {
      Serial.println("New Wifi client SSID: " + String(DEFAULT_SSID) + " pass: " + String(DEFAULT_PWD));
      Serial.println("New Access point SSID: " + String(DEFAULT_APSSID));
      Serial.println("Restarting....");
      WiFi.printDiag(Serial); 
      Serial.flush();
    }    
  }
}

void infoHandler() {
  
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
  httpServer.send(200, "text/json", json);
  httpServer.sendHeader("cache-control", "private, max-age=0, no-cache, no-store");
  json = String();
}

void apiHandler() {
  if (httpServer.hasArg("led")) {
    Serial.println("LED arg found" + httpServer.arg("led"));
    if (httpServer.arg("led").equals("on")) {
      Serial.println("lets turn led on");
      logo = true;
    } else if (httpServer.arg("led").equals("red")) {
      Serial.println("lets turn led on");
      logored = true;
    } else if (httpServer.arg("led").equals("green")) {
      Serial.println("lets turn led on");
      logogreen = true;
    } else {
      Serial.println("lets turn led off");
      logo = false;
      logogreen = false;
      logored = false;
    }
  }
  if (httpServer.hasArg("whirl")) {
    Serial.println("WHIRL arg found" + httpServer.arg("whirl"));
    String ws = httpServer.arg("whirl");
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
  if (httpServer.hasArg("ssid") && httpServer.hasArg("pwd")) {
    newWifi = true;
    newWifiSSID = httpServer.arg("ssid");
    newWifiPwd = httpServer.arg("pwd");
  } 
  if (httpServer.hasArg("hostname")) {
    newWifi = true;
    newWifiHostname = httpServer.arg("hostname");
  }

  httpServer.send(200);
  httpServer.sendHeader("cache-control", "private, max-age=0, no-cache, no-store");
}

void generateHandler() {
  if (httpServer.hasArg("size")) {
    Serial.println("size arg found" + httpServer.arg("size"));
    long bytes = httpServer.arg("size").toInt();
    String top = "<html><header>Generator</header><body>sending " + String(bytes) + " bytes of additional payload.<p>";
    String end = "</body></html>";
    httpServer.setContentLength(bytes+top.length()+end.length());
    httpServer.send(200);
    httpServer.sendHeader("cache-control", "private, max-age=0, no-cache, no-store");
    String chunk = "";
    httpServer.sendContent(top);
    String a = String("a");
    while (bytes > 0) {
      chunk = String("");
      long chunklen = bytes<4096?bytes:4096;
      while (chunk.length() <= chunklen) {
        chunk += a;
      }
      httpServer.sendContent(chunk);
      bytes-=chunklen;
    }
    httpServer.sendContent(end);
  }  
}

void updatePostHandler() {


    // handler for the /update form POST (once file upload finishes)
    httpServer.sendHeader("Connection", "close");
    httpServer.sendHeader("Access-Control-Allow-Origin", "*");
    StreamString error;
    if (Update.hasError()) {
      Update.printError(error);
    }
    httpServer.send(200, "text/plain", (Update.hasError())?error:"OK - "+String(Update.size()) + " bytes transferred");
    ESP.restart();
}

void updatePostUploadHandler() {
  // handler for the file upload, get's the sketch bytes, and writes
  // them through the Update object
  HTTPUpload& upload = httpServer.upload();
  if(upload.status == UPLOAD_FILE_START){
    //WiFiUDP::stopAll(); needed for MDNS or the like?
    if (debug) Serial.println("Update: " + upload.filename);
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if(!Update.begin(maxSketchSpace)){//start with max available size
      if (debug) Update.printError(Serial);
    }
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if (debug) Serial.print(".");
    if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
      if (debug) Update.printError(Serial);
    }
  } else if(upload.status == UPLOAD_FILE_END){
    if(Update.end(true)){ //true to set the size to the current progress
      if (debug) Serial.println("Update Success - uploaded: " + String(upload.totalSize) + ".... rebooting now!");
    } else {
      if (debug) Update.printError(Serial);
    }
    if (debug) Serial.setDebugOutput(false);
  } else if(upload.status == UPLOAD_FILE_ABORTED){
    Update.end();
    if (debug) Serial.println("Update was aborted");
  }
  yield();
}

/*
void indexHtmlHandler() {
    httpServer.send(200, "text/html", String(indexHtml));
    httpServer.sendHeader("cache-control", "private, max-age=0, no-cache, no-store");
}*/

// handles GET request to "/update" and redirects to "index.html/#!pagefirmwareupdate"
void updateHandler() {
    httpServer.send(301, "text/html", "/#!pagefirmwareupdate");
    httpServer.sendHeader("cache-control", "private, max-age=0, no-cache, no-store");
}


void WiFiEvent(WiFiEvent_t event) {
  if(debug) {
    switch(event) {
        case WIFI_EVENT_STAMODE_GOT_IP:
            Serial.println("WiFi connected. IP address: " + String(WiFi.localIP().toString()) + " hostname: "+ WiFi.hostname() + "  SSID: " + WiFi.SSID());
            break;
        case WIFI_EVENT_STAMODE_DISCONNECTED:
            Serial.println("WiFi client lost connection");
            break;
        case WIFI_EVENT_STAMODE_CONNECTED:
            Serial.println("WiFi client connected");
            break;
        case WIFI_EVENT_STAMODE_AUTHMODE_CHANGE:
            Serial.println("WiFi client authentication mode changed.");
            break;
        //case WIFI_STAMODE_DHCP_TIMEOUT:                             THIS IS A NEW CONSTANT ENABLE WITH UPDATED SDK
          //  Serial.println("WiFi client DHCP timeout reached.");
            //break;
        case WIFI_EVENT_SOFTAPMODE_STACONNECTED:
            Serial.println("WiFi accesspoint: new client connected");
            break;
        case WIFI_EVENT_SOFTAPMODE_STADISCONNECTED:
            Serial.println("WiFi accesspoint: client disconnected.");
            break;
        case WIFI_EVENT_SOFTAPMODE_PROBEREQRECVED:
            //Serial.println("WiFi accesspoint: probe request received.");
            break; 
      }
  }
}

void printSpiffsContents() {
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
}

void setupSerial() {
  // checking availability of serial connection
  int serialtimeout = 5000; //ms
  Serial.begin ( 115200 );
  while(!Serial) {
    if (serialtimeout >0) {
      serialtimeout -= 50; 
    } else { 
      debug = false;
      break;
    }
    delay(50);
  }

  if (debug) {
    Serial.println("");
    Serial.println("");
    Serial.println("");
    Serial.println("Welcome to Dynatrace ufo!");
    Serial.println("Resetinfo: " + ESP.getResetInfo());
  }
}

// initialization routines
void setup ( void ) {
  setupSerial();

  pinMode(PIN_FACTORYRESET, INPUT); //, INPUT_PULLUP); use INPUT_PULLUP in case we put reset to ground; currently reset is doing a 3V signal

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

  // initialize ESP8266 file system
  SPIFFS.begin();
  printSpiffsContents();

  // initialize Wifi based on stored config
  WiFi.onEvent(WiFiEvent);
  WiFi.hostname(DEFAULT_HOSTNAME); // note, the hostname is not persisted in the ESP config like the SSID. so it needs to be set every time WiFi is started
  WiFi.begin(); // load SSID and PWD from internal memory
  if (debug) {
    Serial.println("Connecting to Wifi SSID: " + WiFi.SSID() + " as host "+ WiFi.hostname());
    if (WiFi.getMode() > 1) {
      Serial.println("Access Point IP address: " + WiFi.softAPIP().toString());
    }
    //WiFi.printDiag(Serial);
  }

  // setup all web server routes; make sure to use / last
  #define STATICFILES_CACHECONTROL "private, max-age=0, no-cache, no-store"
  httpServer.on ( "/api", apiHandler );
  httpServer.on ( "/info", infoHandler );
  httpServer.on ( "/gen", generateHandler );
  httpServer.serveStatic("/index.html", SPIFFS, "/index.html", STATICFILES_CACHECONTROL);
  //httpServer.serveStatic("/test.html", SPIFFS, "/test.html", STATICFILES_CACHECONTROL);
  //httpServer.serveStatic("/app.js", SPIFFS, "/app.js", STATICFILES_CACHECONTROL);
  //httpServer.serveStatic("/phonon.min.css", SPIFFS, "/phonon.min.css", STATICFILES_CACHECONTROL);
  //httpServer.serveStatic("/phonon.min.js", SPIFFS, "/phonon.min.js", STATICFILES_CACHECONTROL);
  //httpServer.serveStatic("/fonts/material-design-icons.eot", SPIFFS, "/fonts/mdi.eot", STATICFILES_CACHECONTROL);
  //httpServer.serveStatic("/fonts/material-design-icons.svg", SPIFFS, "/fonts/mdi.svg", STATICFILES_CACHECONTROL);
  //httpServer.serveStatic("/fonts/material-design-icons.ttf", SPIFFS, "/fonts/mdi.ttf", STATICFILES_CACHECONTROL);
  //httpServer.serveStatic("/fonts/material-design-icons.woff", SPIFFS, "/fonts/mdi.woff", STATICFILES_CACHECONTROL);
  httpServer.serveStatic("font.woff", SPIFFS, "font.woff", STATICFILES_CACHECONTROL);
  httpServer.serveStatic("font.eot", SPIFFS, "font.eot", STATICFILES_CACHECONTROL);
  httpServer.serveStatic("font.svg", SPIFFS, "font.svg", STATICFILES_CACHECONTROL);
  httpServer.serveStatic("font.ttf", SPIFFS, "font.ttf", STATICFILES_CACHECONTROL);
  httpServer.serveStatic("/", SPIFFS, "/index.html", STATICFILES_CACHECONTROL);
  //httpServer.on ( "/", HTTP_GET, handleRoot );
  /*httpServer.on ( "/test.svg", drawGraph );
  httpServer.on ( "/inline", []() {
    httpServer.send ( 200, "text/plain", "this works as well" );
  } );*/
  httpServer.onNotFound ( handleNotFound );

  // register firmware update HTTP server: 
  //    To upload through terminal you can use: curl -F "image=@ufo.ino.bin" ufo.local/update  
  //    Windows power shell use DOESNT WORK YET: wget -Method POST -InFile ufo.ino.bin -Uri ufo.local/update   
  //httpUpdater.setup(&httpServer); // this adds the URI "./update" to the http server
  if (debug) {
    Serial.println("HTTPUpdateServer ready! Open http://" + String(WiFi.hostname()) + ".local/update in your browser");
  }

  //httpServer.on("/", indexHtmlHandler);
  //httpServer.on("/update", HTTP_GET, indexHtmlHandler);
  httpServer.on("/update", HTTP_GET, updateHandler);
  httpServer.on("/update", HTTP_POST, updatePostHandler, updatePostUploadHandler);

  // start webserver
  httpServer.begin();
}


unsigned int tick = 0;
byte whirlpos = 0;
byte wheelcolor = 0;

void loop ( void ) {
  tick++;

  handleFactoryReset();
  handleNewWifiSettings();
  httpServer.handleClient();
  yield();

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
  yield();
  neopixel_logo.show();
  yield();

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
      neopixel_lowerring.setPixelColor((p % 15), r, g, b);
    }
    if (tick % 50 == 0) {
      whirlpos++;
    }
  } else {
    for (byte i = 0; i<15; i++) {
      neopixel_upperring.setPixelColor(i, 0, 0, 0);
      neopixel_lowerring.setPixelColor(i, 0, 0, 0);
    }
  }

  yield();
  neopixel_upperring.show();
  yield();
  neopixel_lowerring.show();

}



