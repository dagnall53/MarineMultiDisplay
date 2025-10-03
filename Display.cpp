#include "Display.h"
#include "aux_functions.h"
#include <WiFi.h>
#include <Arduino_GFX_Library.h>  // defines colours BLUE etc
#include <FFat.h>                 // defines FATFS functions for local files
#include <N2kMessages.h>          // isNKNA
#include "Keyboard.h"             // for keyboard functions
#include "JpegFunc.h"
#include "VICTRONBLE.h"  //sets #ifndef Victronble_h
#include "src/MarinePageGFX.h"  // Double-buffered graphics
#include "CanvasBridge.h"
#include "FontType.h"
#include "Structures.h"

extern MarinePageGFX* page;
#include "Globals.h"


#include "debug_port.h"



extern bool WIFIGFXBoxdisplaystarted;
extern bool _WideDisplay;
extern _sWiFi_settings_Config Current_Settings;
extern _sDisplay_Config Display_Config;
extern _sWiFi_settings_Config Saved_Settings;
extern _sBoatData BoatData;  // BoatData values, int double etc
extern _sMyVictronDevices victronDevices;
extern _MyColors ColorSettings;
extern const char* Setupfilename;

extern void SaveConfiguration();
extern bool LoadConfiguration();  // replacement for EEPROM READ
extern bool ScanAndConnect(bool display,bool forceFull);

extern void ShowToplinesettings(String Text);
extern void ShowToplinesettings(_sWiFi_settings_Config A, String Text);
extern char* LongtoString(double data);
extern char* LattoString(double data);

extern void DATA_Log_File_Create(fs::FS &fs);


extern boolean CompStruct(_sWiFi_settings_Config A, _sWiFi_settings_Config B);
extern boolean IsConnected;        // may be used in AP_AND_STA to flag connection success (got IP)
extern boolean AttemptingConnect;  // to note that WIFI.begin has been started
extern int NetworksFound;

#include "src/TAMC_GT911.h"
extern TAMC_GT911 ts;
extern bool Touch_available;

void Display(int pageIndex) {
  DEBUG_PORT.printf("IN oVERLAY void Display page<%i> \n",pageIndex);
  Display(false, pageIndex);
}

