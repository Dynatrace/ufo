#include <ESP8266HTTPClient.h>

class DisplayCharter;
class IPDisplay;
class Config;


class DataPolling{

public:

  DataPolling(DisplayCharter* pDisplayLowerRing, DisplayCharter* pDisplayUpperRing, IPDisplay* pIpDisplay, Config* pConfig, bool debug);
  bool Init();
  void Shutdown();
  
  void Poll();

private:
  HTTPClient  httpClient;
  
  DisplayCharter* mpDisplayLowerRing;
  DisplayCharter* mpDisplayUpperRing;
  IPDisplay* mpIpDisplay;
  Config* mpConfig;
  bool mDebug;
  bool mInitialized;

  int miApplicationProblems;
  int miServiceProblems;
  int miInfrastructureProblems;
};



