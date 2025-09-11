
//********* for SERIAL on Wavshare.. MUST SET ******************
//Tools-> CDC on boot ENABLED

// not for s3 versions!! #include <NMEA2000_CAN.h>  // note Should automatically detects use of ESP32 and  use the (https://github.com/ttlappalainen/NMEA2000_esp32) library
///----  // see https://github.com/ttlappalainen/NMEA2000/issues/416#issuecomment-2251908112
#define ESP32_CAN_TX_PIN GPIO_NUM_6  // for the waveshare module boards!
#define ESP32_CAN_RX_PIN GPIO_NUM_0  // for the waveshare module boards!
// #define ESP32_CAN_TX_PIN GPIO_NUM_1  // for the esp32_4 spare pins on 8 way connector boards!
// #define ESP32_CAN_RX_PIN GPIO_NUM_2  // for the esp32_4 spare pins on 8 way connector boards!

#include "N2kMsg.h"
#include "NMEA2000.h"
#include <NMEA2000_esp32xx.h>
#include <N2kMessages.h>
tNMEA2000& NMEA2000 = *(new tNMEA2000_esp32xx());



//Include libraries..
#include <WiFi.h>
#include <WiFiUdp.h>

#include <esp_now.h>
#include <esp_wifi.h>
#include <Arduino_GFX_Library.h>  // aka 'by Moon on our Nation'
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <SD.h>       // was SD.h  // pins set in 4inch.h
#include <PCA9554.h>  // Load the PCA9554 Library
#include <Wire.h>     // Load the Wire Library
#include <TAMC_GT911.h>

// some wifi stuff
//NB these may not be used -- I have tried to do some simplification
IPAddress udp_ap(0, 0, 0, 0);              // the IP address to send UDP data in SoftAP mode
IPAddress udp_st(0, 0, 0, 0);              // the IP address to send UDP data in Station mode
IPAddress sta_ip(0, 0, 0, 0);              // the IP address (or received from DHCP if 0.0.0.0) in Station mode
IPAddress gateway(0, 0, 0, 0);             // the IP address for Gateway in Station mode
IPAddress subnet(0, 0, 0, 0);              // the SubNet Mask in Station mode
IPAddress Null_ip(0, 0, 0, 0);             //  A null IP address for the gateway
IPAddress ap_ip(192, 168, 4, 1);           // the IP address in AP mode. Default and can be changed!
const IPAddress sub255(255, 255, 255, 0);  // the default Subnet Mask in in SoftAP mode

boolean IsConnected = false;  // may be used in AP_AND_STA to flag connection success (got IP)
boolean AttemptingConnect;    // to note that WIFI.begin has been started
int NetworksFound;            // used for scan Networks result. Done at start up!
WiFiUDP Udp;
#define BufferLength 500
char nmea_1[BufferLength];    //serial
char nmea_U[BufferLength];    // NMEA buffer for UDP input port
char nmea_EXT[BufferLength];  // buffer for ESP_now received data
// some helpful defines for easier coding

//MAGIC TREK style File viewer see https://github.com/holgerlembke/

#include <ESPFMfGK.h>  // the thing.
const word filemanagerport = 8080;
// we want a different port than the webserver
ESPFMfGK filemgr(filemanagerport);

#include <FFat.h>  // plan to use FATFS for local files



// my sub files
#include "aux_functions.h"  //.cpp calls  #include "WAV_4inch_pins.h"

#include "Display.h"
//*********** for keyboard*************
#include "Keyboard.h"
//*********** DISPLAY selector *************
#include "WAV_4inch.h"


//TAMC_GT911 ts = TAMC_GT911(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, TOUCH_WIDTH, TOUCH_HEIGHT);



const char soft_version[] = "VERSION W.1";

//********** All boat data (instrument readings) are stored as double in a single structure:
_sBoatData BoatData;  // BoatData values, int double etc
bool dataUpdated;     // flag that Nmea Data has been updated
// _sWiFi_settings_Config (see structures.h) are the settings for the Display:
// If the structure is changed, be sure to change the Key (first figure) so that new defaults and struct can be set.
//
/*int EpromKEY;  char ssid[25];  char password[25];  char UDP_PORT[5]; UDP_ON;Serial_on; ESP_NOW_ON;  N2K_ON;  Log_ON;
  int log_interval_setting;   bool NMEA_log_ON;  bool BLE_enable;*/
//_sWiFi_settings_Config Default_Settings = { 17, "GUESTBOAT", "12345678", "2002", false, false, true, true, false, 1000, false, false };
_sDisplay_Config Default_JSON = { "0.5", 4, 0, "nmeadisplay", "12345678", "SOG", "DEPTH", "WIND", "STW" };  // many display stuff set default
_sDisplay_Config Display_Config;
_sWiFi_settings_Config Saved_Settings;
_sWiFi_settings_Config Current_Settings;
_sWiFi_settings_Config Default_Settings_JSON = { 17, "GUESTBOAT", "12345678", "2002", false, false, true, true, false, 1000, false, false };

const char* Setupfilename = "/config.txt";  // <- SD library uses 8.3 filenames
int MasterFont;                             //global for font! Idea is to use to reset font after 'temporary' seletion of another
String Fontname;
int text_height = 12;  //so we can get them if we change heights etc inside functions
int text_offset = 12;  //offset is not equal to height, as subscripts print lower than 'height'
int text_char_width = 12;
int Display_Page;

bool EspNowIsRunning = false;
char* pTOKEN;
int StationsConnectedtomyAP;

bool hasSD, hasFATS;

#define scansearchinterval 30000

// assists for wifigfx interrupt  box that shows status..  to help turn it off after a time
uint32_t WIFIGFXBoxstartedTime;
bool WIFIGFXBoxdisplaystarted;


//****  My displays are based on '_sButton' structures to define position, width height, borders and colours.
// int h, v, width, height, bordersize;  uint16_t BackColor, TextColor, BorderColor;

_sButton FullSize = { 10, 0, 460, 460, 0, BLUE, WHITE, BLACK };
_sButton FullSizeShadow = { 5, 10, 460, 460, 0, BLUE, WHITE, BLACK };
_sButton CurrentSettingsBox = { 0, 0, 480, 80, 2, BLUE, WHITE, BLACK };  //also used for showing the current settings

_sButton FontBox = { 0, 80, 480, 330, 5, BLUE, WHITE, BLUE };

//_sButton WindDisplay = { 0, 0, 480, 480, 0, BLUE, WHITE, BLACK };  // full screen no border

