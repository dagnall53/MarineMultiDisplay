#include "Display.h"
#include "aux_functions.h"
#include <WiFi.h>
#include <Arduino_GFX_Library.h>  // defines colours BLUE etc
#include <FFat.h>                 // defines FATFS functions for local files
#include <N2kMessages.h>          // isNKNA
#include "Keyboard.h"             // for keyboard functions
#include "JpegFunc.h"
#include "VICTRONBLE.h"  //sets #ifndef Victronble_h


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
extern bool ScanAndConnect(bool display, bool forceFull);

extern void ShowToplinesettings(String Text);
extern void ShowToplinesettings(_sWiFi_settings_Config A, String Text);
extern char* LongtoString(double data);
extern char* LattoString(double data);

extern void DATA_Log_File_Create(fs::FS& fs);


extern boolean CompStruct(_sWiFi_settings_Config A, _sWiFi_settings_Config B);
extern boolean IsConnected;        // may be used in AP_AND_STA to flag connection success (got IP)
extern boolean AttemptingConnect;  // to note that WIFI.begin has been started
extern int NetworksFound;
//for the (new) keyboard
extern char resultBuffer[25];  // same as Password size for simplicity
extern KeyboardMode currentMode;
extern _sButton WIFISHOW;         // for the list of WIFI scanned
char labelBuffer[64];             // Adjust size as needed
const char* label = labelBuffer;  // Now label points to the formatted string

#include "src/TAMC_GT911.h"
extern TAMC_GT911 ts;
extern bool Touch_available;

