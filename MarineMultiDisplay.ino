
//********* for SERIAL on Wavshare.. MUST SET ******************
//Tools-> CDC on boot ENABLED

// not for s3 versions!! #include <NMEA2000_CAN.h>  // note Should automatically detects use of ESP32 and  use the (https://github.com/ttlappalainen/NMEA2000_esp32) library
///----  // see https://github.com/ttlappalainen/NMEA2000/issues/416#issuecomment-2251908112

const char soft_version[] = " V0.11";

//**********  SET DEFINES FOR THE BOARD WE WISH TO Compile for:  GUITRON 480x 480 (default or..)
#define WAVSHARE   // 4 inch  480 by 480                Wavshare use expander chip for chip selects! 
//#define WIDEBOX    // 4.3inch 800 by 400 display Setup
//**********  SET DEFINES

bool _WideDisplay;  // so that I can pass this to sub files

//Include libraries..
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>  // Load the Wire Library
#include <esp_now.h>
#include <esp_wifi.h>
#include <Arduino_GFX_Library.h>  // aka 'by Moon on our Nation'
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <SD.h>  // was SD.h  // pins set in 4inch.h
#include "FS.h"
#include <FFat.h>  // plan to use FATFS for local files
#include <SPIFFS.h>
#include <SD_MMC.h>
#include <LittleFS.h>
#include <TAMC_GT911.h>

//MAGIC TREK style File viewer see https://github.com/holgerlembke/
#include <ESPFMfGK.h>  // the thing.
const word filemanagerport = 8080;
// we want a different port than the webserver
ESPFMfGK filemgr(filemanagerport);

const esp_partition_t* ffat_partition = esp_partition_find_first(
  ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, NULL);// helps find if we have a fats 

bool hasSD, hasFATS, hasSPIFFS,Touch_available;

//*********** DISPLAY Pin identifications selector *************
#ifdef WAVSHARE
#ifdef WIDEBOX
#include "WAV_4.3inch.h"
#else
#include "WAV_4inch.h"
#endif
#else
#include "GUIT_4inch.h"
#endif

#include "debug_port.h"
#include "SDControl.h" // seeing if I can wrap SD_CS() into the filemanager

int Screen_Width;     // Solution to pass OUCH_WIDTH to stuff that does not see module data until later!

#include "N2kMsg.h"
#include "NMEA2000.h"
#include <NMEA2000_esp32xx.h>
#include <N2kMessages.h>
tNMEA2000& NMEA2000 = *(new tNMEA2000_esp32xx());

// some wifi stuff
IPAddress gateway(0, 0, 0, 0);    // the IP address for Gateway in Station mode
IPAddress subnet(0, 0, 0, 0);     // the SubNet Mask in Station mode
IPAddress ap_ip(192, 168, 4, 1);  // the IP address in AP mode. Default and can be changed!
//These added in part to try and send data UDP when Serial is not working, for debug
IPAddress udp_ap(0, 0, 0, 0);  // the IP address to send UDP data (AP mode)
IPAddress udp_st(0, 0, 0, 0);  // the IP address to send UDP data (station mode)
boolean IsConnected = false;  // may be used in AP_AND_STA to flag connection success (got IP)
boolean AttemptingConnect;    // to note that WIFI.begin has been started
int ScanChannelFound;
int NetworksFound;            // used for scan Networks result. Done at start up!
WiFiUDP Udp;
#define BufferLength 500
char nmea_1[BufferLength];    //serial 0183
char nmea_U[BufferLength];    // NMEA buffer for UDP input port
char nmea_EXT[BufferLength];  // buffer for ESP_now received data
// N2K operates via interruts and there are no '0183 version of the N2K messages

bool EspNowIsRunning = false;
char* pTOKEN;
int StationsConnectedtomyAP;
#define scansearchinterval 30000  // 5 secs for tests 30000 for running? 
// assists for wifigfx interrupt  box that shows status..  to help turn it off after a time
uint32_t WIFIGFXBoxstartedTime;
bool WIFIGFXBoxdisplaystarted;




// my sub files

#include "ESP_NOW_files.h"
#include "OTA.h"  // and root webpage
extern bool HaltOtherOperations;

#include "aux_functions.h"

#include "Display.h"
//*********** for keyboard*************
#include "Keyboard.h"



// Load the PCA9554 Library
#ifdef WAVSHARE

#include <PCA9554.h>     // Load the PCA9554 Library
PCA9554 expander(0x20);  // Create an expander object at this expander address
#endif
// touch sensor
TAMC_GT911 ts = TAMC_GT911(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, TOUCH_WIDTH, TOUCH_HEIGHT);
// for victron display pages
#include "VICTRONBLE.h"
int Num_Victron_Devices;
int CommonDisplayWidth;
const char* VictronDevicesSetupfilename = "/vconfig.txt";  // <- SD library uses 8.3 filenames
_sMyVictronDevices victronDevices;

char VictronBuffer[2000];  // way to transfer results in a way similar to NMEA text.

const char* ColorsFilename = "/colortest.txt";  // <- SD library uses 8.3 filenames?
_MyColors ColorSettings;


//********** All boat data (instrument readings) are stored as double in a single structure:
_sBoatData BoatData;  // BoatData values, int double etc
bool dataUpdated;     // flag that Nmea Data has been updated
// _sWiFi_settings_Config (see structures.h) are the settings for the Display:
// If the structure is changed, be sure to change the Key (first figure) so that new defaults and struct can be set.
//
//Config.txt holds both Default and Display settings in one file
const char* Setupfilename = "/config.txt";  // <- SD library uses 8.3 filenames
_sDisplay_Config Saved_Display_Config;
_sDisplay_Config Display_Config;
_sWiFi_settings_Config Saved_Settings;
_sWiFi_settings_Config Current_Settings;

_sDisplay_Config Default_JSON = { "0.5", 4, 1, "nmeadisplay", "12345678", "SOG", "DEPTH", "WIND", "STW","DGRAPH" };  // many display stuff set default
_sWiFi_settings_Config Default_Settings_JSON = { 17, "GUESTBOAT", "12345678", "2002", false, false, true, true, false, 1000, false, true };

int MasterFont;                             //global for font! Idea is to use to reset font after 'temporary' seletion of another
String Fontname;
int text_height = 12;  //so we can get them if we change heights etc inside functions
int text_offset = 12;  //offset is not equal to height, as subscripts print lower than 'height'
int text_char_width = 12;
int Display_Page;


//****  My displays are based on '_sButton' structures to define position, width height, borders and colours.
// int h, v, width, height, bordersize;  uint16_t BackColor, TextColor, BorderColor;



_sButton FontBox = { 0, 80, TOUCH_WIDTH, 330, 5, BLUE, WHITE, BLUE };

//_sButton WindDisplay = { 0, 0, 480, 480, 0, BLUE, WHITE, BLACK };  // full screen no border

//used for single data display
// modified all to lift by 30 pixels to allow a common bottom row display (to show logs and get to settings)


_sButton BigSingleDisplay = { 0, 90, TOUCH_WIDTH, 360, 5, BLUE, WHITE, BLACK };              // used for wind and graph displays
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
_sButton topRightquarter = { TOUCH_WIDTH-240, 0, 240, 240 - 15, 5, BLUE, WHITE, BLACK };
_sButton bottomRightquarter = { TOUCH_WIDTH-240, 240 - 15, 240, 240 - 15, 5, BLUE, WHITE, BLACK };
// for wide display //        int h, v, width, height, bordersize;  uint16_t BackColor, TextColor, BorderColor;
_sButton WideScreenCentral =          { TOUCH_WIDTH-560 ,0, 320, 480 - 30, 5, BLUE, WHITE, BLACK };
_sButton FullScreen = { 0, 0, TOUCH_WIDTH, 460, 5, BLUE, WHITE, BLACK }; 


// these were used for initial tests and for volume control - not needed for most people!! .. only used now for Range change in GPS graphic (?)
_sButton TopLeftbutton = { 0, 0, 75, 45, 5, BLUE, WHITE, BLACK };
_sButton TopRightbutton = { TOUCH_WIDTH-75, 0, 75, 45, 5, BLUE, WHITE, BLACK };

_sButton BottomRightbutton = { TOUCH_WIDTH-75, 405, 75, 45, 5, BLUE, WHITE, BLACK };
_sButton BottomLeftbutton = { 0, 405, 75, 45, 5, BLUE, WHITE, BLACK };


#define sw_width 65
//switches at line 180
_sButton Switch1 = { 20, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };
_sButton Switch2 = { 100, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };
_sButton Switch3 = { 180, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };
_sButton Switch5 = { 260, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };
_sButton Switch4 = { 345, 180, 120, 35, 5, WHITE, BLACK, BLUE };  // big one for eeprom update
_sButton Switch4a = { 345, 140, 120, 35, 5, WHITE, BLACK, BLUE };  // big one for eeprom update
//switches at line 60
_sButton Switch6 = { 20, 60, sw_width, 40, 5, WHITE, BLACK, BLACK };
_sButton Switch7 = { 100, 60, sw_width, 40, 5, WHITE, BLACK, BLACK };
_sButton Switch8 = { 180, 60, sw_width, 40, 5, WHITE, BLACK, BLACK };
_sButton Switch9 = { 260, 60, sw_width, 40, 5, WHITE, BLACK, BLACK };
_sButton Switch10 = { 340, 60, sw_width, 40, 5, WHITE, BLACK, BLACK };
_sButton Switch11 = { 420, 60, sw_width, 40, 5, WHITE, BLACK, BLACK };


//WIDTH dependant settings, h, v, width, height, bordersize
// read TOUCH_WIDTH nd subtract x..

