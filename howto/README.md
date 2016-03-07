#Instructions to build the Dynatrace UFO

see also [partlist](../partlist)

##Print the 10 pieces in 5 batches

####We printed with following settings:
* NO supports
* 3 shells
* 0.2mm layer height (0.1mm layer worked well too)
* 20% infill

####1. UFO case base + microcontroller holder, plus microcontroller clip

black filament
![](print base.jpg)

####2. UFO case top + cable stabilizer + mini-switch-clip

black filament
![](print top.jpg)

####3. separator ring

black filament
![](print separatorring.jpg)

####4. Downlight inlay + first ring

transparent filament
![](print ledring plus downlightinlay.jpg)

####5. Second ring

transparent filament
![](print ledring.jpg)


##Prepare USB cable
Chop off one end of the USB cable and use the 5V line (red) and ground (black) wire for powering the LED strips as well as the Huzzah ESP8266 microcontroller. Make sure to isolate the other wires so to make sure we wont damage USB source or get in contact with the other electronics.
![Power through USB cable](power through usb cable.jpg)

##Prepare LED strips
Cut the LED strip in pieces of 2 strips with 15 LEDs each and 2 strips with 2 LEDs each. Please make sure you cut exactly in the middle of the soldering tabs, because you will need to solder cables later there. Check out the cutting mark.
![Cut the LED strip in pieces](cut a strip of 15 LEDs off and remove protective hull.jpg)
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

![](wired.jpg)
 
Note that the LEDs need to be powered directly from the USB cable, and not from the 3V pin. So all the red 5V cables and separately all the black ground cables are soldered together.

##Putting all together
The Huzzah ESP8266 microcontroller is mounted on top of the downlight inlay and also keeps the 2 short LED strips in their position. For proper isolation, put the printed platform with the 4 pins upside under the Huzzah board and put then all that inside the clip that holds everything in place.
![clip-in microcontroller](mounted microcontroller.jpg)

##Firmware
The initial firmware needs to be programmed using the Serial-USB (FTDI) cable. 
You might need to install a driver on your PC for that cable.
Once you have that completed, you can use the UFO web UI to upload the firmware. 

###Upload firmware available on github
1. connect the ESP8266 to the FTDI cable and via USB to your computer
1. place the ESP8266 in firmware programming mode by pressing the buttons in following sequence:
 * press reset and stay on it
 * press GPIO0 and you should see the red led go on
 * release reset 
 * release GPIO0 and make sure the red led is still on, as it's signaling that the ESP8266 expects a firmware download

![esp8266 upload](esp8266 upload.jpg)
1. run the `ESP_DOWNLOAD_TOOL_V2.4.exe` from the `firmware/flashtool` folder, configure the proper COM port thats assigned to the FTDI cable and program the ESP8266
![flashtool](flashtool.png)

###Build the firmware yourself
1. Install Arduino IDE 1.6.7 and in preferences dialog this board manager URL: `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
![Arduino IDE preferences](arduino preferences.png) Make sure to restart the IDE afterwards.
1. Install ESP8266 board using Arduino IDE Board Manager under the tools menu
![Arduino board manager](install esp8266 board.png)
1. Configure microcontroller in IDE 
 * Adafruit Huzzah ESP8266 board
 * 3MB SPIFFS
 * serial port that's connected to the FTDI serial cable
 * USBtinyISP as Programmer
 * 115200bps 
![board config](board config.png)
1. Add *Adafruit NeoPixel* library, needed to control the LED strips
![neopixel library](install neopixel library.png)
1. use *Tools->Serial Monitor* to see the output of the ESP8266
![serial monitor](serial monitor.png)
1. place the ESP8266 in firmware programming mode by pressing the buttons in following sequence:
 * press reset and stay on it
 * press GPIO0 and you should see the red led go on
 * release reset 
 * release GPIO0 and make sure the red led is still on, as it's signaling that the ESP8266 expects a firmware download
1. Menu *Sketch->Upload CTRL-U* will compile and upload the sketch (firmware) via the serial interface. 
Read more here: [compile and upload firmware through serial](https://learn.adafruit.com/adafruit-huzzah-esp8266-breakout/using-arduino-ide)
1. Upload the Website files to the SPIFFS filesystem. There are two variants to do this:
 * option 1: Put the ESP8266 into programming mode again and upload the entire content set of the data folder through an Arduino IDE plug-in via the serial interface. 
 Note that the serial monitor must be closed when using the *ESP8266 Sketch Data Upload* function from the Adruino IDE tools menu. 
 Also you do need an Arduino IDE plug-in from here: 
[upload file system and web site](https://github.com/esp8266/Arduino/blob/master/doc/filesystem.md#uploading-files-to-file-system) (SPIFFS)

 * option 2 (requireds option 1 executed once to format the file system): 
 (TODO: Format the ESP8266 filesystem using the WebUI or api call `/api?formatfilesystem=true`)
 and upload the all files (*.html, *.css, font.*, ...) that are contained in the `data` folder through the Web UI using the firmware upload form. 
 those files will be automatically put on the ESP8266 file system.

##Configure the UFO
1. Once firmware is uploaded to the UFO, it starts an access point with SSID named "ufo". (if you cannot see the "ufo" SSID, click the WiFi reset button) - the UFO alternates a single blue ring
2. Connect to the "ufo" Wifi and navigate to http://192.168.4.1 - once connected the UFO blinks with a second blue ring
![](configure wifi.png)
3. Configure WiFi settings so that the UFO connects to your WiFi (note, enterprise PEAP authentication is not yet available) - whenever the UFO is trying to connect to your WiFi, the UFO blinks yellow. So you can declare connection success when the yellow stopped blinking.
NOTE: in case you have troubles using the Web UI for setting the WiFi config, you might have more success using the api directly:
`http://192.168.4.1/api?ssid=<ssid>&pwd=<pwd>`
4. Access the Web UI of the UFO as well as the REST interface /api    ***TODO FOR MORE***

##Todo
* implement the original UFO api calls also on ESP8266
* publish the Dynatrace Application Monitoring and Ruxit API infos, DCRUM and Synthetic to follow
* slight improvement on the base, to allow sliding the ring onto the base with LEDs already mounted
* publish pre-compiled binaries and instructions to use the flashing tool
* update web UI to implement desired use-cases  


