#include "DataPolling.h"
#include "DisplayCharter.h"
#include "Config.h"

DataPolling::DataPolling(DisplayCharter* pDisplayLowerRing, DisplayCharter* pDisplayUpperRing, IPDisplay* pIpDisplay, Config* pConfig, bool debug){
  mpDisplayLowerRing = pDisplayLowerRing;
  mpDisplayUpperRing = pDisplayUpperRing;
  mpIpDisplay = pIpDisplay;
  mpConfig = pConfig;
  mDebug = debug;
  mInitialized = false;

  miApplicationProblems = -1;
  miServiceProblems = -1;
  miInfrastructureProblems = -1;
}

bool DataPolling::Init(){

  if (mpConfig->Changed()){
     Shutdown();
  }
  
  if (!mpConfig->Enabled()){
    Shutdown();
    return false;
  }
  if (mInitialized){
    return true;
  }
  
  if (httpClient.begin(String(F("https://")) + mpConfig->dynatraceEnvironmentID + F(".live.dynatrace.com/api/v1/problem/status?Api-Token=") + mpConfig->dynatraceApiKey, F("8e:51:d1:ea:eb:5a:97:ed:af:49:4e:54:22:b1:ab:ce:15:84:0e:f8"))){
   //if (httpClient.begin("http://192.168.183.31/api/v1/problem/status")){
    httpClient.setReuse(true);
    mInitialized = true;
    return true;
  }
  else{
    return false;
  }
}

void DataPolling::Shutdown(){
  if (mInitialized){
    httpClient.end();
    mInitialized = false;
  }
}


