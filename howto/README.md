#Instructions to build the Dynatrace UFO

##Print the 7 pieces in 5 batches

1. UFO case base + microcontroller holder, plus microcontroller clip
black filament
2. UFO case top + cable stabilizer + mini-switch-clip
black filament
3. separator ring
black filament
4. Downlight inlay + first ring
transparent filament
5. Second ring

##Prepare USB cable
Chop off one end of the USB cable and use the 5V line (red) and ground (black) wire for powering the LED strips as well as the Huzzah ESP8266 microcontroller. Make sure to isolate the other wires so to make sure we wont damage USB source or get in contact with the other electronics.
![Power through USB cable](power through usb cable.jpg)

##Prepare LED strips
Cut the LED strip in pieces of 2 strips with 15 LEDs each and 2 strips with 2 LEDs each. Please make sure you cut exactly in the middle of the soldering tabs, because you will need to solder cables later there. Check out the cutting mark.
![Cut the LED strip in pieces](cut a strip of 15 LEDs off an remove protectuve hull.jpg)
![15 LEDs per long strip](15 LEDs per strip.jpg)

##Solder short strips
Solder the short strips in a series accordingly to the data-flow direction. There are arrows on the strip that show you the direction. Data for controlling the bus of LEDs flows from the microcontroller outbound in direction of arrow.
![Short LED strips soldered in series](arrows show how to place strip in series.jpg)
!(2x2 LEDs in series on logo.jpg)

##Wiring
USB cables 5V power goes to "VBat" and the black ground cable goes to "GND"
![soldering power to huzzah](soldering huzzah.jpg)

The white DATA wires that control the LEDs are soldered as follows:
* pin 4 - lower LED ring
* pin 5 - upper LED ring
* pin 2 - downlight LEDs

The blue Wifi reset cables:
* pin 15 - mini-button-switch for WiFi reset, to turn the UFO into Access Point mode; triggers when 3V are on pin 15
* pin 3V (next to pin 15) - goes to the other contact of the mini-button-switch. 
!(wired.jpg)
 
Note that the LEDs need to be powered directly from the USB cable, and not from the 3V pin. So all the red 5V cables and separately all the black ground cables are soldered together.

##Putting all together
The Huzzah ESP8266 microcontroller is mounted on top of the downlight inlay and also keeps the 2 short LED strips in their position. For proper isolation, put the printed platform with the 4 pins upside under the Huzzah board and put then all that inside the clip that holds everything in place.
![clip-in microcontroller](mounted microcontroller.jpg)

##Firmware
FTDI cable
Upload through serial (needed the first time; later firmware can be uploaded through the Web UI).

###Upload firmware available on github
1. TODO tooling
2. TODO  asdf

###Build the firmware yourself
1. Install Arduino IDE 1.6.7 and in preferences dialog this board manager URL: `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
2. [compile and upload firmware through serial](https://learn.adafruit.com/adafruit-huzzah-esp8266-breakout/using-arduino-ide)
3. TODO Uploading file system should not be required to make it simpler;
[upload file system and web site](https://github.com/esp8266/Arduino/blob/master/doc/filesystem.md#uploading-files-to-file-system) (SPIFFS)

 




