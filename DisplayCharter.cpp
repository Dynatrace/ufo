
#include <ArduinoJson.h>
#include <Adafruit_DotStar.h>
#include "DisplayCharter.h"



DisplayCharter::DisplayCharter(){
  Init();
}
void DisplayCharter::Init(){
  for (byte i=0 ; i<RING_LEDCOUNT ; i++)
    ledColor[i] = -1;
  backgroundColor = 0x000000;
  offset = 0;
  whirlSpeed = 0;
  morphPeriod = 0;
  morphingState = 0;
}
void DisplayCharter::SetLeds(byte pos, byte count, unsigned int color){
 for (byte i=0 ; i<count ; i++){
   ledColor[(pos + i) % RING_LEDCOUNT] = color;
 }
}
void DisplayCharter::SetBackground(String color){
  if (color.length() == 6){
    backgroundColor = (int)strtol(color.c_str(), NULL, 16);
  }
}
void DisplayCharter::SetWhirl(byte wspeed, bool clockwise){
  whirlSpeed = wspeed;
  whirlTick = 0xFF - whirlSpeed;
  whirlClockwise = clockwise;
}
void DisplayCharter::SetMorph(int period, byte mspeed){
  morphPeriod = period;
  morphPeriodTick = morphPeriod;

  if (mspeed > 10)
    morphSpeed = 10;
  else
   morphSpeed = mspeed;
}

unsigned int DisplayCharter::ParseLedArg(String argument, unsigned int iPos){
  byte seg = 0;
  String pos;
  String count;
  String color;
  unsigned int i=iPos;
  while ((i < argument.length()) && (seg < 3)){
    char c = argument.charAt(i);
    if (c == '|')
      seg++;
    else switch(seg){
      case 0:
        pos.concat(c);
        break;
      case 1:
        count.concat(c);
        break;
      case 2:
        color.concat(c);
    }
    i++;
  }
  pos.trim();
  count.trim();
  color.trim();

  if ((pos.length() > 0) && (count.length() > 0) && (color.length() == 6)){
    String s = color.substring(2, 4) + color.substring(0, 2) + color.substring(4, 6);
    SetLeds((byte)pos.toInt(), (byte)count.toInt(), (unsigned int)strtol(s.c_str(), NULL, 16));
  }
  return i;
}
void DisplayCharter::ParseWhirlArg(String argument){
  byte seg = 0;
  String wspeed;

  for (unsigned int i=0 ; i< argument.length() ; i++){
    char c = argument.charAt(i);
    if (c == '|'){
      if (++seg == 2)
        break;
    }
    else switch(seg){
      case 0:
        wspeed.concat(c);
        break;
    }
  }
  wspeed.trim();

  if (wspeed.length() > 0){
    SetWhirl((byte)wspeed.toInt(), !seg);
  }
}
void DisplayCharter::ParseMorphArg(String argument){
  byte seg = 0;
  String period;
  String mspeed;

  for (unsigned int i=0 ; i< argument.length() ; i++){
    char c = argument.charAt(i);
    if (c == '|'){
      if (++seg == 2)
        break;
    }
    else switch(seg){
      case 0:
        period.concat(c);
        break;
      case 1:
        mspeed.concat(c);
        break;
    }
  }
  period.trim();
  mspeed.trim();

  if ((period.length() > 0) && (mspeed.length() > 0)){
    SetMorph(period.toInt(), (byte)mspeed.toInt());
  }
}
int DisplayCharter::GetPixelColor(int i){
  int color = ledColor[i];

  if (color == -1)
    return backgroundColor;

  if (morphingState){
    byte g = color >> 16;
    byte r = (color >> 8) & 0xFF;
    byte b = color & 0xFF;
    byte gb = backgroundColor >> 16;
    byte rb = (backgroundColor >> 8) & 0xFF;
    byte bb = backgroundColor & 0xFF;
    byte gn = (g * (100 - morphingPercentage) / 100 + gb * morphingPercentage / 100);
    byte rn = (r * (100 - morphingPercentage) / 100 + rb * morphingPercentage / 100);
    byte bn = (b * (100 - morphingPercentage) / 100 + bb * morphingPercentage / 100);

    return (gn << 16) + (rn << 8) + bn;
  }

  return color;
}
void DisplayCharter::Display(Adafruit_DotStar &dotstar){
  for (byte i=0 ; i<RING_LEDCOUNT ; i++)
    dotstar.setPixelColor((i + offset) % RING_LEDCOUNT, GetPixelColor(i));
   
  if (whirlSpeed){
    if (!--whirlTick){
      if (whirlClockwise){
        if (++offset >= RING_LEDCOUNT)
          offset = 0;
      }
      else{
        if (!offset)
          offset = RING_LEDCOUNT - 1;
        else
          offset--;
      }
      whirlTick = 0xFF - whirlSpeed;
    }
  }

  switch (morphingState){
    case 0:
      if (morphPeriod){
        if (!--morphPeriodTick){
          morphingState = 1;
          morphingPercentage = 0;
          morphSpeedTick = 11 - morphSpeed;
    
          morphPeriodTick = morphPeriod;
        }
      }
      break;
    case 1:
      if (!--morphSpeedTick){
        if (morphingPercentage < 100)
          morphingPercentage+=1;
        else
          morphingState = 2;
  
        morphSpeedTick = 11 - morphSpeed;
      }
      break;
    case 2:
      if (!--morphSpeedTick){
        if (morphingPercentage > 0)
          morphingPercentage-=1;
        else
          morphingState = 0;
  
        morphSpeedTick = 11 - morphSpeed;
      }
      break;   
  }
}



//------------------------------------------------------------------------------------------------------



IPDisplay::IPDisplay(){
  ipspeed = 1100;
  displayCharter = 0;
}


void IPDisplay::ShowIp(String ip, DisplayCharter* displayCh){
  displayCharter = displayCh;
  ipAddress = ip;
  pos = 0;
  color = 0;
  colorValue = 0xFF0000;
  tick = ipspeed;
  shortBreak = false;
}
void IPDisplay::ProcessTick()
{
  if (!displayCharter)
    return;
    
  if (!--tick){
    if (shortBreak){
      displayCharter->Init();
      displayCharter->SetLeds(0, 15, 0x303030);
      tick = 100;
      shortBreak = false;    
      return;
    }
    shortBreak = true;
    tick = ipspeed;

    if (pos >= ipAddress.length()){
      pos = 0;
      if (++color >= 3)
        color = 0;
      switch (color){
        case 0: 
          colorValue = 0xFF0000;
          break;
        case 1: 
          colorValue = 0x00FF00;
          break;
        case 2: 
          colorValue = 0x0000FF;
          break;
          
      }
    }
    displayCharter->Init();
    char c = ipAddress.charAt(pos);
    pos++;
    
    switch (c){
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
        displayCharter->SetLeds(0, c - '0', colorValue);
        break; 
      case '6':
      case '7':
      case '8':
      case '9':
        displayCharter->SetLeds(0, 5, colorValue);
        displayCharter->SetLeds(8, c - '5', colorValue);
        break; 
      case '.':
        displayCharter->SetLeds(0, 1, 0xb0b0b0);
        displayCharter->SetLeds(5, 1, 0xb0b0b0);
        displayCharter->SetLeds(10, 1, 0xb0b0b0);
        break; 
    }
  }
}
void IPDisplay::StopShowingIp(){ 
  if (displayCharter)
    displayCharter->Init();
  displayCharter = 0;
}
