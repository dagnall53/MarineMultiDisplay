
# Marine Multi Display

This project is a development of the NMEADISPLAY Wireless Instrument Repeater Display for Boats. This revised project supports direct NMEA2000 connection and has a completely revised file editor and uses flash memory for configuration files making the use of the SD card optional.
It has three versions, one for the GUITRON 480x480 module, one for the 480x480 4 inch WaveShare Module and one for the 800x480 '4.3inch box' wide Waveshare module.  
The Wavshare modules were selected because they have in built CANBUS and 6-36V power supplies, making them slightly better suited for N2000 bus integration.
With the Guitron module, it is necessary to provide an external 5v power supply (from the boat or N2000 bus) and optionally add a cheap CAN drives such as TJA1050, and wire it (with a 3.3V power supply) to the 8 pin connector. 

Despite sounding more practical for boat use, the Wavshare modules have a very iritating 'feature' in that they do not switch on with power on, and require pressing their reset buttons before the screens turn on. 

<i><small>STANDARD DISCLAIMER: This instrument is intended as an aid to navigation and should not be relied upon as the sole source of information. 
While every effort has been made to ensure the accuracy of message translations and their display, they are not guaranteed. 
The user is responsible for cross-checking data with other sources, exercising judgment, and maintaining situational awareness. 
No liability for any loss, damage, or injury resulting from the use of this instrument will be accepted. </i></small>

## HOW TO INSTALL FIRST TIME
First, plug your module into a com port on your PC. Switch the module on (press the PWRKEY and/or reset! ) and record which port it is using. If confused, check Device Manager and look for the USB-SERIAL CH340 port. 
Remember the port number!

Click the link below to download the file "WebProgram.bat" from the github.
<a href="https://dagnall53.github.io/MarineMultiDisplay/build/MMDWebProgram.bat" download>Download MMD WebProgram.bat</a>

Save this somewhere convenient such as downloads.
NOTE: this program (may) erase any data in the module flash memory. (*)
Run the program ..   Make sure you select the correct version for your module and set the correct USB port. 
It will download the latest binaries to the directory where you saved it and program the hardware. 
It will then delete the binaries and the tool used to upload after it has completed,leaving just the WebProgram.bat file. 

It is possible some PC/ modules/ cables may not program at full speed. If you have trouble programming, try reducing the baud rate from 921600 to 115200. 
If your module becomes corrupted and you cannot connect to USB port, you can usually recver by Pressing Reset, and then hold down BOOT then release reset with boot still pressed, and release boot. This should allow the USB port to start and make the module receptive to reprogramming.
(on the Guitron module, these buttons are hidden under the black cover, which will need to be removed for access.)

## CONFIG FILEs STRUCTURE and Editor 

When connected via a PC, upload of new files (or images) is easy. However when connect by phone, you are effectively limited to just editing existing text files.
ALL configuration files (".txt") for this version of the code ars now contained in three files that are stored in the (SPIFFS) flash and are automatically generated on first start. 
There is no need to add a specially configured SD card.

<img width="928" height="562" alt="Screenshot 2025-09-11 175536" src="https://github.com/user-attachments/assets/c5023189-ab79-46fd-81c7-7a0e337b039e" />
WiFi details and source (UDP,ESPnow,Serial,N2K) switching can be set via the touch interface, but more options are available by editing the configuration files via the filemanager, and this is the recommended method for setting the module.
Most importantly, this allows the user to change what is shown on the "Quad Display" {the default 'page 4' start page}. This configuration is saved in config.txt along with the wifi etc details.
Data to set up the victron displays is set in vconfig.txt and colortest.text.

# MODULE HARDWARE 

###WAVSHARE LCD-4

<img width="200" height="200" alt="waveshare 480" src="https://github.com/user-attachments/assets/3ad72656-72e5-4775-a1ed-1393f1a75a34" />
WIKI: https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4

###GUITRON Module 

<img width="200" height="200" alt="Guitron" src="https://github.com/user-attachments/assets/d9f3aa7f-7baf-40a3-b50e-e2567d49540b" />
"ESP32-S3 Development Board WiFi BT 4 Inch IPS Touch Screen for Arduino LVGL IOT ESP32 86 Box Central Centrol" 
Availble from multiple sources (Possibly has part number JC4848W540C_I )