_sButton StatusBox = { 0, 460, TOUCH_WIDTH, 20, 0, BLACK, WHITE, BLACK };
_sButton WifiStatus = { 60, 180, TOUCH_WIDTH-120, 120, 5, BLUE, WHITE, BLACK };  // big central box for wifi events to pop up - v3.5
_sButton FullSize = { 10, 0, 460, TOUCH_WIDTH-20, 0, BLUE, WHITE, BLACK };
_sButton FullSizeShadow = { 5, 10, TOUCH_WIDTH-20, 460, 0, BLUE, WHITE, BLACK };
_sButton CurrentSettingsBox = { 0, 0, TOUCH_WIDTH, 80, 2, BLUE, WHITE, BLACK };  //also used for showing the current settings

_sButton TOPButton = { 20, 10, TOUCH_WIDTH-50, 35, 5, WHITE, BLACK, BLUE };
_sButton SecondRowButton = { 20, 60, TOUCH_WIDTH-50, 35, 5, WHITE, BLACK, BLUE };
_sButton ThirdRowButton = { 20, 100, TOUCH_WIDTH-50, 35, 5, WHITE, BLACK, BLUE };
_sButton FourthRowButton = { 20, 140, TOUCH_WIDTH-50, 35, 5, WHITE, BLACK, BLUE };
_sButton FifthRowButton = { 20, 180, TOUCH_WIDTH-50, 35, 5, WHITE, BLACK, BLUE };


_sButton Terminal = { 0, 100, TOUCH_WIDTH, 330, 2, WHITE, BLACK, BLUE };
//for selections
_sButton FullTopCenter = { 80, 0, TOUCH_WIDTH-160, 50, 5, BLUE, WHITE, BLACK };

_sButton Full0Center = { 80, 55, TOUCH_WIDTH-160, 50, 5, BLUE, WHITE, BLACK };
_sButton Full1Center = { 80, 110, TOUCH_WIDTH-160, 50, 5, BLUE, WHITE, BLACK };
_sButton Full2Center = { 80, 165, TOUCH_WIDTH-160, 50, 5, BLUE, WHITE, BLACK };
_sButton Full3Center = { 80, 220, TOUCH_WIDTH-160, 50, 5, BLUE, WHITE, BLACK };
_sButton Full4Center = { 80, 275, TOUCH_WIDTH-160, 50, 5, BLUE, WHITE, BLACK };
_sButton Full5Center = { 80, 330, TOUCH_WIDTH-160, 50, 5, BLUE, WHITE, BLACK };
_sButton Full6Center = { 80, 385, TOUCH_WIDTH-160, 50, 5, BLUE, WHITE, BLACK };  // inteferes with settings box do not use!




//#include "esp_task_wdt.h"
#include "esp_partition.h"
// void SDList(char* msg){
//   DEBUG_PORT.println(msg);
//   listDir(SD, "/", 1);
// }

bool LoadConfigs(bool print,bool displ){
  bool FilesOK = true;
  if (LoadConfiguration()) {
   if(print){ DEBUG_PORT.println("USING FATS JSON for wifi and display settings");}
   //if(displ){ gfx->println(F("FATS JSON (WiFi and display settings"));}
    Saved_Display_Config=Display_Config;Saved_Settings=Current_Settings;
    Display_Page = Display_Config.Start_Page;
   } else {
    FilesOK = false;
    Display_Page = 4;  //here for clarity?
    Display_Config = Default_JSON;
    Current_Settings = Default_Settings_JSON;
    if(print){ DEBUG_PORT.println(" USING  defaults for wifi and display settings");}
 //   if(displ){gfx->println(F("Setting WiFi and Display settings to defaults"));}
    SaveConfiguration(SPIFFS, Setupfilename, Display_Config, Current_Settings);
  }
  if (LoadVictronConfiguration()) {
    if(print){  DEBUG_PORT.println("USING FATS JSON for Victron data settings");}
   //  if(displ){gfx->println(F("USING FATS JSON for Victron settings"));}
  } else {  // try to set a useful example
    FilesOK = false;
    if(print){  DEBUG_PORT.println("\n\n***FAILED TO GET Victron JSON FILE****\n**** SAVING DEFAULT on FSTFS****\n\n");}
  //  if(displ){gfx->println(F("Setting Victron settings to defaults"));}
    victronDevices.BLEDebug="FALSE";
    Num_Victron_Devices = 2;
    CommonDisplayWidth = 150;
    victronDevices.Simulate="TRUE";
    for (int index = 0; index < Num_Victron_Devices; index++) {
      strlcpy(victronDevices.charMacAddr[index],"d044984433d2",sizeof(victronDevices.charMacAddr[index]));
      strlcpy(victronDevices.charKey[index], "20bd18fc6ed74d9b6e40c83817d42fc8",sizeof(victronDevices.charKey[index]));
      strlcpy(victronDevices.DisplayShow[index],"VL",sizeof(victronDevices.DisplayShow[index]));
      victronDevices.VICTRON_BLE_RECORD_TYPE[index]=1;
      }
      victronDevices.displayH[0] =160;
      victronDevices.displayV[0]=160;
      victronDevices.displayH[1] =160;
      victronDevices.displayV[1]=0;
      strlcpy(victronDevices.FileCommentName[0], "Comment Example",sizeof(victronDevices.FileCommentName[0]));
      strlcpy(victronDevices.FileCommentName[1] , "Another Example",sizeof(victronDevices.FileCommentName[1]));
      SaveVictronConfiguration();  // should write a default file if it was missing?
    }
  if (LoadDisplayConfiguration()) {
    if(print){  DEBUG_PORT.println("USING FATS JSON for Colours data settings");}
   // if(displ){gfx->println(F("USING FATS JSON for Colours data settings"));}
  } else {
    FilesOK = false;
     if(print){ DEBUG_PORT.println("\n\n***FAILED TO GET Colours JSON FILE****\n**** SAVING DEFAULT on FATFS****\n\n");}
  //  if(displ){gfx->println(F("Setting Victron settings to defaults"));}
    SaveDisplayConfiguration();  // should write a default file if it was missing?
  }

 // if ( FilesOK && displ){gfx->println(F("FFATS files read"));}
return FilesOK;
}


void setup() {
  DEBUG_PORT.begin(115200);
  Screen_Width=TOUCH_WIDTH;  // difficult to pass TOUCH_WIDTH to aux functions 
  _WideDisplay =false;
  #ifdef WIDEBOX 
   _WideDisplay = true;
  #endif
  HaltOtherOperations = false;
  delay(100);
  DEBUG_PORT.print("Starting NMEA Display");
  DEBUG_PORT.println(F(" Using DEBUG_PORT.print "));
  // Serial.println(F(" Using Serial.print "));
  // Serial0.println(F(" Using Serial0.print "));
  DEBUG_PORT.println(_device);DEBUG_PORT.println(soft_version); 
  Display_Config = Default_JSON;
  Current_Settings = Default_Settings_JSON;
  Init_GFX(); // must be before SD setup (at least for Guitron)
  DEBUG_PORT.println(F(" Printing to screen  "));
  gfx->print(_device);
  gfx->println(soft_version);delay(100);
  //Fatfs_Setup();   // set up FATFS // includes gfx prints
  SPIFFS_Setup(); 
    delay(100);listDir(SPIFFS,"/",1);  delay(500);
  Setup_Wire_Expander(TOUCH_SDA, TOUCH_SCL, EX106);
  Touch_available = Touchsetup(); // if before ffats? stops ffats reading!
  delay(100);listDir(SPIFFS,"/",1);  delay(500);
  if (LoadConfigs(true,true)) {
     gfx->println(F("*** All CONFIGS LOADED ***"));
     DEBUG_PORT.println("All Configs Loaded ");
   }else {gfx->println(F("*** DEFAULT CONFIG SET ***"));}
  delay(10);
  SD_Setup(SD_SCK, SD_MISO, SD_MOSI, SDCS);  // NOTE SDCS not SD_CS, which is a function! set up SD card (for logs etc)
    if (hasSD) {
    gfx->println(F("***  SD CARD found ***"));
  } else {
    gfx->println(F("***  NO SD CARD ***"));
  }
    // for tests !readFile(FFat,"/config.txt");
    // FindI2CDevices("LISTING I2C devices");delay(100);// alternate - useful when there are lots of devices   scanI2CMatrix();
  InitNMEA2000();
  keyboard(-1);  //just reset keyboard's static variables
  ConnectWiFiusingCurrentSettings(); // listDir(FFat,"/",1); readFile(FFat,"/config.txt");delay(500);
  SetupWebstuff();  
  Udp.begin(atoi(Current_Settings.UDP_PORT));
  Start_ESP_EXT();             //  Sets esp_now links to the current WiFi.channel etc.
  BLEsetup();                  // listDir(FFat,"/",1);  delay(500);                   // setup Victron BLE interface (does not do much!!)
  setupFilemanager();          // listDir(FFat,"/",1); delay(500);  
  DEBUG_PORT.println("Setup Completed");  delay(500); 
 }

