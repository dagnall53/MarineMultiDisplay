#include "Display.h"
#include "aux_functions.h"
#include <Arduino_GFX_Library.h> // defines colours BLUE etc
#include <FFat.h>  // defines FATFS functions for local files 
#include <N2kMessages.h> // isNKNA 
#include "Keyboard.h" // for keyboard functions
#include "JpegFunc.h"
#include "VICTRONBLE.h"  //sets #ifndef Victronble_h

extern bool WIFIGFXBoxdisplaystarted;
extern  _sWiFi_settings_Config Current_Settings;
extern  _sDisplay_Config Display_Config;
extern  _sWiFi_settings_Config Saved_Settings;
extern _sBoatData BoatData;  // BoatData values, int double etc

extern void  EEPROM_READ();
extern void  EEPROM_WRITE(_sDisplay_Config B, _sWiFi_settings_Config A);

extern void ShowToplinesettings(String Text);
extern void ShowToplinesettings(_sWiFi_settings_Config A, String Text);
extern void WindArrow2(_sButton button, _sInstData Speed, _sInstData& Wind);
extern void WindArrowSub(_sButton button, _sInstData Speed, _sInstData& wind);
extern void DrawMeterPointer(Phv center, double wind, int inner, int outer, int linewidth, uint16_t FILLCOLOUR, uint16_t LINECOLOUR);
extern void DrawCompass(_sButton button);
extern char* LongtoString(double data);
extern char* LattoString(double data);

extern boolean CompStruct(_sWiFi_settings_Config A, _sWiFi_settings_Config B);
extern boolean IsConnected;    // may be used in AP_AND_STA to flag connection success (got IP)
extern boolean AttemptingConnect;      // to note that WIFI.begin has been started
extern int NetworksFound; 


void Display(int page) {
  Display(false, page);
}