//used for single data display
// modified all to lift by 30 pixels to allow a common bottom row display (to show logs and get to settings)
_sButton StatusBox = { 0, 460, 480, 20, 0, BLACK, WHITE, BLACK };
_sButton WifiStatus = { 60, 180, 360, 120, 5, BLUE, WHITE, BLACK };  // big central box for wifi events to pop up - v3.5

_sButton BigSingleDisplay = { 0, 90, 480, 360, 5, BLUE, WHITE, BLACK };              // used for wind and graph displays
_sButton BigSingleTopRight = { 240, 0, 240, 90, 5, BLUE, WHITE, BLACK };             //  ''
_sButton BigSingleTopLeft = { 0, 0, 240, 90, 5, BLUE, WHITE, BLACK };                //  ''
_sButton TopHalfBigSingleTopRight = { 240, 0, 240, 45, 5, BLUE, WHITE, BLACK };      //  ''
_sButton BottomHalfBigSingleTopRight = { 240, 45, 240, 45, 5, BLUE, WHITE, BLACK };  //  ''
//used for nmea RMC /GPS display // was only three lines to start!
_sButton Threelines0 = { 20, 30, 440, 80, 5, BLUE, WHITE, BLACK };
_sButton Threelines1 = { 20, 130, 440, 80, 5, BLUE, WHITE, BLACK };
_sButton Threelines2 = { 20, 230, 440, 80, 5, BLUE, WHITE, BLACK };
_sButton Threelines3 = { 20, 330, 440, 80, 5, BLUE, WHITE, BLACK };
// for the quarter screens on the main page
_sButton topLeftquarter = { 0, 0, 240, 240 - 15, 5, BLUE, WHITE, BLACK };  //h  reduced by 15 to give 30 space at the bottom
_sButton bottomLeftquarter = { 0, 240 - 15, 240, 240 - 15, 5, BLUE, WHITE, BLACK };
_sButton topRightquarter = { 240, 0, 240, 240 - 15, 5, BLUE, WHITE, BLACK };
_sButton bottomRightquarter = { 240, 240 - 15, 240, 240 - 15, 5, BLUE, WHITE, BLACK };



// these were used for initial tests and for volume control - not needed for most people!! .. only used now for Range change in GPS graphic (?)
_sButton TopLeftbutton = { 0, 0, 75, 45, 5, BLUE, WHITE, BLACK };
_sButton TopRightbutton = { 405, 0, 75, 45, 5, BLUE, WHITE, BLACK };
_sButton BottomRightbutton = { 405, 405, 75, 45, 5, BLUE, WHITE, BLACK };
_sButton BottomLeftbutton = { 0, 405, 75, 45, 5, BLUE, WHITE, BLACK };

// buttons for the wifi/settings pages
_sButton TOPButton = { 20, 10, 430, 35, 5, WHITE, BLACK, BLUE };
_sButton SecondRowButton = { 20, 60, 430, 35, 5, WHITE, BLACK, BLUE };
_sButton ThirdRowButton = { 20, 100, 430, 35, 5, WHITE, BLACK, BLUE };
_sButton FourthRowButton = { 20, 140, 430, 35, 5, WHITE, BLACK, BLUE };
_sButton FifthRowButton = { 20, 180, 430, 35, 5, WHITE, BLACK, BLUE };

#define sw_width 55
//switches at line 180
_sButton Switch1 = { 20, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };
_sButton Switch2 = { 100, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };
_sButton Switch3 = { 180, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };
_sButton Switch5 = { 260, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };
_sButton Switch4 = { 345, 180, 120, 35, 5, WHITE, BLACK, BLUE };  // big one for eeprom update
//switches at line 60
_sButton Switch6 = { 20, 60, sw_width, 35, 5, WHITE, BLACK, BLACK };
_sButton Switch7 = { 100, 60, sw_width, 35, 5, WHITE, BLACK, BLACK };
_sButton Switch8 = { 180, 60, sw_width, 35, 5, WHITE, BLACK, BLACK };
_sButton Switch9 = { 260, 60, sw_width, 35, 5, WHITE, BLACK, BLACK };
_sButton Switch10 = { 340, 60, sw_width, 35, 5, WHITE, BLACK, BLACK };
_sButton Switch11 = { 420, 60, sw_width, 35, 5, WHITE, BLACK, BLACK };


_sButton Terminal = { 0, 100, 480, 330, 2, WHITE, BLACK, BLUE };
//for selections
_sButton FullTopCenter = { 80, 0, 320, 50, 5, BLUE, WHITE, BLACK };

_sButton Full0Center = { 80, 55, 320, 50, 5, BLUE, WHITE, BLACK };
_sButton Full1Center = { 80, 110, 320, 50, 5, BLUE, WHITE, BLACK };
_sButton Full2Center = { 80, 165, 320, 50, 5, BLUE, WHITE, BLACK };
_sButton Full3Center = { 80, 220, 320, 50, 5, BLUE, WHITE, BLACK };
_sButton Full4Center = { 80, 275, 320, 50, 5, BLUE, WHITE, BLACK };
_sButton Full5Center = { 80, 330, 320, 50, 5, BLUE, WHITE, BLACK };
_sButton Full6Center = { 80, 385, 320, 50, 5, BLUE, WHITE, BLACK };  // inteferes with settings box do not use!



#include <PCA9554.h>     // Load the PCA9554 Library
PCA9554 expander(0x20);  // Create an expander object at this expander address


void setup() {

  Setup_expander(ExpanderSCL, ExpanderSDA, EX106);
  Serial.begin(115200);
 
  InitNMEA2000();
  Serial.begin(115200);
  Serial.println("Starting NMEA Display ");
  Serial.println(soft_version);
  WiFi.softAPConfig(ap_ip, Null_ip, sub255);
  ConnectWiFiusingCurrentSettings();
  Init_GFX();
  Fatfs_Setup();  // set up FATFS
 // SD_Setup(SD_SCK,SD_MISO,SD_MOSI, SDCS);     // set up SD card (for logs etc)
  if (LoadConfiguration(1,Setupfilename, Display_Config, Current_Settings)) {
    Serial.println(" USING FATS JSON for wifi and display settings");
    Display_Page= Display_Config.Start_Page;
    } else 
    {
      Display_Page = 4;  //set later by eeprom read etc  but here for clarity?
      Display_Config = Default_JSON;
      Current_Settings = Default_Settings_JSON;
    Serial.println(" USING EEPROM data, display set to defaults");
   }

  Display(true, Display_Page);
  // once wifi working..
  setupFilemanager();
}

