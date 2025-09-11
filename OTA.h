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

  Have a FAT Formatted SD Card connected to the SPI port of the ESP8266
  The web root is the SD Card root folder
  File extensions with more than 3 charecters are not supported by the SD Library
  File Names longer than 8 charecters will be truncated by the SD library, so keep filenames shorter
  index.htm is the default index (works on subfolders as well)

  upload the contents of SdRoot to the root of the SD.card and access the editor by going to 
  http://nmeadisplay.local/edit
  To retrieve the contents of SD.card, visit http://nmeadisplay.local/list?dir=/
      dir is the argument that needs to be passed to the function PrintDirectory via HTTP Get request.
  NB  in my version nmeadisplay is settable in the config.txt, so you may need to access 192.168.4.1 if accessing the AP.. 

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
extern bool LoadConfiguration(int FS, const char *filename, _sDisplay_Config &config, _sWiFi_settings_Config &settings);
extern void SaveConfiguration(int FS, const char *filename, _sDisplay_Config &config, _sWiFi_settings_Config &settings);
extern _sWiFi_settings_Config Current_Settings;
extern void EEPROM_WRITE(_sDisplay_Config B, _sWiFi_settings_Config A);

extern void PrintJsonFile(const char *comment, const char *filename);
extern const char *VictronDevicesSetupfilename;
extern _sMyVictronDevices victronDevices;
// nb if victron or display settings are missing, '/save' will create them
extern bool LoadVictronConfiguration(int FS, const char *filename, _sMyVictronDevices &config);
extern void SaveVictronConfiguration(int FS, const char *filename, _sMyVictronDevices &config);

extern bool LoadDisplayConfiguration(int FS, const char *filename, _MyColors &set);
extern void SaveDisplayConfiguration(int FS, const char *filename, _MyColors &set);

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
  Serial.printf("*writeFile  Writing file: [%s]  [%s]\n", path, message);
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("*writeFile Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    //  Serial.println("File written");
  } else {
    Serial.println("*writeFile Write failed");
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
  if (SDexists("/edit/jquery.min.js")) {
    Serial.println("using local js");
    st = "<script src='/edit/jquery.min.js'></script>";
  } else {
    Serial.println("using Internet js");
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
  //Serial.println(" Sending local html version of Root webpage");
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
    Serial.println("MDNS responder started");
    Serial.print("You can now connect to http://");
    Serial.print(Display_Config.PanelName);
    Serial.println(".local");
    WifiGFXinterrupt(8, WifiStatus, "Connected:\n<%s>\n http://%s.local", WiFi.SSID(),Display_Config.PanelName);
  }else{Serial.println(" MDNS responder FAILED to start");WifiGFXinterrupt(8, WifiStatus, "Connected TO\n<%s>", WiFi.SSID());}
  }