void loop() {
  static unsigned long LogInterval;
  static unsigned long DebugInterval;
  static unsigned long SSIDSearchTimer;
  static int WiFiChannel;
  //EventTiming("Timeing ",10); // place start and stop where I want timing 
  delay(1);
 // EventTiming("START");
  server.handleClient();  // for OTA webserver etc. will set HaltOtherOperations for OTA upload to try and overcome Wavshare boards low WDT settings or slow performance(?)
                          // SD_CS("HIGH");
  if (!HaltOtherOperations) { 
    SD_CS("LOW");
    filemgr.handleClient();  // trek style file manager with  SD CS low for any sd work BUT NOT WHILE TRYING TO DO OTA!!
    SD_CS("HIGH");
    //approx 2ms to here
    if(Touch_available){ts.read();}
    //~3.2ms
    EXTHeartbeat();//~3.3ms
    CheckAndUseInputs(); 

    Display(Display_Page);
       
    if (!AttemptingConnect && !IsConnected && (millis() >= SSIDSearchTimer)) {  // repeat at intervals to connect to station.
      SSIDSearchTimer = millis() + scansearchinterval;   if (millis()<= 30000){SSIDSearchTimer = millis() + 200;}                       // fast scan for first 30 secs
      if (StationsConnectedtomyAP == 0) { 
       WiFiChannel++;                                         // avoid scanning if we have someone connected to AP as it will/may disconnect!
       ScanAndConnect(Current_Settings.ssid, WiFiChannel,true); // ScanAndConnect will set AttemptingConnect And do a Wifi.begin if the required SSID has appeared
       if (WiFiChannel >= 12) { WiFiChannel = 1; }
      }  
    }
       
    // switch off WIFIGFXBox after timed interval
    if (WIFIGFXBoxdisplaystarted && (millis() >= WIFIGFXBoxstartedTime + 10000) && (!AttemptingConnect)) {
      WIFIGFXBoxdisplaystarted = false;
      // - see OTA    WebServerActive = false;
      Display(true, -99);delay(10);
      Display(true, Display_Page);
      delay(50);  // change page back, having set zero above which alows the graphics to reset up the boxes etc.
    }

    ///  BLEloop DO not try to do N2000 interrupts and BLE interrupts at the same time ?
    if ((Current_Settings.BLE_enable) && ((Display_Page == -86) || (Display_Page == -87))) {
      BLEloop();
    }
    else{if (Current_Settings.N2K_ON) { NMEA2000.ParseMessages(); }}
  }
}


// Can place functions after Setup and Loop only in ino

void Init_GFX() {
  DEBUG_PORT.println("Graphics Initialization ...");
  #ifdef WAVSHARE
  DEBUG_PORT.println("Using Expander Port Reset sequence");
  expander.digitalWrite(EX103, LOW);
  delay(100);
   expander.digitalWrite(EX103, HIGH);  // Step 3: Release RST HIGH while keeping INT LOW
  delay(10);     

  #endif

 #ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);DEBUG_PORT.println("GFX_BL setting");
  digitalWrite(GFX_BL, HIGH);
 #endif
  // Init Display
  gfx->begin();
  //if GFX> 1.3.1 try and do this as the invert colours write 21h or 20h to 0Dh has been lost from the structure!
  gfx->invertDisplay(false);
  gfx->fillScreen(BLUE);
  gfx->setTextBound(0, 0, TOUCH_WIDTH, 480);
  gfx->setTextColor(WHITE);
  setFont(4);
  gfx->setCursor(0, 20);
  gfx->println(F("*********************"));
  gfx->println(F("***Display Started***"));
  gfx->println(F("*********************"));
  gfx->println(F(""));
}

void InitNMEA2000() {  // make it display device Info on start up..

 DEBUG_PORT.print("NMEA2000 using RX pin ");DEBUG_PORT.println(ESP32_CAN_RX_PIN);
 DEBUG_PORT.print("NMEA2000 using TX pin ");DEBUG_PORT.println(ESP32_CAN_TX_PIN);
 gfx->println(F("Setting up NMEA2000 interface"));
 delay(10);
  NMEA2000.SetN2kCANMsgBufSize(8);
  NMEA2000.SetN2kCANReceiveFrameBufSize(100);
  // Set device information
  char SnoStr[33];
  uint32_t SerialNumber;
  SerialNumber = 9999;
  snprintf(SnoStr, 32, "%lu", SerialNumber);
  DEBUG_PORT.println("   Initializing ...");
  DEBUG_PORT.printf("   Unique ID: <%i> \r\n", SerialNumber);
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
  if (!Current_Settings.N2K_ON) { return; }
  for (iHandler = 0; NMEA2000Handlers[iHandler].PGN != 0 && !(N2kMsg.PGN == NMEA2000Handlers[iHandler].PGN); iHandler++)
    ;
  // we now have the index (iHandler) for the handler matching the received PGN
  if (NMEA2000Handlers[iHandler].PGN != 0) {
    NMEA2000Handlers[iHandler].Handler(N2kMsg);
    known = true;
  }
  if ((Display_Page == -21) || (Display_Page == -22)) {  // only do this terminal debug display if on the N2K viewing debug page!
    char decode[40];
    PGNDecode(N2kMsg.PGN).toCharArray(decode, 35);  // get the discription of the PGN from my string function, trucated to 35 char
    if (known) {
      UpdateLinef(BLACK, 8, Terminal, "N2K:(%i)[%.2X%.5X] %s", N2kMsg.PGN, N2kMsg.Source, N2kMsg.PGN, decode);
    } else {
      UpdateLinef(52685, 8, Terminal, "N2K:(%i)[%.2X%.5X] %s", N2kMsg.PGN, N2kMsg.Source, N2kMsg.PGN, decode);
    }
    //52685 is light gray in RBG565 light gray for pgns we do not decode. (based on handler setup)
  }
  // DEBUG_PORT.print(" N2K :Pgn");
  // DEBUG_PORT.printlnN2kMsg.PGN);
}

//**********  Drivers 
void Fatfs_Setup() {
  bool FMsetup=false;
  hasFATS = false;
  DEBUG_PORT.print("FFAT  START");
  if (FFat.begin(true)) {
    hasFATS = true;
    /*size_t totalBytes();
    size_t usedBytes();
    size_t freeBytes();*/
    uint64_t cardSize = FFat.totalBytes() / (1024 * 1024);
    DEBUG_PORT.printf("  FFat Size: %lluMB", cardSize);
    uint64_t FreeSize = FFat.freeBytes() / (1024 * 1024);
    DEBUG_PORT.printf("FFat freeBytes: %lluMB", FreeSize);
    if ( FFat.totalBytes() == FFat.freeBytes()) {gfx->print("EMPTY - formatting \n");FFat.format();}
    gfx->printf("FFat free: %lluMB ", FreeSize);
    delay(500);
    if (!filemgr.AddFS(FFat, "Flash/FFat", false)) {
      DEBUG_PORT.println(F("Adding FFAT to Filemanager failed."));
    } else { FMsetup=true;
      DEBUG_PORT.println(F("  Added FFAT to Filemanager"));
    }
  } else {
    DEBUG_PORT.println(F("FFat File System not initiated."));
  }
  
  if (!hasFATS) {
      gfx->print(F("---  NO FATFS ---"));
  }
  if (FMsetup){
    DEBUG_PORT.println(F("+ Filemanager*"));
    gfx->println(F("+ Filemanager*"));
  } else {
    gfx->println(F(""));
  }
}
void SPIFFS_Setup() {
  bool FMsetup=false;
  hasSPIFFS = false;
  DEBUG_PORT.print("FFAT  START");
  if (SPIFFS.begin(true)) {
    hasSPIFFS = true;
  size_t totalBytes = SPIFFS.totalBytes();
  size_t usedBytes = SPIFFS.usedBytes();
  size_t freeBytes = totalBytes - usedBytes;

   
    gfx->printf("SPIFFS initiated: %i free",freeBytes);
    delay(500);
    if (!filemgr.AddFS(SPIFFS, "Flash/SPIFFS", false)) {
      DEBUG_PORT.println(F("Adding SPIFFS to Filemanager failed."));
    } else { FMsetup=true;
      DEBUG_PORT.println(F("  Added SPIFFS to Filemanager"));
    }
  } else {
    DEBUG_PORT.println(F("FFat File System not initiated."));
  }
  
  if (!hasSPIFFS) {
      gfx->print(F("---  NO SPIFFS ---"));
  }
  if (FMsetup){
    DEBUG_PORT.println(F("+ Filemanager*"));
    gfx->println(F("+ Filemanager*"));
  } else {
    gfx->println(F(""));
  }
}
//*********** FLASH OVERLOAD  functions So can just be called from sub routines*********
// void EEPROM_WRITE(_sDisplay_Config B, _sWiFi_settings_Config A) {
//   DEBUG_PORT.printf("SAVING CONFIG\n");
//   SaveConfiguration(SPIFFS, Setupfilename, B, A);
// }

//***********  Overload for save and load that keeps file type here for easy change if needed
// I started with FFat but could not overcome crash 
void SaveConfiguration(){//overload of Save configuration in case new data stored 
  SaveConfiguration(SPIFFS, Setupfilename, Display_Config, Current_Settings); 
 }
void SaveVictronConfiguration(){
    SaveVictronConfiguration(SPIFFS, VictronDevicesSetupfilename, victronDevices);
 }
void SaveDisplayConfiguration(){
  SaveDisplayConfiguration(SPIFFS, ColorsFilename, ColorSettings);
 }
bool LoadConfiguration(){//overload of Reload configuration with standard file names in case new data stored 
  return LoadConfiguration(SPIFFS, Setupfilename, Display_Config, Current_Settings); 
 }

bool LoadVictronConfiguration(){
  return LoadVictronConfiguration(SPIFFS, VictronDevicesSetupfilename, victronDevices);
 }
bool LoadDisplayConfiguration(){
  return LoadDisplayConfiguration(SPIFFS, ColorsFilename, ColorSettings);
 }
void PrintJsonFile(const char* comment, const char* filename){
  PrintJsonFile(SPIFFS,comment, filename);
 }