void Display(bool reset, int pageIndex) {  // setups for alternate pages to be selected by pageIndex.
 // DEBUG_PORT.printf("IN void Display page<%i> reset:%s \n",pageIndex,reset True_False);delay(50);  // Give UART time to flush

  static unsigned long flashinterval;
  static bool flash;
  static double startposlat, startposlon;
  double LatD, LongD;  //deltas
  double wind_gnd;
  int magnification, h, v;

  static int LastPageselected;
  static bool DataChanged;
  static int wifissidpointer;
  // some local variables for tests;
  //char blank[] = "null";
  static int SwipeTestLR, SwipeTestUD, volume;
  static bool RunSetup;
  static unsigned int slowdown, timer2;
  //static float wind, SOG, Depth;
  float temp;
  static _sInstData LocalCopy;  // needed only where two digital displays wanted for the same data variable.
  static int fontlocal;
  static int FileIndex, Playing;  // static to hold after selection and before pressing play!
  static int V_offset;            // used in the audio file selection to sort print area
  char Tempchar[30];
  //String tempstring;
  // int FS = 1;  // for font size test
  // int tempint;
  //DEBUG_PORT.println("IN Display_Page");
  if (pageIndex != LastPageselected) {
    WIFIGFXBoxdisplaystarted = false;  // will have reset the screen, so turn off the auto wifibox blanking if there was a wifiiterrupt box
                                       // this (above) saves a timed screen refresh that can clear keyboard stuff
   // DEBUG_PORT.println("IN Display_Page.. load config (Different page)");
    LoadConfiguration();               //Reload configuration in case new data stored
    RunSetup = true;
  }
  if (reset) {
    WIFIGFXBoxdisplaystarted = false;
 //   DEBUG_PORT.println("IN Display_Page.. load config (reset)");
    LoadConfiguration();  //Reload configuration in case new data stored
    page->clearCanvas(BLUE);
    RunSetup = true;
  }
  //generic setup stuff for ALL pages
 //page->GFXBorderBoxPrintf(StatusBox, "%s Page%i Display monitor %i",Display_Config.PanelName,pageIndex, millis()/100);  // common to all pages
  if (RunSetup) {
    DEBUG_PORT.println("IN Display_Page.. Runsetup (page sets..)");
    page->clearCanvas(BLUE);
    page->setTextColor(WHITE);
   }

  // if ((millis() >= flashinterval)) {
  //   flashinterval = millis() + 1000;
  //   StatusBox.PrintLine = 0;  // always start / only use / the top line 0  of this box
  // page->UpdateLinef(3, StatusBox, "%s pageIndex:%i  BoatLOG %s DATALOG %s  ", Display_Config.PanelName, Display_Page,
  //               Current_Settings.Log_ON On_Off, Current_Settings.Data_Log_ON On_Off);
  //   if (Current_Settings.Log_ON || Current_Settings.Data_Log_ON) {
  //     flash = !flash;
  //     if (!flash) {
  //     page->UpdateLinef(3, StatusBox, "%s pageIndex:%i  BoatLOG     DATALOG      ", Display_Config.PanelName, Display_Page);
  //     }
  //   }
  // }
  // add any other generic stuff here
  if (CheckButton(StatusBox)) { Display_Page = 0; }  // go to settings
  // Now specific stuff for each pageIndex

  switch (pageIndex) {  // A pageIndex just to blank.
    case -99:      // pageIndex for just a blank
      if (RunSetup) {
        page->fillScreen(BLACK);
      }
      page->fillScreen(BLACK);
      break;


    case -200:
      if (RunSetup) {  //logo examples
        showPicture("/logo.jpg");
        // jpegDraw("/logo.jpg", jpegDrawCallback, true /* useBigEndian */,
        //          0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        page->GFXBorderBoxPrintf(Full0Center, "Jpg tests -Return to Menu-");
        page->GFXBorderBoxPrintf(Full1Center, "logo.jpg");
        page->GFXBorderBoxPrintf(Full2Center, "logo1.jpg");
        page->GFXBorderBoxPrintf(Full3Center, "logo2.jpg");
        page->GFXBorderBoxPrintf(Full4Center, "logo4.jpg");
        page->GFXBorderBoxPrintf(Full5Center, "logo5.jpg");
      }

      if (CheckButton(Full0Center)) { Display_Page = 0; }
      if (CheckButton(Full1Center)) {
        jpegDraw("/logo.jpg", jpegDrawCallback, true /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        page->GFXBorderBoxPrintf(Full0Center, "logo");
      }
      if (CheckButton(Full2Center)) {
        jpegDraw("/logo1.jpg", jpegDrawCallback, true /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        page->GFXBorderBoxPrintf(Full0Center, "logo1");
      }
      if (CheckButton(Full3Center)) {
        jpegDraw("/logo2.jpg", jpegDrawCallback, false /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        page->GFXBorderBoxPrintf(Full0Center, "logo2");
      }
      if (CheckButton(Full4Center)) {
        jpegDraw("/logo4.jpg", jpegDrawCallback, true /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        page->GFXBorderBoxPrintf(Full0Center, "logo4");
      }
      if (CheckButton(Full5Center)) {
        jpegDraw("/logo5.jpg", jpegDrawCallback, true /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        page->GFXBorderBoxPrintf(Full0Center, "logo5");
      }

      break;

    case -199:  //a test for Screen Colours / fonts
      if (RunSetup) {
        page->fillScreen(BLACK);
        page->setTextColor(WHITE);
        fontlocal = 0;
        SwipeTestLR = 0;
        SwipeTestUD = 0;
        setFont(fontlocal);
        page->GFXBorderBoxPrintf(CurrentSettingsBox, "-TEST Colours- ");
      }

      if (millis() >= slowdown + 10000) {
        slowdown = millis();
        switch (fontlocal) {
          case 1:
            WifiStatus.BackColor = WHITE;
            page->GFXBorderBoxPrintf(WifiStatus, "White ");
            page->GFXBorderBoxPrintf(CurrentSettingsBox, "White ");
            break;
          case 2:
            WifiStatus.BackColor = BLACK;
            page->GFXBorderBoxPrintf(WifiStatus, "BLACK ");
            page->GFXBorderBoxPrintf(CurrentSettingsBox, "BLACK");
            break;
          case 3:
            WifiStatus.BackColor = BLUE;
            page->GFXBorderBoxPrintf(WifiStatus, "BLUE");
            page->GFXBorderBoxPrintf(CurrentSettingsBox, "BLUE ");
            break;
          case 4:
            WifiStatus.BackColor = RED;
            page->GFXBorderBoxPrintf(WifiStatus, "RED ");
            page->GFXBorderBoxPrintf(CurrentSettingsBox, "RED ");
            break;
          case 5:
            WifiStatus.BackColor = GREEN;
            page->GFXBorderBoxPrintf(WifiStatus, "GREEN ");
            page->GFXBorderBoxPrintf(CurrentSettingsBox, "GREEN ");
            break;
        }
        //gfx->fillScreen(BLACK);
        fontlocal = fontlocal + 1;
        if (fontlocal > 5) { fontlocal = 0; }
      }



      break;

      //************** VICTRON PAGES - different way to displa, Single 'button is altered (V H Height ) for each variable display
    case -87:  // pageIndex for graphic display of Vicron data
      if (RunSetup) {
        // jpegDraw("/vicback.jpg", jpegDrawCallback, true /* useBigEndian */,
        //          0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        // // Serial.println("redrawing background");
                 page->clearCanvas(NEAR_BLACK);
               
      }
      // all graphics done in VICTRONBLE
      if (CheckButton(StatusBox)) { Display_Page = 0; }  // go to settings
      if (CheckButton(FullTopCenter)) { Display_Page = -86; }
      break;
    case -86:                                              // pageIndex for text display of Vicron data
      if (RunSetup) { page->GFXBorderBoxPrintf(Terminal, ""); }  // only for setup, not changed data
      if (RunSetup || DataChanged) {
        setFont(3);  // different from most pages, displays in terminal from see ~line 2145
        page->GFXBorderBoxPrintf(FullTopCenter, "Return to VICTRON graphic display");
        if (!Terminal.debugpause) {
          page->Addtitletobutton(Terminal,1,0, "-running-");
        } else {
          page->Addtitletobutton(Terminal,1,0, "-paused-");
        }
        page->GFXBorderBoxPrintf(Switch6, Current_Settings.Data_Log_ON On_Off);
        page->Addtitletobutton(Switch6,1,0, " LOG ");

        page->GFXBorderBoxPrintf(Switch7, victronDevices.Beacons On_Off);
        page->Addtitletobutton(Switch7,1,0, "Beacons");
        page->GFXBorderBoxPrintf(Switch8, victronDevices.BLEDebug On_Off);
        page->Addtitletobutton(Switch8,1,0, "V-Debug");
        page->GFXBorderBoxPrintf(Switch9, victronDevices.Simulate On_Off);
        page->Addtitletobutton(Switch9,1,0, " Sim.");
        page->GFXBorderBoxPrintf(Switch10, Current_Settings.BLE_enable On_Off);
        page->Addtitletobutton(Switch10,1,0, "BLE-ON");
        page->GFXBorderBoxPrintf(Switch11, ColorSettings.SerialOUT On_Off);
        page->Addtitletobutton(Switch11,1,0, "Copy>serial");
        DataChanged = false;
      }
      // if (millis() > slowdown + 500) {
      //   slowdown = millis();
      // }

      if (CheckButton(Switch6)) {
        Current_Settings.Data_Log_ON = !Current_Settings.Data_Log_ON;
       if (Current_Settings.Data_Log_ON) {DATA_Log_File_Create(FFat); }
        DataChanged = true;
      };
      if (CheckButton(Switch7)) {
        victronDevices.Beacons = !victronDevices.Beacons;
        DataChanged = true;
      };

      if (CheckButton(Switch8)) {
        victronDevices.BLEDebug = !victronDevices.BLEDebug;
        DataChanged = true;
      };
      if (CheckButton(Switch9)) {
        victronDevices.Simulate = !victronDevices.Simulate;
        DataChanged = true;
      };
      if (CheckButton(Switch10)) {
        Current_Settings.BLE_enable = !Current_Settings.BLE_enable;
        DataChanged = true;
      };
            if (CheckButton(Switch11)) {
        ColorSettings.SerialOUT = !ColorSettings.SerialOUT;
        DataChanged = true;
      };





      if (CheckButton(FullTopCenter)) { Display_Page = -87; }
      if (CheckButton(Terminal)) {
        Terminal.debugpause = !Terminal.debugpause;
        DataChanged = true;  // for more immediate visual response to touch!
        if (!Terminal.debugpause) {
          page->Addtitletobutton(Terminal,1,0, "-running-");
        } else {
          page->Addtitletobutton(Terminal,1,0, "-paused-");
        }
      }

      break;

    case -22:                                              //  "EXPERIMENT in N2K data"
      if (RunSetup) { page->GFXBorderBoxPrintf(Terminal, ""); }  // only for setup, not changed data
      if (RunSetup || DataChanged) {
        setFont(3);
        page->GFXBorderBoxPrintf(FullTopCenter, "N2K debug ");
        if (!Terminal.debugpause) {
          page->Addtitletobutton(Terminal,1, 0, "TERMINAL");
        } else {
          page->Addtitletobutton(Terminal,1, 0, "-Paused-");
        }
        DataChanged = false;
      }
      // if (millis() > slowdown + 500) {
      //   slowdown = millis();
      // }
      if (CheckButton(FullTopCenter)) { Display_Page = 0; }
      if (CheckButton(Terminal)) {
        Terminal.debugpause = !Terminal.debugpause;
        DataChanged = true;
        if (!Terminal.debugpause) {
          page->Addtitletobutton(Terminal,1, 0, "-running-");
        } else {
          page->Addtitletobutton(Terminal,1, 0, "-paused-");
        }
      }
      // if (CheckButton(Switch9)) {
      //   Current_Settings.ESP_NOW_ON = !Current_Settings.ESP_NOW_ON;
      //   DataChanged = true;
      // };
      // if (CheckButton(Switch11)) {
      //    SaveConfiguration();//(Display_Config, Current_Settings);
      //   delay(50);
      //   // Display_Page = 0;
      //   DataChanged = true;
      // };
      break;

    case -20:  // Experimental / extra stuff
      if (RunSetup || DataChanged) {
        ShowToplinesettings("Now");
        setFont(3);
        setFont(3);
        page->GFXBorderBoxPrintf(Full0Center, "-Test JPegs-");
        page->GFXBorderBoxPrintf(Full1Center, "Check Touch crosshairs");
        page->GFXBorderBoxPrintf(Full2Center, "Check Fonts");
        page->GFXBorderBoxPrintf(Full3Center, "VICTRON devices");
        // page->GFXBorderBoxPrintf(Full3Center, "See NMEA");

        page->GFXBorderBoxPrintf(Full5Center, "Main Menu");
      }
      if (millis() > slowdown + 500) {
        slowdown = millis();
      }
      if (CheckButton(Full0Center)) { Display_Page = -200; }
      if (CheckButton(Full1Center)) { Display_Page = -9; }
      if (CheckButton(Full2Center)) { Display_Page = -10; }
      if (CheckButton(Full3Center)) { Display_Page = -87; }  // V for Victron go straight to graphic display
      //   if (CheckButton(Full4Center)) { Display_Page = -10; }
      if (CheckButton(Full5Center)) { Display_Page = 0; }
      break;

    case -21:  //  "Log and debug "
      if (RunSetup) {
        Terminal.debugpause = false;
         page->clearCanvas(BLUE);
         page->GFXBorderBoxPrintf(Terminal, "will fill with data");
      }  // only for setup, not changed data
      if (RunSetup || DataChanged) {
        setFont(3);
        page->GFXBorderBoxPrintf(FullTopCenter, "Boat/NMEA  Source selects");
        //if (hasSD) {page->GFXBorderBoxPrintf(Switch6, Current_Settings.Log_ON On_Off);
        // page->Addtitletobutton(Switch6, "B LOG");
        // page->GFXBorderBoxPrintf(Switch7, Current_Settings.Data_Log_ON On_Off);
        // page->Addtitletobutton(Switch7, "N LOG");}
        page->GFXBorderBoxPrintf(Switch8, Current_Settings.UDP_ON On_Off);
        page->Addtitletobutton(Switch8,1,0, "UDP");
        page->GFXBorderBoxPrintf(Switch9, Current_Settings.ESP_NOW_ON On_Off);
        page->Addtitletobutton(Switch9,1,0, "ESP-N");
        page->GFXBorderBoxPrintf(Switch10, Current_Settings.N2K_ON On_Off);
        page->Addtitletobutton(Switch10,1,0, "N2K");
        if (!Terminal.debugpause) {
          page->Addtitletobutton(Terminal,1,0, "TERMINAL");
        } else {
          page->Addtitletobutton(Terminal,1,0, "-Paused-");
        }
        DataChanged = false;
      }
      if (millis() > slowdown + 1000) {
        slowdown = millis();
        page->GFXBorderBoxPrintf(Switch11, CompStruct(Saved_Settings, Current_Settings) ? "-same-" : "UPDATE?");
        page->Addtitletobutton(Switch11,1,0, "FLASH");
      }
      if (CheckButton(FullTopCenter)) { Display_Page = 0; }
      if (CheckButton(Terminal)) {
        Terminal.debugpause = !Terminal.debugpause;
        DataChanged = true;
        if (!Terminal.debugpause) {
          page->Addtitletobutton(Terminal,1,0, "-running-");
        } else {
          page->Addtitletobutton(Terminal,1,0, "-paused-");
        }
      }
      // if (CheckButton(Switch6) && hasSD) {
      //   Current_Settings.Log_ON = !Current_Settings.Log_ON;
      //  // NO LOGGING YET if (Current_Settings.Log_ON) { StartInstlogfile(SD); }
      //   DataChanged = true;
      // };

      // if (CheckButton(Switch7) && hasSD) {
      //   Current_Settings.Data_Log_ON = !Current_Settings.Data_Log_ON;
      //  //  NO LOGGING YET if (Current_Settings.Data_Log_ON) { DATA_Log_File_Create(SD); }
      //   DataChanged = true;
      // };

      if (CheckButton(Switch8)) {
        Current_Settings.UDP_ON = !Current_Settings.UDP_ON;
        DataChanged = true;
      };
      if (CheckButton(Switch9)) {
        Current_Settings.ESP_NOW_ON = !Current_Settings.ESP_NOW_ON;
        DataChanged = true;
      };
      if (CheckButton(Switch10)) {
        Current_Settings.N2K_ON = !Current_Settings.N2K_ON;
        DataChanged = true;
      };
      if (CheckButton(Switch11)) {
        SaveConfiguration();  //(Display_Config, Current_Settings);
        delay(50);
        // Display_Page = 0;
        DataChanged = true;
      };



      break;

    case -10:  // a test pageIndex for fonts
      if (RunSetup || DataChanged) {
        page->fillScreen(BLUE);
        setFont(3);
        // page->GFXBorderBoxPrintf(Full0Center, "-Font test -");
        page->GFXBorderBoxPrintf(BottomLeftbutton, "Smaller");
        page->GFXBorderBoxPrintf(BottomRightbutton, "Larger");
      }
      // if (millis() > slowdown + 5000) {
      //   slowdown = millis();
      //  page->fillScreen(BLUE);
      //   // fontlocal = fontlocal + 1;
      //   // if (fontlocal > 15) { fontlocal = 0; } // just use the buttons to change!
      //   temp = 12.3;
      //   setFont(fontlocal);
      // }
      if (millis() > timer2 + 500) {
        timer2 = millis();
        temp = random(-9000, 9000);
        temp = temp / 1000;
        setFont(fontlocal);
        Fontname.toCharArray(Tempchar, 30, 0);
        int FontHt;
        setFont(fontlocal);
        FontHt = text_height;
        setFont(3);
        page->GFXBorderBoxPrintf(CurrentSettingsBox, "FONT:%i name%s height<%i>", fontlocal, Tempchar, FontHt);
        setFont(fontlocal);
        page->GFXBorderBoxPrintf(FontBox, "Test %4.2f", temp);
        DataChanged = false;
      }
      if (CheckButton(Full0Center)) { Display_Page = 0; }

      if (CheckButton(BottomLeftbutton)) {
        fontlocal = fontlocal - 1;
        DataChanged = true;
      }
      if (CheckButton(BottomRightbutton)) {
        fontlocal = fontlocal + 1;
        DataChanged = true;
      }
      break;
    case -9:  ///Touchscreen pointer tests
      if (RunSetup || DataChanged) {
        setFont(4);
        DataChanged = false;
      }
      if (millis() > slowdown + 1000) {
        slowdown = millis();
        //other timed stuff?
      }
      if (Touch_available) {
        if ((ts.isTouched) && (ts.points[0].y >= 200)) {  // nb check on location on screen or it will get reset when you press one of the boxes
          TouchCrosshair(10);
          // DEBUG_PORT.printf(" Pressure test %i  %i %i \n",ts.points[0].size,ts.points[0].x, ts.points[0].y);
        }
      }
      break;

    case -5:  ///Wifiscan

      if (RunSetup || DataChanged) {
        setFont(4);
        if (IsConnected) {
          page->GFXBorderBoxPrintf(TOPButton, "Connected<%s>", Current_Settings.ssid);
        } else {
          page->GFXBorderBoxPrintf(TOPButton, "NOT FOUND:%s", Current_Settings.ssid);
        }
        page->GFXBorderBoxPrintf(TopRightbutton, "Scan");
        page->GFXBorderBoxPrintf(SecondRowButton, " Saved results");
        page->setCursor(0, 140);  //(location of terminal. make better later!)

        if (NetworksFound <= 0) {  // note scan error can give negative number
          NetworksFound = 0;
          page->GFXBorderBoxPrintf(SecondRowButton, " Use keyboard ");
        } else {
          page->GFXBorderBoxPrintf(SecondRowButton, " %i Networks Found", NetworksFound);
          page->fillRect(0, 200, 480, 280, BLUE);  // clear the place wherethe wifi wil be printed
          for (int i = 0; i < NetworksFound; ++i) {
            page->setCursor(0, 200 + ((i + 1) * text_height));
            // Print SSID and RSSI for each network found
            
            if (WiFi.SSID(i).length() > 20) { page->printf("..(toolong).."); }
            page->printf("%i %s ",i + 1,WiFi.SSID(i).substring(0, 20));
           // if (WiFi.SSID(i).length() > 20) { page->printf(".."); }
           // Serial.print(WiFi.SSID(i));
           // Serial.println(WiFi.channel(i));
            page->printf(" (%i) ch<%i> \n",WiFi.RSSI(i),WiFi.channel(i));

           // page->println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
            delay(10);
          }
        }
        DataChanged = false;
      }

      if (millis() > slowdown + 1000) {
        slowdown = millis();
        //other stuff?
        page->GFXBorderBoxPrintf(Switch4a, CompStruct(Saved_Settings, Current_Settings) ? "-same-" : "UPDATE");
        page->Addtitletobutton(Switch4a,1,0, "EEPROM");
      }
      if (Touch_available) {
        if ((ts.isTouched) && (ts.points[0].y >= 200)) {  // nb check on location on screen or it will get reset when you press one of the boxes
          //TouchCrosshair(1);
          wifissidpointer = ((ts.points[0].y - 200) / text_height) - 1;
          int str_len = WiFi.SSID(wifissidpointer).length() + 1;
          char result[str_len];
          //  Serial.printf(" touched at %i  equates to %i ? %s ", ts.points[0].y, wifissidpointer, WiFi.SSID(wifissidpointer));
          //  Serial.printf("  result str_len%i   sizeof settings.ssid%i \n", str_len, sizeof(Current_Settings.ssid));
          if (str_len <= sizeof(Current_Settings.ssid)) {                                       // check small enough for our ssid register array!
            WiFi.SSID(wifissidpointer).toCharArray(result, sizeof(Current_Settings.ssid) - 1);  // I like to keep a spare space!
            if (str_len == 1) {
              page->GFXBorderBoxPrintf(SecondRowButton, "Set via Keyboard?");
            } else {
              page->GFXBorderBoxPrintf(SecondRowButton, "Select<%s>?", result);
            }
          } else {
            page->GFXBorderBoxPrintf(SecondRowButton, "ssid too long ");
          }
        }
      }
      if (CheckButton(TopRightbutton)) {
        page->GFXBorderBoxPrintf(SecondRowButton, " Starting WiFi re-SCAN / reconnect ");
        AttemptingConnect = false;  // so that Scan can do a full scan..
        ScanAndConnect(false,true);
        AttemptingConnect = true; // so scanandconnect in main loop does not run again! (updates networks and makes screen wrong!)
        DataChanged = true;
      }  // do the scan again

      if (CheckButton(Switch4a)) {
        SaveConfiguration();  //(Display_Config, Current_Settings);
        delay(50);
        // Display_Page = 0;
        DataChanged = true;
      };
      if (CheckButton(SecondRowButton)) {
        // Serial.printf(" * Debug wifissidpointer=%i \n",wifissidpointer);
        if ((NetworksFound >= 1) && (wifissidpointer <= NetworksFound)) {
            WiFi.SSID(wifissidpointer).toCharArray(Current_Settings.ssid, sizeof(Current_Settings.ssid) - 1);
          //   Serial.printf("Update ssid to <%s> \n", Current_Settings.ssid);
        } else {
          Serial.println("Update ssid via keyboard");
          Display_Page = -2;
        }
      }
      if (CheckButton(TOPButton)) { Display_Page = -1; }
      break;

    case -4:  // Keyboard setting of UDP port - note keyboard (2) numbers start
      if (RunSetup) {
        setFont(4);
        page->GFXBorderBoxPrintf(TOPButton, "Current <%s>", Current_Settings.UDP_PORT);
        page->GFXBorderBoxPrintf(Full0Center, "Set UDP PORT");
        keyboard(-1);  //reset
        keyboard(2);
        //Use_Keyboard(blank, sizeof(blank));
        Use_Keyboard(Current_Settings.UDP_PORT, sizeof(Current_Settings.UDP_PORT));
      }
      Use_Keyboard(Current_Settings.UDP_PORT, sizeof(Current_Settings.UDP_PORT));
      if (CheckButton(TOPButton)) { Display_Page = -1; }
      break;

    case -3:  // keyboard setting of Password
      if (RunSetup) {
        setFont(4);
        page->GFXBorderBoxPrintf(TOPButton, "Current <%s>", Current_Settings.password);
        page->GFXBorderBoxPrintf(Full0Center, "Set Password");
        keyboard(-1);  //reset
        Use_Keyboard(Current_Settings.password, sizeof(Current_Settings.password));
        keyboard(1);
      }
      Use_Keyboard(Current_Settings.password, sizeof(Current_Settings.password));
      if (CheckButton(Full0Center)) { Display_Page = -1; }
      if (CheckButton(TOPButton)) { Display_Page = -1; }

      break;

    case -2:  //Keyboard set of SSID
      if (RunSetup) {
        setFont(4);
        page->GFXBorderBoxPrintf(Full0Center, "Set SSID");
        page->GFXBorderBoxPrintf(TopRightbutton, "Scan");
        page->Addtitletobutton(TopRightbutton,1,0, "WiFi");
        keyboard(-1);  //reset
        Use_Keyboard(Current_Settings.ssid, sizeof(Current_Settings.ssid));
        keyboard(1);
      }

      Use_Keyboard(Current_Settings.ssid, sizeof(Current_Settings.ssid));
      if (CheckButton(Full0Center)) { Display_Page = -1; }
      if (CheckButton(TopRightbutton)) { Display_Page = -5; }
      break;

    case -1:  // this is the WIFI settings pageIndex
      if (RunSetup || DataChanged) {
        page->fillScreen(BLACK);
        page->setTextColor(BLACK);
        page->setTextSize(1);
        ShowToplinesettings(Saved_Settings, "EEPROM");
        setFont(4);
        page->GFXBorderBoxPrintf(SecondRowButton, "SSID <%s>", Current_Settings.ssid);
        if (IsConnected) {
          page->Addtitletobutton(SecondRowButton,1,0, "Current Setting <CONNECTED>");
        } else {
          page->Addtitletobutton(SecondRowButton,1,0, "Current Setting <NOT CONNECTED>");
        }
        page->GFXBorderBoxPrintf(ThirdRowButton, "Password <%s>", Current_Settings.password);
        page->Addtitletobutton(ThirdRowButton,1,0, "Current Setting");
        page->GFXBorderBoxPrintf(FourthRowButton, "UDP Port <%s>", Current_Settings.UDP_PORT);
        page->Addtitletobutton(FourthRowButton,1,0, "Current Setting");
        page->GFXBorderBoxPrintf(Switch1, Current_Settings.Serial_on On_Off);  //A.Serial_on On_Off,  A.UDP_ON On_Off, A.ESP_NOW_ON On_Off
        page->Addtitletobutton(Switch1,1,0, "Serial");
        page->GFXBorderBoxPrintf(Switch2, Current_Settings.UDP_ON On_Off);
        page->Addtitletobutton(Switch2,1,0, "UDP");
        page->GFXBorderBoxPrintf(Switch3, Current_Settings.ESP_NOW_ON On_Off);
        page->Addtitletobutton( Switch3,1,0, "ESP-Now");
       // Serial.printf(" Compare Saved and Current <%s> \n", CompStruct(Saved_Settings, Current_Settings) ? "-same-" : "UPDATE");
        page->GFXBorderBoxPrintf(Switch4, CompStruct(Saved_Settings, Current_Settings) ? "-same-" : "UPDATE");
        page->Addtitletobutton(Switch4,1,0,"EEPROM");
        page->GFXBorderBoxPrintf(Full5Center, "Logger and Debug");
        setFont(3);
        DataChanged = false;
      }
      if (millis() > slowdown + 1000) {
        slowdown = millis();
      }
      //runsetup to repopulate the text in the boxes!
      if (CheckButton(Switch1)) {
        Current_Settings.Serial_on = !Current_Settings.Serial_on;
        DataChanged = true;
      };
      if (CheckButton(Switch2)) {
        Current_Settings.UDP_ON = !Current_Settings.UDP_ON;
        DataChanged = true;
      };
      if (CheckButton(Switch3)) {
        Current_Settings.ESP_NOW_ON = !Current_Settings.ESP_NOW_ON;
        DataChanged = true;
      };

      if (CheckButton(Switch4)) {
        SaveConfiguration();  //(Display_Config, Current_Settings);
        delay(50);
        // Display_Page = 0;
        DataChanged = true;
      };

      if (CheckButton(TOPButton)) { Display_Page = 0; }
      //if (CheckButton(Full0Center)) { Display_Page = 0; }
      if (CheckButton(SecondRowButton)) { Display_Page = -5; };
      if (CheckButton(ThirdRowButton)) { Display_Page = -3; };
      if (CheckButton(FourthRowButton)) { Display_Page = -4; };
      if (CheckButton(Full5Center)) { Display_Page = -21; };
      break;

    case 0:  // main settings
      if (RunSetup) {
        ShowToplinesettings("Now");
        setFont(4);
        page->GFXBorderBoxPrintf(Full0Center, "-Experimental-");
        page->GFXBorderBoxPrintf(Full1Center, "WIFI Settings");
        page->GFXBorderBoxPrintf(Full2Center, "NMEA DISPLAY");
        page->GFXBorderBoxPrintf(Full3Center, "Debug + LOG");
        page->GFXBorderBoxPrintf(Full4Center, "GPS Display");
        page->GFXBorderBoxPrintf(Full5Center, "Victron Data Display");
        page->GFXBorderBoxPrintf(Full6Center, "Save / Reset ");
      }
      if (millis() > slowdown + 500) {
        slowdown = millis();
      }
      if (CheckButton(Full0Center)) { Display_Page = -20; }
      if (CheckButton(Full1Center)) { Display_Page = -1; }
      if (CheckButton(Full2Center)) { Display_Page = 4; }
      if (CheckButton(Full3Center)) { Display_Page = -21; }
      if (CheckButton(Full4Center)) { Display_Page = 9; }
      if (CheckButton(Full5Center)) { Display_Page = -86; }
      if (CheckButton(Full6Center)) {
        //Display_Page = 4;
        SaveConfiguration();  //(Display_Config, Current_Settings);
        delay(50);
        //    WiFi.disconnect();
        ESP.restart();
      }
      break;

    case 4:  // Quad display
      if (RunSetup) {
        setFont(10);
        page->fillScreen(BLACK);
        page->GFXBorderBoxPrintf(topLeftquarter, "will fill with data");
        page->GFXBorderBoxPrintf(topRightquarter, "will fill with data");
        page->GFXBorderBoxPrintf(bottomRightquarter, "will fill with data");
        page->GFXBorderBoxPrintf(bottomLeftquarter, "will fill with data");
         if (_WideDisplay) {page->GFXBorderBoxPrintf(WideScreenCentral,"will fill with data");}
      }
      ButtonDataSelect(topLeftquarter, 0, Display_Config.FourWayTL, RunSetup);
      ButtonDataSelect(topRightquarter, 1, Display_Config.FourWayTR, RunSetup);
      ButtonDataSelect(bottomRightquarter, 2, Display_Config.FourWayBR, RunSetup);
      ButtonDataSelect(bottomLeftquarter, 3, Display_Config.FourWayBL, RunSetup);
      if (_WideDisplay) { ButtonDataSelect(WideScreenCentral, 4, Display_Config.WideScreenCentral, RunSetup); }

      if (CheckButton(topLeftquarter)) { Display_Page = 20; }     //stw
      if (CheckButton(bottomLeftquarter)) { Display_Page = 23; }  //depth
      if (CheckButton(topRightquarter)) { Display_Page = 21; }    // Wind
      if (CheckButton(bottomRightquarter)) { Display_Page = 22; }
      if (_WideDisplay) {
        if (CheckButton(WideScreenCentral)) { Display_Page = 24; }
      }

      break;
    case 20:  // full screen display (new)
      ButtonDataSelect(FullScreen, 0, Display_Config.FourWayTL, RunSetup);
      if (CheckButton(BigSingleDisplay)) { Display_Page = 4; }  // option for later adding buttons top left / right
      break;
    case 21:  // full screen display (new)
      ButtonDataSelect(FullScreen, 1, Display_Config.FourWayTR, RunSetup);
      if (CheckButton(BigSingleDisplay)) { Display_Page = 4; }
      break;
    case 22:  // full screen display (new)
      ButtonDataSelect(FullScreen, 2, Display_Config.FourWayBR, RunSetup);
      if (CheckButton(BigSingleDisplay)) { Display_Page = 4; }
      break;
    case 23:  // full screen display (new)
      ButtonDataSelect(FullScreen, 3, Display_Config.FourWayBL, RunSetup);
      if (CheckButton(BigSingleDisplay)) { Display_Page = 4; }
      break;
    case 24:  // full screen display (new)
      if (!_WideDisplay) { return; }
      ButtonDataSelect(FullScreen, 4, Display_Config.WideScreenCentral, RunSetup);
      if (CheckButton(BigSingleDisplay)) { Display_Page = 4; }
      break;
    case 9:  // GPS pageIndex
      if (RunSetup) {
        setFont(8);
        page->GFXBorderBoxPrintf(BigSingleDisplay, "");
        page->GFXBorderBoxPrintf(TopHalfBigSingleTopRight, "");
        page->GFXBorderBoxPrintf(BottomHalfBigSingleTopRight, "");
        page->GFXBorderBoxPrintf(BigSingleTopLeft, "Click for graphic");
        setFont(10);
      }
  page->BorderPrintCanvasTwoSize(TopHalfBigSingleTopRight, 154, "%.1f",BoatData.SOG.data);
  page->AddTitleInsideBox(TopHalfBigSingleTopRight, 6, 8, " SOG ");
   page->BorderPrintCanvasTwoSize(BottomHalfBigSingleTopRight,154, "%.1f",BoatData.COG.data);
  page->AddTitleInsideBox(BottomHalfBigSingleTopRight, 6, 8, " COG ");

      if (millis() > slowdown + 1000) {
        slowdown = millis();
        page->GFXBorderBoxPrintf(BigSingleDisplay, "");
        // do this one once a second.. I have not yet got simplified functions testing if previously displayed and greyed yet
        page->setTextColor(BigSingleDisplay.TextColor);
        BigSingleDisplay.PrintLine = 0;
        if (BoatData.SatsInView != NMEA0183DoubleNA) { page->UpdateLinef(8, BigSingleDisplay, "Satellites in view %.0f ", BoatData.SatsInView); }
        if (BoatData.GPSTime != NMEA0183DoubleNA) {
        page->UpdateLinef(9, BigSingleDisplay, "\n");
        page->UpdateLinef(9, BigSingleDisplay, "Date: %06i \n", int(BoatData.GPSDate));
        page->UpdateLinef(9, BigSingleDisplay, "\n");
        page->UpdateLinef(9, BigSingleDisplay, "TIME: %02i:%02i:%02i\n",
                      int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60);
        }
        if (BoatData.Latitude.data != NMEA0183DoubleNA) {
        page->UpdateLinef(9, BigSingleDisplay, "/n");
        page->UpdateLinef(9, BigSingleDisplay, "LAT %s/n", LattoString(BoatData.Latitude.data));
        page->UpdateLinef(9, BigSingleDisplay, "LON %s/n", LongtoString(BoatData.Longitude.data));
        page->UpdateLinef(9, BigSingleDisplay, "/n");
        }

        if (BoatData.MagHeading.data != NMEA0183DoubleNA) { page->UpdateLinef(9, BigSingleDisplay, "Mag Heading: %.4f\n", BoatData.MagHeading); }
        if ((BoatData.Variation != NMEA0183DoubleNA) && (BoatData.Variation != 0) && !N2kIsNA(BoatData.Variation)) { page->UpdateLinef(9, BigSingleDisplay, "Variation: %.4f\n", BoatData.Variation); }
      }
      if (CheckButton(BigSingleTopLeft)) { Display_Page = 10; }
      //if (CheckButton(bottomLeftquarter)) { Display_Page = 4; }  //Loop to the main settings pageIndex
      break;

    case 10:  // GPS pageIndex 2 sort of anchor watch
      static double magnification;
      if (RunSetup || DataChanged) {
        setFont(8);
        page->GFXBorderBoxPrintf(BigSingleDisplay, "");
        page->GFXBorderBoxPrintf(BigSingleTopLeft, "");
        if (BoatData.GPSTime != NMEA0183DoubleNA) {
          page->UpdateLinef(8, BigSingleTopLeft, "Date: %06i ", int(BoatData.GPSDate));
          page->UpdateLinef(8, BigSingleTopLeft, "TIME: %02i:%02i:%02i",
                      int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60);
        }
        if (BoatData.Latitude.data != NMEA0183DoubleNA) {
          page->UpdateLinef(8, BigSingleTopLeft, "LAT: %f", BoatData.Latitude.data);
          page->UpdateLinef(8, BigSingleTopLeft, "LON: %f", BoatData.Longitude.data);
        }

        page->GFXBorderBoxPrintf(BigSingleTopRight, "Show Quad Display");
        page->GFXBorderBoxPrintf(BottomRightbutton, "Zoom in");
        page->GFXBorderBoxPrintf(BottomLeftbutton, "Zoom out");
        magnification = 1111111;  //reset magnification 11111111 = 10 pixels / m == 18m circle.
        DataChanged = false;
      }
      if (millis() > slowdown + 1000) {
        slowdown = millis();
        // do this one once a second.. I have not yet got simplified functions testing if previously displayed and greyed yet
        ///page->setTextColor(BigSingleDisplay.TextColor);
        BigSingleTopLeft.PrintLine = 0;
       page->UpdateLinef(3,BigSingleTopLeft, "%.0f Satellites in view", BoatData.SatsInView);
        if (BoatData.GPSTime != NMEA0183DoubleNA) {
          page->UpdateLinef(8, BigSingleTopLeft, "Date: %06i ", int(BoatData.GPSDate));
          page->UpdateLinef(8, BigSingleTopLeft, "TIME: %02i:%02i:%02i",
                      int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60);
        }
        if (BoatData.Latitude.data != NMEA0183DoubleNA) {
          page->UpdateLinef(8, BigSingleTopLeft, "LAT: %s", LattoString(BoatData.Latitude.data));
          page->UpdateLinef(8, BigSingleTopLeft, "LON: %s", LongtoString(BoatData.Longitude.data));
         // DrawGPSPlot(false, BigSingleDisplay, BoatData, magnification);
        }
      }
      if (CheckButton(topLeftquarter)) { Display_Page = 9; }
      if (CheckButton(BigSingleTopRight)) { Display_Page = 4; }

      if (CheckButton(BottomRightbutton)) {
        magnification = magnification * 1.5;
        Serial.printf(" magification  %f \n", magnification);
      }
      if (CheckButton(BottomLeftbutton)) {
        magnification = magnification / 1.5;
        Serial.printf(" magification  %f \n", magnification);
      }
      if (CheckButton(BigSingleDisplay)) {  // press plot to recenter plot
        if (BoatData.Latitude.data != NMEA0183DoubleNA) {
          DrawGPSPlot(true, BigSingleDisplay, BoatData, magnification);
          Serial.printf(" reset center anchorwatch %f   %f \n", startposlat, startposlon);
          page->GFXBorderBoxPrintf(BigSingleDisplay, "");
          page->GFXBorderBoxPrintf(BottomRightbutton, "zoom in");
          page->GFXBorderBoxPrintf(BottomLeftbutton, "zoom out");
        }
        DataChanged = true;
      }
      break;

    case 15:  // wind instrument TRUE Ground ref - experimental
    
      if (millis() > slowdown + 500) {
        slowdown = millis();
      }
          if (CheckButton(topLeftquarter)) { Display_Page = 4; }
      if (CheckButton(BigSingleDisplay)) { Display_Page = 5; }
      break;

    default:
      Display_Page = 0;
      break;
  }
  LastPageselected = pageIndex;
  RunSetup = false;
}

void TouchCrosshair(int size) {
  for (int i = 0; i < (ts.touches); i++) {
    TouchCrosshair(i, size, WHITE);
  }
}
void TouchCrosshair(int point, int size, uint16_t colour) {
 page->setCursor(ts.points[point].x, ts.points[point].y);
  page->printf("%i %i  ", ts.points[point].x, ts.points[point].y);
  page->drawFastVLine(ts.points[point].x, ts.points[point].y - size, 2 * size, colour);
  page->drawFastHLine(ts.points[point].x - size, ts.points[point].y, 2 * size, colour);
}


bool CheckButton(_sButton& button) {  // trigger on release. needs index (s) to remember which button!
  if (Touch_available) {
    //trigger on release! does not sense !isTouched ..  use Keypressed in each button struct to keep track!
    if (ts.isTouched && !button.Keypressed && (millis() - button.LastDetect >= 250)) {
      if (XYinBox(ts.points[0].x, ts.points[0].y, button.h, button.v, button.width, button.height)) {
        //Serial.printf(" Checkbutton size%i state %i %i \n",ts.points[0].size,ts.isTouched,XYinBox(ts.points[0].x, ts.points[0].y,button.h,button.v,button.width,button.height));
        button.Keypressed = true;
        //button.BorderColor= BLACK;
        button.LastDetect = millis();
      }
      return false;
    }
  }
  if (button.Keypressed && (millis() - button.LastDetect >= 250)) {
    //Serial.printf(" Checkbutton released from  %i %i\n",button.h,button.v);
    button.Keypressed = false;
    //button.BorderColor= WHITE;
    return true;
  }
  return false;
}


// being replaced with page->setfont set in FontType.h /*
// Lookup table for font line heights
/*
static const int fontHeightTable[FONT_COUNT] = {
  11, // FONT_MONO_8              0
  17, // FONT_MONO_12             1
  25, // FONT_MONO_18             2
  11, // FONT_MONO_BOLD_8         3
  17, // FONT_MONO_BOLD_12        4
  25, // FONT_MONO_BOLD_18        5
  36, // FONT_MONO_BOLD_27        6
   9, // FONT_FreeSansBold6       7
  11, // FONT_FreeSansBold8       8
  18, // FONT_FreeSansBold12      9
  27, // FONT_FreeSansBold18      10
  39, // FONT_FreeSansBold27      11
  59, // FONT_FreeSansBold40      12
  88  // FONT_FreeSansBold60      13
};
*/
void setFont(int fontinput) {  //fonts 3..12 are FreeMonoBold in sizes incrementing by 1.5
                               //Notes: May remove some later to save program mem space?
                               // used : 0,1,2,4 for keyboard
                               //      : 0,3,4,8,10,11 in main
  MasterFont = fontinput;
 // page->setFontByIndex(fontinput);
  switch (fontinput) {  //select font and automatically set height/offset based on character '['
      // set the heights and offset to print [ in boxes. Heights in pixels are NOT the point heights!

    case 0:                        // SMALL 8pt
      Fontname = "FreeMono8pt7b";  //9 to 14 high?
      gfx->setFont(&FreeMono8pt7b);
      text_height = (FreeMono8pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMono8pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;
      break;
    case 1:  // standard 12pt
      Fontname = "FreeMono12pt7b";
      gfx->setFont(&FreeMono12pt7b);
      text_height = (FreeMono12pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMono12pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;

      break;
    case 2:  //standard 18pt
      Fontname = "FreeMono18pt7b";
      gfx->setFont(&FreeMono18pt7b);
      text_height = (FreeMono18pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMono18pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;

      break;
    case 3:  //BOLD 8pt
      Fontname = "FreeMonoBOLD8pt7b";
      gfx->setFont(&FreeMonoBold8pt7b);
      text_height = (FreeMonoBold8pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMonoBold8pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;

      break;
    case 4:  //BOLD 12pt
      Fontname = "FreeMonoBOLD12pt7b";
      gfx->setFont(&FreeMonoBold12pt7b);
      text_height = (FreeMonoBold12pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMonoBold12pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;

      break;
    case 5:  //BOLD 18 pt
      Fontname = "FreeMonoBold18pt7b";
      gfx->setFont(&FreeMonoBold18pt7b);
      text_height = (FreeMonoBold18pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMonoBold18pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;
      break;
    case 6:  //BOLD 27 pt
      Fontname = "FreeMonoBold27pt7b";
      gfx->setFont(&FreeMonoBold27pt7b);
      text_height = (FreeMonoBold27pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMonoBold27pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;
      break;
    case 7:  //SANS BOLD 6 pt
      Fontname = "FreeSansBold6pt7b";
      gfx->setFont(&FreeSansBold6pt7b);
      text_height = (FreeSansBold6pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold6pt7bGlyphs[0x38].yOffset);
      text_char_width = 12;
      break;
    case 8:  //SANS BOLD 8 pt
      Fontname = "FreeSansBold8pt7b";
      gfx->setFont(&FreeSansBold8pt7b);
      text_height = (FreeSansBold8pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold8pt7bGlyphs[0x38].yOffset);  // yAdvance is the last variable.. and the one that affects the extra lf on wrap.
      text_char_width = 12;
      break;
    case 9:  //SANS BOLD 12 pt
      Fontname = "FreeSansBold12pt7b";
      gfx->setFont(&FreeSansBold12pt7b);
      text_height = (FreeSansBold12pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold12pt7bGlyphs[0x38].yOffset);
      text_char_width = 12;
      break;
    case 10:  //SANS BOLD 18 pt
      Fontname = "FreeSansBold18pt7b";
      gfx->setFont(&FreeSansBold18pt7b);
      text_height = (FreeSansBold18pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold18pt7bGlyphs[0x38].yOffset);
      text_char_width = 12;
      break;
    case 11:  //sans BOLD 27 pt
      Fontname = "FreeSansBold27pt7b";
      gfx->setFont(&FreeSansBold27pt7b);
      text_height = (FreeSansBold27pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold27pt7bGlyphs[0x38].yOffset);
      text_char_width = 12;
      break;
    case 12:  //sans BOLD 40 pt
      Fontname = "FreeSansBold40pt7b";
      gfx->setFont(&FreeSansBold40pt7b);
      text_height = (FreeSansBold40pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold40pt7bGlyphs[0x38].yOffset);
      text_char_width = 12;
      break;

    case 13:  //sans BOLD 60 pt
      Fontname = "FreeSansBold60pt7b";
      gfx->setFont(&FreeSansBold60pt7b);
      text_height = (FreeSansBold60pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold60pt7bGlyphs[0x38].yOffset);
      text_char_width = 12;
      break;

      //   case 21:  //Mono oblique BOLD 27 pt
      //   Fontname = "FreeMonoBoldOblique27pt7b";
      //   gfx->setFont(&FreeMonoBoldOblique27pt7b);
      //   text_height = (FreeMonoBoldOblique27pt7bGlyphs[0x38].height) + 1;
      //   text_offset = -(FreeMonoBoldOblique27pt7bGlyphs[0x38].yOffset);
      //   text_char_width = 12;
      //   break;
      // case 22:  //Mono oblique BOLD 40 pt
      //   Fontname = "FreeMonoBoldOblique40pt7b";
      //   gfx->setFont(&FreeMonoBoldOblique40pt7b);
      //   text_height = (FreeMonoBoldOblique40pt7bGlyphs[0x38].height) + 1;
      //   text_offset = -(FreeMonoBoldOblique40pt7bGlyphs[0x38].yOffset);
      //   text_char_width = 12;
      //   break;
      //       case 23:  //Mono oblique BOLD 60 pt
      //   Fontname = "FreeMonoBoldOblique60pt7b";
      //   gfx->setFont(&FreeMonoBoldOblique60pt7b);
      //   text_height = (FreeMonoBoldOblique60pt7bGlyphs[0x38].height) + 1;
      //   text_offset = -(FreeMonoBoldOblique60pt7bGlyphs[0x38].yOffset);
      //   text_char_width = 12;
      //   break;


    default:
      Fontname = "FreeMono8pt7b";
      // page->setFont(&FreeMono8pt7b);
      text_height = (FreeMono8pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMono8pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;
      MasterFont = 0;

      break;
  }
}
void ShowGPSinBox(int font, _sButton button) {
  static double lastTime;
  //Serial.printf("In ShowGPSinBox  %i\n",int(BoatData.GPSTime));
  if ((BoatData.GPSTime != NMEA0183DoubleNA) && (BoatData.GPSTime != lastTime)) {
    lastTime = BoatData.GPSTime;
  //  page->GFXBorderBoxPrintf(button, "");
    page->AddTitleInsideBox(button,9, 2, " GPS");
    button.PrintLine = 0;
    if (BoatData.SatsInView != NMEA0183DoubleNA) {page->UpdateLinef(font, button, "Satellites in view %.0f ", BoatData.SatsInView); }
    if (BoatData.GPSTime != NMEA0183DoubleNA) {
      //UpdateLinef(font, button, "");
    page->UpdateLinef(font, button, "Date: %06i ", int(BoatData.GPSDate));
      //UpdateLinef(font, button, "");
    page->UpdateLinef(font, button, "TIME: %02i:%02i:%02i",
                  int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60);
    }
    if (BoatData.Latitude.data != NMEA0183DoubleNA) {
    page->UpdateLinef(font, button, "LAT");
    page->UpdateLinef(font, button, "%s", LattoString(BoatData.Latitude.data));
    page->UpdateLinef(font, button, "LON");
    page->UpdateLinef(font, button, "%s", LongtoString(BoatData.Longitude.data));
    page->UpdateLinef(font, button, "");
    }
  }
}
void ButtonDataSelect(_sButton Position, int Instance, String Choice, bool RunSetup) {  //replaces all the code in earlier versions
  // set text size depending on the button Height
  int magnify;
  int timefont;
  timefont = 12;
  static unsigned int slowdown;
  magnify = 1;
  if (Position.height >= 250) {
    magnify = 3;
    timefont = 13;
  }
  if (RunSetup) slowdown = 0;
 if ((Choice == "WIND") &&(BoatData.WindSpeedK.data!= NMEA0183DoubleNA) && (!BoatData.WindAngleApp.displayed))
 {
       page->DrawCompass(Position);
      page->drawCompassPointer(Position, 20, 50, BoatData.WindAngleApp.data, WHITE,true);
      page->AddTitleInsideBox(Position, 6, 9, "Apparent:%.1fkts",BoatData.WindSpeedK.data);
      BoatData.WindAngleApp.displayed=true;
  }
  /*if (selected dta .data != NMEA0183DoubleNA) */
 page->setShadowX(4); page->setShadowY(4);
  if ((Choice == "SOG")&&(BoatData.SOG.data!= NMEA0183DoubleNA) && (!BoatData.SOG.displayed))
  { page->AutoPrint2Size(Position, "19.9", "%.1f",BoatData.SOG.data);
                            page->AddTitleInsideBox(Position, 6, 9, " SOG ");
                            page->AddTitleInsideBox(Position, 3, 9, " Kts ");
                             BoatData.SOG.displayed=true;}
  if ((Choice == "STW") &&(BoatData.STW.data!= NMEA0183DoubleNA) && (!BoatData.STW.displayed)) 
  {  page->AutoPrint2Size(Position, "19.9", "%.1f",BoatData.STW.data);
                            page->AddTitleInsideBox(Position, 6, 9, " STW ");
                            page->AddTitleInsideBox(Position, 3, 9, " Kts ");
                             BoatData.STW.displayed=true;}
  if ((Choice == "DEPTH") &&(BoatData.WaterDepth.data!= NMEA0183DoubleNA)&& (!BoatData.WaterDepth.displayed) ) 
  { page->AutoPrint2Size(Position, "199.9", "%.1f",BoatData.WaterDepth.data);
                            page->AddTitleInsideBox(Position, 6, 9, " DEPTH ");
                            page->AddTitleInsideBox(Position, 3, 9, " m ");
                             BoatData.WaterDepth.displayed=true;} 
  page->setShadowX(0); page->setShadowY(0);
 if (Choice == "DGRAPH") 
 { page->DrawScrollingGraph(Position, DepthBuffer,10,0); //int min= 20;int max=0;    min, max);
                       page->AddTitleInsideBox(Position, 1, 0, "Fathmometer 10m");
                       page->AddTitleInsideBox(Position, 2, 0, "surface");
                       page->AddTitleInsideBox(Position, 3, 0, "MIN:%i ",20);}
  if (Choice == "DGRAPH2") 
  { page->DrawScrollingGraph(Position, DepthBuffer,50,0); //int min= 20;int max=0;    min, max);
                       page->AddTitleInsideBox(Position, 1, 0, "Fathmometer 50m");
                       page->AddTitleInsideBox(Position, 2, 0, "surface");
                       page->AddTitleInsideBox(Position, 3, 0, "MIN:%i ",50);}

  
  // if (Choice == "DGRAPH2") { SCROLLGraph(RunSetup, Instance, 1, true, Position, BoatData.WaterDepth, 50, 0, 8, "Fathmometer 50m ", "m"); }
  // if (Choice == "STWGRAPH") { SCROLLGraph(RunSetup, Instance, 1, true, Position, BoatData.STW, 0, 10, 8, "STW-Graph ", "kts"); }
  // if (Choice == "SOGGRAPH") { SCROLLGraph(RunSetup, Instance, 1, true, Position, BoatData.SOG, 0, 10, 8, "SOG-Graph ", "kts"); }
  if (Choice == "GPS") { ShowGPSinBox(9, Position); }
  if (Choice == "TIME") {
    if (millis() > slowdown + 10000) {  //FOR the TIME display only make/update copies every 10 second!  else undisplayed copies will be redrawn!
      slowdown = millis();
      setFont(timefont);
      page->GFXBorderBoxPrintf(Position, "%02i:%02i",
                         int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60);
      page->AddTitleInsideBox(Position,9, 3,  "UTC ");
      setFont(10);
    }
  }
  if (Choice == "TIMEL") {
    if (millis() > slowdown + 10000) {  //FOR the TIME display only make/update copies every 10 second!  else undisplayed copies will be redrawn!
      slowdown = millis();
      setFont(timefont);
      page->GFXBorderBoxPrintf(Position, "%02i:%02i",
                         int(BoatData.LOCTime) / 3600, (int(BoatData.LOCTime) % 3600) / 60);
      page->AddTitleInsideBox(Position,9, 3,  "LOCAL ");
      setFont(10);
    }
  }

 
}