void DataPolling::Poll() {

  if (!Init()) return;
 
  // configure server and url
  // NOTE: SSL takes 18kB extra RAM memory!!! leads out-of-memory crash!!!! thus the HTTPserver and all other RAM consuming items must be turned off
  //       the fingerprint is needed to validate the server certificate
 // httpClient.begin(String(F("https://")) + dynatraceEnvironmentID + F(".live.dynatrace.com/api/v1/problem/status?Api-Token=") + dynatraceApiKey, F("CD:57:0F:00:D7:50:2D:8A:F1:2C:5D:6C:AB:5E:E5:B1:3C:08:99:AC"));
    
  if (mDebug) Serial.println(String(F("Before GET. free heap: "))  + String(ESP.getFreeHeap()));
  if (mDebug) Serial.flush();


  int httpCode = httpClient.GET();
  if (httpCode == HTTP_CODE_OK) {
    int iLen = httpClient.getSize();
    if (iLen > 128) {
      if (mDebug) Serial.println(String(F("Dynatrace Problem API call response too large to handle! ")) + String(httpClient.getSize()) + String(F(" bytes")));
      Shutdown();
      return;
    }
    mpIpDisplay->StopShowingIp();

    if (mDebug) Serial.println(String(F("GET resulted in OK. free heap: "))  + String(ESP.getFreeHeap()));
    if (mDebug) Serial.flush();

    String json = httpClient.getString(); // this allocates memory, so lets not get this string if the HTTP response is too large
    if (mDebug) Serial.println(String(F("Dynatrace Problem API call: ")) + json);
    if (mDebug) Serial.flush();
    StaticJsonBuffer < 128 + 2 > jsonBuffer; // attention, too large buffers cause stack overflow and thus crashes!
    JsonObject& jsonObject = jsonBuffer.parseObject((char*)json.c_str());
    
    bool changeDetected = false;
    int i = jsonObject[F("result")][F("openProblemCounts")][F("APPLICATION")];
    Serial.println(i);
    if (i != miApplicationProblems){
      changeDetected = true;
      miApplicationProblems = i;
    }
    i = jsonObject[F("result")][F("openProblemCounts")][F("SERVICE")];
    if (i != miServiceProblems){
      changeDetected = true;
      miServiceProblems = i;
    }
    i = jsonObject[F("result")][F("openProblemCounts")][F("INFRASTRUCTURE")];
    if (i != miInfrastructureProblems){
      changeDetected = true;
      miInfrastructureProblems = i;
    }
    if (mDebug){
      Serial.println(String(F("Dynatrace Problem API call - Application problems: ")) + String(miApplicationProblems));
      Serial.println(String(F("Dynatrace Problem API call - Service problems: ")) + String(miServiceProblems));
      Serial.println(String(F("Dynatrace Problem API call - Infrastructure problems: ")) + String(miInfrastructureProblems));
    }

    if (changeDetected){
      int count = miApplicationProblems + miServiceProblems + miInfrastructureProblems;
      
      mpDisplayUpperRing->Init();
      mpDisplayLowerRing->Init();
      switch (count){
        case 0:
          mpDisplayUpperRing->SetLeds(0, 15, 0xff0000);
          mpDisplayLowerRing->SetLeds(0, 15, 0xff0000);
          mpDisplayUpperRing->SetMorph(4000, 6);
          mpDisplayLowerRing->SetMorph(4000, 6);
          break;
        case 1:
          mpDisplayUpperRing->SetLeds(0, 15, (miApplicationProblems > 0) ? 0x00ff00 : ((miServiceProblems > 0) ? 0x00ffaa : 0xaaff00));
          mpDisplayLowerRing->SetLeds(0, 15, (miApplicationProblems > 0) ? 0x00ff00 : ((miServiceProblems > 0) ? 0x00ffaa : 0xaaff00));
          mpDisplayUpperRing->SetMorph(1000, 8);
          mpDisplayLowerRing->SetMorph(1000, 8);
          break;
        case 2:
          mpDisplayUpperRing->SetLeds(0, 7, (miApplicationProblems > 0) ? 0x00ff00 : ((miServiceProblems > 0) ? 0x00ffaa : 0xaaff00));
          mpDisplayLowerRing->SetLeds(0, 7, (miApplicationProblems > 0) ? 0x00ff00 : ((miServiceProblems > 0) ? 0x00ffaa : 0xaaff00));
          mpDisplayUpperRing->SetLeds(8, 6, (miApplicationProblems > 1) ? 0x00ff00 : ((miApplicationProblems + miServiceProblems > 1) ? 0x00ffaa : 0xaaff00));
          mpDisplayLowerRing->SetLeds(8, 6, (miApplicationProblems > 1) ? 0x00ff00 : ((miApplicationProblems + miServiceProblems > 1) ? 0x00ffaa : 0xaaff00));
          mpDisplayUpperRing->SetWhirl(180, true);
          mpDisplayLowerRing->SetWhirl(180, true);
          break;
        default:
          mpDisplayUpperRing->SetLeds(0, 4, (miApplicationProblems > 0) ? 0x00ff00 : ((miServiceProblems > 0) ? 0x00ffaa : 0xaaff00));
          mpDisplayLowerRing->SetLeds(0, 4, (miApplicationProblems > 0) ? 0x00ff00 : ((miServiceProblems > 0) ? 0x00ffaa : 0xaaff00));
          mpDisplayUpperRing->SetLeds(5, 4, (miApplicationProblems > 1) ? 0x00ff00 : ((miApplicationProblems + miServiceProblems > 1) ? 0x00ffaa : 0xaaff00));
          mpDisplayLowerRing->SetLeds(5, 4, (miApplicationProblems > 1) ? 0x00ff00 : ((miApplicationProblems + miServiceProblems > 1) ? 0x00ffaa : 0xaaff00));
          mpDisplayUpperRing->SetLeds(10, 4, (miApplicationProblems > 2) ? 0x00ff00 : ((miApplicationProblems + miServiceProblems > 2) ? 0x00ffaa : 0xaaff00));
          mpDisplayLowerRing->SetLeds(10, 4, (miApplicationProblems > 2) ? 0x00ff00 : ((miApplicationProblems + miServiceProblems > 2) ? 0x00ffaa : 0xaaff00));
          mpDisplayUpperRing->SetWhirl(180, true);
          mpDisplayLowerRing->SetWhirl(180, true);
          break;
      }
    }
   
  } else {
    if (mDebug) Serial.println(String(F("Dynatrace Problem API call FAILED (error code ")) + httpClient.errorToString(httpCode) + F(")") );
    mpDisplayUpperRing->Init();
    mpDisplayLowerRing->Init();
    mpDisplayUpperRing->SetLeds(0, 3, 0x0000ff);
    mpDisplayLowerRing->SetLeds(0, 3, 0x0000ff);
    mpDisplayUpperRing->SetWhirl(220, true);
    mpDisplayLowerRing->SetWhirl(220, false);
    miApplicationProblems = -1;
    miServiceProblems = -1;
    miInfrastructureProblems = -1;
  }
}