//***********  JSON for configurations ****************
bool LoadVictronConfiguration(fs::FS& fs, const char* filename, _sMyVictronDevices& config) {
  // Open SD file for reading  //Active filesystems are: 0: SD-Card 1: Flash/FFat .
  bool fault = false;
  
  if (&fs == &SD) { SD_CS(LOW); }

  File file = fs.open(filename, FILE_READ);
  if (!file) {
    DEBUG_PORT.println(F("**Failed to read Victron JSON file"));
    fault = true;
  }
  // Allocate a temporary JsonDocument
  char temp[15];
  JsonDocument doc;
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    DEBUG_PORT.println(F("**Failed to deserialise victron JSON file"));
    fault = true;
  }
  strlcpy(temp, doc["BLEDebug"] | "false", sizeof(temp));
  config.BLEDebug = (strcmp(temp, "false"));

  /// BLEDEBUG prints all victron / ble out on serial port 
  //******  SET FALSE regardless of what is in the Vic Config !
  config.BLEDebug=false; 
  config.Beacons=false;  // can switch in in the victron display 

  strlcpy(temp, doc["Simulate"] | "false", sizeof(temp));
  config.Simulate = (strcmp(temp, "false"));   


  Num_Victron_Devices = doc["Num_Devices"] | 4;
  CommonDisplayWidth = doc["CommonDisplayWidth"] | 150;
  for (int index = 0; index < Num_Victron_Devices; index++) {
    strlcpy(config.charMacAddr[index], doc["device" + String(index) + ".mac"] | "macaddress", sizeof(config.charMacAddr[index]));
    strlcpy(config.charKey[index], doc["device" + String(index) + ".key"] | "key", sizeof(config.charKey[index]));
    strlcpy(config.FileCommentName[index], doc["device" + String(index) + ".comment"] | "?name?", sizeof(config.FileCommentName[index]));
    config.VICTRON_BLE_RECORD_TYPE[index] = doc["device" + String(index) + ".VICTRON_BLE_RECORD_TYPE"] | 1;  //default solar Mppt
    config.displayH[index] = doc["device" + String(index) + ".DisplayH"];
    config.displayV[index] = doc["device" + String(index) + ".DisplayV"];
    config.displayHeight[index] = doc["device" + String(index) + ".DisplayHeight"] | 150;
    strlcpy(config.DisplayShow[index], doc["device" + String(index) + ".DisplayShow"] | "PVIA", sizeof(config.DisplayShow[index]));
  }
  // Close the file (Curiously, File's destructor doesn't close the file)

  file.close();
  if (&fs == &SD) { SD_CS(HIGH); }
  return !fault;  // report success
}
void SaveVictronConfiguration(fs::FS& fs, const char* filename, _sMyVictronDevices& config) {
  // USED for adding extra devices or for creating a new file if missing
  //Delete existing file, otherwise the configuration is appended to the file
  File file;
  char buff[15];
  if (&fs == &SD) { SD_CS(LOW); }
  fs.remove(filename);
  // Open file for writing
  file = fs.open(filename, FILE_WRITE);
  if (!file) {
    DEBUG_PORT.println(F("JSON: Victron: Failed to create file"));
    if (&fs == &SD) { SD_CS(HIGH); }
    return;
  }
  DEBUG_PORT.printf(" We expect %i Victron devices", Num_Victron_Devices);
  // Allocate a temporary JsonDocument
  JsonDocument doc;
  doc[" Comment"] = " DisplayShow Options: 'P' Power 'V' Battery Volts 'I' Battery Current";
  doc[" "] = " 'v' second Battery Volts 'i' second Battery Current  (ac charger only)";
  doc[" "] = "'L' Load current 'S' State of charge 'E' error codes 'T' Temperature";
  doc[" "] = " 'A' Aux reading(t or starter) ";
  doc[" "] = "for SmartShunt, IAS will display current, State of charge and (starter V or temperature)";
  doc[" "] = "for Battery Monitor: V will display Voltage (only)";
  doc[" note"] = "Display height is also adjustable for each 'device', and devices can be duplicated";
  doc[" Types known "] = " SOLAR_CHARGER =1,  BATTERY_MONITOR = 2, AC Charger = 8";
  doc["BLEDebug"] = config.BLEDebug True_False; //set.Debug True_False;
  doc["Simulate"] = config.Simulate True_False; 
  doc["Num_Devices"] = Num_Victron_Devices;
  doc["Common_width"] = CommonDisplayWidth;
  // doc[" Comment1"]= "for Shunt, VIA will display Battery Volts, Current, Additional data";
  // doc[" Comment2"]= "for SOLAR, PIA will display solar Power, battery Current, Additional data";
  for (int index = 0; index < Num_Victron_Devices; index++) {
    doc["device" + String(index) + ".mac"] = config.charMacAddr[index];
    doc["device" + String(index) + ".key"] = config.charKey[index];
    doc["device" + String(index) + ".comment"] = config.FileCommentName[index];
    doc["device" + String(index) + ".VICTRON_BLE_RECORD_TYPE"] = config.VICTRON_BLE_RECORD_TYPE[index];
    doc["device" + String(index) + ".DisplayH"] = config.displayH[index];
    doc["device" + String(index) + ".DisplayV"] = config.displayV[index];
    doc["device" + String(index) + ".DisplayHeight"] = config.displayHeight[index];
    doc["device" + String(index) + ".DisplayShow"] = config.DisplayShow[index];
  }

  // Serialize JSON to file
  if (serializeJsonPretty(doc, file) == 0) {  // use 'pretty format' with line feeds
    DEBUG_PORT.println(F("JSON: Failed to write to Victron file"));
  }
  // Close the file, //but print serial as a check
  file.close();
  if (&fs == &SD) { SD_CS(HIGH); }
 // PrintJsonFile("Check after Saving configuration ", filename);
}
void SaveDisplayConfiguration(fs::FS& fs, const char* filename, _MyColors& set) {
  // Delete existing file, otherwise the configuration is appended to the file
  //Active filesystems are: 0: SD-Card 1: Flash/FFat .
  char buff[15];
  File file;
  if (&fs == &SD) { SD_CS(LOW); }
  fs.remove(filename);
  // Open file for writing
  file = fs.open(filename, FILE_WRITE);

  if (!file) {
    DEBUG_PORT.println(F("JSON: Failed to create file"));
    if (&fs == &SD) { SD_CS(HIGH); }
    return;
  }

  // Allocate a temporary JsonDocument
  JsonDocument doc;
  // Set the values in the JSON file.. // NOT ALL ARE read yet!!
  //modify how the display works
  doc["_comments_"] = "Colours as integers";
  doc["WHITE"] = WHITE;
  doc["BLUE"] = BLUE;
  doc["BLACK"] = BLACK;
  doc["GREEN"] = GREEN;
  doc["RED"] = RED;

  doc["TextColor"] = set.TextColor;
  doc["BackColor"] = set.BackColor;
  doc["BorderColor"] = set.BorderColor;
  doc["_comments_"] = "These sizes below are for some font tests in victron display!";
  // doc["BoxH"] = set.BoxH;
  // doc["BoxW"] = set.BoxW;
  doc["FontH"] = set.FontH;
  doc["FontS"] = set.FontS;
  doc["SERIAL_OUT"] = set.SerialOUT True_False;

  doc["ShowRawDecryptedDataFor"] = set.ShowRawDecryptedDataFor;
  doc["Frame"] = set.Frame True_False;
  // Serialize JSON to file
  if (serializeJsonPretty(doc, file) == 0) {  // use 'pretty format' with line feeds
    DEBUG_PORT.println(F("JSON: Failed to write to file"));
  }
  // Close the file, but print serial as a check
  file.close();
  if (&fs == &SD) { SD_CS(HIGH); }
 // PrintJsonFile("Check after Saving configuration ", filename);
}
bool LoadDisplayConfiguration(fs::FS& fs, const char* filename, _MyColors& set) {
  // Openfile for reading  //Active filesystems are: 0: SD-Card 1: Flash/FFat .
  bool fault = false;
  if (&fs == &SD) { SD_CS(LOW); }
   DEBUG_PORT.printf("Free heap before open: %d\n", ESP.getFreeHeap());
   DEBUG_PORT.printf("Internal heap: %d\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
   DEBUG_PORT.printf("PSRAM heap: %d\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

  DEBUG_PORT.println(filename); // Confirm it's correct
  DEBUG_PORT.println(fs.exists(filename) ? "Exists" : "Missing");
  File test = fs.open("/test.txt", FILE_READ);
  DEBUG_PORT.println(" TEST file loads");delay(100);
  File file = fs.open(filename, FILE_READ);

  if (!file) {
    DEBUG_PORT.println(F("**Failed to read JSON file"));
    fault = true;
  }
  // Allocate a temporary JsonDocument
  char temp[15];
  JsonDocument doc;
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    DEBUG_PORT.println(F("**Failed to deserialise JSON file"));
    fault = true;
  }
  // gett here means we can set defaults, regardless!

  set.TextColor = doc["TextColor"] | BLACK;
  set.BackColor = doc["BackColor"] | WHITE;
  set.BorderColor = doc["BorderColor"] | BLUE;
  // set.BoxW = doc["BoxW"] | 46;
  // set.BoxH = doc["BoxH"] | 100;

  set.FontH = doc["FontH"] | WHITE;
  set.FontS = doc["FontS"] | WHITE;
  // strlcpy(temp, doc["Simulate"] | "false", sizeof(temp));
  // set.Simulate = (strcmp(temp, "false"));
  strlcpy(temp, doc["Frame"] | "false", sizeof(temp));
  set.Frame = (strcmp(temp, "false"));
  strlcpy(temp, doc["SERIAL_OUT"] | "false", sizeof(temp));
  set.SerialOUT = (strcmp(temp, "false"));


  set.ShowRawDecryptedDataFor = doc["ShowRawDecryptedDataFor"] | 1;
  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();
  if (&fs == &SD) { SD_CS(HIGH); }
  return !fault;
}
bool LoadConfiguration(fs::FS& fs, const char* filename, _sDisplay_Config& config, _sWiFi_settings_Config& settings) {
  // Open file from fs for reading  //Active filesystems are:  SD: SD-Card  FFat: Flash/FFat .
  // Allocate a temporary JsonDocument
  bool fault = false;
  char temp[15];
  JsonDocument doc; 
  if (&fs == &SD) { SD_CS(LOW); }
  File file = fs.open(filename, FILE_READ);
  if (!file) {
    //DEBUG_PORT.println(F("**Failed to read CONFIGURATION JSON file"));
    fault = true;
  }
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    //DEBUG_PORT.println(F("**Failed to deserialise JSON file"));
    fault = true;
  }
  // Copy values (or defaults) from the JsonDocument to the config / settings
  //if (doc["Start_Page"] == 0) { return false; }  //secondary backup in case the file is present (passes error) but zeroed!
  config.LocalTimeOffset = doc["LocalTimeOffset"] | 0;
  config.Start_Page = doc["Start_Page"] | 4;  // 4 is default page int - no problem...
  strlcpy(temp, doc["Mag_Var"] | "1.15", sizeof(temp));
  BoatData.Variation = atof(temp);  //  (+ = easterly) Positive: If the magnetic north is to the east of true north, the declination is positive (or easterly).

  strlcpy(config.FourWayBR, doc["FourWayBR"] | "SOG", sizeof(config.FourWayBR));
  strlcpy(config.FourWayBL, doc["FourWayBL"] | "DEPTH", sizeof(config.FourWayBL));
  strlcpy(config.FourWayTR, doc["FourWayTR"] | "WIND", sizeof(config.FourWayTR));
  strlcpy(config.FourWayTL, doc["FourWayTL"] | "STW", sizeof(config.FourWayTL));
  strlcpy(config.WideScreenCentral, doc["WideScreenCentral"] | "DGRAPH", sizeof(config.WideScreenCentral));
  strlcpy(config.PanelName,                  // User selectable
          doc["PanelName"] | "NMEADISPLAY",  // <- and default in case Json is corrupt / missing !
          sizeof(config.PanelName));
  strlcpy(config.APpassword,               // User selectable
          doc["APpassword"] | "12345678",  // <- and default in case Json is corrupt / missing !
          sizeof(config.APpassword));

  strlcpy(settings.ssid,                 // <- destination
          doc["ssid"] | "GUESTBOAT",     // <- source and default in case Json is corrupt!
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
  strlcpy(temp, doc["DATALOG"] | "false", sizeof(temp));
  settings.Data_Log_ON = (strcmp(temp, "false"));
  //**************  SET ALL LOGGING FALSE on STARTUP - 
  //  ***  There is not enough room on Flash to simply store everything
  //  MAYBE later add some tests for SD (not on waveshare!) and save to SD 
  settings.Data_Log_ON=false;
  settings.Log_ON=false;
  settings.log_interval_setting = doc["LogInterval"] | 60;

  strlcpy(temp, doc["BLE_enable"] | "false", sizeof(temp));
  settings.BLE_enable = (strcmp(temp, "false"));
  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();
  if (&fs == &SD) { SD_CS(HIGH); }
  //if (!error) { return true; }
  return !fault;
}

void SaveConfiguration(fs::FS& fs, const char* filename, _sDisplay_Config& config, _sWiFi_settings_Config& settings) {
  // Save  file to fs   //Active filesystems are:  SD: SD-Card  FFat: Flash/FFat .
  // need to work with SD some time
  //if (((&fs == &SD)&&(hasSD)) || ((&fs == &FFat)&&(hasFATS))) {
  char buff[15];
  File file;
  if (&fs == &SD) { SD_CS(LOW); }
  fs.remove(filename);
  // Open file for writing
  file = fs.open(filename, FILE_WRITE);
  if (!file) {
    DEBUG_PORT.println(F("JSON: Failed to create file"));
    if (&fs == &SD) { SD_CS(HIGH); }
    return;
  }
  // Allocate a temporary JsonDocument
  JsonDocument doc;
  // Set the values in the JSON file.. // NOT ALL ARE read yet!!
  //now the settings WIFI etc..
   //modify how the display works
  doc["Display Set"] = "if Start_Page=4, The main 'quad' display ";
  doc["other useful settings"] ="-21 terminal  -87 victron ble";
  doc["Start_Page"] = config.Start_Page;
  doc["LocalTimeOffset"] = config.LocalTimeOffset;
  doc["Display Options "] = "are: WIND TIME TIMEL SOG SOGGRAPH STW STWGRAPH GPS DEPTH";
  doc["Graph Options."] = "DGRAPH and DGRAPH2 display 10 and 30 m ranges";
  doc["FourWayBR"] = config.FourWayBR;
  doc["FourWayBL"] = config.FourWayBL;
   doc["FourWayTR"] = config.FourWayTR;
  doc["FourWayTL"] = config.FourWayTL;
  doc["Widescreen is "] =" only available for Wide (800 pixel) screens";
  doc["WideScreenCentral"] = config.WideScreenCentral;
  doc["WIFI settings"] = "Fill in these for your network";
  doc["ssid"] = settings.ssid;
  doc["password"] = settings.password;
  doc["UDP_PORT"] = settings.UDP_PORT;
  doc["PanelName"] = config.PanelName;
  doc["APpassword"] = config.APpassword;
  doc["Serial"] = settings.Serial_on True_False;
  doc["UDP"] = settings.UDP_ON True_False;
  doc["ESP"] = settings.ESP_NOW_ON True_False;
  doc["N2K"] = settings.N2K_ON True_False;
  doc["BLE - Victron setting"] = "only for Victron display pages (-87,-86)";
  doc["BLE_enable"] = settings.BLE_enable True_False;

 // DEBUG_PORT.print("save magvar:");
 // DEBUG_PORT.printf("%5.3f", BoatData.Variation);
  snprintf(buff, sizeof(buff), "%5.3f", BoatData.Variation);
  doc["Mag_Var"] = buff;
 
  doc["Log settings"] = "LOG saves read data in SD file with date as name- BUT only when GPS date has been seen!";
  doc["DATALOG"] = "DATALOG saves every message. Use DATALOG only for debugging!";
  doc["Be Careful"] = " the DATALOG files will become huge";
  doc["LOG"] = settings.Log_ON True_False;
  doc["LogInterval"] = settings.log_interval_setting;
  doc["DATALOG"] = settings.Data_Log_ON True_False;

  // Serialize JSON to file
  if (serializeJsonPretty(doc, file) == 0) {  // use 'pretty format' with line feeds
    DEBUG_PORT.println(F("JSON: Failed to write to file"));
  }
  // Close the file, but can print serial as a check in de
  file.close();
  if (&fs == &SD) { SD_CS(HIGH); }
  Saved_Display_Config=config;Saved_Settings=settings;
 // PrintJsonFile("Check after Saving configuration ", filename);
}

void PrintJsonFile(fs::FS& fs,const char* comment, const char* filename) {
  // Open file for reading
  if (&fs==&SD){SD_CS(LOW);}
  File file = fs.open(filename, FILE_READ);
  DEBUG_PORT.printf(" %s JSON FILE %s is.", comment, filename);
  if (!file) {
    DEBUG_PORT.println(F("Failed to read file"));
    if (&fs==&SD){SD_CS(HIGH);}
    return;
  }
  DEBUG_PORT.println();
  // Extract each characters by one by one
  while (file.available()) {
    DEBUG_PORT.print((char)file.read());
  }
  DEBUG_PORT.println();
  // Close the file
  file.close();
  if (&fs==&SD){SD_CS(HIGH);}
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
  UpdateLinef(7, CurrentSettingsBox, "Ser<%s>UDP<%s>ESP<%s>Log<%s>NMEA<%s>", A.Serial_on On_Off, A.UDP_ON On_Off, A.ESP_NOW_ON On_Off, A.Log_ON On_Off, A.Data_Log_ON On_Off);
  // UpdateLinef(7,CurrentSettingsBox, "Logger settings Log<%s>NMEA<%s>",A.Serial_on On_Off, A.UDP_ON On_Off, A.ESP_NOW_ON On_Off,A.Data_Log_ON On_Off);
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
  if (A.Data_Log_ON != B.Data_Log_ON) { same = false; }

  //DEBUG_PORT.print(" DEBUG ");DEBUG_PORT.print(A.ssid); DEBUG_PORT.print(" and ");DEBUG_PORT.println(B.ssid);
  // these are char strings, so need strcmp to compare ...if strcmp==0 they are equal
  if (strcmp(A.UDP_PORT, B.UDP_PORT) != 0) { same = false; }
  if (strcmp(A.ssid, B.ssid) != 0) { same = false; }
  if (strcmp(A.password, B.password) != 0) { same = false; }

  //DEBUG_PORT.print("Result same = ");DEBUG_PORT.println(same);
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
  // DEBUG_PORT.printf(" ** DEBUG  speed %f    wind %f ",Speed.data,Wind.data);
  bool recent = (Wind.updated >= millis() - 3000);
  if (!Wind.graphed) {  //EventTiming("START");
    WindArrowSub(button, Speed, Wind);
    // EventTiming("STOP");EventTiming("WIND arrow");
  }
  if (Wind.greyed) { return; }

  if (!recent && !Wind.greyed) { WindArrowSub(button, Speed, Wind); }
}

void WindArrowSub(_sButton button, _sInstData Speed, _sInstData& wind) {
  //DEBUG_PORT.printf(" ** DEBUG WindArrowSub speed %f    wind %f \n",Speed.data,wind.data);
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
      UpdateDataTwoSize(1,true, true, 8, 7, button, Speed, "%2.0fkt");
    } else {
      UpdateDataTwoSize(1,true, true, 10, 9, button, Speed, "%2.0fkt");
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



//****************   ALL THE WIFI STUFF *********************************

void wifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      DEBUG_PORT.println("WiFi connected");
      //  gfx->println(" Connected ! ");
      DEBUG_PORT.print("** Connected to : ");
      IsConnected = true;
      AttemptingConnect = false;
      //  gfx->println(" Using :");
      //  gfx->println(WiFi.SSID());
      DEBUG_PORT.print(WiFi.SSID());
      DEBUG_PORT.println(">");
     
      if (MDNS_START()){  //
      DEBUG_PORT.printf("MDNS Started: http://%s.local\n", WiFi.SSID(),Display_Config.PanelName);
          WifiGFXinterrupt(8, WifiStatus, "MDNS Started: http://%s.local", WiFi.SSID(),Display_Config.PanelName);}
        else{WifiGFXinterrupt(8, WifiStatus, "MDNS Fail Connected TO\n<%s>", WiFi.SSID());}
  
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      // take care with printf. It can quickly crash if it gets stuff it cannot deal with.
      //  DEBUG_PORT.printf(" Disconnected.reason %s  isConnected%s   attemptingConnect%s  \n",disconnectreason(info.wifi_sta_disconnected.reason).c_str(),IsConnected On_Off,AttemptingConnect On_Off);
      if (!IsConnected) {
        if (AttemptingConnect) { return; }
        DEBUG_PORT.println("WiFi disconnected");
        DEBUG_PORT.print("WiFi lost reason: ");
        DEBUG_PORT.println(disconnectreason(info.wifi_sta_disconnected.reason));
        if (ScanAndConnect(true,true)) {  // is the required SSID to be found? FULL SCAN
          WifiGFXinterrupt(8, WifiStatus, "Attempting Reconnect to\n<%s>", Current_Settings.ssid);
          DEBUG_PORT.println("Attempting Reconnect");
        }
      } else {
        DEBUG_PORT.println("WiFi Disconnected");
        DEBUG_PORT.print("WiFi Lost Reason: ");
        DEBUG_PORT.println(disconnectreason(info.wifi_sta_disconnected.reason));
        WiFi.disconnect(false);     // changed to false.. Revise?? so that it does this only if no one is connected to the AP ??
        AttemptingConnect = false;  // so that ScanandConnect can do a full scan next time..
        WifiGFXinterrupt(8, WifiStatus, "Disconnected \n REASON:%s\n Retrying:<%s>", disconnectreason(info.wifi_sta_disconnected.reason).c_str(), Current_Settings.ssid);
        IsConnected = false;
      }
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      DEBUG_PORT.print("The ESP32 has received IP address :");
      DEBUG_PORT.println(WiFi.localIP());
      udp_st = Get_UDP_IP(WiFi.localIP(), subnet);
      WifiGFXinterrupt(9, WifiStatus, "CONNECTED TO\n<%s>\nIP:%i.%i.%i.%i\n", WiFi.SSID(),
                       WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
      break;

    case ARDUINO_EVENT_WIFI_AP_START:
      WifiGFXinterrupt(8, WifiStatus, "Soft-AP started\n%s ", WiFi.softAPSSID());
      DEBUG_PORT.println("   10 ESP32 soft-AP start");
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
      DEBUG_PORT.print("   AP IP address: ");
      DEBUG_PORT.println(WiFi.softAPIP());
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
  delay(100); // take a breath after showing on screen!
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

void ScanAndConnect(char* look_for, int ScanChannel,bool print) { // single channel scan  - less disruptive 
  ScanChannelFound = 0;
  unsigned long Updated = millis();
  int n = WiFi.scanNetworks(false, false, false, 50, ScanChannel, nullptr, nullptr);
  for (int i = 0; i < n; ++i) {
    if (String(look_for) == String(WiFi.SSID(i))) {
      ScanChannelFound = WiFi.channel(i);
      IsConnected = false;
      AttemptingConnect = true;
        if (print) { WifiGFXinterrupt(8, WifiStatus, "WIFI found <%s>\n Channel %i \nSignal:%i\n begin...",  
                                     Current_Settings.ssid,ScanChannelFound, String(WiFi.RSSI(i))); }
       WiFi.begin(Current_Settings.ssid, Current_Settings.password, ScanChannelFound);
    }
  }
  // if (ScanChannel != 0) { return ; }     // only repports the full scan
  if(print){if(ScanChannelFound) { DEBUG_PORT.printf("   FOUND <%s> on Ch<%d> in %ums\r\n", look_for, ScanChannelFound, (millis() - Updated)); }
  else { DEBUG_PORT.printf("   not found <%s> on Ch<%d> in %ums\r\n", look_for, ScanChannel, (millis() - Updated)); }   

  }
}


bool ScanAndConnect(bool display){
 return ScanAndConnect(display,false);
}
bool ScanAndConnect(bool display,bool forceFull) {
  static unsigned long ScanInterval;
  static bool found;
  unsigned long ConnectTimeout;
  // do the WIfI/scan(i) and it is independently stored somewhere!!
  // but do not call too often - give it time to run!!
  if ((millis() >= ScanInterval)||forceFull) {
   //     EventTiming("START");
    ScanInterval = millis() + 20000;  //FULL WIFi rescan at 20 sec interval 
    found = false;
    NetworksFound = WiFi.scanNetworks(false, false, true, 250, 0, nullptr, nullptr);
   // EventTiming("STOP");
    delay(1);
   if(display) {DEBUG_PORT.printf(" Scan found <%i> networks:\n", NetworksFound);}
    
  } else {
    if(display) {DEBUG_PORT.printf(" Using saved Scan of <%i> networks:\n", NetworksFound);}
  }

  WiFi.disconnect(false);  // Do NOT turn off wifi if the network disconnects
  int channel = 0;
  long rssiValue;
  for (int i = 0; i < NetworksFound; ++i) {
    if (WiFi.SSID(i).length() <= 25) {
      if(display) {DEBUG_PORT.printf(" <%s> ", WiFi.SSID(i));}
    } else {
      if(display) {DEBUG_PORT.printf(" <name too long> ");}
    }
    if (WiFi.SSID(i) == Current_Settings.ssid) {
      found = true;
      channel = i;
      rssiValue = WiFi.RSSI(i);
    }
    if(display) {DEBUG_PORT.printf("CH:%i signal:%i \n", WiFi.channel(i), WiFi.RSSI(i));}
  }
  if (found) {
    if (display) { WifiGFXinterrupt(8, WifiStatus, "WIFI scan found <%i> networks\n Connecting to <%s> signal:%i\nplease wait", NetworksFound, Current_Settings.ssid, rssiValue); }
    DEBUG_PORT.printf(" Scan found <%s> \n", Current_Settings.ssid);        //gfx->printf("Found <%s> network!\n", Current_Settings.ssid);
    ConnectTimeout = millis() + 3000;                                       // three second connect timeout limit
    WiFi.begin(Current_Settings.ssid, Current_Settings.password, channel);  // faster if we pre-set it the channel??
    IsConnected = false;
    AttemptingConnect = true;
    // keep printing .. inside the box?
    gfx->setTextBound(WifiStatus.h + WifiStatus.bordersize, WifiStatus.v + WifiStatus.bordersize, WifiStatus.width - (2 * WifiStatus.bordersize), WifiStatus.height - (2 * WifiStatus.bordersize));
    gfx->setTextWrap(true);
    while ((WiFi.status() != WL_CONNECTED) && (millis() <= ConnectTimeout)) {
      gfx->print('.');
      DEBUG_PORT.print('.');
      delay(1000);
    }
    if (WiFi.status() != WL_CONNECTED) { WifiGFXinterrupt(8, WifiStatus, "Timeout - will try later"); }
    gfx->setTextBound(0, 0, TOUCH_WIDTH, 480);
  } else {
    AttemptingConnect = false;
    if (display) { WifiGFXinterrupt(8, WifiStatus, "%is WIFI scan found\n <%i> networks\n but not %s\n Will look again in %i seconds", millis() / 1000, NetworksFound, Current_Settings.ssid, scansearchinterval / 1000); }
  }
  return found;
}

void ConnectWiFiusingCurrentSettings() {
  bool result;
  uint32_t StartTime = millis();
  const IPAddress sub255(255, 255, 255, 0);  // the default Subnet Mask in in SoftAP mode
  const IPAddress Null_ip(0, 0, 0, 0);    //  A null IP address for the gateway
  // superceded by WIFI box display "setting up AP" gfx->println("Setting up WiFi");
  WiFi.softAPConfig(ap_ip, Null_ip, sub255);
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
    DEBUG_PORT.println("Soft-AP creation success!");
    DEBUG_PORT.print("   ssidAP: ");

    DEBUG_PORT.println(WiFi.softAPSSID());
    DEBUG_PORT.print("   passAP: ");
    DEBUG_PORT.println(Display_Config.APpassword);
    DEBUG_PORT.print("   AP IP address: ");
    DEBUG_PORT.println(WiFi.softAPIP());
    udp_ap = WiFi.softAPIP();
    udp_ap[3] = 255;  //broadcast
  } else {
    DEBUG_PORT.println("Soft-AP creation failed! set this up..");
    DEBUG_PORT.print("   ssidAP: ");
    DEBUG_PORT.println(WiFi.softAPSSID());
    DEBUG_PORT.print("   AP IP address: ");
    DEBUG_PORT.println(WiFi.softAPIP());
  }
  WiFi.mode(WIFI_AP_STA);
  // all Serial prints etc are now inside ScanAndConnect 'TRUE' will display them.
}

//***********************   END OF WIFI FUNCTIONS *****************************


//SETUP THE WIRE /EXPANDER /SD 
void Setup_Wire_Expander(int SDA, int SCL, int beepPin) {
  Wire.begin(SDA, SCL);
  #ifdef WAVSHARE
  expander.portMode(ALLOUTPUT);  //Set the port as all output
  DEBUG_PORT.println("Confirm expander connected via double Beep ");
  beep(1, beepPin);
  #endif
}

void SD_Setup(int SPISCK, int SPIMISO, int SPIMOSI, int SPISDCS) {
  hasSD = true;
  SPI.begin(SPISCK, SPIMISO, SPIMOSI);
  #ifdef WAVSHARE
  DEBUG_PORT.println("SD Card START.. Wavshare with expander CS");
    SD_CS(HIGH);SD_CS(LOW);delay(10);
    if (!SD.begin()) {
    DEBUG_PORT.println("   Card Mount Failed");
    hasSD = false;
    SD_CS(HIGH);
    return;
  }
  #else
  DEBUG_PORT.println("SD Card START.. Guitron with hard wired CS ");
    if (!SD.begin(SPISDCS)) {
    DEBUG_PORT.println("Card Mount Failed");
    gfx->println("NO SD Card");
    hasSD = false;
    return;
  }
  #endif
  delay(10);
    //hasSD = true;  // set above .. but 

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    DEBUG_PORT.println("No SD card attached");
    hasSD = false;
    #ifdef WAVSHARE
    SD_CS(HIGH);
    #endif
    return;
  }
  DEBUG_PORT.print("  SD Card Type: ");
  if (cardType == CARD_MMC) {
    DEBUG_PORT.print("MMC");
  } else if (cardType == CARD_SD) {
    DEBUG_PORT.print("SDSC");
  } else if (cardType == CARD_SDHC) {
    DEBUG_PORT.print("SDHC");
  } else {
    DEBUG_PORT.print("UNKNOWN");
  }
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  DEBUG_PORT.printf("SD Card Size: %lluMB\n", cardSize);
  if (!filemgr.AddFS(SD, "SD-Card", false)) {// add the SD file manager to the Trek style file display
      DEBUG_PORT.println(F("Adding SD to file manager failed."));
    } else {
      DEBUG_PORT.println(F("  Added SD to file manager"));
    } 
  #ifdef WAVSHARE
    SD_CS(HIGH);
  #endif   
  delay(500);
}

void SD_CS(bool state) {
#ifdef WAVSHARE
  static bool laststate;
    if (laststate != state) {
      expander.digitalWrite(EX104, state);
      delay(1);
    }  //DEBUG_PORT.printf("+++ Setting SD_CD %s +++\n", state? "Deselected":"Enabled");    }  // true:false  DEBUG_PORT.printf("  Setting SD_CS state: %i\n", state); }
    laststate = state;
#endif
}

bool SDexists(const char* path) {  //SD_CS to be done before this is called ! (equivalent to SD_MMC . Exists() function )
  if (!hasSD) { return false; }
  File file = SD.open(path);
  if (file) {
    file.close();
    return true;
  }
  return false;
}

void FindI2CDevices(String text) {
  DEBUG_PORT.println(text);
  for (int i = 0; i < 256; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      DEBUG_PORT.printf("Device detected at %x(hex)  %i(dec) \n", i, i);
      delay(10);
    }
  }
}

