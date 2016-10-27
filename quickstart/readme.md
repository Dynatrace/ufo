#UFO Quickstart

##Power Supply

Connect the UFO to a 2000mA USB charging adapter. 
e.g. the iPad or Windows Surface charger should work fine, but check the specs beforehand. 
Note that the UFO draws more current than standard USB can provide. 
Please be warned!    

##Check Firmware Version

This description is for firmware versions dated after 25th of October 2016. 
If you have older firmware, please follow the firmware updated procedures in the howto guide.

##Configure Wifi
1. As default and when clicking the hardware reset button at the top of the UFO, 
the UFO starts a Wifi access point with an SSID named "ufo". 
(if you cannot see the "ufo" SSID, click the wifi reset button) - 
the UFO alternates a single blue ring
2. Connect a Web browser enabled device (laptop, tablet, smartphone) 
to the UFO's Wifi with the SSID "ufo" and navigate to http://192.168.4.1 - 
Once connected the UFO blinks with a second blue ring
![](configure wifi.png)
3. Configure UFO wifi settings so that the UFO connects to your wifi 
(Note: WPA2 works well, but enterprise WPA2 and PEAP authentication is not yet available) - 
whenever the UFO is trying to connect to your wifi, the UFO blinks yellow. 
So you can declare connection success when the yellow stops blinking.
Note: If you have troubles using the web UI for setting the wifi config, 
you might have more success using the REST API directly:
`http://192.168.4.1/api?ssid=<ssid>&pwd=<pwd>`

Tip: To test the ufo despite enterprise wifi, you can use 
e.g. Windows 10 (anniversary update) mobile hotspot feature or connect it your mobile devices hotspot capability.
![](windows10 hotspot.png)
1. Once Wifi is configured, you have multiple options to access the UFO
  * Option 1: The lower ring visualizes the current IP address digit by digit. 
 192.168... will light 1 led then 9 (5+4) then 2 and so on. 
 A dot is visualized as 3 white leds. 
 The individuL digits are separated by a short white flash. 
 The IP is visualized over and over again until the first api rest call is issued. 
 To stay in sync every IP address visualization run uses a different color.
  * Option 2: in Windows you can use the UPNP (SSDP) discovery 
  in the File Explorers Network view ![windows discovers the ufo](windows discovers ufo.png)
  * Option 3: try http://ufo as the UFO uses the default hostname "UFO" when it registers with DHCP.
  * Option 4: lookup the MAC address in your Wifi Access Point and get the IP address assigned by DHCP
4. Access the web UI of the UFO as well as the REST interface /API    ***TODO FOR MORE***


##Integration with Dynatrace SaaS
* Dynatrace SaaS default integration shows following color codes TODO

##Generic REST Integration