void SetupWebstuff() {
 MDNS_START();
  //**************
  server.on("/", HTTP_GET, []() {
    Serial.println(" handling  root");
    WifiGFXinterrupt(8, WifiStatus, "Running Webserver");
    WebServerActive = true;
    handleRoot();
  });
  server.on("/Reset", HTTP_GET, []() {
    handleRoot();
    if (LoadConfiguration(1,Setupfilename, Display_Config, Current_Settings)) { EEPROM_WRITE(Display_Config, Current_Settings); }  // stops overwrite with bad JSON data!!
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
    if (LoadConfiguration(1,Setupfilename, Display_Config, Current_Settings)) {
      Serial.println("***Updating EEPROM from ");
      EEPROM_WRITE(Display_Config, Current_Settings);
    }  // stops overwrite with bad JSON data!!
    if (LoadVictronConfiguration(1,VictronDevicesSetupfilename, victronDevices)) {
      PrintJsonFile(" Check Updated Victron settings after Web initiated SAVE ", VictronDevicesSetupfilename);
      Serial.println("***Updated Victron data settings");
    }
    if (LoadDisplayConfiguration(1,ColorsFilename, ColorSettings)) {
      Serial.println(" USING JSON for Colours data settings");
    } else {
      Serial.println("\n\n***FAILED TO GET Colours JSON FILE****\n**** SAVING DEFAULT and Making File on SD****\n\n");
      SaveDisplayConfiguration(1,ColorsFilename, ColorSettings);  // should write a default file if it was missing?
    }
    delay(50);
    Display(true, Display_Page);
    delay(50);
  });

  server.on("/OTA", HTTP_GET, []() {
    WifiGFXinterrupt(8, WifiStatus, "Ready for OTA");
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex());
  });
  server.on("/ota", HTTP_GET, []() {
    WifiGFXinterrupt(8, WifiStatus, "Ready for OTA");
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex());
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
        Serial.printf("Update: %s\n", upload.filename.c_str());
        Serial.printf("   current size: %u   total size %u\n", upload.currentSize, upload.totalSize);
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
         Serial.printf("%dk ", next / 1024);
          next += chunk_size;
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {             //true to set the size to the current progress
        WifiGFXinterrupt(9, WifiStatus, "SW UPDATED");
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
          delay(500);
        } else {
          Update.printError(Serial);
        }
      }
    });


  //*********** END of OTA stuff *****************


  server.begin();
  Serial.println("HTTP server started");
}