void Display(bool reset, int page) {  // setups for alternate pages to be selected by page.
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
  if (page != LastPageselected) {
    WIFIGFXBoxdisplaystarted = false;  // will have reset the screen, so turn off the auto wifibox blanking if there was a wifiiterrupt box
                                       // this (above) saves a timed screen refresh that can clear keyboard stuff
    RunSetup = true;
  }
  if (reset) {
    WIFIGFXBoxdisplaystarted = false;
    RunSetup = true;
  }
  //generic setup stuff for ALL pages
  if (RunSetup) {
    gfx->fillScreen(BLUE);
    gfx->setTextColor(WHITE);
    setFont(3);
    GFXBorderBoxPrintf(StatusBox, "");  // common to all pages
  }
  if ((millis() >= flashinterval)) {
    flashinterval = millis() + 1000;
    StatusBox.PrintLine = 0;  // always start / only use / the top line 0  of this box
    UpdateLinef(3, StatusBox, "%s page:%i  Log Status %s NMEA %s  ", Display_Config.PanelName, Display_Page,
                Current_Settings.Log_ON On_Off, Current_Settings.NMEA_log_ON On_Off);
    if (Current_Settings.Log_ON || Current_Settings.NMEA_log_ON) {
      flash = !flash;
      if (!flash) {
        UpdateLinef(3, StatusBox, "%s page:%i  Log Status     NMEA     ", Display_Config.PanelName, Display_Page);
      }
    }
  }
  // add any other generic stuff here
  if (CheckButton(StatusBox)) { Display_Page = 0; }  // go to settings
  // Now specific stuff for each page

  switch (page) {  // just show the logos on the sd card top page
    case -200:
      if (RunSetup) {
        showPicture("/logo.jpg");
        // jpegDraw("/logo.jpg", jpegDrawCallback, true /* useBigEndian */,
        //          0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        GFXBorderBoxPrintf(Full0Center, "Jpg tests -Return to Menu-");
        GFXBorderBoxPrintf(Full1Center, "logo.jpg");
        GFXBorderBoxPrintf(Full2Center, "logo1.jpg");
        GFXBorderBoxPrintf(Full3Center, "logo2.jpg");
        GFXBorderBoxPrintf(Full4Center, "logo4.jpg");
        GFXBorderBoxPrintf(Full5Center, "logo4.jpg");
      }

      if (CheckButton(Full0Center)) { Display_Page = 0; }
      if (CheckButton(Full1Center)) {
        jpegDraw("/logo.jpg", jpegDrawCallback, true /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        GFXBorderBoxPrintf(Full0Center, "logo");
      }
      if (CheckButton(Full2Center)) {
        jpegDraw("/logo1.jpg", jpegDrawCallback, true /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        GFXBorderBoxPrintf(Full0Center, "logo1");
      }
      if (CheckButton(Full3Center)) {
        jpegDraw("/logo2.jpg", jpegDrawCallback, false /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        GFXBorderBoxPrintf(Full0Center, "logo2");
      }
      if (CheckButton(Full4Center)) {
        jpegDraw("/logo4.jpg", jpegDrawCallback, true /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        GFXBorderBoxPrintf(Full0Center, "logo4");
      }
      if (CheckButton(Full5Center)) {
        jpegDraw("/logo4.jpg", jpegDrawCallback, true /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        GFXBorderBoxPrintf(Full0Center, "logo4");
      }

      break;

    case -99:  //a test for Screen Colours / fonts
      if (RunSetup) {
        gfx->fillScreen(BLACK);
        gfx->setTextColor(WHITE);
        fontlocal = 0;
        SwipeTestLR = 0;
        SwipeTestUD = 0;
        setFont(fontlocal);
        GFXBorderBoxPrintf(CurrentSettingsBox, "-TEST Colours- ");
      }

      if (millis() >= slowdown + 10000) {
        slowdown = millis();
        switch (fontlocal){
        case 1:
        WifiStatus.BackColor=WHITE;
        GFXBorderBoxPrintf(WifiStatus, "White ");
        GFXBorderBoxPrintf(CurrentSettingsBox, "White ");
        break;
                case 2:
        WifiStatus.BackColor=BLACK;
        GFXBorderBoxPrintf(WifiStatus, "BLACK ");
        GFXBorderBoxPrintf(CurrentSettingsBox, "BLACK");
        break;
                       case 3:
        WifiStatus.BackColor=BLUE;
        GFXBorderBoxPrintf(WifiStatus, "BLUE");
        GFXBorderBoxPrintf(CurrentSettingsBox, "BLUE ");
        break;
                       case 4:
        WifiStatus.BackColor=RED;
        GFXBorderBoxPrintf(WifiStatus, "RED ");
        GFXBorderBoxPrintf(CurrentSettingsBox, "RED ");
        break;
                       case 5:
        WifiStatus.BackColor=GREEN;
        GFXBorderBoxPrintf(WifiStatus, "GREEN ");
        GFXBorderBoxPrintf(CurrentSettingsBox, "GREEN ");
        break;

        }
        



        //gfx->fillScreen(BLACK);
        fontlocal = fontlocal + 1;
        if (fontlocal > 5) { fontlocal = 0; }
      }



      break;
    case -87:  // page for graphic display of Vicron data
      if (RunSetup) {
        jpegDraw("/vicback.jpg", jpegDrawCallback, true /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        Serial.println("redrawing background");
      }

      // all graphics done in VICTRONBLE
      if (CheckButton(FullTopCenter)) { Display_Page = -86; }
      break;
    case -86:                                              // page for text display of Vicron data
      if (RunSetup) { GFXBorderBoxPrintf(Terminal, ""); }  // only for setup, not changed data
      if (RunSetup || DataChanged) {
        setFont(3);  // different from most pages, displays in terminal from see ~line 2145
        GFXBorderBoxPrintf(FullTopCenter, "Return to VICTRON graphic display");
        if (!Terminal.debugpause) {
          AddTitleBorderBox(0, Terminal, "-running-");
        } else {
          AddTitleBorderBox(0, Terminal, "-paused-");
        }
        DataChanged = false;
      }
      // if (millis() > slowdown + 500) {
      //   slowdown = millis();
      // }
      if (CheckButton(FullTopCenter)) { Display_Page = -87; }
      if (CheckButton(Terminal)) {
        Terminal.debugpause = !Terminal.debugpause;
        DataChanged = true;  // for more immediate visual response to touch!
        if (!Terminal.debugpause) {
          AddTitleBorderBox(0, Terminal, "-running-");
        } else {
          AddTitleBorderBox(0, Terminal, "-paused-");
        }
      }

      break;

    case -22:                                              //  "EXPERIMENT in N2K data"
      if (RunSetup) { GFXBorderBoxPrintf(Terminal, ""); }  // only for setup, not changed data
      if (RunSetup || DataChanged) {
        EEPROM_READ();  // makes sure eeprom update data is latest and synchronised! 
        setFont(3);
        GFXBorderBoxPrintf(FullTopCenter, "N2K debug ");
        if (!Terminal.debugpause) {
          AddTitleBorderBox(0, Terminal, "TERMINAL");
        } else {
          AddTitleBorderBox(0, Terminal, "-Paused-");
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
          AddTitleBorderBox(0, Terminal, "-running-");
        } else {
          AddTitleBorderBox(0, Terminal, "-paused-");
        }
      }
   

      // if (CheckButton(Switch9)) {
      //   Current_Settings.ESP_NOW_ON = !Current_Settings.ESP_NOW_ON;
      //   DataChanged = true;
      // };
      // if (CheckButton(Switch11)) {
      //   EEPROM_WRITE(Display_Config, Current_Settings);
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
        GFXBorderBoxPrintf(Full0Center, "-Test JPegs-");
        GFXBorderBoxPrintf(Full1Center, "Check SD /Audio");
        GFXBorderBoxPrintf(Full2Center, "Check Fonts");
        GFXBorderBoxPrintf(Full3Center, "VICTRON devices");
        // GFXBorderBoxPrintf(Full3Center, "See NMEA");

        GFXBorderBoxPrintf(Full5Center, "Main Menu");
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

    case -21:                                              //  "Log and debug "
      if (RunSetup) {Terminal.debugpause=false; GFXBorderBoxPrintf(Terminal, ""); }  // only for setup, not changed data
      if (RunSetup || DataChanged) {
        EEPROM_READ();  // makes sure eeprom update data is latest and synchronised! 
        setFont(3);
        GFXBorderBoxPrintf(FullTopCenter, "Boat/NMEA Log and Source selects");
        GFXBorderBoxPrintf(Switch6, Current_Settings.Log_ON On_Off);
        AddTitleBorderBox(0, Switch6, "B LOG");
        GFXBorderBoxPrintf(Switch7, Current_Settings.NMEA_log_ON On_Off);
        AddTitleBorderBox(0, Switch7, "N LOG");
        GFXBorderBoxPrintf(Switch8, Current_Settings.UDP_ON On_Off);
        AddTitleBorderBox(0, Switch8, "UDP");
        GFXBorderBoxPrintf(Switch9, Current_Settings.ESP_NOW_ON On_Off);
        AddTitleBorderBox(0, Switch9, "ESP-N");
        GFXBorderBoxPrintf(Switch10, Current_Settings.N2K_ON On_Off);
        AddTitleBorderBox(0, Switch10, "N2K");

        GFXBorderBoxPrintf(Switch11, CompStruct(Saved_Settings, Current_Settings) ? "-same-" : "UPDATE");
        AddTitleBorderBox(0, Switch11, "EEPROM");

        if (!Terminal.debugpause) {
          AddTitleBorderBox(0, Terminal, "TERMINAL");
        } else {
          AddTitleBorderBox(0, Terminal, "-Paused-");
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
          AddTitleBorderBox(0, Terminal, "-running-");
        } else {
          AddTitleBorderBox(0, Terminal, "-paused-");
        }
      }
      if (CheckButton(Switch6)) {
        Current_Settings.Log_ON = !Current_Settings.Log_ON;
       // NO LOGGING YET if (Current_Settings.Log_ON) { StartInstlogfile(); }
        DataChanged = true;
      };

      if (CheckButton(Switch7)) {
        Current_Settings.NMEA_log_ON = !Current_Settings.NMEA_log_ON;
       //  NO LOGGING YET if (Current_Settings.NMEA_log_ON) { StartNMEAlogfile(); }
        DataChanged = true;
      };

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
        EEPROM_WRITE(Display_Config, Current_Settings);
        delay(50);
        // Display_Page = 0;
        DataChanged = true;
      };



      break;

    case -10:  // a test page for fonts
      if (RunSetup || DataChanged) {
        gfx->fillScreen(BLUE);
        setFont(3);
        // GFXBorderBoxPrintf(Full0Center, "-Font test -");
        GFXBorderBoxPrintf(BottomLeftbutton, "Smaller");
        GFXBorderBoxPrintf(BottomRightbutton, "Larger");
      }
      // if (millis() > slowdown + 5000) {
      //   slowdown = millis();
      //   //gfx->fillScreen(BLUE);
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
        GFXBorderBoxPrintf(CurrentSettingsBox, "FONT:%i name%s height<%i>", fontlocal, Tempchar, FontHt);
        setFont(fontlocal);
        GFXBorderBoxPrintf(FontBox, "Test %4.2f", temp);
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


    case -5:  ///Wifiscan

      if (RunSetup || DataChanged) {
        setFont(4);
        if (IsConnected) {
          GFXBorderBoxPrintf(TOPButton, "Connected<%s>", Current_Settings.ssid);
        } else {
          GFXBorderBoxPrintf(TOPButton, "NOT FOUND:%s", Current_Settings.ssid);
        }
        GFXBorderBoxPrintf(TopRightbutton, "Scan");
        GFXBorderBoxPrintf(SecondRowButton, " Saved results");
        gfx->setCursor(0, 140);  //(location of terminal. make better later!)

        if (NetworksFound <= 0) {  // note scan error can give negative number
          NetworksFound = 0;
          GFXBorderBoxPrintf(SecondRowButton, " Use keyboard ");
        } else {
          GFXBorderBoxPrintf(SecondRowButton, " %i Networks Found", NetworksFound);
          gfx->fillRect(0, 200, 480, 280, BLUE);  // clear the place wherethe wifi wil be printed
          for (int i = 0; i < NetworksFound; ++i) {
            gfx->setCursor(0, 200 + ((i + 1) * text_height));
            // Print SSID and RSSI for each network found
            gfx->print(i + 1);
            gfx->print(": ");
            //if (WiFi.SSID(i).length() > 20) { gfx->print("..(toolong).."); }
           // gfx->print(WiFi.SSID(i).substring(0, 20));
           // if (WiFi.SSID(i).length() > 20) { gfx->print(".."); }
            // Serial.print(WiFi.SSID(i));
            // Serial.println(WiFi.channel(i));
            // gfx->print(" (");
            // gfx->print(WiFi.RSSI(i));
            // gfx->print(")");
            // gfx->print(" ch<");
            // gfx->print(WiFi.channel(i));
            // gfx->println(">");


            //    gfx->println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
            delay(10);
          }
        }
        DataChanged = false;
      }

      if (millis() > slowdown + 1000) {
        slowdown = millis();
        //other stuff?
      }

      // if ((ts.isTouched) && (ts.points[0].y >= 200)) {  // nb check on location on screen or it will get reset when you press one of the boxes
      //   //TouchCrosshair(1);
      //   wifissidpointer = ((ts.points[0].y - 200) / text_height) - 1;
      //   int str_len = WiFi.SSID(wifissidpointer).length() + 1;
      //   char result[str_len];
      //   //  Serial.printf(" touched at %i  equates to %i ? %s ", ts.points[0].y, wifissidpointer, WiFi.SSID(wifissidpointer));
      //   //  Serial.printf("  result str_len%i   sizeof settings.ssid%i \n", str_len, sizeof(Current_Settings.ssid));
      //   if (str_len <= sizeof(Current_Settings.ssid)) {                                       // check small enough for our ssid register array!
      //     WiFi.SSID(wifissidpointer).toCharArray(result, sizeof(Current_Settings.ssid) - 1);  // I like to keep a spare space!
      //     if (str_len == 1) {
      //       GFXBorderBoxPrintf(SecondRowButton, "Set via Keyboard?");
      //     } else {
      //       GFXBorderBoxPrintf(SecondRowButton, "Select<%s>?", result);
      //     }
      //   } else {
      //     GFXBorderBoxPrintf(SecondRowButton, "ssid too long ");
      //   }
      // }
      if (CheckButton(TopRightbutton)) {
        GFXBorderBoxPrintf(SecondRowButton, " Starting WiFi re-SCAN / reconnect ");
        AttemptingConnect = false;  // so that Scan can do a full scan..
       // ScanAndConnect(false);
        DataChanged = true;
      }  // do the scan again
      if (CheckButton(SecondRowButton)) {
        // Serial.printf(" * Debug wifissidpointer=%i \n",wifissidpointer);
        if ((NetworksFound >= 1) && (wifissidpointer <= NetworksFound)) {
       //   WiFi.SSID(wifissidpointer).toCharArray(Current_Settings.ssid, sizeof(Current_Settings.ssid) - 1);
       //   Serial.printf("Update ssid to <%s> \n", Current_Settings.ssid);
          Display_Page = -1;
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
        GFXBorderBoxPrintf(TOPButton, "Current <%s>", Current_Settings.UDP_PORT);
        GFXBorderBoxPrintf(Full0Center, "Set UDP PORT");
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
        GFXBorderBoxPrintf(TOPButton, "Current <%s>", Current_Settings.password);
        GFXBorderBoxPrintf(Full0Center, "Set Password");
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
        GFXBorderBoxPrintf(Full0Center, "Set SSID");
        GFXBorderBoxPrintf(TopRightbutton, "Scan");
        AddTitleBorderBox(0, TopRightbutton, "WiFi");
        keyboard(-1);  //reset
        Use_Keyboard(Current_Settings.ssid, sizeof(Current_Settings.ssid));
        keyboard(1);
      }

      Use_Keyboard(Current_Settings.ssid, sizeof(Current_Settings.ssid));
      if (CheckButton(Full0Center)) { Display_Page = -1; }
      if (CheckButton(TopRightbutton)) { Display_Page = -5; }
      break;

    case -1:  // this is the WIFI settings page
      if (RunSetup || DataChanged) {
        gfx->fillScreen(BLACK);
        gfx->setTextColor(BLACK);
        gfx->setTextSize(1);
        EEPROM_READ();
        ShowToplinesettings(Saved_Settings, "EEPROM");
        setFont(4);
        GFXBorderBoxPrintf(SecondRowButton, "SSID <%s>", Current_Settings.ssid);
        if (IsConnected) {
          AddTitleBorderBox(0, SecondRowButton, "Current Setting <CONNECTED>");
        } else {
          AddTitleBorderBox(0, SecondRowButton, "Current Setting <NOT CONNECTED>");
        }
        GFXBorderBoxPrintf(ThirdRowButton, "Password <%s>", Current_Settings.password);
        AddTitleBorderBox(0, ThirdRowButton, "Current Setting");
        GFXBorderBoxPrintf(FourthRowButton, "UDP Port <%s>", Current_Settings.UDP_PORT);
        AddTitleBorderBox(0, FourthRowButton, "Current Setting");
        GFXBorderBoxPrintf(Switch1, Current_Settings.Serial_on On_Off);  //A.Serial_on On_Off,  A.UDP_ON On_Off, A.ESP_NOW_ON On_Off
        AddTitleBorderBox(0, Switch1, "Serial");
        GFXBorderBoxPrintf(Switch2, Current_Settings.UDP_ON On_Off);
        AddTitleBorderBox(0, Switch2, "UDP");
        GFXBorderBoxPrintf(Switch3, Current_Settings.ESP_NOW_ON On_Off);
        AddTitleBorderBox(0, Switch3, "ESP-Now");
        Serial.printf(" Compare Saved and Current <%s> \n", CompStruct(Saved_Settings, Current_Settings) ? "-same-" : "UPDATE");
        GFXBorderBoxPrintf(Switch4, CompStruct(Saved_Settings, Current_Settings) ? "-same-" : "UPDATE");
        AddTitleBorderBox(0, Switch4, "EEPROM");
        GFXBorderBoxPrintf(Full5Center, "Logger and Debug");
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
        EEPROM_WRITE(Display_Config, Current_Settings);
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
        GFXBorderBoxPrintf(Full0Center, "-Experimental-");
        GFXBorderBoxPrintf(Full1Center, "WIFI Settings");
        GFXBorderBoxPrintf(Full2Center, "NMEA DISPLAY");
        GFXBorderBoxPrintf(Full3Center, "Debug + LOG");
        GFXBorderBoxPrintf(Full4Center, "GPS Display");
        GFXBorderBoxPrintf(Full5Center, "Victron Display");
        GFXBorderBoxPrintf(Full6Center, "Save / Reset ");
      }
      if (millis() > slowdown + 500) {
        slowdown = millis();
      }
      if (CheckButton(Full0Center)) { Display_Page = -20; }
      if (CheckButton(Full1Center)) { Display_Page = -1; }
      if (CheckButton(Full2Center)) { Display_Page = 4; }
      if (CheckButton(Full3Center)) { Display_Page = -21; }
      if (CheckButton(Full4Center)) { Display_Page = 9; }
      if (CheckButton(Full5Center)) { Display_Page = -87; }
      if (CheckButton(Full6Center)) {
        //Display_Page = 4;
        EEPROM_WRITE(Display_Config, Current_Settings);
        delay(50);
    //    WiFi.disconnect();
        ESP.restart();
      }
      break;

    case 4:  // Quad display
        if (RunSetup) {
        setFont(10);
        gfx->fillScreen(BLACK);
        if (String(Display_Config.FourWayTR) == "WIND") {
          DrawCompass(topRightquarter); // only draw the compass once!
          AddTitleInsideBox(8, 3, topRightquarter, "WIND APP ");
        }
     //   GFXBorderBoxPrintf(topLeftquarter, "");
       // AddTitleInsideBox(9, 3, topLeftquarter, "STW ");
        //AddTitleInsideBox(9, 2, topLeftquarter, " Kts");  //font,position
        setFont(10);
        //SCROLLGraph(RunSetup, 0, 1, true, bottomLeftquarter, BoatData.WaterDepth, 50, 0, 8, "Fathmometer 50m ", "m"); 
      }
      if (millis() > slowdown + 1000) {
        slowdown = millis();  //only make/update copies every second!  else undisplayed copies will be redrawn!
        // could have more complex that accounts for displayed already?
        setFont(12);
        if (String(Display_Config.FourWayTR) == "TIME") {
          GFXBorderBoxPrintf(topRightquarter, "%02i:%02i",
                             int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60);
          AddTitleInsideBox(9, 3, topRightquarter, "UTC ");
        }
        if (String(Display_Config.FourWayTR) == "TIMEL") {
          GFXBorderBoxPrintf(topRightquarter, "%02i:%02i",
                             int(BoatData.LOCTime) / 3600, (int(BoatData.LOCTime) % 3600) / 60);
          AddTitleInsideBox(9, 3, topRightquarter, "LOCAL ");
        }
        setFont(10);
      }

      //UpdateDataTwoSize(true, true, 13, 11, topLeftquarter, BoatData.STW, "%.1f");


      if (String(Display_Config.FourWayTR) == "WIND") { WindArrow2(topRightquarter, BoatData.WindSpeedK, BoatData.WindAngleApp); }


      //seeing if JSON setting of (bottom two sides of) quad is useful.. TROUBLE with two scrollGraphss so there is now extra 'instances' settings allowing two to run simultaneously!! ?
       if (String(Display_Config.FourWayTL) == "DEPTH") { UpdateDataTwoSize(RunSetup, "DEPTH", " M", true, true, 13, 11, topLeftquarter, BoatData.WaterDepth, "%.1f"); }
      if (String(Display_Config.FourWayTL) == "SOG") { UpdateDataTwoSize(RunSetup, "SOG", " Kts", true, true, 13, 11, topLeftquarter, BoatData.SOG, "%.1f"); }
      if (String(Display_Config.FourWayTL) == "STW") { UpdateDataTwoSize(RunSetup, "STW", " Kts", true, true, 13, 11, topLeftquarter, BoatData.STW, "%.1f"); }



      if (String(Display_Config.FourWayBL) == "DEPTH") { UpdateDataTwoSize(RunSetup, "DEPTH", " M", true, true, 13, 11, bottomLeftquarter, BoatData.WaterDepth, "%.1f"); }
      if (String(Display_Config.FourWayBL) == "SOG") { UpdateDataTwoSize(RunSetup, "SOG", " Kts", true, true, 13, 11, bottomLeftquarter, BoatData.SOG, "%.1f"); }
      if (String(Display_Config.FourWayBL) == "STW") { UpdateDataTwoSize(RunSetup, "STW", " Kts", true, true, 13, 11, bottomLeftquarter, BoatData.STW, "%.1f"); }

      if (String(Display_Config.FourWayBL) == "GPS") { ShowGPSinBox(9, bottomLeftquarter); }

      if (String(Display_Config.FourWayBL) == "DGRAPH") { SCROLLGraph(RunSetup, 0, 1, true, bottomLeftquarter, BoatData.WaterDepth, 10, 0, 8, "Fathmometer 10m ", "m"); }
      if (String(Display_Config.FourWayBL) == "DGRAPH2") { SCROLLGraph(RunSetup, 0, 1, true, bottomLeftquarter, BoatData.WaterDepth, 50, 0, 8, "Fathmometer 50m ", "m"); }
      if (String(Display_Config.FourWayBL) == "STWGRAPH") { SCROLLGraph(RunSetup, 0, 1, true, bottomLeftquarter, BoatData.STW, 0, 10, 8, "STW ", "kts"); }
      if (String(Display_Config.FourWayBL) == "SOGGRAPH") { SCROLLGraph(RunSetup, 0, 1, true, bottomLeftquarter, BoatData.SOG, 0, 10, 8, "SOG ", "kts"); }

      // note use of SCROLLGraph2
      if (String(Display_Config.FourWayBR) == "DEPTH") { UpdateDataTwoSize(RunSetup, "DEPTH", " M", true, true, 13, 11, bottomRightquarter, BoatData.WaterDepth, "%.1f"); }
      if (String(Display_Config.FourWayBR) == "SOG") { UpdateDataTwoSize(RunSetup, "SOG", " Kts", true, true, 13, 11, bottomRightquarter, BoatData.SOG, "%.1f"); }
      if (String(Display_Config.FourWayBR) == "STW") { UpdateDataTwoSize(RunSetup, "STW", " Kts", true, true, 13, 11, bottomRightquarter, BoatData.STW, "%.1f"); }

      if (String(Display_Config.FourWayBR) == "GPS") { ShowGPSinBox(9, bottomRightquarter); }

      if (String(Display_Config.FourWayBR) == "DGRAPH") { SCROLLGraph(RunSetup, 1, 1, true, bottomRightquarter, BoatData.WaterDepth, 10, 0, 8, "Fathmometer 10m ", "m"); }
      if (String(Display_Config.FourWayBR) == "DGRAPH2") { SCROLLGraph(RunSetup, 1, 1, true, bottomRightquarter, BoatData.WaterDepth, 50, 0, 8, "Fathmometer 50m ", "m"); }
      if (String(Display_Config.FourWayBR) == "SOGGRAPH") { SCROLLGraph(RunSetup, 1, 1, true, bottomRightquarter, BoatData.SOG, 0, 10, 8, "SOG ", "kts"); }
      if (String(Display_Config.FourWayBR) == "STWGRAPH") { SCROLLGraph(RunSetup, 1, 1, true, bottomRightquarter, BoatData.STW, 0, 10, 8, "STW ", "kts"); }


      if (CheckButton(topLeftquarter)) { Display_Page = 6; }     //stw
      if (CheckButton(bottomLeftquarter)) { Display_Page = 7; }  //depth
      if (CheckButton(topRightquarter)) { Display_Page = 5; }    // Wind
      if (CheckButton(bottomRightquarter)) {
        if (String(Display_Config.FourWayBR) == "GPS") {
          Display_Page = 9;
        } else {
          Display_Page = 8;
        }  //SOG
      }

      break;

    case 5:  // wind instrument
      if (RunSetup) {
        setFont(10);
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        AddTitleInsideBox(8, 2, BigSingleTopRight, " deg");
        DrawCompass(BigSingleDisplay);
        AddTitleInsideBox(8, 3, BigSingleDisplay, "WIND Apparent ");
      }
      if (millis() > slowdown + 500) {
        slowdown = millis();
      }

      WindArrow2(BigSingleDisplay, BoatData.WindSpeedK, BoatData.WindAngleApp);
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopRight, BoatData.WindAngleApp, "%.1f");
      if (CheckButton(BigSingleDisplay)) { Display_Page = 15; }
      if (CheckButton(topLeftquarter)) { Display_Page = 4; }
      break;

    case 6:  //STW Speed Through WATER GRAPH
      if (RunSetup) {
        setFont(10);
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        GFXBorderBoxPrintf(BigSingleTopLeft, "");
        AddTitleInsideBox(8, 3, BigSingleTopRight, "STW");
        AddTitleInsideBox(8, 2, BigSingleTopRight, "Kts");
        AddTitleInsideBox(8, 3, BigSingleTopLeft, "SOG");
        AddTitleInsideBox(8, 2, BigSingleTopLeft, "Kts");
        GFXBorderBoxPrintf(BigSingleDisplay, "");
      }

      SCROLLGraph(RunSetup, 0, 3, true, BigSingleDisplay, BoatData.STW, 0, 10, 9, "STW Graph ", "Kts");
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopLeft, BoatData.SOG, "%.1f");
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopRight, BoatData.STW, "%.1f");
      if (CheckButton(BigSingleDisplay)) { Display_Page = 16; }
      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(BigSingleTopLeft)) { Display_Page = 8; }
      //if (CheckButton(bottomLeftquarter)) { Display_Page = 9; }
      //if (CheckButton(bottomRightquarter)) { Display_Page = 4; }

      break;

    case 16:  //STW large
      if (RunSetup) {
        setFont(10);
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        GFXBorderBoxPrintf(BigSingleTopLeft, "");
        AddTitleInsideBox(8, 3, BigSingleTopRight, "STW");
        AddTitleInsideBox(8, 2, BigSingleTopRight, "Kts");
        AddTitleInsideBox(8, 3, BigSingleDisplay, "STW");
        AddTitleInsideBox(8, 2, BigSingleDisplay, "Kts");
        AddTitleInsideBox(8, 3, BigSingleTopLeft, "SOG");
        AddTitleInsideBox(8, 2, BigSingleTopLeft, "Kts");
        GFXBorderBoxPrintf(BigSingleDisplay, "");
      }
      LocalCopy = BoatData.STW;
      UpdateDataTwoSize(3, true, true, 13, 12, BigSingleDisplay, LocalCopy, "%.1f");  // note magnify 3!! so needs the local copy
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopLeft, BoatData.SOG, "%.1f");
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopRight, BoatData.STW, "%.1f");

      if (CheckButton(BigSingleDisplay)) { Display_Page = 6; }
      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(BigSingleTopLeft)) { Display_Page = 8; }
      break;

    case 7:  // Depth  (fathmometer 30)  (circulate 7/11/17)
      if (RunSetup) {
        setFont(11);
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        AddTitleInsideBox(8, 3, BigSingleTopRight, "Depth");
        AddTitleInsideBox(8, 2, BigSingleTopRight, " m");
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        //AddTitleInsideBox(9,3,BigSingleDisplay, "Fathmometer 30m");
      }

      SCROLLGraph(RunSetup, 0, 3, true, BigSingleDisplay, BoatData.WaterDepth, 30, 0, 9, "Fathmometer 30m ", "m");
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopRight, BoatData.WaterDepth, "%.1f");

      if (CheckButton(BigSingleTopRight)) { Display_Page = 4; }
      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(topLeftquarter)) { Display_Page = 4; }
      if (CheckButton(BigSingleDisplay)) { Display_Page = 11; }
      break;

    case 11:  // Depth (fathmometer 1 0) different range
      if (RunSetup) {
        setFont(10);
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        AddTitleInsideBox(8, 1, BigSingleTopRight, "Depth");
        AddTitleInsideBox(8, 2, BigSingleTopRight, "m");
        GFXBorderBoxPrintf(BigSingleDisplay, "");
      }

      SCROLLGraph(RunSetup, 0, 3, true, BigSingleDisplay, BoatData.WaterDepth, 10, 0, 9, "Fathmometer 10m ", "m");
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopRight, BoatData.WaterDepth, "%.1f");

      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(topLeftquarter)) { Display_Page = 4; }
      if (CheckButton(BigSingleDisplay)) { Display_Page = 17; }
      if (CheckButton(topRightquarter)) { Display_Page = 7; }
      break;

    case 17:  // Depth (big digital)
      if (RunSetup) {
        setFont(10);
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        AddTitleInsideBox(8, 1, BigSingleTopRight, "Depth");
        AddTitleInsideBox(8, 2, BigSingleTopRight, "m");
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        AddTitleInsideBox(10, 2, BigSingleDisplay, "m");
      }
      LocalCopy = BoatData.WaterDepth;  //WaterDepth, "%4.1f m");  // nb %4.1 will give leading printing space- giving formatting issues!
      UpdateDataTwoSize(4, true, true, 12, 10, BigSingleDisplay, LocalCopy, "%.1f");
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopRight, BoatData.WaterDepth, "%.1f");
      if (CheckButton(topLeftquarter)) { Display_Page = 4; }
      if (CheckButton(BigSingleDisplay)) { Display_Page = 7; }
      //if (CheckButton(topRightquarter)) { Display_Page = 4; }
      break;

    case 8:  //SOG  graph
      if (RunSetup || DataChanged) {
        setFont(11);
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        GFXBorderBoxPrintf(BigSingleTopLeft, "");
        AddTitleInsideBox(8, 3, BigSingleTopRight, "STW");
        AddTitleInsideBox(8, 2, BigSingleTopRight, "Kts");
        AddTitleInsideBox(8, 3, BigSingleTopLeft, "SOG");
        AddTitleInsideBox(8, 2, BigSingleTopLeft, "Kts");
        GFXBorderBoxPrintf(BigSingleDisplay, "");

        DataChanged = false;
      }

      SCROLLGraph(RunSetup, 0, 3, true, BigSingleDisplay, BoatData.SOG, 0, 10, 9, "SOG Graph ", "kts");
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopRight, BoatData.STW, "%.1f");
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopLeft, BoatData.SOG, "%.1f");

      //if (CheckButton(Full0Center)) { Display_Page = 4; }
      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(BigSingleDisplay)) { Display_Page = 18; }  //18 is BIG SOG
      //if (CheckButton(bottomLeftquarter)) { Display_Page = 9; }
      if (CheckButton(BigSingleTopRight)) { Display_Page = 6; }
      if (CheckButton(BigSingleTopLeft)) { Display_Page = 4; }
      break;

    case 18:  //BIG SOG
      if (RunSetup || DataChanged) {
        setFont(11);
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        GFXBorderBoxPrintf(BigSingleTopLeft, "");
        AddTitleInsideBox(8, 3, BigSingleTopRight, "STW");
        AddTitleInsideBox(8, 2, BigSingleTopRight, "Kts");
        AddTitleInsideBox(8, 3, BigSingleTopLeft, "SOG");
        AddTitleInsideBox(8, 2, BigSingleTopLeft, "Kts");
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        DataChanged = false;
      }
      if (millis() > slowdown + 500) {
        slowdown = millis();
      }
      LocalCopy = BoatData.SOG;
      UpdateDataTwoSize(3, true, true, 13, 12, BigSingleDisplay, LocalCopy, "%.1f");  //note magnify 3 is a repeat display so needs the localCopy
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopRight, BoatData.STW, "%.1f");
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopLeft, BoatData.SOG, "%.1f");

      //if (CheckButton(Full0Center)) { Display_Page = 4; }
      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(BigSingleDisplay)) { Display_Page = 8; }  //18 is BIG SOG
      //if (CheckButton(bottomLeftquarter)) { Display_Page = 9; }
      if (CheckButton(BigSingleTopRight)) { Display_Page = 6; }
      if (CheckButton(BigSingleTopLeft)) { Display_Page = 4; }
      break;

    case 9:  // GPS page
      if (RunSetup) {
        setFont(8);
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        GFXBorderBoxPrintf(TopHalfBigSingleTopRight, "");
        GFXBorderBoxPrintf(BottomHalfBigSingleTopRight, "");
        GFXBorderBoxPrintf(BigSingleTopLeft, "Click for graphic");
        setFont(10);
      }
      UpdateDataTwoSize(true, true, 9, 8, TopHalfBigSingleTopRight, BoatData.SOG, "SOG: %.1f kt");
      UpdateDataTwoSize(true, true, 9, 8, BottomHalfBigSingleTopRight, BoatData.COG, "COG: %.1f d");
      if (millis() > slowdown + 1000) {
        slowdown = millis();
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        // do this one once a second.. I have not yet got simplified functions testing if previously displayed and greyed yet
        gfx->setTextColor(BigSingleDisplay.TextColor);
        BigSingleDisplay.PrintLine = 0;
        if (BoatData.SatsInView != NMEA0183DoubleNA) { UpdateLinef(8, BigSingleDisplay, "Satellites in view %.0f ", BoatData.SatsInView); }
        if (BoatData.GPSTime != NMEA0183DoubleNA) {
          UpdateLinef(9, BigSingleDisplay, "");
          UpdateLinef(9, BigSingleDisplay, "Date: %06i ", int(BoatData.GPSDate));
          UpdateLinef(9, BigSingleDisplay, "");
          UpdateLinef(9, BigSingleDisplay, "TIME: %02i:%02i:%02i",
                      int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60);
        }
        if (BoatData.Latitude.data != NMEA0183DoubleNA) {
          UpdateLinef(9, BigSingleDisplay, "");
          UpdateLinef(9, BigSingleDisplay, "LAT %s", LattoString(BoatData.Latitude.data));
          UpdateLinef(9, BigSingleDisplay, "LON %s", LongtoString(BoatData.Longitude.data));
          UpdateLinef(9, BigSingleDisplay, "");
        }

        if (BoatData.MagHeading.data != NMEA0183DoubleNA) { UpdateLinef(9, BigSingleDisplay, "Mag Heading: %.4f", BoatData.MagHeading); }
        if ((BoatData.Variation != NMEA0183DoubleNA)&& (BoatData.Variation != 0) &&!N2kIsNA(BoatData.Variation)) {UpdateLinef(9, BigSingleDisplay, "Variation: %.4f", BoatData.Variation);}
      }
      if (CheckButton(BigSingleTopLeft)) { Display_Page = 10; }
      //if (CheckButton(bottomLeftquarter)) { Display_Page = 4; }  //Loop to the main settings page
      break;

    case 10:  // GPS page 2 sort of anchor watch
      static double magnification;
      if (RunSetup || DataChanged) {
        setFont(8);
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        GFXBorderBoxPrintf(BigSingleTopLeft, "");
        if (BoatData.GPSTime != NMEA0183DoubleNA) {
          UpdateLinef(8, BigSingleTopLeft, "Date: %06i ", int(BoatData.GPSDate));
          UpdateLinef(8, BigSingleTopLeft, "TIME: %02i:%02i:%02i",
                      int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60);
        }
        if (BoatData.Latitude.data != NMEA0183DoubleNA) {
          UpdateLinef(8, BigSingleTopLeft, "LAT: %f", BoatData.Latitude.data);
          UpdateLinef(8, BigSingleTopLeft, "LON: %f", BoatData.Longitude.data);
        }

        GFXBorderBoxPrintf(BigSingleTopRight, "Show Quad Display");
        GFXBorderBoxPrintf(BottomRightbutton, "Zoom in");
        GFXBorderBoxPrintf(BottomLeftbutton, "Zoom out");
        magnification = 1111111;  //reset magnification 11111111 = 10 pixels / m == 18m circle.
        DataChanged = false;
      }
      if (millis() > slowdown + 1000) {
        slowdown = millis();
        // do this one once a second.. I have not yet got simplified functions testing if previously displayed and greyed yet
        ///gfx->setTextColor(BigSingleDisplay.TextColor);
        BigSingleTopLeft.PrintLine = 0;
        // UpdateLinef(3,BigSingleTopLeft, "%.0f Satellites in view", BoatData.SatsInView);
        if (BoatData.GPSTime != NMEA0183DoubleNA) {
          UpdateLinef(8, BigSingleTopLeft, "Date: %06i ", int(BoatData.GPSDate));
          UpdateLinef(8, BigSingleTopLeft, "TIME: %02i:%02i:%02i",
                      int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60);
        }
        if (BoatData.Latitude.data != NMEA0183DoubleNA) {
          UpdateLinef(8, BigSingleTopLeft, "LAT: %s", LattoString(BoatData.Latitude.data));
          UpdateLinef(8, BigSingleTopLeft, "LON: %s", LongtoString(BoatData.Longitude.data));
          DrawGPSPlot(false, BigSingleDisplay, BoatData, magnification);
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
          GFXBorderBoxPrintf(BigSingleDisplay, "");
          GFXBorderBoxPrintf(BottomRightbutton, "zoom in");
          GFXBorderBoxPrintf(BottomLeftbutton, "zoom out");
        }
        DataChanged = true;
      }
      break;

    case 15:  // wind instrument TRUE Ground ref - experimental
      if (RunSetup) {
        setFont(10);
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        AddTitleInsideBox(8, 2, BigSingleTopRight, " deg");
        DrawCompass(BigSingleDisplay);
        AddTitleInsideBox(8, 3, BigSingleDisplay, "WIND ground ");
      }
      if (millis() > slowdown + 500) {
        slowdown = millis();
      }
      UpdateDataTwoSize(true, true, 9, 8, TopHalfBigSingleTopRight, BoatData.WindAngleApp, "app %.1f");
      WindArrow2(BigSingleDisplay, BoatData.WindSpeedK, BoatData.WindAngleGround);
      UpdateDataTwoSize(true, true, 9, 8, BottomHalfBigSingleTopRight, BoatData.WindAngleGround, "gnd %.1f");

      if (CheckButton(topLeftquarter)) { Display_Page = 4; }
      if (CheckButton(BigSingleDisplay)) { Display_Page = 5; }
      break;
    default:
      Display_Page = 0;
      break;
  }
  LastPageselected = page;
  RunSetup = false;
}

  bool CheckButton(_sButton & button) {  // trigger on release. needs index (s) to remember which button!
    //trigger on release! does not sense !isTouched ..  use Keypressed in each button struct to keep track!
    // if (ts.isTouched && !button.Keypressed && (millis() - button.LastDetect >= 250)) {
    //   if (XYinBox(ts.points[0].x, ts.points[0].y, button.h, button.v, button.width, button.height)) {
    //     //Serial.printf(" Checkbutton size%i state %i %i \n",ts.points[0].size,ts.isTouched,XYinBox(ts.points[0].x, ts.points[0].y,button.h,button.v,button.width,button.height));
    //     button.Keypressed = true;
    //     button.LastDetect = millis();
    //   }
    //   return false;
    // }
    if (button.Keypressed && (millis() - button.LastDetect >= 250)) {
      //Serial.printf(" Checkbutton released from  %i %i\n",button.h,button.v);
      button.Keypressed = false;
      return true;
    }
    return false;
  }

  void setFont(int fontinput) {  //fonts 3..12 are FreeMonoBold in sizes incrementing by 1.5
                               //Notes: May remove some later to save program mem space?
                               // used : 0,1,2,4 for keyboard
                               //      : 0,3,4,8,10,11 in main
  MasterFont = fontinput;
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
      gfx->setFont(&FreeMono8pt7b);
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
    GFXBorderBoxPrintf(button, "");
    AddTitleInsideBox(9, 2, button, " GPS");
    button.PrintLine = 0;
    if (BoatData.SatsInView != NMEA0183DoubleNA) { UpdateLinef(font, button, "Satellites in view %.0f ", BoatData.SatsInView); }
    if (BoatData.GPSTime != NMEA0183DoubleNA) {
      //UpdateLinef(font, button, "");
      UpdateLinef(font, button, "Date: %06i ", int(BoatData.GPSDate));
      //UpdateLinef(font, button, "");
      UpdateLinef(font, button, "TIME: %02i:%02i:%02i",
                  int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60);
    }
    if (BoatData.Latitude.data != NMEA0183DoubleNA) {
      UpdateLinef(font, button, "LAT");
      UpdateLinef(font, button, "%s", LattoString(BoatData.Latitude.data));
      UpdateLinef(font, button, "LON");
      UpdateLinef(font, button, "%s", LongtoString(BoatData.Longitude.data));
      UpdateLinef(font, button, "");
    }
  }
}