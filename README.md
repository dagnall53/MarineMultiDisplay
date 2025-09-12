
# Marine Multi Display
This project is a version of the NMEADISPLAY Wireless Instrument Repeater Display for Boats coded for the WaveShare Module
<img width="432" height="426" alt="waveshare 480" src="https://github.com/user-attachments/assets/3ad72656-72e5-4775-a1ed-1393f1a75a34" />

WIKI: https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4

<i>STANDARD DISCLAIMER: This instrument is intended as an aid to navigation and should not be relied upon as the sole source of information. 
While every effort has been made to ensure the accuracy of message translations and their display, they are not guaranteed. 
The user is responsible for cross-checking data with other sources, exercising judgment, and maintaining situational awareness. 
No liability for any loss, damage, or injury resulting from the use of this instrument will be accepted. </i>

This project now supports direct nMEA2000 connection. 
It (will eventually) also work with NMEA0183 data from any wireless NMEA Gateway that sends NMEA 0183 instrument readings on UDP.
But for now it just supports the N2K and Victron displays.
Working with this board has been problematic compared to the Guitron version. It seems very sensitive to WDT timouts. Especially for the SD card. 
So the SD card has been disabled (as well as the touch screen) on this code.  
However, a redeeming feature is that I have made use of the Flash memory to act as a FATFS file server and replaced the editor on the old version with a VERY nice editor from https://github.com/holgerlembke/.
This new editor is worth keeping and I may update the original NMEA display in the future to use this editor and the Flash/FATFS for file storage. 
<img width="928" height="562" alt="Screenshot 2025-09-11 175536" src="https://github.com/user-attachments/assets/c5023189-ab79-46fd-81c7-7a0e337b039e" />


## HOW TO INSTALL FIRST TIME
(NOT TESTED YET for THIS VERSION) 
First, plug your module into a com port on your PC. 
Switch the module on (press the PWRKEY) and record which port it is using. 
If confused, check Device Manager and look for the USB-SERIAL CH340 port. 
Remember the port number!

Click the link below to download the file "WebProgram.bat" from the github.
<a href="https://dagnall53.github.io/MarineMultiDisplay/build/WebProgram.bat" download>Download WebProgram.bat</a>

Save this somewhere convenient such as downloads.
Run the program .. 
It will download the latest binaries to the directory where you saved it and program the hardware. 
It will then delete the binaries and the tool used to upload after it has completed,leaving just the WebProgram.bat file. 

##Changed FILE STRUCTURE
The original program saved only the start page, WiFi details and source (UDP,ESPnow,Serial,N2K) switching. With the exception of the Start page, these could be set via the touch interface.
Later versions added the ability to change what is shown on the "Quad Display" {the default 'page 4' start page}. This configuration is saved in config.txt along with the wifi etc details.
Data to set up the victron displays is set in vconfig.txt and colortest.text.
In the origial code, these were stored on SD and if the SD is not present, default versions will be set, but cannot be altered.
In the MarineMultiDisplay version, these files are now stored in ESP32 Flash and so the SD card is not required. 

All config files are editable via the (new) editor.




## FATFS files 
The root of the FATFS should have (at least) these files:
config.txt, ( a json file with user settings) and 
vconfig.txt ( a json with settings for the ble victron mode )
colortest.txt ( a json with settings that will eventually allow global day/night colours and also has some simulation/debug settings for the BLE part of the display)
  These txt files may (should!) self initiate if not present, but its better to have defaults present! 
Add 
logo4.jpg (the new generic start screen image), 
v3small.jpg (used in the webbrowser start screen).
and loading.jpg, (a picture that appears during OTA updates). 
(these can be found on the NMEADISPLAY github, or you can add your own 400*400 JPG files)
The colour rendering on the Waveshare board is much less impressive than on the Guitron module

## Connecting to NMEA 2000
Is done automatically and does not require the module to be wirlessly connected to a multiplexer.
Wireless communication is necessary to change settings. 

## Connecting to your Wifi SSID 

If you happen to have a boat with a WiFi SSID "GuestBoat" and password 12345678, you will connect instantly as this is the default.
For your SSID, Go to Settings WiFi,  Click on "set SSID", and you will be presented with a scan of available networks.

You can select a network by touching it and it will update in the second box and show (eg) Select<GUESTBOAT>?
if you touch this, it will select that SSID and return you to the main WiFi Settings page. 
Press EEPROM "UPDATE" and then "Save/Reset " to save this new SSID in the eeprom.
Settings made via the screen will be copied into the config.txt file on the SD card and may also be modified wirelessly. (see next).
Settings on the config.txt (from SD card) take priority when starting.

