#ifndef _OTA_H_
#define _OTA_H_
/*
 BASED ON  SDWebServer - Example WebServer with SD Card backend for esp8266

  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the WebServer library for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  
*/


#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Arduino_GFX_Library.h>
#include "Structures.h"

#include <SPI.h>
#include <SD.h>  // was SDh

bool HaltOtherOperations;
extern void SD_CS(bool state);
//#include "LittleFS.h"
extern bool hasFS;
extern bool hasSD;
static bool INSTlogFileStarted = false;
static bool NMEAINSTlogFileStarted = false;

extern _sDisplay_Config Display_Config;

extern const char *Setupfilename;
extern bool LoadConfiguration(fs::FS &fs,const char* filename, _sDisplay_Config& config, _sWiFi_settings_Config& settings);
extern void SaveConfiguration(fs::FS &fs, const char *filename, _sDisplay_Config &config, _sWiFi_settings_Config &settings);
extern _sWiFi_settings_Config Current_Settings;
extern void EEPROM_WRITE(_sDisplay_Config B, _sWiFi_settings_Config A);

extern void PrintJsonFile(const char *comment, const char *filename);
extern const char *VictronDevicesSetupfilename;
extern _sMyVictronDevices victronDevices;
// nb if victron or display settings are missing, '/save' will create them
extern bool LoadVictronConfiguration(fs::FS &fs, const char *filename, _sMyVictronDevices &config);
extern void SaveVictronConfiguration(fs::FS &fs, const char *filename, _sMyVictronDevices &config);

extern bool LoadDisplayConfiguration(fs::FS &fs, const char *filename, _MyColors &set);
extern void SaveDisplayConfiguration(fs::FS &fs, const char *filename, _MyColors &set);

extern const char *ColorsFilename;
extern _MyColors ColorSettings;
extern void showPicture(const char *name);
// extern Arduino_ST7701_RGBPanel *gfx ;  // declare the gfx structure so I can use GFX commands in Keyboard.cpp and here...
extern Arduino_RGB_Display *gfx;  //  change if alternate (not 'Arduino_RGB_Display' ) display !
extern void setFont(int);
extern const char soft_version[];
//const char *host = "NMEADisplay";
extern _sBoatData BoatData;
extern void WifiGFXinterrupt(int font, _sButton &button, const char *fmt, ...);
extern _sButton WifiStatus;
extern int Display_Page;
extern void Display(bool reset, int page);

bool WebServerActive;
WebServer server(80);
File SDuploadFile;
char SavedFile[30];
char InstLogFileName[25];
char NMEALogFileName[25];

extern bool SDexists(const char *path);  // only for SD library

// //****************write file etc from examples and  from (eg) https://randomnerdtutorials.com/esp32-data-logging-temperature-to-microsd-card/
void writeFile(fs::FS &fs, const char *path, const char *message) {
  DEBUG_PORT.printf("*writeFile  Writing file: [%s]  [%s]\n", path, message);
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    DEBUG_PORT.println("*writeFile Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    //  DEBUG_PORT.println("File written");
  } else {
    DEBUG_PORT.println("*writeFile Write failed");
  }
  file.close();
}



// //***************************************************