void loop() {
  static unsigned long LogInterval;
  static unsigned long DebugInterval;
  static unsigned long SSIDSearchInterval;
  NMEA2000.ParseMessages();
  Display(Display_Page);

  if (!AttemptingConnect && !IsConnected && (millis() >= SSIDSearchInterval)) {  // repeat at intervals to check..
    SSIDSearchInterval = millis() + scansearchinterval;                          //
    if (StationsConnectedtomyAP == 0) {                                          // avoid scanning if we have someone connected to AP as it will/may disconnect!
      ScanAndConnect(true);
    }  // ScanAndConnect will set AttemptingConnect And do a Wifi.begin if the required SSID has appeared
  }
  // switch off WIFIGFXBox after timed interval
  if (WIFIGFXBoxdisplaystarted && (millis() >= WIFIGFXBoxstartedTime + 10000) && (!AttemptingConnect)) {
    WIFIGFXBoxdisplaystarted = false;
 // for now - see OTA    WebServerActive = false;
    Display(true, Display_Page);
    delay(50);  // change page back, having set zero above which alows the graphics to reset up the boxes etc.
  }


  yield();

  //  SD_CS("LOW");
  filemgr.handleClient();  // trek style file manager with  SD CS low for any sd work
  //  server.handleClient();  // for OTA webserver etc. need to turn on SD .. . ;
  //  SD_CS("HIGH");
}

void wifiSetup() {
  WiFi.softAPConfig(ap_ip, Null_ip, sub255);
  ConnectWiFiusingCurrentSettings();
  //SetupWebstuff();

  keyboard(-1);  //reset keyboard display update settings
  Udp.begin(atoi(Current_Settings.UDP_PORT));
}

// Can place functions after Setup and Loop only in ino

void Init_GFX() {
  Serial.println("GFX_BL setting");
  #ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
  #endif
  // Init Display
  gfx->begin();
  //if GFX> 1.3.1 try and do this as the invert colours write 21h or 20h to 0Dh has been lost from the structure!
  gfx->invertDisplay(false);
  gfx->fillScreen(BLUE);
  gfx->setTextBound(0, 0, 480, 480);
  gfx->setTextColor(WHITE);
  setFont(4);
  gfx->setCursor(40, 20);
  gfx->println(F("***Display Started***"));
}

void InitNMEA2000() {  // make it display device Info on start up..
  NMEA2000.SetN2kCANMsgBufSize(8);
  NMEA2000.SetN2kCANReceiveFrameBufSize(100);
  // Set device information
  char SnoStr[33];
  uint32_t SerialNumber;
  SerialNumber = 9999;
  snprintf(SnoStr, 32, "%lu", SerialNumber);
  Serial.println("NMEA2000 Initialization ...");
  Serial.printf("   Unique ID: <%i> \r\n", SerialNumber);
  NMEA2000.SetDeviceInformation(SerialNumber,  // Unique number. Use e.g. Serial number.
                                130,           // Device function=Display. See codes on https://web.archive.org/web/20190531120557/https://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                120,           // Device class=Display Device.
                                2046,          // Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                                4              //marine
  );
  NMEA2000.SetProductInformation(SnoStr,                   // N2kVersion
                                 001,                      // Manufacturer's product code
                                 "Simple NMEA Display",    // Manufacturer's Model ID
                                 "TEST",                   //N2kSwCode
                                 "Guitron ESP32s 4 inch",  // N2kModelVersion
                                 3                         //,                            // LoadEquivalency (of 50mA loads)
                                                           //2102,                           // N2kversion default 2102
                                                           //0                           // CertificationLevel
  );

  NMEA2000.EnableForward(false);                        // we are not  forwarding / streaming anything
  NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode, 15);  // needs this to enable device information send at start up?
  NMEA2000.SetMsgHandler(HandleNMEA2000Msg);            // see main ino)
  NMEA2000.Open();
}

 //******* Define a handler for the interrupt to work *******
 #include "N2KDataRX.h"  // where the handler functions actually are !!

typedef struct {
  unsigned long PGN;
  void (*Handler)(const tN2kMsg& N2kMsg);
} tNMEA2000Handler;

//  This selects which function the handler will call, depending on PGN  (actual functions are in N2kDataRx files)
tNMEA2000Handler NMEA2000Handlers[] = {
  { 129029l, &HandleGNSS },
  { 126992L, &HandleGNSSSystemTime },
  { 128259L, &HandleBoatSpeed },
  { 130306L, &HandleWind },
  { 128267L, &HandleDepth },
  { 129026L, &HandleCOGSOG },
  { 129025L, &HandlePosition },
  // {126996L, &HandleMFRData},
  // {60928L,  &HandleMFRData},
  { 0, 0 }
};

void HandleNMEA2000Msg(const tN2kMsg& N2kMsg) {  // simplified version from data display
  int iHandler;                                  // enumerate handlers - how many do we have?
  bool known;
  known = false;
  for (iHandler = 0; NMEA2000Handlers[iHandler].PGN != 0 && !(N2kMsg.PGN == NMEA2000Handlers[iHandler].PGN); iHandler++)
    ;
  // we now have the index (iHandler) for the handler matching the received PGN
  if (NMEA2000Handlers[iHandler].PGN != 0) {
    NMEA2000Handlers[iHandler].Handler(N2kMsg);
    known = true;
  }
   if ((Display_Page == -21 ) ||(Display_Page == -21 )){ // only do this terminal debug display if on the N2K viewing debug page! 
   char decode[40];
    PGNDecode(N2kMsg.PGN).toCharArray(decode,35); // get the discription of the PGN from my string function, trucated to 35 char
    if(known) {UpdateLinef(BLACK, 8, Terminal, "N2K:(%i)[%.2X%.5X] %s",N2kMsg.PGN,N2kMsg.Source, N2kMsg.PGN, decode);}
    else{UpdateLinef(52685, 8, Terminal, "N2K:(%i)[%.2X%.5X] %s",N2kMsg.PGN,N2kMsg.Source, N2kMsg.PGN, decode);}
    //52685 is light gray in RBG565 light gray for pgns we do not decode. (based on handler setup)
  }
  // Serial.print(" N2K :Pgn");
  // Serial.println(N2kMsg.PGN);
}