The Display does a scan on startup and will attempt to automatically connect IF it finds the correct SSID on startup. 
it will retry the scan once every 30 seconds if it does not find the SSID. 
This will interrupt the display, but since you are not connected .. there is nothing to display! 

## USING FILE Manager to select settings.
On web browser go to (IP:8080)
EG 192.168.4.1:8080 
This will bring up the new Trek style file browser.
There are three JSON files that control operations:
The "config.txt". controls WiFi settings and major display modes, and is backed up by EEPROM, so that the SD card is not essential for basic operations.
Vconfig.txt and colortest.txt are only stored on the SD and are related mainly to Victron BLE functions.
In the config.txt you can select which 'page' is displayed after startup by changing the number: "Start_Page" 4 is the quad display and the default. 
From version 3.97, you can select what is displayed in the various 'quarters' with the JAVASCRIPT entries: 
e.g.  "FourWayBR": "SOG",
      "FourWayBL": "DEPTH",
  other options are : STW GPS SOGGRAPH STWGRAPH DGRAPH and DGRAPH2 TIME, TIMEL 
on Saving (SDsave), the settings will be implemented on the display.  

To edit any of these, Click on the [E] alongside the file name and press save when finished. 

The Display will check files on startup. If the file is not present it will use defaults. 

### Webbrowser:

There is a web interface that can be connected to by pointing a browser at http://nmeadisplay.local/ (default)
If you change the panel name, you will need to point to the new name: eg http://panel2.local (etc).
You can also point directly to the IP address as shown on the WiFi settings page. 


## Navigating the Display touch screen

<b>there is a common 'click for settings' at the bottom of every screen that will take you directly to the list of menu functions.</b>

The module will start with the "Quad" instrument display. Touching each quadrant will select a different display page.
This is a simplified view of the original mapping.
![Screen Navigation](https://github.com/user-attachments/assets/f05d7e21-4c72-45cd-ae81-91a27ed20897)

Here is a short video tour of Version 1 of the software : https://youtube.com/shorts/24qs9CJK5vo?si=zCDUuTbXkYfHtEDB
(Version 2 and 3 hav better graphics and a modified menu, but are essentailly similar).

From version 4, the what instrurment data will be displayed in each of the 'quadrants' of the 'Quad' display can be selected from variables in the config.txt file. 

### NMEA DATA and UDP

Whilst the main way to send instrument data to the Display is via NMEA(0183) over UDP, the project also accepts 'ESP-NOW' from suitable multiplexers such as VELA-Naviga types: 
https://www.vela-navega.com/index.php/multiplexers
The module also natively supports NME2000. 


## MODULE Hardware Requirements

The code is based on the WAVESHARE  4.0 Inch ESP32-S3  Touch Screen LCD.
There are sometimes two versions. I bough the one without Touch by mistake.
I have not confirmed Touch operation.




# Victron BLE device display

I have been adding the ability to switch the display to show Victron BLE data from suitable Victron devices.
![victrondisplay](https://github.com/user-attachments/assets/a0685f92-06f6-4189-8c27-e6e044bc54d0)
This uses regular, repeated, BLE scanning for Victron devices and a subsequent screen display of parameters obtained. 
This is immensely powerful, but interrupts the main wifi for one second every time it scans for BLE devices.
It cannot therefore be used simultaneously with the standard NMEA display routines. 
The Victron display is accessible via "experimental". I would emphasize that this feature/capability is experimental!

There are two critical files for the display are vconfig.txt. (which has the Victron device Mac, key and names), and colortest.txt which has some settings to allow simulation of victron devices and other inputs useful during development.
The display page uses a jpg (vicback.jpg) as a background for a more colourful display.
The code recognises only Solar chargers , SmartShunt and ( tested and found to be very faulty! ) IP65 AC chargers.
The graphical format of the display is defined in VICTRONBLE.cpp and (vconfig.txt).
Each device has selected data displayed in a box with selectable position and height (but with fixed fonts and width).

There is a Simulate option, selectable in colortest, this initiates a crude simulation, suitable for assisting in code development.
This works most logically with my default Vconfig.txt file. 
I have seen some issues with Simulate crashing the display very badly:(needed reinstall of code). But I think this has been fixed in V4.30 



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