void scanI2CMatrix() {
  DEBUG_PORT.println("\nScanning I2C bus...\n");
  // Header row
  DEBUG_PORT.print("    ");
  for (uint8_t col = 0; col < 16; col++) {
    DEBUG_PORT.printf("%02X ", col);
  }
  DEBUG_PORT.println();
  // Address matrix
  for (uint8_t row = 0; row < 8; row++) {
    DEBUG_PORT.printf("%02X: ", row << 4);
    for (uint8_t col = 0; col < 16; col++) {
      uint8_t addr = (row << 4) | col;
      Wire.beginTransmission(addr);
      uint8_t error = Wire.endTransmission();
      if (error == 0) {
        DEBUG_PORT.print("██ ");  // Device found
      } else if (error == 4) {
        DEBUG_PORT.print("?? ");  // Unknown error
      } else {
        DEBUG_PORT.print("-- ");  // No device
      }
    }
    DEBUG_PORT.println();
  }
  DEBUG_PORT.println("\nScan complete.");
}
//***************** TOUCH SETUP 
bool Touchsetup() {  // look for 0x5d (GT911_ADDR1)and setup
  bool result = false;
  // is already started ! using Expander pin definition in setup() Wire.begin(TOUCH_SDA,TOUCH_SCL);                         // start the wire interface
  //FindI2CDevices("- List I2C DEVICES-");delay(200);// for development testing
  //gt911 initialization, must be added, otherwise the touch screen will not be recognized
  //initialization begin
  DEBUG_PORT.println("Setting up Touch device ");
#ifdef WAVSHARE
  DEBUG_PORT.println("Using Expander Port Reset sequence");
  expander.digitalWrite(EX101, HIGH);
  expander.digitalWrite(EX102, HIGH);
  expander.digitalWrite(EX104, HIGH);
  pinMode(TOUCH_INT, OUTPUT);    //
  digitalWrite(TOUCH_INT, LOW);  // Step 1 LOW =set to  I2C address 0x5D
  delay(100);
 // expander.digitalWrite(EX103, LOW);
  expander.digitalWrite(EX101, LOW);  // Step 2: Pull RST LOW to begin reset
  delay(100);
  expander.digitalWrite(EX101, HIGH);
 // expander.digitalWrite(EX103, HIGH);  // Step 3: Release RST HIGH while keeping INT LOW
  delayMicroseconds(100);              // ≥100 µs
  pinMode(TOUCH_INT, INPUT);           // Step 4: Float INT pin (input mode)
                                       // Step 5: Wait for GT911 to boot
  delay(100);                          // 5–10 ms
#endif
  //initialization end
  DEBUG_PORT.printf("Checking for Touch chip at I2C address:%X ",GT911_ADDR1);
  Wire.beginTransmission(GT911_ADDR1);
  byte error = Wire.endTransmission();
  if (error == 0) {
    result = true;
    // prints later!
  } else if (error == 2) {
    DEBUG_PORT.println("Received NACK on transmit of address.");
  } else if (error == 3) {
    DEBUG_PORT.println("Received NACK on transmit of data.");
  } else if (error == 4) {
    DEBUG_PORT.println("Other error.");
  } else {
    DEBUG_PORT.println("No device found.");
  }
  delay(10);  // allow time to print!
               // Optional: Check INT pin state
               // if (digitalRead(TOUCH_INT) == LOW) {
               //   DEBUG_PORT.println("GT911 ready (INT LOW)");
               // } else {
               //   DEBUG_PORT.println("GT911 not ready (INT HIGH)");
               // }
  if (result) {
    DEBUG_PORT.println("Device found: begin");
    ts.begin(GT911_ADDR1);  // remove address for wavshare? 
    delay(100);
    /*#define ROTATION_LEFT      (uint8_t)0
#define ROTATION_INVERTED  (uint8_t)1
#define ROTATION_RIGHT     (uint8_t)2
#define ROTATION_NORMAL    (uint8_t)3*/
    ts.setRotation(TOUCH_ROTATION);
    //ts.setRotation(ROTATION_INVERTED);

  }
 if (result) {DEBUG_PORT.println("TOUCH setup OK");
      } else {DEBUG_PORT.println("* NO TOUCH");
    }
  return result;
}


