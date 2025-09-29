#ifndef Display_Func
#define Display_Func
#include "Structures.h"
#include <Arduino_GFX_Library.h> // defines colours BLUE etc
#include "debug_port.h"
#include "FontType.h"

extern int MasterFont;  //global for font! Idea is to use to reset font after 'temporary' seletion of another
extern String Fontname;
extern int text_height ;      //so we can get them if we change heights etc inside functions
extern int text_offset ;      //offset is not equal to height, as subscripts print lower than 'height'
extern int text_char_width;
extern int Display_Page;



void Display(int pageIndex);
void Display(bool reset, int pageIndex) ;
bool CheckButton(_sButton & button);
void setFont(int fontinput);
void ShowGPSinBox(int font, _sButton button);
void TouchCrosshair(int size);
void TouchCrosshair(int point, int size, uint16_t colour);
void ButtonDataSelect( _sButton Position, int Instance, String Choice,bool RunSetup);

//****  My displays are based on '_sButton' structures to define position, width height, borders and colours.
// int h, v, width, height, bordersize;  uint16_t BackColor, TextColor, BorderColor;

extern _sButton FullSize;
extern _sButton FullSizeShadow;
extern _sButton CurrentSettingsBox;  //also used for showing the current settings

extern _sButton FontBox;

//extern _sButton;  // full screen no border

//used for single data display
// modified all to lift by 30 pixels to allow a common bottom row display (to show logs and get to settings)
extern _sButton StatusBox;
extern _sButton WifiStatus;  // big central box for wifi events to pop up - v3.5

extern _sButton BigSingleDisplay;              // used for wind and graph displays
extern _sButton BigSingleTopRight;             //  ''
extern _sButton BigSingleTopLeft;                //  ''
extern _sButton TopHalfBigSingleTopRight;      //  ''
extern _sButton BottomHalfBigSingleTopRight;  //  ''
extern _sButton FullScreen;
//used for nmea RMC /GPS display // was only three lines to start!
extern _sButton Threelines0;
extern _sButton Threelines1 ;
extern _sButton Threelines2;
extern _sButton Threelines3 ;
// for the quarter screens on the main pageIndex
extern _sButton topLeftquarter;  //h  reduced by 15 to give 30 space at the bottom
extern _sButton bottomLeftquarter ;
extern _sButton topRightquarter;
extern _sButton bottomRightquarter ;
extern _sButton WideScreenCentral;



// these were used for initial tests and for volume control - not needed for most people!! .. only used now for Range change in GPS graphic (?)
extern _sButton TopLeftbutton;
extern _sButton TopRightbutton ;
extern _sButton BottomRightbutton;
extern _sButton BottomLeftbutton ;

// buttons for the wifi/settings pages
extern _sButton TOPButton;
extern _sButton SecondRowButton ;
extern _sButton ThirdRowButton;
extern _sButton FourthRowButton ;
extern _sButton FifthRowButton;


//switches at line 180
extern _sButton Switch1;
extern _sButton Switch2;
extern _sButton Switch3 ;
extern _sButton Switch5 ;
extern _sButton Switch4 ;  // big one for eeprom update
extern _sButton Switch4a ;  // big one for eeprom update one line up

extern _sButton Switch6;
extern _sButton Switch7 ;
extern _sButton Switch8;
extern _sButton Switch9 ;
extern _sButton Switch10 ;
extern _sButton Switch11 ;


extern _sButton Terminal;
//for selections
extern _sButton FullTopCenter;
extern _sButton Full0Center;
extern _sButton Full1Center;
extern _sButton Full2Center ;
extern _sButton Full3Center ;
extern _sButton Full4Center;
extern _sButton Full5Center ;
extern _sButton Full6Center ;  // inteferes with settings box do not use!




#endif
