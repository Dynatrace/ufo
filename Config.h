#include <ESP8266WiFi.h>

class Config{

public:
  Config(bool debug);

  bool Read();
  bool Write();
  bool Delete();

  bool Enabled();

public:
  bool   enabled;
  String dynatraceEnvironmentID;
  String dynatraceApiKey; 
  int    pollingIntervalS;

private:
  bool mDebug;
  
};

