
#include <ArduinoJson.h>
#include <Adafruit_DotStar.h>

#define RING_LEDCOUNT 15

class DisplayCharter
{
  public:
    DisplayCharter();
    void Init();
    void SetLeds(byte pos, byte count, unsigned int color);
    void SetBackground(String color);
    void SetWhirl(byte wspeed, bool clockwise);
    void SetMorph(int period, byte mspeed);
    unsigned int ParseLedArg(String argument, unsigned int iPos);
    void ParseWhirlArg(String argument);
    void ParseMorphArg(String argument);
    void Display(Adafruit_DotStar &dotstar);

  private:
    int GetPixelColor(int i);
    
  private:
    unsigned int ledColor[RING_LEDCOUNT];
    unsigned int backgroundColor;
    byte whirlSpeed;
    bool whirlClockwise;
    byte offset;
    byte whirlTick;
    byte morphingState;
    int  morphPeriod;
    int morphPeriodTick;
    byte morphSpeed;
    byte morphSpeedTick;
    byte morphingPercentage;
};



//------------------------------------------------------------------------------------------------------

class IPDisplay
{
  public:
    IPDisplay();
    void ShowIp(String ip, DisplayCharter* displayCh);
    void ProcessTick();
    void StopShowingIp();

    
  private:
    bool showIp;
    String ipAddress;
    unsigned int pos;
    unsigned int tick;
    bool shortBreak;
    unsigned int color;
    unsigned int colorValue;
    unsigned int ipspeed;
    DisplayCharter* displayCharter;
};


