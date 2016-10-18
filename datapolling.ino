
void urlSetup(){
  httpClient.begin(String(F("https://")) + dynatraceEnvironmentID + F(".live.dynatrace.com/api/v1/problem/status?Api-Token=") + dynatraceApiKey, F("CD:57:0F:00:D7:50:2D:8A:F1:2C:5D:6C:AB:5E:E5:B1:3C:08:99:AC"));
  httpClient.setReuse(true);
}

void pollDynatraceProblemAPI() {

  if (wifiStationOK) {
    if (debug) Serial.println(String(F("Poll Dynatrace SaaS/Manageged environment now. Free heap: ")) + String(ESP.getFreeHeap()));
    if (debug) Serial.flush();
  } else {
    //if (debug) Serial.println(F("Poll Dynatrace now - DEFERRED - WIFI NOT YET AVAILABLE"));
    return;
  }

  // configure server and url
  // NOTE: SSL takes 18kB extra RAM memory!!! leads out-of-memory crash!!!! thus the HTTPserver and all other RAM consuming items must be turned off
  //       the fingerprint is needed to validate the server certificate
 // httpClient.begin(String(F("https://")) + dynatraceEnvironmentID + F(".live.dynatrace.com/api/v1/problem/status?Api-Token=") + dynatraceApiKey, F("CD:57:0F:00:D7:50:2D:8A:F1:2C:5D:6C:AB:5E:E5:B1:3C:08:99:AC"));
    
  if (debug) Serial.println(String(F("http.begin executed. free heap: "))  + String(ESP.getFreeHeap()));
  if (debug) Serial.flush();


  int httpCode = httpClient.GET();
  if (httpCode == HTTP_CODE_OK) {
    if (httpClient.getSize() > 128) {
      if (debug) Serial.println(String(F("Dynatrace Problem API call response too large to handle! ")) + String(httpClient.getSize()) + String(F(" bytes")));
      httpClient.end();
      urlSetup();
      return;
    }

    if (debug) Serial.println(String(F("GET resulted in OK. free heap: "))  + String(ESP.getFreeHeap()));
    if (debug) Serial.flush();

    String json = httpClient.getString(); // this allocates memory, so lets not get this string if the HTTP response is too large
      
    if (debug) Serial.println(String(F("Dynatrace Problem API call: ")) + json);
    if (debug) Serial.flush();
    StaticJsonBuffer < 128 + 2 > jsonBuffer; // attention, too large buffers cause stack overflow and thus crashes!
    JsonObject& jsonObject = jsonBuffer.parseObject(json);

    bool changeDetected = false;
    int i = jsonObject[F("result")][F("openProblemCounts")][F("APPLICATION")];
    if (i != applicationProblems){
      changeDetected = true;
      applicationProblems = i;
    }
    i = jsonObject[F("result")][F("openProblemCounts")][F("SERVICE")];
    if (i != serviceProblems){
      changeDetected = true;
      serviceProblems = i;
    }
    i = jsonObject[F("result")][F("openProblemCounts")][F("INFRASTRUCTURE")];
    if (i != infrastructureProblems){
      changeDetected = true;
      infrastructureProblems = i;
    }
    if (debug){
      Serial.println(String(F("Dynatrace Problem API call - Application problems: ")) + String(applicationProblems));
      Serial.println(String(F("Dynatrace Problem API call - Service problems: ")) + String(serviceProblems));
      Serial.println(String(F("Dynatrace Problem API call - Infrastructure problems: ")) + String(infrastructureProblems));
    }

    if (changeDetected){
      int count = applicationProblems + serviceProblems + infrastructureProblems;
      
      displayCharter_upperring.Init();
      displayCharter_lowerring.Init();
      switch (count){
        case 0:
          displayCharter_upperring.SetLeds(0, 15, 0xff0000);
          displayCharter_lowerring.SetLeds(0, 15, 0xff0000);
          displayCharter_upperring.SetMorph(4000, 6);
          displayCharter_lowerring.SetMorph(4000, 6);
          break;
        case 1:
          displayCharter_upperring.SetLeds(0, 15, (applicationProblems > 0) ? 0x00ff00 : ((serviceProblems > 0) ? 0x00ffaa : 0xaaff00));
          displayCharter_lowerring.SetLeds(0, 15, (applicationProblems > 0) ? 0x00ff00 : ((serviceProblems > 0) ? 0x00ffaa : 0xaaff00));
          displayCharter_upperring.SetMorph(1000, 8);
          displayCharter_lowerring.SetMorph(1000, 8);
          break;
        case 2:
          displayCharter_upperring.SetLeds(0, 7, (applicationProblems > 0) ? 0x00ff00 : ((serviceProblems > 0) ? 0x00ffaa : 0xaaff00));
          displayCharter_lowerring.SetLeds(0, 7, (applicationProblems > 0) ? 0x00ff00 : ((serviceProblems > 0) ? 0x00ffaa : 0xaaff00));
          displayCharter_upperring.SetLeds(8, 6, (applicationProblems > 1) ? 0x00ff00 : ((applicationProblems + serviceProblems > 1) ? 0x00ffaa : 0xaaff00));
          displayCharter_lowerring.SetLeds(8, 6, (applicationProblems > 1) ? 0x00ff00 : ((applicationProblems + serviceProblems > 1) ? 0x00ffaa : 0xaaff00));
          displayCharter_upperring.SetWhirl(180, true);
          displayCharter_lowerring.SetWhirl(180, true);
          break;
        default:
          displayCharter_upperring.SetLeds(0, 4, (applicationProblems > 0) ? 0x00ff00 : ((serviceProblems > 0) ? 0x00ffaa : 0xaaff00));
          displayCharter_lowerring.SetLeds(0, 4, (applicationProblems > 0) ? 0x00ff00 : ((serviceProblems > 0) ? 0x00ffaa : 0xaaff00));
          displayCharter_upperring.SetLeds(5, 4, (applicationProblems > 1) ? 0x00ff00 : ((applicationProblems + serviceProblems > 1) ? 0x00ffaa : 0xaaff00));
          displayCharter_lowerring.SetLeds(5, 4, (applicationProblems > 1) ? 0x00ff00 : ((applicationProblems + serviceProblems > 1) ? 0x00ffaa : 0xaaff00));
          displayCharter_upperring.SetLeds(10, 4, (applicationProblems > 2) ? 0x00ff00 : ((applicationProblems + serviceProblems > 2) ? 0x00ffaa : 0xaaff00));
          displayCharter_lowerring.SetLeds(10, 4, (applicationProblems > 2) ? 0x00ff00 : ((applicationProblems + serviceProblems > 2) ? 0x00ffaa : 0xaaff00));
          displayCharter_upperring.SetWhirl(180, true);
          displayCharter_lowerring.SetWhirl(180, true);
          break;
      }
    }
   
  } else {
    if (debug) Serial.println(String(F("Dynatrace Problem API call FAILED (error code ")) + httpClient.errorToString(httpCode) + F(")") );
    displayCharter_upperring.Init();
    displayCharter_lowerring.Init();
    displayCharter_upperring.SetLeds(0, 3, 0x0000ff);
    displayCharter_lowerring.SetLeds(0, 3, 0x0000ff);
    displayCharter_upperring.SetWhirl(220, true);
    displayCharter_lowerring.SetWhirl(220, false);
    applicationProblems = -1;
    serviceProblems = -1;
    infrastructureProblems = -1;
  }
  //httpClient.end();



  /* {
    result: {
    totalOpenProblemsCount: 4,
    openProblemCounts: {
      APPLICATION: 1,
      INFRASTRUCTURE: 3,
      SERVICE: 0
    }
    }
    } */

}
