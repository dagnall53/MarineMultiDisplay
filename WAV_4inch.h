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
to get DEBUG_PORT.print working 
(or set in tools-> USB CDC )

 ******************************************************************************/
 
#ifndef _WAV_DEF_H
#define _WAV_DEF_H


//define the operations here so I can modify if needed without losing edits if GFX for Arduino is updated 
static const uint8_t st7701_type1b_init_operations[] = {
    BEGIN_WRITE,
    WRITE_COMMAND_8, 0xFF,
    WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x10,

    WRITE_C8_D16, 0xC0, 0x3B, 0x00,
    WRITE_C8_D16, 0xC1, 0x0D, 0x02,
    WRITE_C8_D16, 0xC2, 0x31, 0x05,
    WRITE_C8_D8, 0xCD, 0x08,          // guitron uses 00 

    WRITE_COMMAND_8, 0xB0, // Positive Voltage Gamma Control
    WRITE_BYTES, 16,
    0x00, 0x11, 0x18, 0x0E,
    0x11, 0x06, 0x07, 0x08,
    0x07, 0x22, 0x04, 0x12,
    0x0F, 0xAA, 0x31, 0x18,

    WRITE_COMMAND_8, 0xB1, // Negative Voltage Gamma Control
    WRITE_BYTES, 16,
    0x00, 0x11, 0x19, 0x0E,
    0x12, 0x07, 0x08, 0x08,
    0x08, 0x22, 0x04, 0x11,
    0x11, 0xA9, 0x32, 0x18,

    // PAGE1
    WRITE_COMMAND_8, 0xFF,
    WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x11,

    WRITE_C8_D8, 0xB0, 0x60, // Vop=4.7375v
    WRITE_C8_D8, 0xB1, 0x32, // VCOM=32
    WRITE_C8_D8, 0xB2, 0x07, // VGH=15v
    WRITE_C8_D8, 0xB3, 0x80,
    WRITE_C8_D8, 0xB5, 0x49, // VGL=-10.17v
    WRITE_C8_D8, 0xB7, 0x85,
    WRITE_C8_D8, 0xB8, 0x21, // AVDD=6.6 & AVCL=-4.6
    WRITE_C8_D8, 0xC1, 0x78,
    WRITE_C8_D8, 0xC2, 0x78,

    WRITE_COMMAND_8, 0xE0,
    WRITE_BYTES, 3, 0x00, 0x1B, 0x02,

    WRITE_COMMAND_8, 0xE1,
    WRITE_BYTES, 11,
    0x08, 0xA0, 0x00, 0x00,
    0x07, 0xA0, 0x00, 0x00,
    0x00, 0x44, 0x44,

    WRITE_COMMAND_8, 0xE2,
    WRITE_BYTES, 12,
    0x11, 0x11, 0x44, 0x44,
    0xED, 0xA0, 0x00, 0x00,
    0xEC, 0xA0, 0x00, 0x00,

    WRITE_COMMAND_8, 0xE3,
    WRITE_BYTES, 4, 0x00, 0x00, 0x11, 0x11,

    WRITE_C8_D16, 0xE4, 0x44, 0x44,

    WRITE_COMMAND_8, 0xE5,
    WRITE_BYTES, 16,
    0x0A, 0xE9, 0xD8, 0xA0,
    0x0C, 0xEB, 0xD8, 0xA0,
    0x0E, 0xED, 0xD8, 0xA0,
    0x10, 0xEF, 0xD8, 0xA0,

    WRITE_COMMAND_8, 0xE6,
    WRITE_BYTES, 4, 0x00, 0x00, 0x11, 0x11,

    WRITE_C8_D16, 0xE7, 0x44, 0x44,

    WRITE_COMMAND_8, 0xE8,
    WRITE_BYTES, 16,
    0x09, 0xE8, 0xD8, 0xA0,
    0x0B, 0xEA, 0xD8, 0xA0,
    0x0D, 0xEC, 0xD8, 0xA0,
    0x0F, 0xEE, 0xD8, 0xA0,

    WRITE_COMMAND_8, 0xEB,
    WRITE_BYTES, 7,
    0x02, 0x00, 0xE4, 0xE4,
    0x88, 0x00, 0x40,

    WRITE_C8_D16, 0xEC, 0x3C, 0x00,

    WRITE_COMMAND_8, 0xED,
    WRITE_BYTES, 16,
    0xAB, 0x89, 0x76, 0x54,
    0x02, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0x20,
    0x45, 0x67, 0x98, 0xBA,

    //-----------VAP & VAN---------------
    WRITE_COMMAND_8, 0xFF,
    WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x13,

    WRITE_C8_D8, 0xE5, 0xE4,

    WRITE_COMMAND_8, 0xFF,
    WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x00,

    WRITE_COMMAND_8, 0x21,   // 0x20 normal, 0x21 IPS
    WRITE_C8_D8, 0x3A, 0x60, // 0x70 RGB888, 0x60 RGB666, 0x50 RGB565

    WRITE_COMMAND_8, 0x11, // Sleep Out
    END_WRITE,

    DELAY, 120,

    BEGIN_WRITE,
    WRITE_COMMAND_8, 0x29, // Display On
    END_WRITE};