## FLASH file Storage 
The root of the FLASH (SPIFFS) should have (at least) these files:
config.txt, ( a json file with user settings) and 
vconfig.txt ( a json with settings for the ble victron mode )
colortest.txt ( a json with settings that will eventually allow global day/night colours and also has some simulation/debug settings for the BLE part of the display)

These txt files may (should!) self initiate if not present, but its better to have defaults present! 
Examples are available in the EXAMPLES folder on github. In particular, the example Victron configurations (colortest.txt and vconfig.txt and vicback.jpg set up a simulation to show the Victron display capability.

Other files may be added :
(note- I do not recommed using graphics on the Wavshare boards as the colour quality is poor - this may be an issue in the driver ? and the display does not really need Jpegs! )
logo4.jpg (the new generic start screen image), 
v3small.jpg (used in the webbrowser start screen).
and loading.jpg, (a picture that appears during OTA updates). 

## Connecting to NMEA 2000
Is done automatically and does not require the module to be wirelessly connected to a multiplexer.

BUT - Wireless communication is necessary to change settings. I would recommend normally connecting the display to a WiFi multiplexer just to allow it to stop searching for the network.

For the Guiton device, add a CAN Driver module such as the TJA1050 and a 3.3V voltage regulator.
Connect the bottom left connection of the 8 way socket to TJA "TX" and the connection above to TJA "RX".
Connect the 5v power supply (Bottom Right) to the 3.3v regulator input, and the output of the 3.3V regulator to the VCC connection of the TJA.
Connect the NMEA2000 (or boat 12V) to a 5V regulator input and connect the 5V output to the 5V power supply on the module.
Connect Ground (top pins of the 8 way socket) to the TJA ground and Regulator grounds. 
Lastly, connect the CANH and CANl from the TJA to the NMEA2000 canbus. 

## CAUTIONS

The wavshare displays are sensitive to voltage and do not initialise the touch sensors if the power input is insufficient, as it may be if powered only by USB.

## Connecting to your Wifi SSID 
You can connect to the module's access point by connecting and then using a browser to connect to 192.168.4.1 

If you happen to have a boat with a WiFi SSID "GuestBoat" and password 12345678, you will connect instantly as this is the default.
With touchscreen, Go to Settings WiFi,  Click on "set SSID", and you will be presented with a scan of available networks.

You can select a network by touching it and it will update in the second box and show (eg) Select<GUESTBOAT>?
if you touch this, it will select that SSID and return you to the main WiFi Settings page. 
Press EEPROM "UPDATE" and then "Save/Reset " to save this new SSID in the eeprom.
Settings made via the screen will be copied into the config.txt file on flash and may also be modified wirelessly. (see next).
Settings on the config.txt (from flash) take priority when starting.

The Display does a scan on startin he wifi SSID page and this may take a while (2-4seconds).
it will retry the scan once every 30 seconds if it does not find the SSID. 
This may interrupt the display. 

### Webbrowser:

There is a web interface that can be connected to by pointing a browser at http://nmeadisplay.local/ (default)
If you change the panel name, you will need to point to the new name: eg http://panel2.local (etc).
You can also point directly to the IP address as shown on the WiFi settings page. 

## USING FILE Manager to select settings.

On web browser go to (IP:8080)
EG 192.168.4.1:8080 
This will bring up the new Trek style file browser.
There are three JSON files that control operations:
The "config.txt". controls WiFi settings and major display modes, and is backed up by EEPROM, so that the SD card is not essential for basic operations.
Vconfig.txt and colortest.txt are only stored on the SD and are related mainly to Victron BLE functions.
In the config.txt you can select which 'pageIndex' is displayed after startup by changing the number: "Start_Page" 4 is the quad display and the default. 
From version 3.97, you can select what is displayed in the various 'quarters' with the JAVASCRIPT entries: 
e.g.  "FourWayBR": "SOG",
      "FourWayBL": "DEPTH",
  other options are : STW GPS SOGGRAPH STWGRAPH DGRAPH and DGRAPH2 TIME, TIMEL 
on Saving (SDsave), the settings will be implemented on the display.  

To edit any of these, Click on the [E] alongside the file name and press save when finished. 

The Display will check files on startup. If the file is not present it will use defaults. 

## Navigating the Display touch screen

<b>there is a common 'click for settings' at the bottom of every screen that will take you directly to the list of menu functions.</b>

The module will start with the "Quad" instrument display. Touching each quadrant will select a different display page.
This is a simplified view of the original mapping.
![Screen Navigation](https://github.com/user-attachments/assets/f05d7e21-4c72-45cd-ae81-91a27ed20897)

Which instrument data will be displayed in each of the 'quadrants' of the 'Quad' display can be selected from variables in the config.txt file. 

### NMEA DATA and UDP and N2K

Whilst the main way to send instrument data to the Display was originally via NMEA(0183) over UDP, the project also accepts 'ESP-NOW' from suitable multiplexers such as VELA-Naviga types and N2K if the CAN driver is fitted.: 
https://www.vela-navega.com/index.php/multiplexers


### Victron BLE device display

I have been adding the ability to switch the display to show Victron BLE data from suitable Victron devices.
![victrondisplay](https://github.com/user-attachments/assets/a0685f92-06f6-4189-8c27-e6e044bc54d0)
This uses regular, repeated, BLE scanning for Victron devices and a subsequent screen display of parameters obtained. 
This is immensely powerful, but interrupts the main wifi for one second every time it scans for BLE devices.
It cannot therefore be used simultaneously with the standard NMEA display routines. 

There are two critical files for the display are vconfig.txt. (which has the Victron device Mac, key and names), and colortest.txt which has some settings to allow simulation of victron devices and other inputs useful during development.
The display pageIndex uses a jpg (vicback.jpg) as a background for a more colourful display.
The code recognises only Solar chargers , SmartShunt and IP65 AC chargers.
The graphical format of the display is defined in VICTRONBLE.cpp and (vconfig.txt).
Each device has selected data displayed in a box with selectable position and height (but with fixed fonts and width).
You will need to obtain the MAC and KEY via the victron app (see the device settings) and your devices must be connectable via the app.  

There is a Simulate option, selectable in colortest, this initiates a crude simulation, suitable for assisting in code development.
This works most logically with my default Vconfig.txt file. 
I have seen some issues with Simulate crashing the display very badly:(needed reinstall of code). But I think(hope) this has been fixed.

For reference see also  https://github.com/Fabian-Schmidt/esphome-victron_ble


GUIT version 1978885 bytes 29/09

=======

# COMPILE NOTES

File viewer see https://github.com/holgerlembke/

Add NMEA0183 and NMEA2000 libraries from https://github.com/ttlappalainen
I have used some of the Timo Lappinen NMEA01983 conversion functions. so null data returns as NMEA0183DoubleNA (==1e-9)

Based on concepts from AndreasSzep WiFi, https://github.com/AndrasSzep/NMEA0183-WiFi
Note: I have tried to avoid use of "Strings" as used in the original Keyboard and Andreas Szep concepts, and modified the codes to use only strings as char arrays. 

The GFX is based on GFX Library for Arduino and I am using Version 1.5.5 :
Getting the ST7701 display to work correctly was part of the initial reason for my "cheap display" Github.
the author of the GFX code sometimes updates the drivers, so if you use a different driver you may need to make small modifications:
for the Guitron version I used :  st7701_type9_init_operations,  sizeof(st7701_type9_init_operations));
But for this wavshare I have used type_1


I started the Victron decode with the https://github.com/hoberman/Victron_BLE_Advertising_example, 
but later moved to using code snippets from   https://github.com/Fabian-Schmidt/esphome-victron_ble  
Other useful sources are noted in the victron cpp and .h.

# OTHER NOTES 

The main 'User' Settings for the display are now kept in JSON fioles stored on the FATFS 

Use of SD card: (for logs (later) )
Apparently, File Names longer than 8 characters may be truncated by the SD library, so keep filenames shorter.
I have noted particular issues when uploading  jquery.min.js  as this truncated to jquery.m.js  - but editing the name in the Filename box was succesul and it was stored on the SD with the full name.