inline uint16_t color565(uint8_t r, uint8_t g, uint8_t b)
{
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void drawRGBGradientAndBitfieldBars(){
  const int w = 480;
  const int h = 10;
  const int barWidth = w / 6;  // 6 bars for 6 bits per channel
  // ─── RGB Gradient Bars ───
  for (int x = 0; x < w; ++x)
  {
    uint8_t intensity = (x * 255) / (w - 1);

    page->fillRect(x, 0, 1, h, color565(0, 0, intensity));       // Blue
    page->fillRect(x, h, 1, h, color565(0, intensity, 0));       // Green
    page->fillRect(x, 2 * h, 1, h, color565(intensity, 0, 0));   // Red
  }

  // ─── RGB Bitfield Bars ───
  for (int i = 0; i < 6; ++i)  // 6 bits per channel
  {
    uint8_t red   = 1 << i;  // R0–R5 (scaled to 6-bit)
    uint8_t green = 1 << i;  // G0–G5
    uint8_t blue  = 1 << i;  // B0–B5

    // Scale to 8-bit space for color565
    uint8_t red8   = red << 2;
    uint8_t green8 = green << 2;
    uint8_t blue8  = blue << 2;

    int x = i * barWidth;

    page->fillRect(x, 3 * h, barWidth, h, color565(0, 0, blue8));   // Blue bitfield
    page->fillRect(x, 4 * h, barWidth, h, color565(0, green8, 0));  // Green bitfield
    page->fillRect(x, 5 * h, barWidth, h, color565(red8, 0, 0));    // Red bitfield

    DEBUG_PORT.printf("Bitfield Bar %d: R=%d G=%d B=%d\n", i, red8, green8, blue8);
  }
}


void Display(int pageIndex) {
  DEBUG_PORT.printf("IN overlay void Display page<%i> \n", pageIndex);
  Display(false, pageIndex);
}

void DoNewKeyboard() {
  static bool wasTouched;
  drawKeyboard(currentMode);
  if ((ts.isTouched)) {  // Touch is active
    int x = ts.points[0].x;
    int y = ts.points[0].y;
    if (!wasTouched) {
      handleTouch(x, y);  // Process key press once
      wasTouched = true;  // Mark as touched to prevent repeat
    }
  } else {
    wasTouched = false;  // Reset when touch is released
  }
}
void ShowGPSDATA(int font, _sButton &button, _sBoatData BoatData) {
    button.lastY = 2;
    page->UpdateLinef(font, button, " \n");
    if (BoatData.SatsInView != NMEA0183DoubleNA) { page->UpdateLinef(font, button, " Satellites in view %.0f \n", BoatData.SatsInView); }
    page->UpdateLinef(font, button, " Date: %06i \n", int(BoatData.GPSDate));
    page->UpdateLinef(font, button, " TIME: %02i:%02i:%02i\n",
                      int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60);
    if (BoatData.Latitude.data != NMEA0183DoubleNA) {
      page->UpdateLinef(font, button, " LAT: %s\n", LattoString(BoatData.Latitude.data));
      page->UpdateLinef(font, button, " LON: %s\n", LongtoString(BoatData.Longitude.data));
    }
    if (BoatData.MagHeading.data != NMEA0183DoubleNA) { 
      page->UpdateLinef(font, button, "Mag Heading: %.4f\n", BoatData.MagHeading); }
    if ((BoatData.Variation != NMEA0183DoubleNA) && (BoatData.Variation != 0) && !N2kIsNA(BoatData.Variation)) { 
      page->UpdateLinef(font, button, "Variation: %.4f\n", BoatData.Variation); }

 
}

void Display(bool reset, int pageIndex) {  // setups for alternate pages to be selected by pageIndex.
                                           // DEBUG_PORT.printf("IN void Display page<%i> reset:%s \n",pageIndex,reset True_False);delay(50);  // Give UART time to flush

  static unsigned long flashinterval;
  static bool flash, DoScan;
  static double startposlat, startposlon;
  double LatD, LongD;  //deltas
  double wind_gnd;
  int magnification, h, v;

  static int LastPageselected;
  static bool DataChanged;
  static int wifissidpointer;

  // some local variables for tests;
  //char blank[] = "null";
  static bool RunSetup;
  static bool TooLong;
  static unsigned int slowdown, timer2;
  //static float wind, SOG, Depth;
  float temp;
  static _sInstData LocalCopy;  // needed only where two digital displays wanted for the same data variable.
  static int fontlocal;
  char Tempchar[30];  //for fontnames for display

  if (pageIndex != LastPageselected) {
    WIFIGFXBoxdisplaystarted = false;  // will have reset the screen, so turns off the wifibox if there is a  page index change
                                       // this (above) saves a timed screen refresh that can clear keyboard stuff
                                       // DEBUG_PORT.println("IN Display_Page.. load config (Different page)");
    RunSetup = true;
  }
  if (reset) {
    WIFIGFXBoxdisplaystarted = false;
    //   DEBUG_PORT.println("IN Display_Page.. load config (reset)");
    page->clearCanvas(BLUE);
    RunSetup = true;
  }
  //generic setup stuff for ALL pages
  //page->GFXBorderBoxPrintf(StatusBox, "%s Page%i Display monitor %i",Display_Config.PanelName,pageIndex, millis()/100);  // common to all pages
  if (RunSetup) {
    LoadConfiguration();  //Reload configuration in case new data stored
   // DEBUG_PORT.println("IN Display_Page.. Runsetup (page sets..)");
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
    case -99:           // pageIndex for just a blank
      if (RunSetup) {
        page->fillScreen(BLACK);
      }
      page->fillScreen(BLACK);
      break;


    case -200:
      if (RunSetup || DataChanged) {  //logo examples
     
        page->showPicture("/logo.jpg");
        
        page->GFXBorderBoxPrintf(Full0Center, "Jpg tests -Return to Menu-");
        page->GFXBorderBoxPrintf(Full1Center, "logo.jpg");
        page->GFXBorderBoxPrintf(Full2Center, "ColourTestBars");
        page->GFXBorderBoxPrintf(Full3Center, "vicback.jpg");
        page->GFXBorderBoxPrintf(Full4Center, "Bars.jpg");
        page->GFXBorderBoxPrintf(Full5Center, "Colortest.jpg");
        drawRGBGradientAndBitfieldBars();
      }

      if (CheckButton(Full0Center)) { Display_Page = 0; }
      if (CheckButton(Full1Center)) {
        page->showPicture("/logo.jpg");DataChanged=true;
      }
      if (CheckButton(Full2Center)) {
        drawRGBGradientAndBitfieldBars();
        DataChanged=true;
       // page->showPicture("/logo1.jpg");
      }
      if (CheckButton(Full3Center)) {
        page->showPicture("/vicback.jpg");DataChanged=true;
      }
      if (CheckButton(Full4Center)) {
        page->showPicture("/Bars.jpg");DataChanged=true;  // drawJPEGToTextCanvas and showPicture were written / corrected at separate points: showPicture was updated to be the same and needs to be asimilated
      }
      if (CheckButton(Full5Center)) {
        page->showPicture("/Colortest.jpg");DataChanged=true;
      }

      break;

    case -199:  //a test for Screen Colours / fonts
      if (RunSetup) {
        page->fillScreen(BLACK);
        page->setTextColor(WHITE);
        fontlocal = 0;
        //setFont(fontlocal);
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
        page->clearCanvas(BLACK);
        page->showPicture("/vicback.jpg");  // page->Jpegshow ??
      }
      // all graphics done in VICTRONBLE
      if (CheckButton(StatusBox)) { Display_Page = 0; }  // go to settings
      if (CheckButton(FullTopCenter)) { Display_Page = -86; }
      break;
    case -86:                                                    // pageIndex for text display of Vicron data
      if (RunSetup) { page->GFXBorderBoxPrintf(Terminal, ""); }  // only for setup, not changed data
      if (RunSetup || DataChanged) {
        //setFont(3);  // different from most pages, displays in terminal from see ~line 2145
        page->GFXBorderBoxPrintf(FullTopCenter, "VICTRON Graphic Display");
        if (!Terminal.debugpause) {
          page->Addtitletobutton(Terminal, 1, 0, "-running-");
        } else {
          page->Addtitletobutton(Terminal, 1, 0, "-paused-");
        }
        // page->GFXBorderBoxPrintf(Switch6, Current_Settings.Data_Log_ON On_Off);
        // page->Addtitletobutton(Switch6, 1, 0, " LOG ");

        page->GFXBorderBoxPrintf(Switch7, victronDevices.Beacons On_Off);
        page->Addtitletobutton(Switch7, 1, 0, "Beacon");
        page->GFXBorderBoxPrintf(Switch8, victronDevices.BLEDebug On_Off);
        page->Addtitletobutton(Switch8, 1, 0, "V-Debug");
        page->GFXBorderBoxPrintf(Switch9, victronDevices.Simulate On_Off);
        page->Addtitletobutton(Switch9, 1, 0, " Sim.");
        page->GFXBorderBoxPrintf(Switch10, Current_Settings.BLE_enable On_Off);
        page->Addtitletobutton(Switch10, 1, 0, "BLE-ON");
        page->GFXBorderBoxPrintf(Switch11, ColorSettings.SerialOUT On_Off);
        page->Addtitletobutton(Switch11, 1, 0, "Send>USB");
        DataChanged = false;
      }
      if (!Terminal.debugpause) {
        page->Addtitletobutton(Terminal, 1, 0, "-running-");
      } else {
        page->Addtitletobutton(Terminal, 1, 0, "-paused-");
      }
      // if (millis() > slowdown + 500) {
      //   slowdown = millis();
      // }

      // if (CheckButton(Switch6)) {
      //   Current_Settings.Data_Log_ON = !Current_Settings.Data_Log_ON;
      //   if (Current_Settings.Data_Log_ON) { DATA_Log_File_Create(FFat); }
      //   DataChanged = true;
      // };
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
          page->Addtitletobutton(Terminal, 1, 0, "-running-");
        } else {
          page->Addtitletobutton(Terminal, 1, 0, "-paused-");
        }
      }

      break;

    case -22:                                                    //  "EXPERIMENT in N2K data"
      if (RunSetup) { page->GFXBorderBoxPrintf(Terminal, ""); }  // only for setup, not changed data
      if (RunSetup || DataChanged) {
        page->GFXBorderBoxPrintf(FullTopCenter, "N2K debug ");
        if (!Terminal.debugpause) {
          page->Addtitletobutton(Terminal, 1, 0, "TERMINAL");
        } else {
          page->Addtitletobutton(Terminal, 1, 0, "-Paused-");
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
          page->Addtitletobutton(Terminal, 1, 0, "-running-");
        } else {
          page->Addtitletobutton(Terminal, 1, 0, "-paused-");
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
        //setFont(3);
        //setFont(3);
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
        //setFont(3);
        page->GFXBorderBoxPrintf(FullTopCenter, "Boat/NMEA  Source selects");
        //if (hasSD) {page->GFXBorderBoxPrintf(Switch6, Current_Settings.Log_ON On_Off);
        // page->Addtitletobutton(Switch6, "B LOG");
        // page->GFXBorderBoxPrintf(Switch7, Current_Settings.Data_Log_ON On_Off);
        // page->Addtitletobutton(Switch7, "N LOG");}
        page->GFXBorderBoxPrintf(Switch8, Current_Settings.UDP_ON On_Off);
        page->Addtitletobutton(Switch8, 1, 0, "UDP");
        page->GFXBorderBoxPrintf(Switch9, Current_Settings.ESP_NOW_ON On_Off);
        page->Addtitletobutton(Switch9, 1, 0, "ESP-N");
        page->GFXBorderBoxPrintf(Switch10, Current_Settings.N2K_ON On_Off);
        page->Addtitletobutton(Switch10, 1, 0, "N2K");
        if (!Terminal.debugpause) {
          page->Addtitletobutton(Terminal, 1, 0, "TERMINAL");
        } else {
          page->Addtitletobutton(Terminal, 1, 0, "-Paused-");
        }
        DataChanged = false;
      }
      if (millis() > slowdown + 1000) {
        slowdown = millis();
        page->GFXBorderBoxPrintf(Switch11, CompStruct(Saved_Settings, Current_Settings) ? "-same-" : "UPDATE");
        page->Addtitletobutton(Switch11, 1, 0, "FLASH");
      }
      if (CheckButton(FullTopCenter)) { Display_Page = 0; }
      if (CheckButton(Terminal)) {
        Terminal.debugpause = !Terminal.debugpause;
        DataChanged = true;
        if (!Terminal.debugpause) {
          page->Addtitletobutton(Terminal, 1, 0, "-running-");
        } else {
          page->Addtitletobutton(Terminal, 1, 0, "-paused-");
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
      }
      if (millis() > timer2 + 500) {
        timer2 = millis();
        temp = random(-9000, 9000);
        temp = temp / 1000;
        Fontname.toCharArray(Tempchar, 30, 0);
        // Measure parts
        int16_t x1, y1;
        uint16_t w1, h1;
        gfx->getTextBounds("9", 0, 0, &x1, &y1, &w1, &h1);
        page->GFXBorderBoxPrintf(CurrentSettingsBox, "FONT:%i name%s height<%i>", fontlocal, fontNameTable[fontlocal], h1);
        FontBox.Font = fontlocal;
        page->GFXBorderBoxPrintf(FontBox, "Test %4.2f", temp);
        DataChanged = false;
        page->GFXBorderBoxPrintf(MidLeftButton, "font -");
        page->GFXBorderBoxPrintf(MidRightButton, "font +");
      }
      if (CheckButton(Full0Center)) { Display_Page = 0; }

      if (CheckButton(MidLeftButton)) {
        fontlocal = fontlocal - 1;
        DataChanged = true;
      }
      if (CheckButton(MidRightButton)) {
        fontlocal = fontlocal + 1;
        DataChanged = true;
      }
      break;
    case -9:  ///Touchscreen pointer tests
      if (RunSetup || DataChanged) {
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

      if (RunSetup) {
        DoScan = true;
      } else {
        if (DoScan) {
          AttemptingConnect = false;  // so that Scan can do a full scan..
          ScanAndConnect(false, true);
          AttemptingConnect = true;
          DoScan = false;
        }
      }
      if (IsConnected) {
        page->GFXBorderBoxPrintf(TOPButton, "Connected<%s>", Current_Settings.ssid);
      } else {
        page->GFXBorderBoxPrintf(TOPButton, "NOT Connected:%s", Current_Settings.ssid);
      }

      if (DoScan) {
        page->GFXBorderBoxPrintf(SecondRowButton, "WiFi scanning");
      } else {
        page->GFXBorderBoxPrintf(TopRightbutton, "re Scan?");
      }


      if (NetworksFound <= 0) {  // note scan error can give negative number
        NetworksFound = 0;
      } else {
        //page->GFXBorderBoxPrintf(SecondRowButton, " %i Networks Found", NetworksFound);
        for (int i = 0; i < NetworksFound; ++i) {
          // Print SSID and RSSI for each network found
          if (WiFi.SSID(i).length() > 20) {
            snprintf(labelBuffer, sizeof(labelBuffer),
                     "%i %s ",
                     i + 1,
                     WiFi.SSID(i).substring(0, 20).c_str());
            //drawBoxedKey(WIFISHOW, 0, 75 + ((i + 1) * WIFISHOW.height), 400, 40, " Too long", 9);
          } else {
            snprintf(labelBuffer, sizeof(labelBuffer),
                     "%i %s (%i) ch<%i>",
                     i + 1,
                     WiFi.SSID(i).substring(0, 20).c_str(),
                     WiFi.RSSI(i),
                     WiFi.channel(i));
          }
          drawBoxedKey(WIFISHOW, 0, 75 + ((i + 1) * WIFISHOW.height), 400, 40, label, 8);
        }
        // page->println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
        delay(10);
        DataChanged = false;
      }

      page->GFXBorderBoxPrintf(Switch5a, CompStruct(Saved_Settings, Current_Settings) ? "-same-" : "UPDATE");
      page->Addtitletobutton(Switch5a, 1, 0, "EEPROM");

      if (millis() > slowdown + 1000) {
        slowdown = millis();
        //other stuff?
      }

      if (Touch_available) {
        if ((ts.isTouched) && (ts.points[0].y >= 105)) {  // nb check on location on screen or scan will get reset when you press one of the top boxes
          //TouchCrosshair(1);
          wifissidpointer = ((ts.points[0].y - 75) / WIFISHOW.height) - 1;
          int str_len = WiFi.SSID(wifissidpointer).length() + 1;
          char result[str_len];
          // DEBUG_PORT.printf(" touched at %i  equates to %i ? %s ", ts.points[0].y, wifissidpointer, WiFi.SSID(wifissidpointer));
          // DEBUG_PORT.printf("  result str_len%i   sizeof settings.ssid%i \n", str_len, sizeof(Current_Settings.ssid));
          if (str_len <= sizeof(Current_Settings.ssid)) {                                       // check small enough for our ssid register array!
            WiFi.SSID(wifissidpointer).toCharArray(result, sizeof(Current_Settings.ssid) - 1);  // I like to keep a spare space!
            if (str_len == 1) {
              TooLong = true;
              page->GFXBorderBoxPrintf(SecondRowButton, "Set via Keyboard?");
            } else {
              page->GFXBorderBoxPrintf(SecondRowButton, "Select<%s>?", result);
              TooLong = false;
            }
          } else {
            TooLong = true;
            page->GFXBorderBoxPrintf(SecondRowButton, "ssid too long ! ");
          }
        }
      }
      if (CheckButton(SecondRowButton)) {
        if (!TooLong) {
          WiFi.SSID(wifissidpointer).toCharArray(Current_Settings.ssid, sizeof(Current_Settings.ssid) - 1);
          SaveConfiguration();
          Display_Page = -1;
        } else {
          Display_Page = -2;
        }
        //
      }


      if (CheckButton(TopRightbutton)) {

        AttemptingConnect = false;  // so that Scan can do a full scan..
        ScanAndConnect(false, true);
        AttemptingConnect = true;  // so scanandconnect in main loop does not run again! (updates networks and makes screen wrong!)
        DataChanged = true;
      }  // do the scan again

      if (CheckButton(Switch5a)) {
        SaveConfiguration();  //(Display_Config, Current_Settings);
        delay(50);
        // Display_Page = 0;
        DataChanged = true;
      };
      //if (CheckButton(TOPButton)) { Display_Page = -1; }
      break;

    case -4:  // Keyboard setting of UDP port - note keyboard (2) numbers start
      if (RunSetup || DataChanged) {
        page->GFXBorderBoxPrintf(TOPButton, "UDP_PORT is<%s>", Current_Settings.UDP_PORT);
      }
      page->GFXBorderBoxPrintf(ThirdRowButton, "Set<%s>?", resultBuffer);
      DoNewKeyboard();
      if (CheckButton(ThirdRowButton)) {
        strncpy(Current_Settings.UDP_PORT, resultBuffer, sizeof(Current_Settings.UDP_PORT));
        Current_Settings.UDP_PORT[sizeof(Current_Settings.UDP_PORT) - 1] = '\0';
        SaveConfiguration();
        DataChanged = true;
        Display_Page = -1;
      }

      if (CheckButton(TOPButton)) { Display_Page = -1; }
      break;

    case -3:  // keyboard setting of Password
      if (RunSetup || DataChanged) {
        page->GFXBorderBoxPrintf(TOPButton, "pw=<%s>", Current_Settings.password);
      }
      page->GFXBorderBoxPrintf(ThirdRowButton, "Set<%s>?", resultBuffer);
      DoNewKeyboard();


      if (CheckButton(ThirdRowButton)) {
        strncpy(Current_Settings.password, resultBuffer, sizeof(Current_Settings.password));
        Current_Settings.password[sizeof(Current_Settings.password) - 1] = '\0';
        SaveConfiguration();
        DataChanged = true;
        Display_Page = -1;
      }
      if (CheckButton(TOPButton)) { Display_Page = -1; }

      break;

    case -2:  //Keyboard set of SSID
      if (RunSetup || DataChanged) {
        page->GFXBorderBoxPrintf(TOPButton, "ssid=<%s>", Current_Settings.ssid);
      }
      page->GFXBorderBoxPrintf(ThirdRowButton, "Set<%s>?", resultBuffer);
      DoNewKeyboard();
      if (CheckButton(ThirdRowButton)) {
        strncpy(Current_Settings.ssid, resultBuffer, sizeof(Current_Settings.ssid));
        Current_Settings.ssid[sizeof(Current_Settings.ssid) - 1] = '\0';
        SaveConfiguration();
        DataChanged = true;
        Display_Page = -1;
      }

      if (CheckButton(TOPButton)) { Display_Page = -1; }
      break;

    case -1:  // this is the WIFI settings pageIndex
      if (RunSetup || DataChanged) {
        ShowToplinesettings(Saved_Settings, " Flash/JSON ");
      }
      page->GFXBorderBoxPrintf(Full0Center, "SSID <%s>", Current_Settings.ssid);
      if (IsConnected) {
        page->Addtitletobutton(Full0Center, 1, 0, "Current Setting <CONNECTED>");
      } else {
        page->Addtitletobutton(Full0Center, 1, 0, "Current Setting <NOT CONNECTED>");
      }
      page->GFXBorderBoxPrintf(Full1Center, "Password <%s>", Current_Settings.password);
      page->Addtitletobutton(Full1Center, 1, 0, "Current Setting");
      page->GFXBorderBoxPrintf(Full2Center, "UDP Port <%s>", Current_Settings.UDP_PORT);
      page->Addtitletobutton(Full2Center, 1, 0, "Current Setting");

      page->GFXBorderBoxPrintf(Switch0, Current_Settings.Serial_on On_Off);  //A.Serial_on On_Off,  A.UDP_ON On_Off, A.ESP_NOW_ON On_Off
      page->Addtitletobutton(Switch0, 1, 0, "Serial");
      page->GFXBorderBoxPrintf(Switch1, Current_Settings.UDP_ON On_Off);
      page->Addtitletobutton(Switch1, 1, 0, "UDP");
      page->GFXBorderBoxPrintf(Switch2, Current_Settings.ESP_NOW_ON On_Off);
      page->Addtitletobutton(Switch2, 1, 0, "ESP-Now");
      // DEBUG_PORT.printf(" Compare Saved and Current <%s> \n", CompStruct(Saved_Settings, Current_Settings) ? "-same-" : "UPDATE");
      page->GFXBorderBoxPrintf(Switch5, CompStruct(Saved_Settings, Current_Settings) ? "-same-" : "UPDATE");
      page->Addtitletobutton(Switch5, 1, 0, "EEPROM");
      page->GFXBorderBoxPrintf(Full5Center, "Logger and Debug");


      if (millis() > slowdown + 1000) {
        slowdown = millis();
      }
      //runsetup to repopulate the text in the boxes!
      if (CheckButton(Switch0)) {
        Current_Settings.Serial_on = !Current_Settings.Serial_on;
        DataChanged = true;
      };
      if (CheckButton(Switch1)) {
        Current_Settings.UDP_ON = !Current_Settings.UDP_ON;
        DataChanged = true;
      };
      if (CheckButton(Switch2)) {
        Current_Settings.ESP_NOW_ON = !Current_Settings.ESP_NOW_ON;
        DataChanged = true;
      };

      if (CheckButton(Switch5)) {
        SaveConfiguration();  //(Display_Config, Current_Settings);
        delay(50);
        // Display_Page = 0;
        DataChanged = true;
      };

      //if (CheckButton(TOPButton)) { Display_Page = 0; }
      //if (CheckButton(Full0Center)) { Display_Page = 0; }
      if (CheckButton(Full0Center)) { Display_Page = -5; };
      if (CheckButton(Full1Center)) { Display_Page = -3; };
      if (CheckButton(Full2Center)) { Display_Page = -4; };
      if (CheckButton(Full5Center)) { Display_Page = -21; };
      break;

    case 0:  // main settings
      if (RunSetup) {
        page->clearCanvas(BLUE);
        ShowToplinesettings("Settings Now: ");
        page->GFXBorderBoxPrintf(Full0Center, "-Experimental-");
        page->GFXBorderBoxPrintf(Full1Center, "WIFI Settings");
        page->GFXBorderBoxPrintf(Full2Center, "NMEA DISPLAY");
        page->GFXBorderBoxPrintf(Full3Center, "NMEA Debug");
        page->GFXBorderBoxPrintf(Full4Center, "GPS Display");
        page->GFXBorderBoxPrintf(Full5Center, "BLE DEBUG");
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
        //setFont(10);
        page->fillScreen(BLACK);
        page->GFXBorderBoxPrintf(topLeftquarter, "will fill with data");
        page->GFXBorderBoxPrintf(topRightquarter, "will fill with data");
        page->GFXBorderBoxPrintf(bottomRightquarter, "will fill with data");
        page->GFXBorderBoxPrintf(bottomLeftquarter, "will fill with data");
        if (_WideDisplay) { page->GFXBorderBoxPrintf(WideScreenCentral, "will fill with data"); }
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
      }
      page->GFXBorderBoxPrintf(BigSingleTopLeft, "Click for graphic");
      page->AutoPrint2Size(TopHalfBigSingleTopRight, "19.9", "%.1f\n", BoatData.SOG.data);
      page->AddTitleInsideBox(TopHalfBigSingleTopRight, 1, 8, " SOG ");
      page->AutoPrint2Size(BottomHalfBigSingleTopRight, "19.9", "%.1f\n", BoatData.COG.data);
      page->AddTitleInsideBox(BottomHalfBigSingleTopRight, 1, 8, " COG ");

      if (millis() > slowdown + 1000) {
        slowdown = millis();
        page->GFXBorderBoxPrintf(BigSingleDisplay, "");
       if (BoatData.GPSTime != NMEA0183DoubleNA) {  ShowGPSDATA(9, BigSingleDisplay,BoatData);}
       }
      if (CheckButton(BigSingleTopLeft)) { Display_Page = 10; }
      //if (CheckButton(bottomLeftquarter)) { Display_Page = 4; }  //Loop to the main settings pageIndex
      break;

    case 10:  // GPS pageIndex 2 sort of anchor watch
      static double magnification;
      float pixel;
      pixel = magnification / 111111;
      if (RunSetup) { magnification = 1111111; }
      if (RunSetup || DataChanged) {
        page->fillScreen(BLUE);
        page->clearCanvas(BLUE);  // fill screen not work, clear canvas does?        
        
        DrawGPSPlot(true,  BoatData, 1);  //draws circle
      //  page->fillCircle(BigSingleDisplay.h + (BigSingleDisplay.width / 2), BigSingleDisplay.v + (BigSingleDisplay.height / 2), (BigSingleDisplay.height) / 2, BigSingleDisplay.BorderColor);
        DataChanged = false;
      }
      if (millis() > slowdown + 1000) {
        slowdown = millis();

      //  page->drawRoundRect(200, 200, 170, 130, 10, GREEN); easy place to put graphics tests! 
      //page->drawBoatOutline(BigSingleDisplay.h + (BigSingleDisplay.width / 2), BigSingleDisplay.v + (BigSingleDisplay.height / 2),150);
      //page->fillCircle(100, 200, 5, RED);
        page->drawCircle(BigSingleDisplay.h + (BigSingleDisplay.width / 2), BigSingleDisplay.v + (BigSingleDisplay.height / 2), (BigSingleDisplay.height) / 2, WHITE);
        page->GFXBorderBoxPrintf(BigSingleTopLeft, "");
        if (BoatData.GPSTime != NMEA0183DoubleNA) {  ShowGPSDATA(8, BigSingleTopLeft,BoatData);}  
        page->AddTitleInsideBox(BigSingleDisplay, 2, 9, "Magnification= %4.1f pixels/m", pixel);
        page->AddTitleInsideBox(BigSingleDisplay, 1, 9, "Circle= %4.1f m", float((BigSingleDisplay.height) / (2 * pixel)));
        page->GFXBorderBoxPrintf(BigSingleTopRight, "Show Quad Display");
        page->GFXBorderBoxPrintf(BottomRightbutton, "Zoom in");
        page->GFXBorderBoxPrintf(BottomLeftbutton, "Zoom out");
        DrawGPSPlot(false,  BoatData, magnification);
      }
      if (CheckButton(topLeftquarter)) { Display_Page = 9; }
      if (CheckButton(BigSingleTopRight)) { Display_Page = 4; }

      if (CheckButton(BottomRightbutton)) {
        magnification = magnification * 1.5;
        DEBUG_PORT.printf(" magification  %f \n", magnification);
        DataChanged = true;
      }
      if (CheckButton(BottomLeftbutton)) {
        magnification = magnification / 1.5;
        DEBUG_PORT.printf(" magification  %f \n", magnification);
        DataChanged = true;
      }
      if (CheckButton(BigSingleDisplay)) {  // press plot to recenter plot
        if (BoatData.Latitude.data != NMEA0183DoubleNA) {
          // DEBUG_PORT.printf("Ture valu n GrawGPS updtes static variables  reset center anchorwatch %f   %f \n", BoatData.Latitude.data, BoatData.Longitude.data);
          DrawGPSPlot(true, BoatData, magnification);
          DataChanged = true;
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
  page->fillCircle(ts.points[point].x, ts.points[point].y, 10, WHITE);
  TinyButton.h = ts.points[point].x;
  TinyButton.v = ts.points[point].y;
  page->DrawBox(TinyButton);
  page->drawFastVLine(ts.points[point].x, ts.points[point].y - size, 2 * size, colour);
  page->drawFastHLine(ts.points[point].x - size, ts.points[point].y, 2 * size, colour);
}


bool CheckButton(_sButton& button) {  // trigger on release. needs index (s) to remember which button!
  if (Touch_available) {
    //trigger on release! does not sense !isTouched ..  use Keypressed in each button struct to keep track!
    if (ts.isTouched && !button.Keypressed && (millis() - button.LastDetect >= 250)) {
      if (XYinBox(ts.points[0].x, ts.points[0].y, button.h, button.v, button.width, button.height)) {
        //DEBUG_PORT.printf(" Checkbutton size%i state %i %i \n",ts.points[0].size,ts.isTouched,XYinBox(ts.points[0].x, ts.points[0].y,button.h,button.v,button.width,button.height));
        button.Keypressed = true;
        //button.BorderColor= BLACK;
        button.LastDetect = millis();
      }
      return false;
    }
  }
  if (button.Keypressed && (millis() - button.LastDetect >= 250)) {
    //DEBUG_PORT.printf(" Checkbutton released from  %i %i\n",button.h,button.v);
    button.Keypressed = false;
    //button.BorderColor= WHITE;
    return true;
  }
  return false;
}



void ShowGPSinBox(int font, _sButton button) {
  static double lastTime;
  //DEBUG_PORT.printf("In ShowGPSinBox  %i\n",int(BoatData.GPSTime));
  if ((BoatData.GPSTime != NMEA0183DoubleNA) && (BoatData.GPSTime != lastTime)) {
    lastTime = BoatData.GPSTime;
    //  page->GFXBorderBoxPrintf(button, "");
    page->AddTitleInsideBox(button, 9, 2, " GPS");
    button.PrintLine = 0;
    if (BoatData.SatsInView != NMEA0183DoubleNA) { page->UpdateLinef(font, button, "Satellites in view %.0f ", BoatData.SatsInView); }
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
  if ((Choice == "WIND") && (BoatData.WindSpeedK.data != NMEA0183DoubleNA) && (!BoatData.WindAngleApp.displayed)) {
    page->DrawCompass(Position);
    page->drawCompassPointer(Position, 20, 50, BoatData.WindAngleApp.data, WHITE, true);
    page->AddTitleInsideBox(Position, 6, 9, "Apparent:%.1fkts", BoatData.WindSpeedK.data);
    BoatData.WindAngleApp.displayed = true;
  }
  /*if (selected dta .data != NMEA0183DoubleNA) */
  page->setShadowX(4);
  page->setShadowY(4);
  if ((Choice == "SOG") && (BoatData.SOG.data != NMEA0183DoubleNA) && (!BoatData.SOG.displayed)) {
    page->AutoPrint2Size(Position, "19.9", "%.1f", BoatData.SOG.data);
    page->AddTitleInsideBox(Position, 6, 9, " SOG ");
    page->AddTitleInsideBox(Position, 3, 9, " Kts ");
    BoatData.SOG.displayed = true;
  }
  if ((Choice == "STW") && (BoatData.STW.data != NMEA0183DoubleNA) && (!BoatData.STW.displayed)) {
    page->AutoPrint2Size(Position, "19.9", "%.1f", BoatData.STW.data);
    page->AddTitleInsideBox(Position, 6, 9, " STW ");
    page->AddTitleInsideBox(Position, 3, 9, " Kts ");
    BoatData.STW.displayed = true;
  }
  if ((Choice == "DEPTH") && (BoatData.WaterDepth.data != NMEA0183DoubleNA) && (!BoatData.WaterDepth.displayed)) {
    page->AutoPrint2Size(Position, "199.9", "%.1f", BoatData.WaterDepth.data);
    page->AddTitleInsideBox(Position, 6, 9, " DEPTH ");
    page->AddTitleInsideBox(Position, 3, 9, " m ");
    BoatData.WaterDepth.displayed = true;
  }
  page->setShadowX(0);
  page->setShadowY(0);
  if (Choice == "DGRAPH") {
    page->DrawScrollingGraph(Position, DepthBuffer, 10, 0);  //int min= 20;int max=0;    min, max);
    page->AddTitleInsideBox(Position, 1, 0, "Fathmometer 10m");
    page->AddTitleInsideBox(Position, 2, 0, "surface");
    page->AddTitleInsideBox(Position, 3, 0, "MIN:%i ", 20);
  }
  if (Choice == "DGRAPH2") {
    page->DrawScrollingGraph(Position, DepthBuffer, 50, 0);  //int min= 20;int max=0;    min, max);
    page->AddTitleInsideBox(Position, 1, 0, "Fathmometer 50m");
    page->AddTitleInsideBox(Position, 2, 0, "surface");
    page->AddTitleInsideBox(Position, 3, 0, "MIN:%i ", 50);
  }


  // if (Choice == "DGRAPH2") { SCROLLGraph(RunSetup, Instance, 1, true, Position, BoatData.WaterDepth, 50, 0, 8, "Fathmometer 50m ", "m"); }
  if (Choice == "STWGRAPH") { 
     page->DrawScrollingGraph(Position, STWBuffer, 0, 10);  //
    page->AddTitleInsideBox(Position, 1, 0, "STW ");
    page->AddTitleInsideBox(Position, 2, 0, "10Kt");
    page->AddTitleInsideBox(Position, 3, 0, "MIN:%i ", 0);
     }
  if (Choice == "SOGGRAPH") {     
    page->DrawScrollingGraph(Position, SOGBuffer, 0, 10);  //
    page->AddTitleInsideBox(Position, 1, 0, "SOG");
    page->AddTitleInsideBox(Position, 2, 0, "10 Kt");
    page->AddTitleInsideBox(Position, 3, 0, "MIN:%i ", 0);
     }
  if (Choice == "GPS") { ShowGPSinBox(9, Position); }
  if (Choice == "TIME") {
    if (millis() > slowdown + 10000) {  //FOR the TIME display only make/update copies every 10 second!  else undisplayed copies will be redrawn!
      slowdown = millis();
      page->AutoPrint2Size(Position, "19.99", "%02i:%02i",
                               int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60);
      page->AddTitleInsideBox(Position, 6, 9, "UTC ");
      //setFont(10);
    }
  }
  if (Choice == "TIMEL") {
    if (millis() > slowdown + 10000) {  //FOR the TIME display only make/update copies every 10 second!  else undisplayed copies will be redrawn!
      slowdown = millis();
      page->AutoPrint2Size(Position, "19.99", "%02i:%02i",
                               int(BoatData.LOCTime) / 3600, (int(BoatData.LOCTime) % 3600) / 60);
      page->AddTitleInsideBox(Position, 6, 9, "LOCAL ");
      //setFont(10);
    }
  }
} 