const char _device[]=  "WAVSHARE ESP32-S3-Touch-LCD-4 480x480";
  #define ESP32_CAN_TX_PIN GPIO_NUM_6  // for the waveshare 4 module boards!
  #define ESP32_CAN_RX_PIN GPIO_NUM_0  // 
// taken from examples 
Arduino_DataBus *bus = new Arduino_SWSPI(
    GFX_NOT_DEFINED /* DC */, 42 /* CS */,
    2 /* SCK */, 1 /* MOSI */, GFX_NOT_DEFINED /* MISO */);

Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    40 /* DE */, 39 /* VSYNC */, 38 /* HSYNC */, 41 /* PCLK */,
    46 /* R0 */, 3 /* R1 */, 8 /* R2 */, 18 /* R3 */, 17 /* R4 */,
     //10, 11 , 14 , 13 , 12, 9 ,  //??
     //  11, 10 , 14 , 13 , 12, 9 ,  //?? darkest is now a mid intensity
    10, 14 , 13 , 12 , 11, 9 ,  //??better seems like a big step half way on gradient test but block colours ok and in intensity order G0... G5  
    //14 /* G0 */, 13 /* G1 */, 12 /* G2 */, 11 /* G3 */, 10 /* G4 */, 9 /* G5 */, // Brightest expected block is bright, but next lower intensity is Black then rest decrease in intensity to the left   original as per other wavshare examples 
    5 /* B0 */, 45 /* B1 */, 48 /* B2 */, 47 /* B3 */, 21 /* B4 */,
    1 /* hsync_polarity */, 10 /* hsync_front_porch */, 8 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
    1 /* vsync_polarity */, 10 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 20 /* vsync_back_porch */);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    480 /* width */, 480 /* height */, rgbpanel, 2 /* rotation */, true /* auto_flush */,
    bus, GFX_NOT_DEFINED /* RST */, st7701_type1b_init_operations, sizeof(st7701_type1b_init_operations));


  
/* Exploring Wavshare type X inits for correct colours .. 
TESTING MODIFY  Arduino\libraries\GFX_Library_for_Arduino\src\display\Arduino_RGB_Display.h Line 511     
          WRITE_COMMAND_8, 0x21,   // 0x20 normal, 0x21 IPS WRITE_C8_D8, 0x3A, 0x60// 0x70 RGB888, 0x60 RGB666, 0x50 RGB565
    WRITE_COMMAND_8      WRITE_C8_D8 
1       0x21,                0x3A, 0x60          wrong colour details,fixed a bit by pin re-allocations  basic colour ok 
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
to get DEBUG_PORT.print working 
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

