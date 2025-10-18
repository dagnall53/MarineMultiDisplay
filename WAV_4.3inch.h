/*******************************************************************************
Pins and defines for GFX - various versions!
FOR WAVESHARE 4.3 inch LCD 800 by 400 in BOX WITH CAN BUS --NOTE THEY HAVE MANY SLIGHTLY SIMILAR NAMES
https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3B/ESP32-S3-Touch-LCD-4.3B-Sch.pdf
https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3B
also need ?
#define ARDUINO_USB_MODE 1
#define ARDUINO_USB_CDC_ON_BOOT 1
to get DEBUG_PORT.print working 
(or set in tools-> USB CDC )

 ******************************************************************************/
 
#ifndef _WAV_DEF_H
#define _WAV_DEF_H

const char _device[]=  "Wavshare ESP32-S3-Touch-LCD-4.3B 800x480 module";
  #define ESP32_CAN_TX_PIN GPIO_NUM_15  // for the waveshare '4.3B' 800x480 module boards! ST7272
  #define ESP32_CAN_RX_PIN GPIO_NUM_16  // 

Arduino_DataBus *bus = new Arduino_SWSPI(
  GFX_NOT_DEFINED /* DC */,
  GFX_NOT_DEFINED /* CS /12*/,                // Chip Select pin
  9 /* SCK /SCL /11*/,               // Clock pin
  8 /* SDA /10? */,             // Master Out Slave In pin
  GFX_NOT_DEFINED /* MISO */  // Master In Slave Out pin (not used)
);

#define LCD_WIDTH                  (800) // LCD width in pixels
#define LCD_HEIGHT                 (480) // LCD height in pixels

#define LCD_RGB_IO_DE              (5)   // Data enable pin number
#define LCD_RGB_IO_VSYNC           (3)   // VSYNC pin number
#define LCD_RGB_IO_HSYNC           (46)  // HSYNC pin number
#define LCD_RGB_IO_PCLK            (7)   // Pixel clock pin number

#define LCD_RGB_TIMING_HFP         (8)   // Horizontal front porch
#define LCD_RGB_TIMING_HPW         (4)   // Horizontal pulse width
#define LCD_RGB_TIMING_HBP         (8)   // Horizontal back porch
#define LCD_RGB_TIMING_VFP         (8)   // Vertical front porch
#define LCD_RGB_TIMING_VPW         (4)   // Vertical pulse width
#define LCD_RGB_TIMING_VBP         (8)   // Vertical back porch

#define LCD_RGB_IO_DATA0           (14)  // RGB data pin 0
#define LCD_RGB_IO_DATA1           (38)  // RGB data pin 1
#define LCD_RGB_IO_DATA2           (18)  // RGB data pin 2
#define LCD_RGB_IO_DATA3           (17)  // RGB data pin 3
#define LCD_RGB_IO_DATA4           (10)  // RGB data pin 4
#define LCD_RGB_IO_DATA5           (39)  // RGB data pin 5
#define LCD_RGB_IO_DATA6           (0)   // RGB data pin 6
#define LCD_RGB_IO_DATA7           (45)  // RGB data pin 7
#define LCD_RGB_IO_DATA8           (48)  // RGB data pin 8
#define LCD_RGB_IO_DATA9           (47)  // RGB data pin 9
#define LCD_RGB_IO_DATA10          (21)  // RGB data pin 10
#define LCD_RGB_IO_DATA11          (1)   // RGB data pin 11
#define LCD_RGB_IO_DATA12          (2)   // RGB data pin 12
#define LCD_RGB_IO_DATA13          (42)  // RGB data pin 13
#define LCD_RGB_IO_DATA14          (41)  // RGB data pin 14
#define LCD_RGB_IO_DATA15          (40)  // RGB data pin 15

#define LCD_RGB_BOUNCE_BUFFER_SIZE (LCD_WIDTH * 10)
#define LCD_RGB_TIMING_FREQ_HZ     (16 * 1000 * 1000) // RGB timing frequency

// Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
//   LCD_RGB_IO_DE, LCD_RGB_IO_VSYNC, LCD_RGB_IO_HSYNC, LCD_RGB_IO_PCLK,
//   LCD_RGB_IO_DATA11, LCD_RGB_IO_DATA12, LCD_RGB_IO_DATA13, LCD_RGB_IO_DATA14, LCD_RGB_IO_DATA15,
//   LCD_RGB_IO_DATA5, LCD_RGB_IO_DATA6, LCD_RGB_IO_DATA7, LCD_RGB_IO_DATA8, LCD_RGB_IO_DATA9, LCD_RGB_IO_DATA10,
//   LCD_RGB_IO_DATA0, LCD_RGB_IO_DATA1, LCD_RGB_IO_DATA2, LCD_RGB_IO_DATA3, LCD_RGB_IO_DATA4,
//   0 /* hsync_polarity */, LCD_RGB_TIMING_HFP, LCD_RGB_TIMING_HPW, LCD_RGB_TIMING_HBP,
//   0 /* vsync_polarity */, LCD_RGB_TIMING_VFP, LCD_RGB_TIMING_VPW, LCD_RGB_TIMING_VBP,
//   1 /* pclk_active_neg */, 13000000 /* prefer_speed */, false /* useBigEndian */,
//   0 /* de_idle_high */, 0 /* pclk_idle_high */, 0);
// from C:\Users\dagna\OneDrive\DocOneDrive\Arduino\libraries\GFX_Library_for_Arduino\examples\PDQgraphicstest\Arduino_GFX_dev_device.h

// And with my corrected pins for green! 
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(  // MY BOARD modified pin numbers 4.3  R's from 3,4,5,6,7  1,2,42,41,40  G2,G3,G4,G5,G6,G7   39,0,45,48,47,21, B3,4,5,6,B7   14,38,18,17,10
  5 /* DE */, 3 /* VSYNC */, 46 /* HSYNC */, 7 /* PCLK */,
  1,2,42,41,40, //R pins R3,4,5,6,R7  1,2,42,41,40
  39,0,45,48,47,21, //G pins
  10,14,38,18,17,    //B pins  pin definition FAULT in wavshare data - or fix to match GFX for arduino ? unknwown, but works 
 // 14,38,18,17,10,    //B pins
  0 /* hsync_polarity */, 8 /* hsync_front_porch */, 8 /* hsync_pulse_width */, 43 /* hsync_back_porch */,
  0 /* vsync_polarity */, 8 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 12 /* vsync_back_porch */,
  1 /* pclk_active_neg */, 13000000 /* prefer_speed */, false /* useBigEndian */,
  0 /* de_idle_high */, 0 /* pclk_idle_high */, 0 /* bounce_buffer_size_px */);

// Initialize display 
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
     800 /* width */, 480 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */);

/* Exploring Wavshare type X inits for correct colours .. 
?? 
*/

/*******************************************************************************
Pins and defines for GFX - various versions!
FOR WAVESHARE 4.3 BOX 
https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3B/ESP32-S3-Touch-LCD-4.3B-Sch.pdf


also need ?
#define ARDUINO_USB_MODE 1
#define ARDUINO_USB_CDC_ON_BOOT 1
to get DEBUG_PORT.print working 
(or set in tools-> USB CDC )

 ******************************************************************************/
 
//** OTHER PINS

//#define TFT_BL GFX_BL  // or EX105 ?  not used??
//#define I2C_SDA_PIN 15
//#define I2C_SCL_PIN 7

//SD card interface 

#define SD_SCK  12
#define SD_MISO 13
#define SD_MOSI 11
#define SDCS  -1 // NOTE not SD_CS, which is a function! is port ex104  ?? Not called up?? 

//** 12/08/2025 ... not working!! touch interface **************************


#define TOUCH_INT 4          
#define TOUCH_RST  254 // -1 not acepted by TAMC GT911 as it is a UINT_8t variable. I modified to detect 254 as "not present"          //-1-1          // EX101 will reset it at the start ?
#define TOUCH_SDA  8
#define TOUCH_SCL  9

#define TOUCH_WIDTH  800
#define TOUCH_HEIGHT 480

#define TOUCH_ROTATION ROTATION_NORMAL
#define TOUCH_MAP_X1 480
#define TOUCH_MAP_X2 0
#define TOUCH_MAP_Y1 800
#define TOUCH_MAP_Y2 0

#define TOUCH_ROTATION ROTATION_INVERTED


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

#define ExpanderSDA TOUCH_SDA
#define ExpanderSCL TOUCH_SCL

// oter stuff 
#define I2C_MASTER_SCL_IO           9       /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           8       /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0       /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define GPIO_INPUT_IO_4    4
#define GPIO_INPUT_PIN_SEL  1ULL<<GPIO_INPUT_IO_4

#endif // _WAV_DEF_H