// Slightly more flexible way of defining page.. allows if statements ..required for changed displayname..
String html_Question() {
  String st = "<!DOCTYPE html>\r\n";
  st += "<html><head>";
  st += "<br><b><center>Compiled on ";
  st += __DATE__;
  st += " ";
  st += __TIME__;
  st += "</center> <br><center> Compiled with:";
  st += String(ESP_ARDUINO_VERSION_MAJOR);
  st += ".";
  st += String(ESP_ARDUINO_VERSION_MINOR);
  st += ".";
  st += String(ESP_ARDUINO_VERSION_PATCH);
  st += " (based on ESP-IDF:";
  st += String(esp_get_idf_version());
  st += ")</center><br><center>Code Comments</b><br></center><left>";
  st += "<title>NavDisplay ";
  st += String(soft_version);
  st += "</title>";
  st += " </head>";
  st += "<body><center><h1 ><a>This device's panel name is ";
  st += String(Display_Config.PanelName);
  st += "</h1><br>\r\n</a><h1>Software :";
  st += String(soft_version);
  st += "</h1><br> I may add user help instructions here:<br>";
  st += "</center></body></html>";
  return st;
}
// So I can modify the Display Panel Name! but also so that OTA works even without SD card present
//
/*the main html web page, with modified names etc    */
//prettified version
String html_startws() {
  String logs, filename;
  String st =
    "<html> <head> <meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<meta http-equiv='content-type' content='text/html;' charset='utf-8'>"
    "<title>NavDisplay</title>"
    "<style>"
    "body {background-color: white;color: black;text-align: center;font-family: sans-serif;color: #777;}"
    ".title {font-size: 2em; margin: 20px 0;text-align: center;}"
    ".version {text-align: center;margin-bottom: 10px;}"
    ".button-link {display: inline-block;padding: 0px 15px;background-color: #006;"
    "color: white;text-decoration: none;border-radius: 4px;margin: 5px 0;"
    "border: 1px solid #666;transition: background-color 0.3s;font-size: 15px;"
    "background: #3498db; color: #fff;height: auto;}"
    ".button-link:hover {background-color: #666;}"
    ".button-linkSmall {display: inline-block;padding: 0px 15px;background-color: #006;"
    "color: white;text-decoration: none;border-radius: 4px;margin: 5px 0;"
    "border: 1px solid #666;transition: background-color 0.3s;font-size: 10px;"
    "background: #3498db; color: #fff;height: auto;}"
    "h1 {margin: 10px 0;font-size: 18px;}</style></head>"
    "<body>"
    //"<img src='/v3small.jpg'><br>"
    "<div class='title'>";
  
      
  st += String(Display_Config.PanelName);
  st += "</div>"
        "<div class='version'>";
  st += String(soft_version);
  st += "</div>"
        "<h1><a class='button-link' href='http://";
  st += String(Display_Config.PanelName);
  st += ".local/Reset'>RESTART</a></h1>"
        "<h1><a class='button-link' href='http://";
  st += String(Display_Config.PanelName);
  st += ".local/OTA'>OTA UPDATE</a></h1>"
        "<h1><a class='button-link' href='http://";
  st += String(Display_Config.PanelName);
  st += ".local:8080'> File Editor</a></h1><br>"
        "<div class='version'>Saved Log files on SD </div>";
        SD_CS(LOW);
  File dir = SD.open("/logs");
  for (int cnt = 0; true; ++cnt) {
    File entry = dir.openNextFile();
    DEBUG_PORT.println(entry.path());
    if (!entry) { break; }
    filename = String(entry.path());
    st += "<h1 ><a class='button-linkSmall' href='http://";
    st += String(Display_Config.PanelName);
    st += ".local";
    st += filename;
    st += "'> ";
    st += " View ";
    st += filename;
    st += "</a></h1>";
    entry.close();
  }
  SD_CS(HIGH);
  st += "</body></html> ";
  return st;
}


/* Style  Noted: Arduino will mis-colour some parts*/

String style =
  "<style>#file-input,input{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}"
  "input{background:#f1f1f1;border:0;padding:0 15px}body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}"
  "#file-input{padding:0;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer}"
  "#bar,#prgbar{background-color:#f1f1f1;border-radius:10px}#bar{background-color:#3498db;width:0%;height:10px}"
  "form{background:#fff;max-width:258px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}"
  ".btn{background:#3498db;color:#fff;cursor:pointer}</style>";