void Fatfs_Setup() {
  hasFATS = false;
  Serial.println("FFAT  START");
  if (FFat.begin(true)) {
    hasFATS = true;
    gfx->print("FFAT initiated  ");
    /*size_t totalBytes();
    size_t usedBytes();
    size_t freeBytes();*/
    uint64_t cardSize = FFat.totalBytes() / (1024 * 1024);
    Serial.printf("  FFat Size: %lluMB\n", cardSize);
    gfx->printf("FFat  Size: %lluMB ", cardSize);
    uint64_t FreeSize = FFat.freeBytes() / (1024 * 1024);
    Serial.printf("FFat freeBytes: %lluMB\n", FreeSize);
    gfx->printf("FFat  freeBytes: %lluMB\n", FreeSize);
    delay(1500);
    if (!filemgr.AddFS(FFat, "Flash/FFat", false)) {
      Serial.println(F("Adding FFAT to Filemanager failed."));
    } else {
      Serial.println(F("  Adding FFAT to Filemanager"));
    }
  } else {
    Serial.println(F("FFat File System not initiated."));
  }
}
//*********** EEPROM functions *********
void EEPROM_WRITE(_sDisplay_Config B, _sWiFi_settings_Config A) {
  // save my current settings
  // ALWAYS Write the Default display page!  may change this later and save separately?!!
  Serial.printf("SAVING EEPROM\n key:%i \n", A.EpromKEY);
  EEPROM.put(1, A.EpromKEY);  // separate and duplicated so it can be checked by itsself first in case structures change
  EEPROM.put(10, A);
  EEPROM.commit();
  delay(50);
  //NEW also save as a JSON on the SD card SD card will overwrite current settings on setup..
  SaveConfiguration(1, Setupfilename, B, A);
  // SaveVictronConfiguration(VictronDevicesSetupfilename,victronDevices); // should write a default file if it was missing?
}
void EEPROM_READ() {
  int key;
  EEPROM.begin(512);
  Serial.print("READING EEPROM ");
  gfx->println(" EEPROM READING ");
  EEPROM.get(1, key);
  // Serial.printf(" read %i  default %i \n", key, Default_Settings.EpromKEY);
  if (key == Default_Settings_JSON.EpromKEY) {
    EEPROM.get(10, Saved_Settings);
    Serial.println("- Key OK");
    gfx->println(" Key OK");
  } else {
    Saved_Settings = Default_Settings_JSON;
    gfx->println("Using DEFAULTS");
    Serial.println("Using DEFAULTS");
    EEPROM_WRITE(Default_JSON, Default_Settings_JSON);
  }
}

bool LoadConfiguration(int FS,const char* filename, _sDisplay_Config& config, _sWiFi_settings_Config& settings) {
  // Open SD file for reading  //Active filesystems are: 0: SD-Card 1: Flash/FFat .
  // Allocate a temporary JsonDocument
  char temp[15];
  JsonDocument doc;
  File file;
  if(FS==0){ SD_CS(LOW);
    if (!SDexists(filename)) { Serial.printf("**JSON file %s did not exist on FATS\n Using defaults\n", filename);
        SD_CS(HIGH); return false;
     }
    file = SD.open(filename, FILE_READ);
    if (!file) {
     Serial.println(F("**Failed to read JSON file"));
     SD_CS(HIGH);
      return false;
    }
      
    }
  if(FS==1){
    if (!FFat.exists(filename)) {
     Serial.printf("**JSON file %s did not exist on FATS\n Using defaults\n", filename);
     return false;
    }
  file = FFat.open(filename, FILE_READ);
    if (!file) {
     Serial.println(F("**Failed to read JSON file"));
     return false;
    }
  }
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println(F("**Failed to deserialise JSON file"));
    return false;
    }


  // Copy values (or defaults) from the JsonDocument to the config / settings
  if (doc["Start_Page"] == 0) { return false; }  //secondary backup in case the file is present (passes error) but zeroed!
  config.LocalTimeOffset = doc["LocalTimeOffset"] | 0;
  config.Start_Page = doc["Start_Page"] | 4;  // 4 is default page int - no problem...
  strlcpy(temp, doc["Mag_Var"] | "1.15", sizeof(temp));
  BoatData.Variation = atof(temp);  //  (+ = easterly) Positive: If the magnetic north is to the east of true north, the declination is positive (or easterly).

  strlcpy(config.FourWayBR, doc["FourWayBR"] | "SOG", sizeof(config.FourWayBR));
  strlcpy(config.FourWayBL, doc["FourWayBL"] | "DEPTH", sizeof(config.FourWayBL));
  strlcpy(config.FourWayTR, doc["FourWayTR"] | "WIND", sizeof(config.FourWayTR));
  strlcpy(config.FourWayTL, doc["FourWayTL"] | "STW", sizeof(config.FourWayTL));
  strlcpy(config.PanelName,                  // User selectable
          doc["PanelName"] | "NMEADISPLAY",  // <- and default in case Json is corrupt / missing !
          sizeof(config.PanelName));
  strlcpy(config.APpassword,               // User selectable
          doc["APpassword"] | "12345678",  // <- and default in case Json is corrupt / missing !
          sizeof(config.APpassword));

  // only change settings if we have read the file! else we will use the EEPROM settings
  if (!error) {
    strlcpy(settings.ssid,                 // <- destination
            doc["ssid"] | "GuestBoat",     // <- source and default in case Json is corrupt!
            sizeof(settings.ssid));        // <- destination's capacity
    strlcpy(settings.password,             // <- destination
            doc["password"] | "12345678",  // <- source and default
            sizeof(settings.password));    // <- destination's capacity
    strlcpy(settings.UDP_PORT,             // <- destination
            doc["UDP_PORT"] | "2003",      // <- source and default.
            sizeof(settings.UDP_PORT));    // <- destination's capacity

    strlcpy(temp, doc["Serial"] | "false", sizeof(temp));
    settings.Serial_on = (strcmp(temp, "false"));

    strlcpy(temp, doc["UDP"] | "true", sizeof(temp));
    settings.UDP_ON = (strcmp(temp, "false"));
    strlcpy(temp, doc["ESP"] | "false", sizeof(temp));
    settings.ESP_NOW_ON = (strcmp(temp, "false"));
    strlcpy(temp, doc["N2K"] | "false", sizeof(temp));
    settings.N2K_ON = (strcmp(temp, "false"));
    strlcpy(temp, doc["LOG"] | "false", sizeof(temp));
    settings.Log_ON = (strcmp(temp, "false"));
    strlcpy(temp, doc["NMEALOG"] | "false", sizeof(temp));
    settings.NMEA_log_ON = (strcmp(temp, "false"));
    settings.log_interval_setting = doc["LogInterval"] | 60;
  }

  strlcpy(temp, doc["BLE_enable"] | "false", sizeof(temp));
  settings.BLE_enable = (strcmp(temp, "false"));


  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();
  if(FS==0){SD_CS(HIGH);}
  if (!error) { return true; }
  return false;
}
void SaveConfiguration(int FS, const char* filename, _sDisplay_Config& config, _sWiFi_settings_Config& settings) {
  // Delete existing file, otherwise the configuration is appended to the file
  //Active filesystems are: 0: SD-Card 1: Flash/FFat .
  char buff[15];
  File file;
  if (FS == 0) {
    if (!hasSD) {
      SD_CS(HIGH);
      return;
    }
    SD_CS(LOW);
    SD.remove(filename);
    // Open file for writing
    file = SD.open(filename, FILE_WRITE);
  }
  if (FS == 1) {
    if (!hasFATS) { return; }
    FFat.remove(filename);
    // Open file for writing
    file = FFat.open(filename, FILE_WRITE);
  }
  if (!file) {
    Serial.println(F("JSON: Failed to create SD file"));
    if (FS == 0) { SD_CS(HIGH); }
    return;
  }
  // Allocate a temporary JsonDocument
  JsonDocument doc;
  // Set the values in the JSON file.. // NOT ALL ARE read yet!!
  //modify how the display works
  doc["Start_Page"] = config.Start_Page;
  doc["LocalTimeOffset"] = config.LocalTimeOffset;
  Serial.print("save magvar:");
  Serial.printf("%5.3f", BoatData.Variation);
  snprintf(buff, sizeof(buff), "%5.3f", BoatData.Variation);
  doc["Mag_Var"] = buff;
  doc["PanelName"] = config.PanelName;
  doc["APpassword"] = config.APpassword;



  //now the settings WIFI etc..
  doc["ssid"] = settings.ssid;
  doc["password"] = settings.password;
  doc["UDP_PORT"] = settings.UDP_PORT;
  doc["Serial"] = settings.Serial_on True_False;
  doc["UDP"] = settings.UDP_ON True_False;
  doc["ESP"] = settings.ESP_NOW_ON True_False;
  doc["N2K"] = settings.N2K_ON True_False;
  doc["LogComments0"] = "LOG saves read data in file with date as name- BUT only when GPS date has been seen!";
  doc["LogComments1"] = "NMEALOG saves every message. Use NMEALOG only for debugging!";
  doc["LogComments2"] = "or the NMEALOG files will become huge";
  doc["LOG"] = settings.Log_ON True_False;
  doc["LogInterval"] = settings.log_interval_setting;
  doc["NMEALOG"] = settings.NMEA_log_ON True_False;
  doc["DisplayComment1"] = "These settings allow modification of the bottom two 'quad' displays";
  doc["DisplayComment2"] = "options are : SOG SOGGRAPH STW STWGRAPH GPS DEPTH DGRAPH DGRAPH2 ";
  doc["DisplayComment3"] = "DGRAPH  and DGRAPH2 display 10 and 30 m ranges respectively";
  doc["FourWayBR"] = config.FourWayBR;
  doc["FourWayBL"] = config.FourWayBL;
  doc["DisplayComment4"] = "Top row right can be WIND or TIME (UTC) or TIMEL (LOCAL)";
  doc["FourWayTR"] = config.FourWayTR;
  doc["FourWayTL"] = config.FourWayTL;

  doc["_comment_"] = "These settings below apply only to Victron display pages";
  doc["BLE_enable"] = settings.BLE_enable True_False;



  // Serialize JSON to file
  if (serializeJsonPretty(doc, file) == 0) {  // use 'pretty format' with line feeds
    Serial.println(F("JSON: Failed to write to SD file"));
  }
  // Close the file, but print serial as a check
  file.close();
  if (FS == 0) { SD_CS(HIGH); }
  PrintJsonFile("Check after Saving configuration ", filename);
}