//*****************  TIME UPDATE (LOCAL ONLY: IF GPS FAILS GPS time will not update)

void timeupdate() {
  static unsigned long tick;
  while ((millis() >= tick)) {
    tick = tick + 1000;  // not millis or you can get 'slip'
    BoatData.LOCTime = BoatData.LOCTime + 1;
  }
}


void CheckAndUseInputs() {  //multiinput capable, will check serial /wifi sources in sequence
  static unsigned long MAXScanInterval;
  MAXScanInterval = millis() + 500;
  // DEBUG_PORT.printf(" C&U<%i>",millis()-Interval);Interval=millis();
  if ((Current_Settings.ESP_NOW_ON)) {  // ESP_now can work even if not actually 'connected', so for now, do not risk the while loop!
                                        // old.. only did one line of nmea_EXT..
                                        // if (nmea_EXT[0] != 0) { UseNMEA(nmea_EXT, 3); }
    // while (Test_ESP_NOW() && (millis() <= MAXScanInterval)) {
    //   UseNMEA(nmea_EXT, 3);
    //   // runs multiple times to clear the buffer.. use delay to allow other things to work.. print to show if this is the cause of start delays while debugging!
    //   vTaskDelay(1);
    // }
    if (Test_ESP_NOW()) {UseNMEA(nmea_EXT, 3);}
  }
  // DEBUG_PORT.printf(" ca<%i>",millis()-Interval);Interval=millis();
  //N2K is directly converted to display structures  only use for debugging
  // if (Current_Settings.N2K_ON) {
  //   if (NewN2Kdata()) { UseNMEA(nmea_N2K, 5); } // just for debug!!
  // }
  // // DEBUG_PORT.printf(" cb<%i>",millis()-Interval);Interval=millis();
  if (Current_Settings.UDP_ON) {
    if (Test_U()) { UseNMEA(nmea_U, 2); }
  }
  //DEBUG_PORT.printf(" cd<%i>",millis()-Interval);Interval=millis();

  // For printing Victrn debug messages 
  if (Current_Settings.BLE_enable) {
    if (VictronBuffer[0] != 0) { UseNMEA(VictronBuffer, 4); }
  }
  // DEBUG_PORT.printf(" ce<%i>\n",millis()-Interval);Interval=millis();
}