/* OTA Server Index Page */
String serverIndex() {
  String st;
  if (FFat.exists("/edit/jquery.min.js")) {
    DEBUG_PORT.println("using Ffats /edit/ javascript");
    st = "<script src='/edit/jquery.min.js'></script>";
  } else {
    DEBUG_PORT.println("using Internet js");
    st = "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>";
  }

  /*"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"*/
  /*"<script src='/edit/jquery.min.js'></script>"*/
  st += "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
        "<input type='file' name='update' id='file' onchange='sub(this)' style=display:none>"
        "<h1>OTA Interface for NMEA DISPLAY</h1>"
        "<br><center>"
        + String(soft_version) + "</center> <br>"
                                 "<label id='file-input' for='file'>   Choose file...</label>"
                                 "<input type='submit' class=btn value='Update'>"
                                 "<br><br>"
                                 "<div id='prg'></div>"
                                 "<br><div id='prgbar'><div id='bar'></div></div><br></form>"
                                 "<script>"
                                 "function sub(obj){"
                                 "var fileName = obj.value.split('\\\\');"
                                 "document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];"
                                 "};"
                                 "$('form').submit(function(e){"
                                 "e.preventDefault();"
                                 "var form = $('#upload_form')[0];"
                                 "var data = new FormData(form);"
                                 "$.ajax({"
                                 "url: '/update',"
                                 "type: 'POST',"
                                 "data: data,"
                                 "contentType: false,"
                                 "processData:false,"
                                 "xhr: function() {"
                                 "var xhr = new window.XMLHttpRequest();"
                                 "xhr.upload.addEventListener('progress', function(evt) {"
                                 "if (evt.lengthComputable) {"
                                 "var per = evt.loaded / evt.total;"
                                 "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
                                 "$('#bar').css('width',Math.round(per*100) + '%');"
                                 "}"
                                 "}, false);"
                                 "return xhr;"
                                 "},"
                                 "success:function(d, s) {"
                                 "console.log('success!') "
                                 "},"
                                 "error: function (a, b, c) {"
                                 "}"
                                 "});"
                                 "});"
                                 "</script>"
        + style;
  return st;
}

void handleRoot() {
  //DEBUG_PORT.println(" Sending local html version of Root webpage");
  server.send(200, "text/html", html_startws() + "\r\n");
}

void handleQuestion() {
  server.sendContent(html_Question());
  server.sendContent("");
  server.sendContent("");
  server.client().stop();
}

void MDNS_START(){ 
  delay(10) ;
  if (MDNS.begin(Display_Config.PanelName)) {
    MDNS.addService("http", "tcp", 80);
    DEBUG_PORT.println("MDNS responder started");
    DEBUG_PORT.print("You can now connect to http://");
    DEBUG_PORT.print(Display_Config.PanelName);
    DEBUG_PORT.println(".local");
    WifiGFXinterrupt(8, WifiStatus, "Connected:\n<%s>\n http://%s.local", WiFi.SSID(),Display_Config.PanelName);
  }else{DEBUG_PORT.println(" MDNS responder FAILED to start");WifiGFXinterrupt(8, WifiStatus, "Connected TO\n<%s>", WiFi.SSID());}
  }