void PrintJsonFile(const char* comment, const char* filename) {
  // Open file for reading
  SD_CS(LOW);
  File file = SD.open(filename, FILE_READ);
  Serial.printf(" %s JSON FILE %s is.", comment, filename);
  if (!file) {
    Serial.println(F("Failed to read file"));
    SD_CS(HIGH);
    return;
  }
  Serial.println();
  // Extract each characters by one by one
  while (file.available()) {
    Serial.print((char)file.read());
  }
  Serial.println();
  // Close the file
  file.close();
  SD_CS(HIGH);
}

void ShowToplinesettings(_sWiFi_settings_Config A, String Text) {
  // int local;
  // local = MasterFont;
  // SETS MasterFont, so cannot use MasterFont directly in last line and have to save it!
  long rssiValue = WiFi.RSSI();
  gfx->setTextSize(1);
  gfx->setTextColor(CurrentSettingsBox.TextColor);
  CurrentSettingsBox.PrintLine = 0;
  // 7 is smallest Bold Font
  UpdateLinef(7, CurrentSettingsBox, "%s:SSID<%s>PWD<%s>UDPPORT<%s>", Text, A.ssid, A.password, A.UDP_PORT);
  UpdateLinef(7, CurrentSettingsBox, "IP:%i.%i.%i.%i  RSSI %i", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3], rssiValue);
  UpdateLinef(7, CurrentSettingsBox, "Ser<%s>UDP<%s>ESP<%s>Log<%s>NMEA<%s>", A.Serial_on On_Off, A.UDP_ON On_Off, A.ESP_NOW_ON On_Off, A.Log_ON On_Off, A.NMEA_log_ON On_Off);
  // UpdateLinef(7,CurrentSettingsBox, "Logger settings Log<%s>NMEA<%s>",A.Serial_on On_Off, A.UDP_ON On_Off, A.ESP_NOW_ON On_Off,A.NMEA_log_ON On_Off);
}
void ShowToplinesettings(String Text) {
  ShowToplinesettings(Current_Settings, Text);
}
boolean CompStruct(_sWiFi_settings_Config A, _sWiFi_settings_Config B) {  // Does NOT compare the display page number or key!
  bool same = true;
  // have to check each variable individually
  //if (A.EpromKEY == B.EpromKEY) { same = true; }

  if (A.UDP_ON != B.UDP_ON) { same = false; }
  if (A.ESP_NOW_ON != B.ESP_NOW_ON) { same = false; }
  if (A.N2K_ON != B.N2K_ON) { same = false; }
  if (A.Serial_on != B.Serial_on) { same = false; }
  if (A.Log_ON != B.Log_ON) { same = false; }
  if (A.NMEA_log_ON != B.NMEA_log_ON) { same = false; }

  //Serial.print(" DEBUG ");Serial.print(A.ssid); Serial.print(" and ");Serial.println(B.ssid);
  // these are char strings, so need strcmp to compare ...if strcmp==0 they are equal
  if (strcmp(A.UDP_PORT, B.UDP_PORT) != 0) { same = false; }
  if (strcmp(A.ssid, B.ssid) != 0) { same = false; }
  if (strcmp(A.password, B.password) != 0) { same = false; }

  //Serial.print("Result same = ");Serial.println(same);
  return same;
}
//****************  GRAPHICS etc STUFF ************************
char* LattoString(double data) {
  static char buff[25];
  double pos;
  pos = data;
  int degrees = int(pos);
  float minutes = (pos - degrees) * 60;
  bool direction;
  direction = (pos >= 0);
  snprintf(buff, sizeof(buff), " %2ideg %6.3fmin %s", abs(degrees), abs(minutes), direction ? "N" : "S");

  return buff;
}
char* LongtoString(double data) {
  static char buff[25];
  double pos;
  pos = data;
  int degrees = int(pos);
  float minutes = (pos - degrees) * 60;
  bool direction;
  direction = (pos >= 0);
  snprintf(buff, sizeof(buff), "%3ideg %6.3fmin %s", abs(degrees), abs(minutes), direction ? "E" : "W");
  return buff;
}