void UseNMEA(char* buf, int type) {
  if (buf[0] != 0) {
    // print serial version if on the wifi page terminal window page.
    // data log raw NMEA and when and where it came from.
    // type 4 is Victron data
    /*TIME: %02i:%02i:%02i",
                      int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60,*/
    if (Current_Settings.Data_Log_ON) {
      if (BoatData.GPSTime != NMEA0183DoubleNA) {
        if (type == 1) { DATA_Log(FFat," %02i:%02i:%02i UTC: SER:%s", int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60, buf); }
        if (type == 2) { DATA_Log(FFat," %02i:%02i:%02i UTC: UDP:%s", int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60, buf); }
        if (type == 3) { DATA_Log(FFat," %02i:%02i:%02i UTC: ESP:%s", int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60, buf); }
        if (type == 4) { DATA_Log(FFat,"\n%.3f BLE: Victron:%s", float(millis()) / 1000, buf); }
      } else {
        if (type == 1) { DATA_Log(FFat,"%.3f SER:%s", float(millis()) / 1000, buf); }
        if (type == 2) { DATA_Log(FFat,"%.3f UDP:%s", float(millis()) / 1000, buf); }
        if (type == 3) { DATA_Log(FFat,"%.3f ESP:%s", float(millis()) / 1000, buf); }
        if (type == 4) { DATA_Log(FFat,"\n %.3f VIC:%s", float(millis()) / 1000, buf); }
      }
    }
    // 8 is snasBold8pt small font and seems to wrap to give a space before the second line
    if ((Display_Page == -86)) {  //Terminal.debugpause built into in UpdateLinef as part of button characteristics
      if (type == 4) {UpdateLinef(BLACK, 8, Terminal, "%s", buf);}  //8 readable ! 7 small enough to avoid line wrap issue?
    }

    if ((Display_Page == -21)) {  //Terminal.debugpause built into in UpdateLinef as part of button characteristics
      if (type == 4) {UpdateLinef(BLACK, 8, Terminal, "Victron:%s", buf);} // 7 small enough to avoid line wrap issue?
      if (type == 2) {UpdateLinef(BLUE, 8, Terminal, "UDP:%s", buf);}// 7 small enough to avoid line wrap issue?
      if (type == 3) {UpdateLinef(RED, 8, Terminal, "ESP:%s", buf);}
      if (type == 1) { UpdateLinef(GREEN, 8, Terminal, "Ser:%s", buf);}
    }
    // now decode it for the displays to use
    if (ColorSettings.SerialOUT) {DEBUG_PORT.println(buf);}

    if (type != 4) { // Now process the NMEA0183 messages into BoatData so we can use the data. (N2K is dealt with elsewhere!)
      pTOKEN = buf;                                               // pToken is used in processPacket to separate out the Data Fields
      if (processPacket(buf, BoatData)) { dataUpdated = true; };  // NOTE processPacket will search for CR! so do not remove it and then do page updates if true ?
    }
    /// WILL NEED new process packet equivalent to deal with VICTRON data
    buf[0] = 0;  //clear buf  when finished!
    return;
  }
}

