/*******************************************************************************
Pins and defines for GFX - various versions!
FOR WAVESHARE 4inch LCD 
https://www.waveshare.com/esp32-s3-touch-lcd-4.htm
https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4

     (V3)
     https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-4/ESP32-S3-Touch-LCD-4_V3.0.pdf

also need ?
#define ARDUINO_USB_MODE 1
#define ARDUINO_USB_CDC_ON_BOOT 1
to get Serial.print working 
(or set in tools-> USB CDC )

 ******************************************************************************/
 
#ifndef _WAV_DEF_H
#define _WAV_DEF_H

const char _device[]=  "WAVSHARE ESP32-S3-Touch-LCD-4";
  #define ESP32_CAN_TX_PIN GPIO_NUM_6  // for the waveshare 4 module boards!
  #define ESP32_CAN_RX_PIN GPIO_NUM_0  // 
// taken from examples 
Arduino_DataBus *bus = new Arduino_SWSPI(
    GFX_NOT_DEFINED /* DC */, 42 /* CS */,
    2 /* SCK */, 1 /* MOSI */, GFX_NOT_DEFINED /* MISO */);

Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    40 /* DE */, 39 /* VSYNC */, 38 /* HSYNC */, 41 /* PCLK */,
    46 /* R0 */, 3 /* R1 */, 8 /* R2 */, 18 /* R3 */, 17 /* R4 */,
    14 /* G0 */, 13 /* G1 */, 12 /* G2 */, 11 /* G3 */, 10 /* G4 */, 9 /* G5 */,
    5 /* B0 */, 45 /* B1 */, 48 /* B2 */, 47 /* B3 */, 21 /* B4 */,
    1 /* hsync_polarity */, 10 /* hsync_front_porch */, 8 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
    1 /* vsync_polarity */, 10 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 20 /* vsync_back_porch */);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    480 /* width */, 480 /* height */, rgbpanel, 2 /* rotation */, true /* auto_flush */,
    bus, GFX_NOT_DEFINED /* RST */, st7701_type1_init_operations, sizeof(st7701_type1_init_operations));


  
/* Exploring Wavshare type X inits for correct colours .. 
MODIFY  Arduino\libraries\GFX_Library_for_Arduino\src\display\Arduino_RGB_Display.h Line 511     
          WRITE_COMMAND_8, 0x21,   // 0x20 normal, 0x21 IPS WRITE_C8_D8, 0x3A, 0x60// 0x70 RGB888, 0x60 RGB666, 0x50 RGB565
    WRITE_COMMAND_8      WRITE_C8_D8 
1       0x21,                0x3A, 0x60          wrong colour details, basic colour ok 
1 MODIFIED 0x21              0x3A  0x50             Should only be changed to 565 but colours still not as good as original 1 
2       0x21,                0x3A, 0x77          (24bit colour) wrong colours overall, 
3,4,5,6,7,8 unusabl
8       0x20                 0x3A, 0x50                      wrong, Top part unused? (like 2?)
9        /commented out        0x3A, 0x60 very wrong but usable
9 modified: 0x21            0x3A, 0x60          like 2 (NOT 1 interestingly, some other factors must be different )
                            0x3A, 0x50                  (same as above.. )
 ------------NOTES :  THESE SEEM THE MOST CRITICAl ETTINGS ------------------------------------
 WRITE_C8_D8, 0xCD, 0x00 //  08/ 00(Line 1358)
 //WRITE_COMMAND_8, 0x20, // 0x20 normal, 0x21 IPS (Line 1450)
 WRITE_C8_D8, 0x3A, 0x60, // 0x70 RGB888, 0x60 RGB666, 0x50 RGB565 (Line 1451)
 
*/

/*******************************************************************************


also need ?
#define ARDUINO_USB_MODE 1
#define ARDUINO_USB_CDC_ON_BOOT 1
to get Serial.print working 
(or set in tools-> USB CDC )

 ******************************************************************************/
 
//** OTHER PINS

//SD card interface 

#define SD_SCK  2
#define SD_MISO 4
#define SD_MOSI 1
#define SDCS  -1 // NOTE not SD_CS, which is a function! is port ex104  ?? Not called up?? 


#define TOUCH_INT 16          
#define TOUCH_RST  254 // -1 not acepted by TAMC GT911 as it is a UINT_8t variable. I modified TAMC and include it in src now to detect 254 as "not present"          //-1-1          // EX101 will reset it at the start ?
#define TOUCH_SDA  15
#define TOUCH_SCL  7

#define TOUCH_WIDTH  480
#define TOUCH_HEIGHT 480

#define TOUCH_ROTATION ROTATION_NORMAL
#define TOUCH_MAP_X1 480
#define TOUCH_MAP_X2 0
#define TOUCH_MAP_Y1 480
#define TOUCH_MAP_Y2 0

#define TOUCH_ROTATION ROTATION_NORMAL


//***         waveshare has a port expander ******************
/*****IO EXPANDER uses 9554 ***********************************
/*
https://github.com/Tinyu-Zhao/PCA9554

*/
// #include <PCA9554.h>     // Load the PCA9554 Library
// #include <Wire.h>        // Load the Wire Library

//  PCA9554 Addressing
//  Address     A2  A1  A0
//  0x20        L   L   L < THIS ONE !!
//  0x21        L   L   H < schematic says this this one on waveshare board
//  0x22        L   H   L
//  0x23        L   H   H
//  0x24        H   L   L
//  0x25        H   L   H
//  0x26        H   H   L
//  0x27        H   H   H
// usage: expander.digitalWrite(n,LOW);expander.digitalWrite(n,HIGH)

// P0 is EX101 P1 is ex102 etc. 
#define EX101 0 //TP_RST TOUCH_RST
#define EX102 1 //BL_EN
#define EX103 2 //LCD_RST
#define EX104 3 //SD_CS  SD_CS(LOW);
#define EX105 4 //TF VLED FB?
#define EX106 5 //BUZZER enable
#define ExpanderSDA 15
#define ExpanderSCL 7
// (same as #define TOUCH_SDA  15
// #define TOUCH_SCL  7)



#endif // _WAV_DEF_H