// Append data to the SD card (DON'T MODIFY THIS FUNCTION)
void appendFile(fs::FS &fs, const char *path, const char *message) {
  // Serial.printf("Appending to file: %s\n", path);
  if (!hasSD){return;}
  SD_CS(LOW);
  File file = fs.open(path, FILE_APPEND);
  if (!file) {SD_CS(HIGH);
    // Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    //  Serial.println("Message appended");
  } else {
    //  Serial.println("Append failed");
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
  // Serial.printf("  ***** LOG FILE DEBUG ***  use: <%6i>  to make name..  ",int(BoatData.GPSDate));
  snprintf(InstLogFileName, 25, "/logs/%6i.log", int(BoatData.GPSDate));
  //  Serial.printf("  <%s> \n",InstLogFileName);
  SD_CS(LOW);
  File file = SD.open(InstLogFileName);
  SD_CS(HIGH);
  if (!file) {
    //Serial.println("File doesn't exist");
    INSTlogFileStarted = true;
    Serial.printf("Creating <%s> Instrument LOG file. and header..\n", InstLogFileName);
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
    Serial.println("Log File already exists.. appending");
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
    //Serial.println("File doens't exist");
    NMEAINSTlogFileStarted = true;
    Serial.printf("Creating NMEA LOG file. and header..\n");
    writeFile(SD, "/logs/NMEA.log", "NMEA data headings\r\nTime(s): Source:NMEA......\r\n");
    file.close();
    SD_CS(HIGH);
    return;
  } else {
    NMEAINSTlogFileStarted = true;
    Serial.println("NMEA log File already exists.. appending");
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
  // Serial.printf("  Logging to:<%s>", NMEALogFileName);
  // Serial.print("  Log  data: ");
  // Serial.println(msg);
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
  // Serial.printf("  Logging to:<%s>", InstLogFileName);
  // Serial.print("  Log  data: ");
  // Serial.println(msg);
  appendFile(SD, InstLogFileName, msg);
}




// void ShowFreeSpace() {
//   // Calculate free space (volume free clusters * blocks per clusters / 2)
//SD_CS(LOW);
//   long lFreeKB = SDvol()->freeClusterCount();
//   lFreeKB *= SDvol()->blocksPerCluster()/2;
//SD_CS(HIGH);
//   // Display free space
//   Serial.print("Free space: ");
//   Serial.print(lFreeKB);
//   Serial.println(" KB");
// }
// String EditorHTM(){ attempt to save this as code and npt as SD file..  do this once we get the DS working more reliably? 
//   String st;
//   st = "<!DOCTYPE html><html lang='-en'-><head><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>SD Editor</title><style type='-text/css'- media='-screen'->.contextMenu{z-index:300;position:absolute;left:5px;border:1px solid #444;background-color:#F5F5F5;display:none;box-shadow:0 0 10px rgb(0 0 0 / .4);font-size:12px;font-family:sans-serif;font-weight:700}.contextMenu ul{list-style:none;top:0;left:0;margin:0;padding:0}.contextMenu li{position:relative;min-width:60px;cursor:pointer}.contextMenu span{color:#444;display:inline-block;padding:6px}.contextMenu li:hover{background:#444}.contextMenu li:hover span{color:#EEE}.css-treeview ul,.css-treeview li{padding:0;margin:0;list-style:none}.css-treeview input{position:absolute;opacity:0}.css-treeview{font:normal 11px Verdana,Arial,Sans-serif;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;-moz-user-select:none;-webkit-user-select:none;user-select:none}.css-treeview span{color:blue;cursor:pointer}.css-treeview span:hover{text-decoration:underline}.css-treeview input+label+ul{margin:0 0 0 22px}.css-treeview input~ul{display:none}.css-treeview label,.css-treeview label::before{cursor:pointer}.css-treeview input:disabled+label{cursor:default;opacity:.6}.css-treeview input:checked:not(:disabled)~ul{display:block}.css-treeview label,.css-treeview label::before{background:url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAACgCAYAAAAFOewUAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAApxJREFUeNrslM1u00AQgGdthyalFFOK+ClIIKQKyqUVQvTEE3DmAhLwAhU8QZoH4A2Q2gMSFace4MCtJ8SPBFwAkRuiHKpA6sRN/Lu7zG5i14kctaUqRGhGXnu9O/Pt7MzsMiklvF+9t2kWTDvyIrAsA0aKRRi1T0C/hJ4LUbt5/8rNpWVlp8RSr9J40b48fxFaTQ9+ft8EZ6MJYb0Ok+dnYGpmPgXwKIAvLx8vYXc5GdMAQJgQEkpjRTh36TS2U+DWW/D17WuYgm8pwJyY1npZsZKOxImOV1I/h4+O6vEg5GCZBpgmA6hX8wHKUHDRBXQYicQ4rlc3Tf0VMs8DHBS864F2YFspjgUYjKX/Az3gsdQd2eeBHwmdGWXHcgBGSkZXOXohcEXebRoQcAgjqediNY+AVyu3Z3sAKqfKoGMsewBeEIOPgQxxPJIjcGH6qtL/0AdADzKGnuuD+2tLK7Q8DhHHbOBW+KEzcHLuYc82MkEUekLiwuvVH+guQBQzOG4XdAb8EOcRcqQvDkY2iCLuxECJ43JobMXoutqGgDa2T7UqLKwt9KRyuxKVByqVXXqIoCCUCAqhUOioTWC7G4TQEOD0APy2/7G2Xpu1J4+lxeQ4TXBbITDpoVelRN/BVFbwu5oMMJUBhoXy5tmdRcMwymP2OLQaLjx9/vnBo6V3K6izATmSnMa0Dq7ferIohJhr1p01zrlz49rZF4OMs8JkX23vVQzYp+wbYGV/KpXKjvspl8tsIKCrMNAYFxj2GKS5ZWxg4ewKsJfaGMIY5KXqPz8LBBj6+yDvVP79+yDp/9F9oIx3OisHWwe7Oal0HxCAAAQgAAEIQAACEIAABCAAAQhAAAIQgAAEIAABCEAAAhCAAAQgwD8E/BZgAP0qhKj3rXO7AAAAAElFTkSuQmCC) no-repeat}.css-treeview label,.css-treeview span,.css-treeview label::before{display:inline-block;height:16px;line-height:16px;vertical-align:middle}.css-treeview label{background-position:18px 0}.css-treeview label::before{content:'-'-;width:16px;margin:0 22px 0 0;vertical-align:middle;background-position:0 -32px}.css-treeview input:checked+label::before{background-position:0 -16px}@media screen and (-webkit-min-device-pixel-ratio:0){.css-treeview{-webkit-animation:webkit-adjacent-element-selector-bugfix infinite 1s}@-webkit-keyframes webkit-adjacent-element-selector-bugfix{from{padding:0}to{padding:0}}}@media (min-width:500px){#uploader{position:absolute;top:0;right:0;left:0;height:28px;line-height:24px;padding-left:10px;background-color:#444;color:#EEE}#filecmds{position:absolute;top:28px;right:0;left:0;height:28px;line-height:24px;padding-left:10px;background-color:#444;color:#EEE}#tree{position:absolute;top:56px;bottom:0;left:0;width:20%;padding:8px}#editor,#preview{position:absolute;top:56px;right:0;bottom:0;left:20%;width:80%;height:80vh}#preview{background-color:#EEE;padding:5px}}@media (max-width:500px){#uploader{position:absolute;top:0;right:0;left:20%;width:80%;height:66px;line-height:20px;padding-left:1px;padding-top:1px;background-color:#444;color:#EEE}#filecmds{position:absolute;top:0;right:0;left:0;width:20%;height:46px;line-height:20px;padding-left:0;padding-top:21px;background-color:#444;color:#EEE}#tree{position:absolute;top:12vh;bottom:0;left:0;width:25%;padding:8px}#editor,#preview{position:absolute;top:12vh;width:75%;right:0;bottom:0;left:25%;height:78vh}#preview{background-color:#EEE;padding:5px}}</style><script>function createFileUploader(element,tree,editor){var xmlHttp;var input=document.createElement('-input'-);input.type='-file'-;input.multiple=false;input.name='-data'-;document.getElementById(element).appendChild(input);var path=document.createElement('-input'-);path.id='-upload-path'-;path.type='-text'-;path.name='-path'-;path.defaultValue='-/'-;document.getElementById(element).appendChild(path);var button=document.createElement('-button'-);button.innerHTML='Upload';document.getElementById(element).appendChild(button);var mkdir=document.createElement('-button'-);mkdir.innerHTML='MkDir';document.getElementById(element).appendChild(mkdir);var mkfile=document.createElement('-button'-);mkfile.innerHTML='MkFile';document.getElementById(element).appendChild(mkfile);function httpPostProcessRequest(){if (xmlHttp.readyState==4){if(xmlHttp.status !=200) alert('-ERROR['-+xmlHttp.status+'-]: '-+xmlHttp.responseText);else{tree.refreshPath(path.value)}}}function createPath(p){xmlHttp=new XMLHttpRequest();xmlHttp.onreadystatechange=httpPostProcessRequest;var formData=new FormData();formData.append('-path'-,p);xmlHttp.open('-PUT'-,'-/edit'-);xmlHttp.send(formData)}mkfile.onclick=function(e){if(path.value.indexOf('-.'-)===-1) return;createPath(path.value);editor.loadurl(path.value)};mkdir.onclick=function(e){if(path.value.length < 2) return;var dir=path.value if(dir.indexOf('-.'-) !==-1){if(dir.lastIndexOf('-/'-)===0) return;dir=dir.substring(0,dir.lastIndexOf('-/'-))}createPath(dir)};button.onclick=function(e){if(input.files.length===0){return}xmlHttp=new XMLHttpRequest();xmlHttp.onreadystatechange=httpPostProcessRequest;var formData=new FormData();formData.append('-data'-,input.files[0],path.value);xmlHttp.open('-POST'-,'-/edit'-);xmlHttp.send(formData)}input.onchange=function(e){if(input.files.length===0) return;var filename=input.files[0].name;var ext=/(?:\.([^.]+))?$/.exec(filename)[1];var name=/(.*)\.[^.]+$/.exec(filename)[1];if(typeof name !==undefined){if(name.length>8) name=name.substring(0,8);filename=name}if(typeof ext !==undefined){if(ext==='-html'-) ext='-htm'-;else if(ext==='-jpeg'-) ext='-jpg'-;filename=filename+'-.'-+ext}if(path.value==='-/'- || path.value.lastIndexOf('-/'-)===0){path.value='-/'-+filename}else{path.value=path.value.substring(0,path.value.lastIndexOf('-/'-)+1)+filename}}}function createTree(element,editor){var preview=document.getElementById('-preview'-);var treeRoot=document.createElement('-div'-);treeRoot.className='-css-treeview'-;document.getElementById(element).appendChild(treeRoot);function loadDownload(path){document.getElementById('download-frame').src=path+'-?download=true'-}function loadPreview(path){document.getElementById('-editor'-).style.display='-none'-;preview.style.display='-block'-;preview.innerHTML='<img src='-'+path+''- style='-max-width:100%; max-height:100%; margin:auto; display:block;'- />'}function fillFolderMenu(el,path){var list=document.createElement('-ul'-);el.appendChild(list);var action=document.createElement('-li'-);list.appendChild(action);var isChecked=document.getElementById(path).checked;var expnd=document.createElement('-li'-);list.appendChild(expnd);if(isChecked){expnd.innerHTML='-<span>Collapse</span>'-;expnd.onclick=function(e){document.getElementById(path).checked=false;if(document.body.getElementsByClassName('contextMenu').length>0) document.body.removeChild(el)};var refrsh=document.createElement('-li'-);list.appendChild(refrsh);refrsh.innerHTML='-<span>Refresh</span>'-;refrsh.onclick=function(e){var leaf=document.getElementById(path).parentNode;if(leaf.childNodes.length==3) leaf.removeChild(leaf.childNodes[2]);httpGet(leaf,path);if(document.body.getElementsByClassName('contextMenu').length>0) document.body.removeChild(el)}}else{expnd.innerHTML='-<span>Expand</span>'-;expnd.onclick=function(e){document.getElementById(path).checked=true;var leaf=document.getElementById(path).parentNode;if(leaf.childNodes.length==3) leaf.removeChild(leaf.childNodes[2]);httpGet(leaf,path);if(document.body.getElementsByClassName('contextMenu').length>0) document.body.removeChild(el)}}var upload=document.createElement('-li'-);list.appendChild(upload);upload.innerHTML='-<span>Upload</span>'-;upload.onclick=function(e){var pathEl=document.getElementById('-upload-path'-);if(pathEl){var subPath=pathEl.value;if(subPath.lastIndexOf('-/'-) < 1) pathEl.value=path+subPath;else pathEl.value=path.substring(subPath.lastIndexOf('-/'-))+subPath}if(document.body.getElementsByClassName('contextMenu').length>0) document.body.removeChild(el)};var delFile=document.createElement('-li'-);list.appendChild(delFile);delFile.innerHTML='-<span>Delete</span>'-;delFile.onclick=function(e){httpDelete(path);if(document.body.getElementsByClassName('contextMenu').length>0) document.body.removeChild(el)}}function fillFileMenu(el,path){var list=document.createElement('-ul'-);el.appendChild(list);var action=document.createElement('-li'-);list.appendChild(action);if(isTextFile(path)){action.innerHTML='-<span>Edit</span>'-;action.onclick=function(e){editor.loadurl(path);if(document.body.getElementsByClassName('contextMenu').length>0) document.body.removeChild(el)}}else if(isImageFile(path)){action.innerHTML='-<span>Preview</span>'-;action.onclick=function(e){loadPreview(path);if(document.body.getElementsByClassName('contextMenu').length>0) document.body.removeChild(el)}}var download=document.createElement('-li'-);list.appendChild(download);download.innerHTML='-<span>Download</span>'-;download.onclick=function(e){// console.log('download 354');// console.log(path);// console.log(document.body.getElementsByClassName('contextMenu').length);loadDownload(path);if(document.body.getElementsByClassName('contextMenu').length>0) document.body.removeChild(el)};var delFile=document.createElement('-li'-);list.appendChild(delFile);delFile.innerHTML='-<span>Delete</span>'-;delFile.onclick=function(e){httpDelete(path);if(document.body.getElementsByClassName('contextMenu').length>0) document.body.removeChild(el)}}function showContextMenu(e,path,isfile){var divContext=document.createElement('-div'-);var scrollTop=document.body.scrollTop ? document.body.scrollTop :document.documentElement.scrollTop;var scrollLeft=document.body.scrollLeft ? document.body.scrollLeft :document.documentElement.scrollLeft;var left=e.clientX+scrollLeft;var top=e.clientY+scrollTop;divContext.className='contextMenu';divContext.style.display='block';divContext.style.left=left+'px';divContext.style.top=top+'px';if(isfile) fillFileMenu(divContext,path);else fillFolderMenu(divContext,path);document.body.appendChild(divContext);var width=divContext.offsetWidth;var height=divContext.offsetHeight;divContext.onmouseout=function(e){if(e.clientX < left || e.clientX>(left+width) || e.clientY < top || e.clientY>(top+height)){if(document.body.getElementsByClassName('contextMenu').length>0) document.body.removeChild(divContext)}}}function createTreeLeaf(path,name,size){var leaf=document.createElement('-li'-);leaf.id=name.toLowerCase();var label=document.createElement('-span'-);label.textContent=name.toLowerCase();leaf.appendChild(label);leaf.onclick=function(e){if(isTextFile(leaf.id)){editor.loadurl(leaf.id)}else if(isImageFile(leaf.id)){loadPreview(leaf.id)}};leaf.oncontextmenu=function(e){e.preventDefault();e.stopPropagation();showContextMenu(e,leaf.id,true)};return leaf}function createTreeBranch(path,name,disabled){var leaf=document.createElement('-li'-);var check=document.createElement('-input'-);check.type='-checkbox'-;check.id=name.toLowerCase();if(typeof disabled !=='-undefined'- && disabled) check.disabled='-disabled'-;leaf.appendChild(check);var label=document.createElement('-label'-);label.for=check.id;label.textContent=name.toLowerCase();leaf.appendChild(label);check.onchange=function(e){if(check.checked){if(leaf.childNodes.length==3) leaf.removeChild(leaf.childNodes[2]);httpGet(leaf,check.id)}};label.onclick=function(e){if(!check.checked){check.checked=true;if(leaf.childNodes.length==3) leaf.removeChild(leaf.childNodes[2]);httpGet(leaf,check.id)}else{check.checked=false}};leaf.oncontextmenu=function(e){e.preventDefault();e.stopPropagation();showContextMenu(e,check.id,false)}return leaf}function addList(parent,path,items){var list=document.createElement('-ul'-);parent.appendChild(list);var ll=items.length;for(var i=0;i < ll;i++){var item=items[i];var itemEl;if(item.type==='-file'-){itemEl=createTreeLeaf(path,item.name,item.size)}else{itemEl=createTreeBranch(path,item.name)}list.appendChild(itemEl)}}function isTextFile(path){var ext=/(?:\.([^.]+))?$/.exec(path)[1];if(typeof ext !==undefined){switch(ext){case '-csv'-:case '-log'-:case '-txt'-:case '-htm'-:case '-html'-:case '-js'-:case '-json'-:case '-c'-:case '-h'-:case '-cpp'-:case '-css'-:case '-xml'-:return true}}return false}function isImageFile(path){var ext=/(?:\.([^.]+))?$/.exec(path)[1];if(typeof ext !==undefined){switch(ext){case '-png'-:case '-jpg'-:case '-gif'-:case '-ico'-:return true}}return false}this.refreshPath=function(path){if(path.lastIndexOf('/') < 1){path='/';treeRoot.removeChild(treeRoot.childNodes[0]);httpGet(treeRoot,'-/'-)}else{path=path.substring(0,path.lastIndexOf('/'));var leaf=document.getElementById(path).parentNode;if(leaf.childNodes.length==3) leaf.removeChild(leaf.childNodes[2]);httpGet(leaf,path)}};function delCb(path){return function(){if (xmlHttp.readyState==4){if(xmlHttp.status !=200){alert('-ERROR['-+xmlHttp.status+'-]: '-+xmlHttp.responseText)}else{if(path.lastIndexOf('/') < 1){path='/';treeRoot.removeChild(treeRoot.childNodes[0]);httpGet(treeRoot,'-/'-)}else{path=path.substring(0,path.lastIndexOf('/'));var leaf=document.getElementById(path).parentNode;if(leaf.childNodes.length==3) leaf.removeChild(leaf.childNodes[2]);httpGet(leaf,path)}}}}}function httpDelete(filename){xmlHttp=new XMLHttpRequest();xmlHttp.onreadystatechange=delCb(filename);var formData=new FormData();formData.append('-path'-,filename);xmlHttp.open('-DELETE'-,'-/edit'-);xmlHttp.send(formData)}function getCb(parent,path){return function(){if (xmlHttp.readyState==4){//clear loading if(xmlHttp.status==200) addList(parent,path,JSON.parse(xmlHttp.responseText))}}}function httpGet(parent,path){xmlHttp=new XMLHttpRequest(parent,path);xmlHttp.onreadystatechange=getCb(parent,path);xmlHttp.open('-GET'-,'-/list?dir='-+path,true);xmlHttp.send(null);//start loading}httpGet(treeRoot,'-/'-);return this}function createEditor(element,file,lang,theme,type){function getLangFromFilename(filename){// console.log('at 565');// console.log(filename);var pathEl=document.getElementById('-upload-path'-);if(pathEl){if (filename !==null){pathEl.value=filename}};// console.log(' got to 569');var lang='-plain'-;var ext=/(?:\.([^.]+))?$/.exec(filename)[1];if(typeof ext !==undefined){switch(ext){case '-txt'-:lang='-plain'-;break;case '-htm'-:lang='-html'-;break;case '-js'-:lang='-javascript'-;break;case '-c'-:lang='-c_cpp'-;break;case '-cpp'-:lang='-c_cpp'-;break;case '-css'-:case '-scss'-:case '-php'-:case '-html'-:case '-json'-:case '-xml'-:lang=ext}}return lang}if(typeof file==='-undefined'-) file='-/index.htm'-;if(typeof lang==='-undefined'-){lang=getLangFromFilename(file)}if(typeof theme==='-undefined'-) theme='-textmate'-;if(typeof type==='-undefined'-){type='-text/'-+lang;if(lang==='-c_cpp'-) type='-text/plain'-}var xmlHttp=null;var editor=ace.edit(element);//post function httpPostProcessRequest(){if (xmlHttp.readyState==4){if(xmlHttp.status !=200) alert('-ERROR['-+xmlHttp.status+'-]: '-+xmlHttp.responseText)}}function httpPost(filename,data,type){xmlHttp=new XMLHttpRequest();xmlHttp.onreadystatechange=httpPostProcessRequest;var formData=new FormData();formData.append('-data'-,new Blob([data],{type:type}),filename);xmlHttp.open('-POST'-,'-/edit'-);xmlHttp.send(formData)}//get function httpGetProcessRequest(){if (xmlHttp.readyState==4){document.getElementById('-preview'-).style.display='-none'-;document.getElementById('-editor'-).style.display='-block'-;if(xmlHttp.status==200) editor.setValue(xmlHttp.responseText);else editor.setValue('-'-);editor.clearSelection()}}function httpGet(theUrl){xmlHttp=new XMLHttpRequest();xmlHttp.onreadystatechange=httpGetProcessRequest;xmlHttp.open('-GET'-,theUrl,true);xmlHttp.send(null)}if(lang !=='-plain'-) editor.getSession().setMode('-ace/mode/'-+lang);editor.setTheme('ace/theme/'+theme);editor.$blockScrolling=Infinity;editor.getSession().setUseSoftTabs(true);editor.getSession().setTabSize(2);editor.setHighlightActiveLine(true);editor.setShowPrintMargin(false);<!-- Save Button -->var saveButton=document.createElement('button');saveButton.innerHTML='SDsave';document.getElementById('filecmds').appendChild(saveButton);saveButton.onclick=function(e){httpPost(file,editor.getValue()+'',type)};<!-- Download Button -->//var downloadButton=document.createElement('button');//downloadButton.innerHTML='Download';//document.getElementById('filecmds').appendChild(downloadButton);//downloadButton.onclick=function(e){//console.log(' download variables 657 ');//var pathEl=document.getElementById('upload-path');//console.log(pathEl.value);// if(pathEl){if (pathEl.value !==null){//loadDownload(pathEl.value);// document.getElementById('download-frame').src=pathEl.value+'?download=true';//}};//loadDownload(path);//};<!-- Delete Button -->// var deleteButton=document.createElement('button');// deleteButton.innerHTML='Delete';// document.getElementById('filecmds').appendChild(deleteButton);// deleteButton.onclick=function(e){// console.log(' delete button 672 ');// var pathEl=document.getElementById('upload-path');// console.log(pathEl.value);// if(pathEl){if (pathEl.value !==null){// createTree.httpDelete(pathEl);// if(document.body.getElementsByClassName('contextMenu').length>0){document.body.removeChild(el)}//}}//};<!-- /Save Button-->editor.commands.addCommand({name:'saveCommand',bindKey:{win:'Ctrl-S',mac:'Command-S'},exec:function(editor){httpPost(file,editor.getValue()+'',type)},readOnly:false});editor.commands.addCommand({name:'undoCommand',bindKey:{win:'Ctrl-Z',mac:'Command-Z'},exec:function(editor){editor.getSession().getUndoManager().undo(false)},readOnly:false});editor.commands.addCommand({name:'redoCommand',bindKey:{win:'Ctrl-Shift-Z',mac:'Command-Shift-Z'},exec:function(editor){editor.getSession().getUndoManager().redo(false)},readOnly:false});httpGet(file);editor.loadUrl=function(filename){file=filename;lang=getLangFromFilename(file);type='text/'+lang;if(lang !=='plain') editor.getSession().setMode('ace/mode/'+lang);httpGet(file)}return editor}function onBodyLoad(){var vars={};var parts=window.location.href.replace(/[?&]+([^=&]+)=([^&]*)/gi,function(m,key,value){vars[key]=value});var editor=createEditor('editor',vars.file,vars.lang,vars.theme);var tree=createTree('tree',editor);createFileUploader('uploader',tree,editor)};</script><script>function loadScript(src,onLoad,onError){const script=document.createElement('script');script.src=src;script.onload=onLoad;script.onerror=onError;document.head.appendChild(script)}loadScript('https://cdnjs.cloudflare.com/ajax/libs/ace/1.1.9/ace.js',()=>{console.log('Online version loaded successfully')},()=>{console.warn('Online version failed, loading fallback');loadScript('/edit/ace.js',()=>{console.log('Fallback loaded')})});</script></head><body onload='onBodyLoad();'><div id='uploader'></div><div id='filecmds'></div><div id='tree'></div><div id='editor'></div><div id='preview' style='display:none;'></div>"
//   "<iframe id=download-frame style='display:none;'></iframe></body></html>";
//   return st;
// }


#endif