// Draw the compass pointer at an angle in degrees
void WindArrow2(_sButton button, _sInstData Speed, _sInstData& Wind) {
  // Serial.printf(" ** DEBUG  speed %f    wind %f ",Speed.data,Wind.data);
  bool recent = (Wind.updated >= millis() - 3000);
  if (!Wind.graphed) {  //EventTiming("START");
    WindArrowSub(button, Speed, Wind);
    // EventTiming("STOP");EventTiming("WIND arrow");
  }
  if (Wind.greyed) { return; }

  if (!recent && !Wind.greyed) { WindArrowSub(button, Speed, Wind); }
}

void WindArrowSub(_sButton button, _sInstData Speed, _sInstData& wind) {
  //Serial.printf(" ** DEBUG WindArrowSub speed %f    wind %f \n",Speed.data,wind.data);
  bool recent = (wind.updated >= millis() - 3000);
  Phv center;
  int rad, outer, inner;
  static int lastfont;
  static double lastwind;
  center.h = button.h + button.width / 2;
  center.v = button.v + button.height / 2;
  rad = (button.height - (2 * button.bordersize)) / 2;  // height used as more likely to be squashed in height
  outer = (rad * 82) / 100;                             //% of full radius (at full height) (COMPASS has .83 as inner circle)
  inner = (rad * 28) / 100;                             //25% USe same settings as pointer
  DrawMeterPointer(center, lastwind, inner, outer, 2, button.BackColor, button.BackColor);
  if (wind.data != NMEA0183DoubleNA) {
    if (wind.updated >= millis() - 3000) {
      DrawMeterPointer(center, wind.data, inner, outer, 2, button.TextColor, BLACK);
    } else {
      wind.greyed = true;
      DrawMeterPointer(center, wind.data, inner, outer, 2, LIGHTGREY, LIGHTGREY);
    }
  }
  lastwind = wind.data;
  wind.graphed = true;
  lastfont = MasterFont;
  if (Speed.data != NMEA0183DoubleNA) {
    if (rad <= 130) {
      UpdateDataTwoSize(true, true, 8, 7, button, Speed, "%2.0fkt");
    } else {
      UpdateDataTwoSize(true, true, 10, 9, button, Speed, "%2.0fkt");
    }
  }

  setFont(lastfont);
}

void DrawMeterPointer(Phv center, double wind, int inner, int outer, int linewidth, uint16_t FILLCOLOUR, uint16_t LINECOLOUR) {  // WIP
  Phv P1, P2, P3, P4, P5, P6;
  P1 = translate(center, wind - linewidth, outer);
  P2 = translate(center, wind + linewidth, outer);
  P3 = translate(center, wind - (4 * linewidth), inner);
  P4 = translate(center, wind + (4 * linewidth), inner);
  P5 = translate(center, wind, inner);
  P6 = translate(center, wind, outer);
  PTriangleFill(P1, P2, P3, FILLCOLOUR);
  PTriangleFill(P2, P3, P4, FILLCOLOUR);
  Pdrawline(P5, P6, LINECOLOUR);
}

Phv translate(Phv center, double angle, int rad) {  // 'full version with full accuracy cos and sin
  Phv moved;
  moved.h = center.h + (rad * sin(angle * 0.0174533));
  moved.v = center.v - (rad * cos(angle * 0.0174533));  // v is minus as this is positive  down in gfx
  return moved;
}

void DrawCompass(_sButton button) {
  //x y are center in drawcompass
  int x, y, rad;
  x = button.h + button.width / 2;
  y = button.v + button.height / 2;
  rad = (button.height - (2 * button.bordersize)) / 2;
  int Rad1, Rad2, Rad3, Rad4, inner;
  Rad1 = rad * 0.83;  //200
  Rad2 = rad * 0.86;  //208
  Rad3 = rad * 0.91;  //220
  Rad4 = rad * 0.94;
  inner = (rad * 28) / 100;                                                            //28% USe same settings as pointer // keep border same as other boxes..
  gfx->fillRect(button.h, button.v, button.width, button.height, button.BorderColor);  // width and height are for the OVERALL box.
  gfx->fillRect(button.h + button.bordersize, button.v + button.bordersize, button.width - (2 * button.bordersize), button.height - (2 * button.bordersize), button.BackColor);
  //gfx->fillRect(x - rad, y - rad, rad * 2, rad * 2, button.BackColor);
  gfx->fillCircle(x, y, rad, button.TextColor);   //white
  gfx->fillCircle(x, y, Rad1, button.BackColor);  //bluse
  gfx->fillCircle(x, y, inner - 1, button.TextColor);
  gfx->fillCircle(x, y, inner - 5, button.BackColor);
  //rad =240 example Rad2 is 200 to 208   bar is 200 to 239 wind colours 200 to 220
  // colour segments
  gfx->fillArc(x, y, Rad3, Rad1, 270 - 45, 270, RED);
  gfx->fillArc(x, y, Rad3, Rad1, 270, 270 + 45, GREEN);
  //Mark 12 linesarks at 30 degrees
  for (int i = 0; i < (360 / 30); i++) { gfx->fillArc(x, y, rad, Rad1, i * 30, (i * 30) + 1, BLACK); }  //239 to 200
  for (int i = 0; i < (360 / 10); i++) { gfx->fillArc(x, y, rad, Rad4, i * 10, (i * 10) + 1, BLACK); }  // dots at 10 degrees
}

void wifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {

  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("WiFi connected");
      //  gfx->println(" Connected ! ");
      Serial.print("** Connected to : ");
      IsConnected = true;
      AttemptingConnect = false;
      //  gfx->println(" Using :");
      //  gfx->println(WiFi.SSID());
      Serial.print(WiFi.SSID());
      Serial.println(">");
      WifiGFXinterrupt(9, WifiStatus, "CONNECTED TO\n<%s>", WiFi.SSID());
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      // take care with printf. It can quickly crash if it gets stuff it cannot deal with.
      //  Serial.printf(" Disconnected.reason %s  isConnected%s   attemptingConnect%s  \n",disconnectreason(info.wifi_sta_disconnected.reason).c_str(),IsConnected On_Off,AttemptingConnect On_Off);
      if (!IsConnected) {
        if (AttemptingConnect) { return; }
        Serial.println("WiFi disconnected");
        Serial.print("WiFi lost reason: ");
        Serial.println(disconnectreason(info.wifi_sta_disconnected.reason));
        if (ScanAndConnect(true)) {  // is the required SSID to be found?
          WifiGFXinterrupt(8, WifiStatus, "Attempting Reconnect to\n<%s>", Current_Settings.ssid);
          Serial.println("Attempting Reconnect");
        }
      } else {
        Serial.println("WiFi Disconnected");
        Serial.print("WiFi Lost Reason: ");
        Serial.println(disconnectreason(info.wifi_sta_disconnected.reason));
        WiFi.disconnect(false);     // changed to false.. Revise?? so that it does this only if no one is connected to the AP ??
        AttemptingConnect = false;  // so that ScanandConnect can do a full scan next time..
        WifiGFXinterrupt(8, WifiStatus, "Disconnected \n REASON:%s\n Retrying:<%s>", disconnectreason(info.wifi_sta_disconnected.reason).c_str(), Current_Settings.ssid);
        IsConnected = false;
      }
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("The ESP32 has received IP address :");
      Serial.println(WiFi.localIP());
      WifiGFXinterrupt(9, WifiStatus, "CONNECTED TO\n<%s>\nIP:%i.%i.%i.%i\n", WiFi.SSID(),
                       WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
      //setupFilemanager();
      break;

    case ARDUINO_EVENT_WIFI_AP_START:
      WifiGFXinterrupt(8, WifiStatus, "Soft-AP started\n%s ", WiFi.softAPSSID());
      Serial.println("   10 ESP32 soft-AP start");
      break;


    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:  //12 a station connected to ESP32 soft-AP
      StationsConnectedtomyAP = StationsConnectedtomyAP + 1;
      WifiGFXinterrupt(8, WifiStatus, "Station Connected\nTo AP\n Total now %i", StationsConnectedtomyAP);
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:  //13 a station disconnected from ESP32 soft-AP
      StationsConnectedtomyAP = StationsConnectedtomyAP - 1;
      if (StationsConnectedtomyAP == 0) {}
      WifiGFXinterrupt(8, WifiStatus, "Station Disconnected\nfrom AP\n Total now %i", StationsConnectedtomyAP);

      break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:  //14 ESP32 soft-AP assign an IP to a connected station
      WifiGFXinterrupt(8, WifiStatus, "Station Connected\nTo AP\nNow has Assigned IP");
      Serial.print("   AP IP address: ");
      Serial.println(WiFi.softAPIP());
      break;
  }
}

void WifiGFXinterrupt(int font, _sButton& button, const char* fmt, ...) {  //quick interrupt of gfx to show WIFI events..
  if (Display_Page <= -1) { return; }                                      // do not interrupt the settings pages!
  if (Display_Config.Start_Page == -87) { return; }                        // do not do the screen shows on BLE page                                                                // version of add centered text, multi line from /void MultiLineInButton(int font, _sButton &button,const char *fmt, ...)
  static char msg[300] = { '\0' };
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  static char* token;
  const char delimiter[2] = "\n";  //  NB when i used  "static const char delimiter = '\n';"  I got big problems ..
  char* pch;
  GFXBorderBoxPrintf(button, "");  // clear the button
  pch = strtok(msg, delimiter);    // split (tokenise)  msg at the delimiter
  // print each separated line centered... starting from line 1
  button.PrintLine = 1;
  while (pch != NULL) {
    CommonSub_UpdateLine(button.TextColor, font, button, pch);
    pch = strtok(NULL, delimiter);
  }
  WIFIGFXBoxdisplaystarted = true;
  WIFIGFXBoxstartedTime = millis();
}

String disconnectreason(int reason) {
  switch (reason) {
    case 1: return "UNSPECIFIED"; break;
    case 2: return "AUTH_EXPIRE"; break;
    case 3: return "AUTH_LEAVE"; break;
    case 4: return "ASSOC_EXPIRE"; break;
    case 5: return "ASSOC_TOOMANY"; break;
    case 6: return "NOT_AUTHED"; break;
    case 7: return "NOT_ASSOCED"; break;
    case 8: return "ASSOC_LEAVE"; break;
    case 9: return "ASSOC_NOT_AUTHED"; break;
    case 10: return "DISASSOC_PWRCAP_BAD"; break;
    case 11: return "DISASSOC_SUPCHAN_BAD"; break;
    case 13: return "IE_INVALID"; break;
    case 14: return "MIC_FAILURE"; break;
    case 15: return "4WAY_HANDSHAKE_TIMEOUT"; break;
    case 16: return "GROUP_KEY_UPDATE_TIMEOUT"; break;
    case 17: return "IE_IN_4WAY_DIFFERS"; break;
    case 18: return "GROUP_CIPHER_INVALID"; break;
    case 19: return "PAIRWISE_CIPHER_INVALID"; break;
    case 20: return "AKMP_INVALID"; break;
    case 21: return "UNSUPP_RSN_IE_VERSION"; break;
    case 22: return "INVALID_RSN_IE_CAP"; break;
    case 23: return "802_1X_AUTH_FAILED"; break;
    case 24: return "CIPHER_SUITE_REJECTED"; break;
    case 200: return "BEACON_TIMEOUT"; break;
    case 201: return "NO_AP_FOUND"; break;
    case 202: return "AUTH_FAIL"; break;
    case 203: return "ASSOC_FAIL"; break;
    case 204: return "HANDSHAKE_TIMEOUT"; break;
    default: return "Unknown"; break;
  }
  return "Unknown";
}