void SetupWebstuff() {
 MDNS_START();
  //**************
  server.on("/", HTTP_GET, []() {
    DEBUG_PORT.println(" handling  root");
    WifiGFXinterrupt(8, WifiStatus, "Running Webserver");
    WebServerActive = true;
    handleRoot();
  });
  server.on("/Reset", HTTP_GET, []() {
    handleRoot();
    if (LoadConfiguration(FFat,Setupfilename, Display_Config, Current_Settings)) { EEPROM_WRITE(Display_Config, Current_Settings); }  // stops overwrite with bad JSON data!!
    // Victron is never set up by the touchscreen only via SD editor so NO SaveVictronConfiguration(VictronDevicesSetupfilename,victronDevices);// save config with bytes ??
    WifiGFXinterrupt(8, WifiStatus, "RESTARTING");
    handleRoot();  // hopefully this will prevent the webbrowser keeping the/reset active and auto reloading last web command (and thus resetting!) ?
    server.sendHeader("Connection", "close");
    delay(150);
    WiFi.disconnect();
    ESP.restart();
  });
  server.on("/Save", HTTP_GET, []() {
    handleRoot();
    if (LoadConfiguration(FFat,Setupfilename, Display_Config, Current_Settings)) {
      DEBUG_PORT.println("***Updating EEPROM from ");
      EEPROM_WRITE(Display_Config, Current_Settings);
    }  // stops overwrite with bad JSON data!!
    if (LoadVictronConfiguration(FFat,VictronDevicesSetupfilename, victronDevices)) {
      PrintJsonFile(" Check Updated Victron settings after Web initiated SAVE ", VictronDevicesSetupfilename);
      DEBUG_PORT.println("***Updated Victron data settings");
    }
    if (LoadDisplayConfiguration(FFat,ColorsFilename, ColorSettings)) {
      DEBUG_PORT.println(" USING JSON for Colours data settings");
    } else {
      DEBUG_PORT.println("\n\n***FAILED TO GET Colours JSON FILE****\n**** SAVING DEFAULT and Making File on SD****\n\n");
      SaveDisplayConfiguration(FFat,ColorsFilename, ColorSettings);  // should write a default file if it was missing?
    }
    delay(50);
    Display(true, Display_Page);
    delay(50);
  });

  server.on("/OTA", HTTP_GET, []() {
    WifiGFXinterrupt(9, WifiStatus, "Halting for OTA");
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex());
    HaltOtherOperations=true;
  });
  server.on("/ota", HTTP_GET, []() {
    WifiGFXinterrupt(9, WifiStatus, "Halting for OTA");
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex());
    HaltOtherOperations=true;
  });


  /*handling uploading firmware file */
  server.on(
    "/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      WiFi.disconnect();
      ESP.restart();
    },
    []() {
      HTTPUpload &upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        HaltOtherOperations=true;
         setFont(9);
        gfx->setTextColor(BLACK);
        gfx->fillScreen(BLUE);
        delay(10);
        // // showPicture("/loading.jpg");
        gfx->setCursor(0, 40);
        gfx->setTextWrap(true);
        gfx->printf("Update: %s\n", upload.filename.c_str());
        DEBUG_PORT.printf("Update: %s\n", upload.filename.c_str());
        DEBUG_PORT.printf("   current size: %u   total size %u\n", upload.currentSize, upload.totalSize);
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {  //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        /* flashing firmware to ESP32*/
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
        //monitor upload milestones (100k 200k 300k)?
        uint16_t chunk_size = 51200;
        static uint32_t next = 51200;
        if (upload.totalSize >= next) {
         gfx->printf(" %dk ", next / 1024);
         DEBUG_PORT.printf("%dk ", next / 1024);
          next += chunk_size;
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {             //true to set the size to the current progress
        HaltOtherOperations=false;
        WifiGFXinterrupt(9, WifiStatus, "SW UPDATED");
        DEBUG_PORT.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
          delay(500);
        } else {
          Update.printError(Serial);
        }
      }
    });


  //*********** END of OTA stuff *****************


  server.begin();
  DEBUG_PORT.println("HTTP server started");
}



// Append data to the SD card (DON'T MODIFY THIS FUNCTION)
void appendFile(fs::FS &fs, const char *path, const char *message) {
  // DEBUG_PORT.printf("Appending to file: %s\n", path);
  if (!hasSD){return;}
  SD_CS(LOW);
  File file = fs.open(path, FILE_APPEND);
  if (!file) {SD_CS(HIGH);
    // DEBUG_PORT.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    //  DEBUG_PORT.println("Message appended");
  } else {
    //  DEBUG_PORT.println("Append failed");
  }
  file.close();
  SD_CS(HIGH);
}




