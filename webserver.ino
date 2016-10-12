
void updatePostHandler() {
  // handler for the /update form POST (once file upload finishes)
  //httpServer.sendHeader("Connection", "close");
  //httpServer.sendHeader("Access-Control-Allow-Origin", "*");
  StreamString error;
  if (Update.hasError()) {
    Update.printError(error);
  }
  httpReboot((Update.hasError()) ? error : String(F("Upload succeeded! ")) + String(uploadSize) + String(F(" bytes transferred<p>")));
}

String parseFileName(String &path) {
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

File uploadFile;
void updatePostUploadHandler() {
  // handler for the file upload, get's the sketch bytes, and writes
  // them through the Update object
  HTTPUpload& upload = httpServer.upload();
  String filename = parseFileName(upload.filename);

  if (filename.endsWith(".bin")) { // handle firmware upload
    if (upload.status == UPLOAD_FILE_START) {
      //WiFiUDP::stopAll(); needed for MDNS or the like?
      if (debug) Serial.println("Update: " + upload.filename);
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) { //start with max available size
        if (debug) Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (debug) Serial.print(".");
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        if (debug) Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      uploadSize = Update.size();
      if (Update.end(true)) { //true to set the size to the current progress
        if (debug) Serial.println(String(F("Update Success - uploaded: ")) + String(upload.totalSize) + String(F(".... rebooting now!")));
      } else {
        if (debug) Update.printError(Serial);
      }
      if (debug) Serial.setDebugOutput(false);
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
      uploadSize = Update.size();
      Update.end();
      if (debug) Serial.println(F("Update was aborted"));
    }
  } else { // handle file upload
    if (upload.status == UPLOAD_FILE_START) {
      if (debug) Serial.println(String(F("uploading to SPIFFS: /")) + filename);
      uploadFile = SPIFFS.open("/" + filename, "w");
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (debug) Serial.print(".");
      if (uploadFile.write(upload.buf, upload.currentSize) != upload.currentSize) {
        if (debug) Serial.println(String(F("ERROR writing file ")) + String(uploadFile.name()) + String(F("to SPIFFS.")));
        uploadFile.close();
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      uploadSize = upload.totalSize;
      if (uploadFile.size() == upload.totalSize) {
        if (debug) Serial.println(String(F("Upload to SPIFFS Succeeded - uploaded: ")) + String(upload.totalSize) + String(F(".... rebooting now!")));
      } else {
        if (debug) Serial.println(String(F("Upload to SPIFFS FAILED: ")) + String(uploadFile.size()) + String(F(" bytes of ")) + String(upload.totalSize));
      }
      uploadFile.close();
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
      uploadSize = upload.totalSize;
      uploadFile.close();
      if (debug) Serial.println(F("Upload to SPIFFS was aborted"));
    }
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
  httpServer.sendHeader(F("cache-control"), F("private, max-age=0, no-cache, no-store"));
  httpServer.send(301, F("text/html"), F("/#!pagefirmwareupdate"));
}



// send a HTML response about reboot progress
// reboot device therafter.
void httpReboot(String message) {
  String response = F("<!DOCTYPE html>"
                    "<html>"
                    "<head>"
                    "<title>Dynatrace UFO configuration changed. Rebooting now... </title>"
                    "<META http-equiv='refresh' content='10;URL=/'>"
                    "</head>"
                    "<body>"
                    "<center>");
  response +=             message;
  response +=            F("Device is being rebooted. Redirecting to homepage in 10 seconds...</center>"
                         "</body>"
                         "</html>");

  httpServer.sendHeader(F("cache-control"), F("private, max-age=0, no-cache, no-store"));
  httpServer.send(200, F("text/html"), response);
  ESP.restart();
}

// HTTP not found response
void handleNotFound() {
  String message = F("File Not Found\n\n");
  message += F("URI: ");
  message += httpServer.uri();
  message += F("\nMethod: ");
  message += ( httpServer.method() == HTTP_GET ) ? F("GET") : F("POST");
  message += F("\nArguments: ");
  message += httpServer.args();
  message += F("\n");

  for ( uint8_t i = 0; i < httpServer.args(); i++ ) {
    message += " " + httpServer.argName ( i ) + ": " + httpServer.arg ( i ) + "\n";
  }
  httpServer.sendHeader(F("cache-control"), F("private, max-age=0, no-cache, no-store"));
  httpServer.send ( 404, F("text/plain"), message );
}

void infoHandler() {

  String json = "{";
  json += "\"heap\":\"" + String(ESP.getFreeHeap()) + "\"";
  json += ", \"ssid\":\"" + String(WiFi.SSID()) + "\"";
  json += ", \"ipaddress\":\"" + WiFi.localIP().toString() + "\"";
  json += ", \"ipgateway\":\"" + WiFi.gatewayIP().toString() + "\"";
  json += ", \"ipdns\":\"" + WiFi.dnsIP().toString() + "\"";
  json += ", \"ipsubnetmask\":\"" + WiFi.subnetMask().toString() + "\"";
  json += ", \"macaddress\":\"" + WiFi.macAddress() + "\"";
  json += ", \"hostname\":\"" + WiFi.hostname() + "\"";
  json += ", \"apipaddress\":\"" + WiFi.softAPIP().toString() + "\"";
  json += ", \"apconnectedstations\":\"" + String(WiFi.softAPgetStationNum()) + "\"";
  json += ", \"wifiautoconnect\":\"" + String(WiFi.getAutoConnect()) + "\"";
  json += ", \"firmwareversion\":\"" + String(FIRMWARE_VERSION) + "\"";
  json += "}";
  httpServer.sendHeader(F("cache-control"), F("private, max-age=0, no-cache, no-store"));
  httpServer.send(200, F("text/json"), json);
  json = String();
}

// Dynatrace SaaS/Managed integration:
// setup a custom Problem Notification Web Hook and provide following JSON elements
// {"ProblemState":"{State}","ProblemImpact":"{ProblemImpact}"}
// Example post data sent using Windows PowerShell
// Invoke-RestMethod -Method Post -Uri "http://ufo/dynatrace" -Body ('{"ProblemState":"{State}","ProblemImpact":"{ProblemImpact}"}') -ContentType ("application/json")
void dynatracePostHandler() {
  if (httpServer.hasArg(F("plain"))) { // POST data is stored in the "plain" argument
    StaticJsonBuffer<512> jsonBuffer;
    if (debug) Serial.println(String(F("Dynatrace JSON POST data: ")) + httpServer.arg("plain"));
    JsonObject& jsonObject = jsonBuffer.parseObject(httpServer.arg("plain"));

    // TODO HANDLE Dynatrace PROBLEM DATA ################################################################################################

  } else {
    if (debug) Serial.println(String(F("Dynatrace JSON POST data MISSING!")));
  }

  httpServer.sendHeader(F("cache-control"), F("private, max-age=0, no-cache, no-store"));
  httpServer.send(200);
}

