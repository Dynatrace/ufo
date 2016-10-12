void apiHandler() {
  ipDisplay.StopShowingIp();

  /*if (debug) { // print arguments
    for (int i = 0; i < httpServer.args(); i++) {
      Serial.println(F("API call argument: ") + httpServer.argName(i) + F(" value: ") + httpServer.arg(i));
    }
    if (httpServer.args() == 0) {
      Serial.println(F("API call with no arguments"));
    } else {
      Serial.println(F("   ---end of arguments---"));
    }
  }*/
  // adjust logo brightness (on/off right now)
  if (httpServer.hasArg(F("logo"))) {
    if (httpServer.arg(F("logo")).equals(F("on"))) {
      ledstrip_logo.setBrightness(255);
    } else if (httpServer.arg(F("logo")).equals(F("default"))) {
       ledsSetLogoDefault();
    } else {
      ledstrip_logo.setBrightness(0);
    }
  }

  // to quickly set the RGB colors of the logo remotely
  if (httpServer.hasArg(F("logoled"))) {
    byte led = byte(httpServer.arg(F("logoled")).toInt());
    byte r = byte(httpServer.arg("r").toInt());
    byte g = byte(httpServer.arg("g").toInt());
    byte b = byte(httpServer.arg("b").toInt());
    ledstrip_logo.setPixelColor(led, r, g, b);
  }

  if (httpServer.hasArg(F("top_init"))){
    displayCharter_upperring.Init();
  }
  if (httpServer.hasArg(F("top"))){
    String argument = httpServer.arg(F("top"));
    unsigned int i = 0;
    unsigned int ret = 0;
    
    while (i < argument.length())
      i = displayCharter_upperring.ParseLedArg(argument, i);
  }
  if (httpServer.hasArg(F("top_bg"))){
    String argument = httpServer.arg(F("top_bg"));
    if (argument.length() == 6)
      displayCharter_upperring.SetBackground(argument.substring(2, 4) + argument.substring(0, 2) + argument.substring(4, 6)); 
  }
  if (httpServer.hasArg(F("top_whirl"))){
    String argument = httpServer.arg(F("top_whirl"));
    displayCharter_upperring.ParseWhirlArg(argument);
  } 
  if (httpServer.hasArg(F("top_morph"))){
    String argument = httpServer.arg(F("top_morph"));
    displayCharter_upperring.ParseMorphArg(argument);
  } 
    
  if (httpServer.hasArg(F("bottom_init"))){
    displayCharter_lowerring.Init();
  }
  if (httpServer.hasArg(F("bottom"))){
    String argument = httpServer.arg(F("bottom"));
    unsigned int i = 0;
    unsigned int ret = 0;
    
    while (i < argument.length())
      i = displayCharter_lowerring.ParseLedArg(argument, i);
  }
  if (httpServer.hasArg(F("bottom_bg"))){
    String argument = httpServer.arg(F("bottom_bg"));
    if (argument.length() == 6)
      displayCharter_lowerring.SetBackground(argument.substring(2, 4) + argument.substring(0, 2) + argument.substring(4, 6)); 
  }
  if (httpServer.hasArg(F("bottom_whirl"))){
    String argument = httpServer.arg(F("bottom_whirl"));
    displayCharter_lowerring.ParseWhirlArg(argument);
  } 
  if (httpServer.hasArg(F("bottom_morph"))){
    String argument = httpServer.arg(F("bottom_morph"));
    displayCharter_lowerring.ParseMorphArg(argument);
  } 

  if (httpServer.hasArg(F("dynatrace-environmentid")) || httpServer.hasArg(F("dynatrace-apikey"))) {
    //if (debug) Serial.println(F("Storing Dynatrace SaaS/Managed environment integration settings"));
    dynatraceEnvironmentID = httpServer.arg(F("dynatrace-environmentid"));
    dynatraceApiKey = httpServer.arg(F("dynatrace-apikey"));
    //if (debug) Serial.println("Stored: " + httpServer.arg("dynatrace-environmentid") + " / " + httpServer.arg("dynatrace-apikey"));
    boolean saved = configWrite();
    //if (debug) Serial.println("Config saved. " + String(saved) + "  rebooting.....");
    //if (debug) Serial.flush();
    httpReboot(F("Configuration succeeded!"));
  }

  // note its required to provide both arguments SSID and PWD
  if (httpServer.hasArg(F("ssid")) && httpServer.hasArg(F("pwd"))) {
    String newWifiSSID = httpServer.arg(F("ssid"));
    String newWifiPwd = httpServer.arg(F("pwd"));

    // if SSID is given, also update wifi credentials
    if (newWifiSSID.length()) {
      WiFi.mode(WIFI_STA);
      WiFi.begin(newWifiSSID.c_str(), newWifiPwd.c_str() );
    }

    if (debug) {
      Serial.println(String(F("New Wifi settings: ")) + newWifiSSID + " / " + newWifiPwd);
      Serial.println(String(F("Restarting....")));
      Serial.flush();
    }

    httpReboot(String(F("New WIFI settings accepted. Mac address: ")) + WiFi.macAddress() + String(F("<p/>")));

  }
  if (httpServer.hasArg(F("hostname"))) {
    String newWifiHostname = httpServer.arg(F("hostname"));
    //TODO##################################################################
  }

  httpServer.sendHeader(F("cache-control"), F("private, max-age=0, no-cache, no-store"));
  httpServer.send(200);
}