bool Test_Serial_1() {  // UART0 port P1
  static bool LineReading_1 = false;
  static int Skip_1 = 1;
  static int i_1;
  static bool line_1;  //has found a full line!
  unsigned char b;
  if (!line_1) {                  // ONLY get characters if we are NOT still processing the last line message!
    while (Serial.available()) {  // get the character
      b = Serial.read();
      if (LineReading_1 == false) {
        nmea_1[0] = b;
        i_1 = 1;
        LineReading_1 = true;
      }  // Place first character of line in buffer location [0]
      else {
        nmea_1[i_1] = b;
        i_1 = i_1 + 1;
        if (b == 0x0A) {       //0A is LF
          nmea_1[i_1] = 0x00;  // put end in buffer.
          LineReading_1 = false;
          line_1 = true;
          return true;
        }
        if (i_1 > 150) {
          LineReading_1 = false;
          i_1 = 0;
          line_1 = false;
          return false;
        }
      }
    }
  }
  line_1 = false;
  return false;
}

bool Test_U() {  // check if udp packet (UDP is sent in lines..) has arrived
  static int Skip_U = 1;
  // if (!line_U) {  // only process if we have dealt with the last line.
  nmea_U[0] = 0x00;
  int packetSize = Udp.parsePacket();
  if (packetSize) {  // Deal with UDP packet
    if (packetSize >= (BufferLength)) {
#if ESP_ARDUINO_VERSION_MAJOR == 3
      Udp.clear();
#else
      Udp.flush();
#endif
      return false;
    }  // Simply discard if too long
    int len = Udp.read(nmea_U, BufferLength);
    unsigned char b = nmea_U[0];
    nmea_U[len] = 0;
    // nmea_UpacketSize = packetSize;
    //DEBUG_PORT.print(nmea_U);
    //line_U = true;
    return true;
  }  // udp PACKET DEALT WITH
     // }
  return false;
}



void beep(int num, int beepPin) {
#ifdef WAVSHARE
  for (int i = 1; i <= num; i++) {
    expander.digitalWrite(beepPin, HIGH);  //Buzzer ON! (confirms Expander is set up)
    delay(50);
    expander.digitalWrite(beepPin, LOW);  //Buzzer off!
    delay(150);
  }
  delay(300);  // allows cadence to differentiate beep4 from beep2 beep2
#endif
}



bool TestFATSPartition() {
  if (ffat_partition) {
    DEBUG_PORT.print("FFat partition found.");
    DEBUG_PORT.printf(" Size: %d bytes\n", ffat_partition->size);
    return true;
  }
  DEBUG_PORT.println("No FFat partition found.");

  return false;
}
bool TestFATSFormatted() {
  uint8_t sector[512];
  esp_err_t err = esp_partition_read(ffat_partition, 0, sector, sizeof(sector));
  if (err == ESP_OK && sector[510] == 0x55 && sector[511] == 0xAA) {
    DEBUG_PORT.println(" and likely valid FAT filesystem present.");
    return true;
  }
  DEBUG_PORT.println("No valid FAT signature found.");
  return false;
}


//***************** Functions added to try and explore why the Serial port is not active sometimes should send on UDP -when connected!

void UDPSEND(const char* buf) {  // Send UDP on AP and ST (if connected) the one that saves repetitious code!
  if (IsConnected) {
    Udp.beginPacket(udp_st, atoi(Current_Settings.UDP_PORT));
    Udp.print(buf);
    Udp.endPacket();
  }
  Udp.beginPacket(udp_ap, atoi(Current_Settings.UDP_PORT));
  Udp.print(buf);
  Udp.endPacket();
}

void Debugf(const char* fmt, ...) {  //complete object type suitable for holding the information needed by the macros va_start, va_copy, va_arg, and va_end.
  if (!EspNowIsRunning) { return; }
  static char msg[BufferLength] = { '\0' };  // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  // add checksum?
  int len = strlen(msg);
  UDPSEND(msg);
}

IPAddress Get_UDP_IP(IPAddress ip, IPAddress mk) {
  uint16_t ip_h = ((ip[0] << 8) | ip[1]);  // high 2 bytes
  uint16_t ip_l = ((ip[2] << 8) | ip[3]);  // low 2 bytes
  uint16_t mk_h = ((mk[0] << 8) | mk[1]);  // high 2 bytes
  uint16_t mk_l = ((mk[2] << 8) | mk[3]);  // low 2 bytes
  // reverse the mask
  mk_h = ~mk_h;
  mk_l = ~mk_l;
  // bitwise OR the net IP with the reversed mask
  ip_h = ip_h | mk_h;
  ip_l = ip_l | mk_l;
  // ip to return the result
  ip[0] = highByte(ip_h);
  ip[1] = lowByte(ip_h);
  ip[2] = highByte(ip_l);
  ip[3] = lowByte(ip_l);
  return ip;
}

//************ TIMING FUNCTIONS FOR TESTING PURPOSES ONLY ******************
//Note this is also an example of how useful Function overloading can be!! 
void EventTiming(String input ){
  EventTiming(input,1); // 1 should be ignored as this is start or stop! but will also give immediate print!
}

void EventTiming(String input, int number){//.. prints when string !=start or Stop! so print can be somewhere else Event timing, Usage START, STOP , 'Descrption text'   Number waits for the Nth call before serial.printing results (Description text + results).  
  static unsigned long Start_time;
  static unsigned long timedInterval;
  static unsigned long _MaxInterval;
  static unsigned long SUMTotal;
  static int calls = 0;  static int reads = 0;
  long NOW=micros();
  if (input == "START") { Start_time = NOW; return ;}
  if (input == "STOP") {timedInterval= NOW- Start_time; SUMTotal=SUMTotal+timedInterval;
                       if (timedInterval >= _MaxInterval){_MaxInterval=timedInterval;}
                       reads++;  
                       return; }
  calls++; 
  if (calls < number){return;}
  if (reads>=2){ 
   // if (OutOfSetup) { SetSerialFor_115200();}
    if (calls >= 1) {DEBUG_PORT.print("\r\n TIMING ");DEBUG_PORT.print(input); DEBUG_PORT.print(" Using ("); DEBUG_PORT.print(reads);DEBUG_PORT.print(") Samples"); 
        DEBUG_PORT.print(" last: ");DEBUG_PORT.print(timedInterval);
        DEBUG_PORT.print("us Average: ");DEBUG_PORT.print(SUMTotal/reads);
        DEBUG_PORT.print("us  Max: ");DEBUG_PORT.print(_MaxInterval);DEBUG_PORT.print("uS ="); DEBUG_PORT.print(_MaxInterval/1000);DEBUG_PORT.println("mS");delay(100);}
    else {DEBUG_PORT.print("\r\n TIMED ");DEBUG_PORT.print(input); DEBUG_PORT.print(" was :");DEBUG_PORT.print(timedInterval);DEBUG_PORT.println("uS");delay(100);}
    _MaxInterval=0;SUMTotal=0;reads=0; calls=0;
  //  if (OutOfSetup) { SetSerialFor_DefaultBaud(); }
  }
}

