#include <ESP8266WebServer.h>
#include <FS.h>

class IPDisplay;
class DisplayCharter;
class Adafruit_DotStar;
class Config;

class MyFunctionRequestHandler : public RequestHandler {
public:

    MyFunctionRequestHandler(DisplayCharter* pDisplayLowerRing, DisplayCharter* pDisplayUpperRing, IPDisplay* pIpDisplay, Adafruit_DotStar* pLedstripLogo, Config* pConfig, bool debug);
    
    bool canHandle(HTTPMethod requestMethod, String requestUri) override  { return true; }
    bool canUpload(String requestUri) override  { return true; }

    bool handle(ESP8266WebServer& server, HTTPMethod requestMethod, String requestUri) override;

    void upload(ESP8266WebServer& server, String requestUri, HTTPUpload& upload) override;

private:
    bool handleStatic(ESP8266WebServer& server, String& actualUrl, const __FlashStringHelper* url, const __FlashStringHelper* staticFile, const __FlashStringHelper* contentType, bool mayCache);
    void httpReboot(ESP8266WebServer& server, String message);
    String parseFileName(String& path);
    void infoHandler(ESP8266WebServer& server);
    void dtIntegrationHandler(ESP8266WebServer& server);
    void apiHandler(ESP8266WebServer& server);
    void dynatracePostHandler(ESP8266WebServer& server);


private:
    DisplayCharter* mpDisplayLowerRing;
    DisplayCharter* mpDisplayUpperRing;
    IPDisplay* mpIpDisplay;
    Adafruit_DotStar* mpLedstripLogo;
    Config* mpConfig;
    
    bool mDebug;
    int  miUploadSize;
    File mUploadFile;
    bool mbRebootRequired;

};