void StartInstlogfile() {
  INSTlogFileStarted = false;
  if (!hasSD) { return; }
  // If the data.txt file doesn't exist
  // Create a file on the SD card and write the data labels
  if (BoatData.GPSDate == 0) { return; }
  if (BoatData.GPSDate == NMEA0183DoubleNA) { return; }  // and check for NMEA0183DoubleNA
  //We have a date so we can use this for the file name!
  // DEBUG_PORT.printf("  ***** LOG FILE DEBUG ***  use: <%6i>  to make name..  ",int(BoatData.GPSDate));
  snprintf(InstLogFileName, 25, "/logs/%6i.log", int(BoatData.GPSDate));
  //  DEBUG_PORT.printf("  <%s> \n",InstLogFileName);
  SD_CS(LOW);
  File file = SD.open(InstLogFileName);
  SD_CS(HIGH);
  if (!file) {
    //DEBUG_PORT.println("File doesn't exist");
    INSTlogFileStarted = true;
    DEBUG_PORT.printf("Creating <%s> Instrument LOG file. and header..\n", InstLogFileName);
    // data will be added by a see the  LOG( fmt ...) in the main loop at 5 sec intervals
    /*    int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60,
        BoatData.STW.data,  BoatData.WaterDepth.data, BoatData.WindSpeedK.data,BoatData.WindAngleApp);
        */
    SD_CS(LOW);
    writeFile(SD, InstLogFileName, "LOG data headings\r\n Local Time ,STW ,MagHdg, SOG, COG,Depth ,Windspeed,WindAngleApp\r\n");
    SD_CS(HIGH);
    file.close();
    return;
  } else {
    INSTlogFileStarted = true;
    DEBUG_PORT.println("Log File already exists.. appending");
  }
  file.close();
}

void StartNMEAlogfile() {
  if (!hasSD) {
    NMEAINSTlogFileStarted = false;
    return;
  }
  // If the data.txt file doesn't exist
  // Create a (FIXED NAME!) file on the SD card and write the data labels
  SD_CS(LOW);
  File file = SD.open("/logs/NMEA.log");
  if (!file) {
    //DEBUG_PORT.println("File doens't exist");
    NMEAINSTlogFileStarted = true;
    DEBUG_PORT.printf("Creating NMEA LOG file. and header..\n");
    writeFile(SD, "/logs/NMEA.log", "NMEA data headings\r\nTime(s): Source:NMEA......\r\n");
    file.close();
    SD_CS(HIGH);
    return;
  } else {
    NMEAINSTlogFileStarted = true;
    DEBUG_PORT.println("NMEA log File already exists.. appending");
  }
  SD_CS(HIGH);
  file.close();
}

void NMEALOG(const char *fmt, ...) {
  if (!NMEAINSTlogFileStarted) {
    StartNMEAlogfile();
    return;
  }
  static char msg[800] = { '\0' };  // used in message buildup VICTRON log can be well in excess of 128 as it can have multiple devices showing
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 799, fmt, args);
  va_end(args);
  int len = strlen(msg);
  // DEBUG_PORT.printf("  Logging to:<%s>", NMEALogFileName);
  // DEBUG_PORT.print("  Log  data: ");
  // DEBUG_PORT.println(msg);
  appendFile(SD, "/logs/NMEA.log", msg);
}

void INSTLOG(const char *fmt, ...) {
  if (!INSTlogFileStarted) {
    StartInstlogfile();
    return;
  }
  static char msg[300] = { '\0' };  // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  // DEBUG_PORT.printf("  Logging to:<%s>", InstLogFileName);
  // DEBUG_PORT.print("  Log  data: ");
  // DEBUG_PORT.println(msg);
  appendFile(SD, InstLogFileName, msg);
}




// void ShowFreeSpace() {
//   // Calculate free space (volume free clusters * blocks per clusters / 2)
//SD_CS(LOW);
//   long lFreeKB = SDvol()->freeClusterCount();
//   lFreeKB *= SDvol()->blocksPerCluster()/2;
//SD_CS(HIGH);
//   // Display free space
//   DEBUG_PORT.print("Free space: ");
//   DEBUG_PORT.print(lFreeKB);
//   DEBUG_PORT.println(" KB");
// }


#endif