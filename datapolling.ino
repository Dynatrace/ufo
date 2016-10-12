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
  httpClient.begin("https://" + dynatraceEnvironmentID + ".live.dynatrace.com/api/v1/problem/status?Api-Token=" + dynatraceApiKey, F("CD:57:0F:00:D7:50:2D:8A:F1:2C:5D:6C:AB:5E:E5:B1:3C:08:99:AC"));

  if (debug) Serial.println(String(F("http.begin executed. free heap: "))  + String(ESP.getFreeHeap()));
  if (debug) Serial.flush();


  int httpCode = httpClient.GET();
  if (httpCode == HTTP_CODE_OK) {
    if (httpClient.getSize() > 128) {
      if (debug) Serial.println(String(F("Dynatrace Problem API call response too large to handle! ")) + String(httpClient.getSize()) + " bytes");
      httpClient.end();
      return;
    }

    if (debug) Serial.println(String(F("GET resulted in OK. free heap: "))  + String(ESP.getFreeHeap()));
    if (debug) Serial.flush();

    String json = httpClient.getString(); // this allocates memory, so lets not get this string if the HTTP response is too large
    if (debug) Serial.println(String(F("Dynatrace Problem API call: ")) + json);
    if (debug) Serial.flush();
    StaticJsonBuffer < 128 + 2 > jsonBuffer; // attention, too large buffers cause stack overflow and thus crashes!
    JsonObject& jsonObject = jsonBuffer.parseObject(json);
    long applicationProblems = jsonObject["result"]["totalOpenProblemsCount"];
    //long applicationProblems = jsonObject["result"]["openProblemCounts"]["APPLICATION"];
    if (debug) Serial.println(String(F("Dynatrace Problem API call - Application problems: ")) + String(applicationProblems));
    if (applicationProblems) {
      redcountUpperring = 14;
      redcountLowerring = 14;
    } else {
      redcountUpperring = 1;
      redcountLowerring = 1;
    }
  } else {
    if (debug) Serial.println(String(F("Dynatrace Problem API call FAILED (error code ")) + httpClient.errorToString(httpCode) + ")" );
    redcountUpperring = 7;
    redcountLowerring = 8;
  }
  httpClient.end();


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
