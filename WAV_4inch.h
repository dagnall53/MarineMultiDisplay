/*******************************************************************************
Pins and defines for GFX - various versions!
FOR WAVESHARE 4inch LCD 
 ******************************************************************************/
 
#ifndef _ESPGFDEF_H_
#define _ESPGFDEF_H_

//****  Later  GFX VERSIONS HAVE this Refactored  *BUS config and miss the ips (colour inversion??) setup  
// P0 is EX101 P1 is ex102 etc. 
#define EX101 0 //TP_RST
#define EX102 1 //BL_EN
#define EX103 2//LCD_RST
#define EX104 3 //SD_CS

#define GFX_BL EX102 // is port ex102

Arduino_DataBus *bus = new Arduino_SWSPI(
  GFX_NOT_DEFINED /* DC */,
  42 /* CS /12*/,                // Chip Select pin
  2 /* SCK /SCL /11*/,               // Clock pin
  1 /* SDA /10? */,             // Master Out Slave In pin
  GFX_NOT_DEFINED /* MISO */  // Master In Slave Out pin (not used)
);

Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(  // MY BOARD modified pin numbers
  40 /* DE */, 39 /* VSYNC */, 38 /* HSYNC */, 41 /* PCLK */,
  46 /* R0 */, 3 /* R1 */, 8 /* R2 */, 18 /* R3 */, 17 /* R4 */,
  14 /* G0/P22 */, 13 /* G1/P23 */, 12 /* G2/P24 */, 11 /* G3/P25 */, 10 /* G4/P26 */, 9 /* G5 */,
  5 /* B0 */, 45 /* B1 */, 48 /* B2 */, 47 /* B3 */, 21 /* B4 */ ,
  1 /* hsync_polarity */,      // Horizontal sync polarity
  10 /* hsync_front_porch */,  // Horizontal front porch duration
  8 /* hsync_pulse_width */,   // Horizontal pulse width
  50 /* hsync_back_porch */,   // Horizontal back porch duration '80 works as well.. set at 50 
  1 /* vsync_polarity */,      // Vertical sync polarity
  10 /* vsync_front_porch */,  // Vertical front porch duration
  8 /* vsync_pulse_width */,   // Vertical pulse width
  20 /* vsync_back_porch */    // Vertical back porch duration
                               // ,1, 30000000 // Uncomment for additional parameters if needed
);

// Initialize ST7701 display // see comments at end of  https://github.com/Makerfabs/ESP32-S3-Parallel-TFT-with-Touch-4inch
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
  480 /* width */,  480 /* height */,  rgbpanel,
  2 /* rotation */,  true /* auto_flush */,  bus, // as defined in Arduino_DataBus *bus 
  GFX_NOT_DEFINED /* RST */,  st7701_type1_init_operations,  sizeof(st7701_type1_init_operations));  ///DAGNALL NOTE  type 1 selected in GFX clock demo - I think it should be type 9 ?

/* V3.3 compiler & gfx 1.6.0 incompatibility notes:  type9 seems to flicker: 
V3 & 1.5.5 type9 & 1  both crash horribly 
//notes on Arduino_GFX/src/display/Arduino_RGB_Display.h
my location: C:\Users\admin\OneDrive\DocOneDrive\Arduino\libraries\GFX_Library_for_Arduino\src\display\Arduino_RGB_Display.h
COLOUR INVERSION:
type 1 and type 9 differ in IPs settings (WRITE_COMMAND_8, 0x20, // 0x20 normal, 0x21 IPS )
and MDT  (WRITE_C8_D8, 0xCD, 0x00   08/ 00)  MDT: isRGB pixel format argument. MDT=”0”, 'normal'.  MDT=”1”,(set to 8) pixel collect to DB[17:0]. PAGE 276 of manual
Type9 has it set to 0 (Line 1358) - Type1 to 08 
Both use RGB666 which seems essential... but im not sure why as it is physicaly wired 565!

Note: Use Type1 & the colours will be inverted!  
in Type 9 is no setting of 'IPS' (it is commented out), it sets 'MDT' to 0 (Line 1358): and RGB 666 (0x60): this works.
checking combinations to see if I can change flicker effect: using 2.0.17 :
using 0x21 and 0 0x60 :Inverted colours
using 0x21 and CD=0x08 0x60;  still inverted
using 0x20 and CD =0x08 0x60; v. strange partially correct pallette
using  0x20 08  0x50         TRY 565 : 0X50 ...BETTER but not correct coverage of full spectrum
using  0x20,00,50 ;              565 again.. still strange MDT does not seem to affect this ? 
using 0x20, 00 60 Good results 
using //(deleted), 00 60 Good results == type9 default  
 ------------------------------------------------
 WRITE_C8_D8, 0xCD, 0x00 //  08/ 00(Line 1358)
 //WRITE_COMMAND_8, 0x20, // 0x20 normal, 0x21 IPS (Line 1450)
 WRITE_C8_D8, 0x3A, 0x60, // 0x70 RGB888, 0x60 RGB666, 0x50 RGB565 (Line 1451)
 
*/
//** OTHER PINS

#define TFT_BL GFX_BL
#define I2C_SDA_PIN 15
#define I2C_SCL_PIN 7


#define SD_SCK  2
#define SD_MISO 4
#define SD_MOSI 1
#define SD_CS   EX104 // is port ex104

//touch **************************
#include <TAMC_GT911.h>

#define TOUCH_INT 16          //-1 = not connected
#define TOUCH_RST -1          // important both are not -1 ?
#define TOUCH_SDA  15
#define TOUCH_SCL  7
#define TOUCH_WIDTH  480
#define TOUCH_HEIGHT 480

#define TOUCH_ROTATION ROTATION_NORMAL
#define TOUCH_MAP_X1 480
#define TOUCH_MAP_X2 0
#define TOUCH_MAP_Y1 480
#define TOUCH_MAP_Y2 0




#endif // _ESPGFDEF_H_