bool ScanAndConnect(bool display) {
  static unsigned long ScanInterval;
  static bool found;
  unsigned long ConnectTimeout;
  // do the WIfI/scan(i) and it is independently stored somewhere!!
  // but do not call too often - give it time to run!!
  if (millis() >= ScanInterval) {
    ScanInterval = millis() + 20000;
    found = false;
    NetworksFound = WiFi.scanNetworks(false, false, true, 250, 0, nullptr, nullptr);
    delay(100);
    Serial.printf(" Scan found <%i> networks:\n", NetworksFound);
  } else {
    Serial.printf(" Using saved Scan of <%i> networks:\n", NetworksFound);
  }

  WiFi.disconnect(false);  // Do NOT turn off wifi if the network disconnects
  int channel = 0;
  long rssiValue;
  for (int i = 0; i < NetworksFound; ++i) {
    if (WiFi.SSID(i).length() <= 25) {
      Serial.printf(" <%s> ", WiFi.SSID(i));
    } else {
      Serial.printf(" <name too long> ");
    }
    if (WiFi.SSID(i) == Current_Settings.ssid) {
      found = true;
      channel = i;
      rssiValue = WiFi.RSSI(i);
    }
    Serial.printf("CH:%i signal:%i \n", WiFi.channel(i), WiFi.RSSI(i));
  }
  if (found) {
    if (display) { WifiGFXinterrupt(8, WifiStatus, "WIFI scan found <%i> networks\n Connecting to <%s> signal:%i\nplease wait", NetworksFound, Current_Settings.ssid, rssiValue); }
    Serial.printf(" Scan found <%s> \n", Current_Settings.ssid);  //gfx->printf("Found <%s> network!\n", Current_Settings.ssid);
    ConnectTimeout = millis() + 3000; // three second connect timeout limit 
    WiFi.begin(Current_Settings.ssid, Current_Settings.password, channel);  // faster if we pre-set it the channel??
    IsConnected = false;
    AttemptingConnect = true;
    // keep printing .. inside the box?
    gfx->setTextBound(WifiStatus.h + WifiStatus.bordersize, WifiStatus.v + WifiStatus.bordersize, WifiStatus.width - (2 * WifiStatus.bordersize), WifiStatus.height - (2 * WifiStatus.bordersize));
    gfx->setTextWrap(true);
    while ((WiFi.status() != WL_CONNECTED) && (millis() <= ConnectTimeout)) {
      gfx->print('.');
      Serial.print('.');
      delay(1000);
    }
    if (WiFi.status() != WL_CONNECTED) { WifiGFXinterrupt(8, WifiStatus,"Timeout - will try later"); }
    gfx->setTextBound(0, 0, 480, 480);
  } else {
    AttemptingConnect = false;
    if (display) { WifiGFXinterrupt(8, WifiStatus, "%is WIFI scan found\n <%i> networks\n but not %s\n Will look again in %i seconds", millis() / 1000, NetworksFound, Current_Settings.ssid, scansearchinterval / 1000); }
  }
  return found;
}
void ConnectWiFiusingCurrentSettings() {
  bool result;
  uint32_t StartTime = millis();
  // superceded by WIFI box display "setting up AP" gfx->println("Setting up WiFi");
  WiFi.disconnect(false, true);  // clean the persistent memory in case someone else set it !! eg ESPHOME!!
  delay(10);
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  // WiFi.onEvent(WiFiEventPrint); // serial print for debugging
  WiFi.onEvent(wifiEvent);  // Register the event handler
                            // start the display's AP - potentially with NULL pasword
  if ((String(Display_Config.APpassword) == "NULL") || (String(Display_Config.APpassword) == "null") || (String(Display_Config.APpassword) == "")) {
    result = WiFi.softAP(Display_Config.PanelName);
  } else {
    result = WiFi.softAP(Display_Config.PanelName, Display_Config.APpassword);
  }
  delay(5);
  if (result == true) {
    Serial.println("Soft-AP creation success!");
    Serial.print("   ssidAP: ");

    Serial.println(WiFi.softAPSSID());
    Serial.print("   passAP: ");
    Serial.println(Display_Config.APpassword);
    Serial.print("   AP IP address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Soft-AP creation failed! set this up..");
    Serial.print("   ssidAP: ");
    Serial.println(WiFi.softAPSSID());
    Serial.print("   AP IP address: ");
    Serial.println(WiFi.softAPIP());
  }
  WiFi.mode(WIFI_AP_STA);
  // all Serial prints etc are now inside ScanAndConnect 'TRUE' will display them.
}
void Setup_expander(int SDA,int SCL, int beepPin){
 Wire.begin(SDA,SCL);
  expander.portMode(ALLOUTPUT);  //Set the port as all output
 // Serial.println("Confirm expander connected via  Beep ");
  expander.digitalWrite(beepPin, HIGH);  //Buzzer ON! (confirms Expander is set up)
  delay(25);
  expander.digitalWrite(beepPin, LOW);  //Buzzer off!
  delay(150);
    expander.digitalWrite(beepPin, HIGH);  //Buzzer ON! (confirms Expander is set up)
  delay(25);
  expander.digitalWrite(beepPin, LOW);  //Buzzer off!
  delay(150);
}
void SD_Setup(int SPISCK, int SPIMISO, int SPIMOSI, int SPISDCS) {
    hasSD = false;
    Serial.println("SD Card START");
    if (SPISDCS == -1) {SD_CS(HIGH);SD_CS(LOW);}
    SPI.begin(SPISCK, SPIMISO, SPIMOSI);
    delay(10);
    if(!SD.begin()){ Serial.println("Card Mount Failed");gfx->println("NO SD Card");SD_CS(HIGH); return;}
    else{ hasSD = true;  // picture will be  run in setup, after load config
      if (!filemgr.AddFS(SD, "SD-Card", false)) { Serial.println(F("Adding SD to file manager failed."));}
        else{ Serial.println(F("  Added SD to file manager"));} // add the SD file manager to the Trek style file display 
    }
    uint8_t cardType = SD.cardType();
  
    if (cardType == CARD_NONE) {
      Serial.println("No SD card attached");hasSD = false;SD_CS(HIGH);
      return;
    }
    Serial.print("  SD Card Type: ");
    gfx->println(" ");  // or it starts outside text bound??
    gfx->print("SD Card Type: ");
    if (cardType == CARD_MMC) {
      Serial.print("MMC");
      gfx->println("MMC");
    } else if (cardType == CARD_SD) {
      Serial.print("SDSC");
      gfx->println("SCSC");
    } else if (cardType == CARD_SDHC) {
      Serial.print("SDHC");
      gfx->println("SDHC");
    } else {
      Serial.print("UNKNOWN");
      gfx->println("Unknown");
    }
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB", cardSize);
    gfx->printf("SD Card Size: %lluMB\n", cardSize);
    // Serial.println("*** SD card contents  (to three levels) ***");
    delay(500);
}


void SD_CS( bool state){
  if (SDCS == -1){  //SDCS set -1 for Wavshare using Expander.
  static bool laststate;
  if (laststate != state) {expander.digitalWrite(EX104,state);}//Serial.printf("+++ Setting SD_CD %s +++\n", state? "Deselected":"Enabled");    }  // true:false  Serial.printf("  Setting SD_CS state: %i\n", state); }

  laststate=state;
  }

}
bool SDexists(const char* path) {  //SD_CS to be done before this is called ! (equivalent to SD_MMC . Exists() function )
  
  File file = SD.open(path);
  if (file) { file.close(); return true;}
    return false;
}
