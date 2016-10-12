
void ledsSetup() {
  displayCharter_lowerring.Init();

  // setup dotstar ledstrips
  ledstrip_logo.begin();
  ledsSetLogoDefault();
  ledstrip_logo.show();

  ledstrip_lowerring.begin();
  ledstrip_lowerring.clear();
  ledstrip_lowerring.show();
  ledstrip_upperring.begin();
  ledstrip_upperring.clear();
  ledstrip_upperring.show();
}

void dotstarSetColor(Adafruit_DotStar &dotstar, byte r, byte g, byte b ) {
  for (unsigned short i = 0; i < dotstar.numPixels(); i++) {
    dotstar.setPixelColor(i, r, g, b);
  }
}

void ledsSetLogoDefault() {
  ledstrip_logo.setPixelColor(0, 0, 100, 255); // http://ufo/api?logoled=0&r=0&g=100&b=255
  ledstrip_logo.setPixelColor(1, 125, 255, 0); // http://ufo/api?logoled=1&r=125&g=255&b=0
  ledstrip_logo.setPixelColor(2, 0, 255, 0); // http://ufo/api?logoled=2&r=0&g=255&b=0
  ledstrip_logo.setPixelColor(3, 255, 0, 255); // http://ufo/api?logoled=3&r=255&g=0&b=255
  ledstrip_logo.setBrightness(255);
}

