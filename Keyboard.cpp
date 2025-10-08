

#include <Arduino_GFX_Library.h> 
// also includes Arduino etc, so variable names are understood
#include "Structures.h"
#include "debug_port.h"
#include "Keyboard.h"
// Based on Felix Biego's keyboard   https://github.com/fbiego/esp32-touch-keyboard/tree/main
// modified to use strings (char array) and not Strings.
#include "FONTS/fonts.h"
#include "src/MarinePageGFX.h"  // Double-buffered graphics
#include "CanvasBridge.h"
#include "FontType.h"
#include "Structures.h"

extern MarinePageGFX* page;

#include "src/TAMC_GT911.h"
//---------------- 7/10/25 keyboard test 

KeyboardMode currentMode = LC;

// define in main ino or extern in main .. 
char resultBuffer[25] = {0};  
const int keyHeight = 40;
const int screenWidth = 480;  // or 800
const int maxKeys = 32;
int Keyboard_X =0; // not used (yet)
int Keyboard_Y = 200;//not used (yet)

struct Key {
  int x, y, w, h;
  const char* label;
};

Key keyMap[maxKeys];
int keyCount = 0;
_sButton Key_Master = { 0, 0, keyHeight, screenWidth/10, 2, WHITE, NEAR_BLACK,NEAR_BLACK, 9 };  //WHITE, NEAR_BLACK, BLUE };
// Draw a boxed key 
void drawBoxedKey(_sButton Key_Master,int x, int y, int w, int h, const char* label, int font) {
  // NEW   similar to victron display, as I know that works ! 
  _sButton KDisplayOuterbox;
  KDisplayOuterbox.Font=font;
  KDisplayOuterbox.bordersize = Key_Master.bordersize;
  KDisplayOuterbox.width = w;
  KDisplayOuterbox.TextColor = Key_Master.TextColor;
  KDisplayOuterbox.BorderColor = Key_Master.BorderColor;
  KDisplayOuterbox.BackColor = Key_Master.BackColor;
  KDisplayOuterbox.height = h;
  KDisplayOuterbox.h = x;
  KDisplayOuterbox.v = y;
  page->GFXBorderBoxPrintf(KDisplayOuterbox,"%s",label);  //in gfx , Used to blank the previous stuff! Reset PrintLine lats x etc  
}

// Draw the keyboard in the selected mode
void drawKeyboard(KeyboardMode mode) {
  currentMode = mode;
  keyCount = 0;
  int kw = screenWidth / 10;
  int y = 280;

  const char* top[3][10] = {
    {"q","w","e","r","t","y","u","i","o","p"},
    {"Q","W","E","R","T","Y","U","I","O","P"},
    {"1","2","3","4","5","6","7","8","9","0"}
  };

  const char* mid[3][9] = {
    {"a","s","d","f","g","h","j","k","l"},
    {"A","S","D","F","G","H","J","K","L"},
    {"!","@","#","$","%","^","&","*","("}
  };

  const char* bot[3][8] = {
    {"z","x","c","v","b","n","m",","},
    {"Z","X","C","V","B","N","M",","},
    {")","-","_","+","=","/","\\","?"}
  };

  for (int i = 0; i < 10; i++) {
    drawBoxedKey(Key_Master,i * kw, y, kw, keyHeight, top[mode][i],9);
    keyMap[keyCount++] = {i * kw, y, kw, keyHeight, top[mode][i]};
  }
  y += keyHeight;
  for (int i = 0; i < 9; i++) {
    drawBoxedKey(Key_Master,(i + 0.5) * kw, y, kw, keyHeight, mid[mode][i],9);
    keyMap[keyCount++] = {(int)((i + 0.5) * kw), y, kw, keyHeight, mid[mode][i]};
  }
  y += keyHeight;
  drawBoxedKey(Key_Master,0, y, kw, keyHeight, "Shift",7);
  keyMap[keyCount++] = {0, y, kw, keyHeight, "Shift"};
  for (int i = 0; i < 8; i++) {
    drawBoxedKey(Key_Master,(i + 1) * kw, y, kw, keyHeight, bot[mode][i],9);
    keyMap[keyCount++] = {(i + 1) * kw, y, kw, keyHeight, bot[mode][i]};
  }
  y += keyHeight;  
  drawBoxedKey(Key_Master,0, y, 3*kw/2, keyHeight, "Clr",8);
  keyMap[keyCount++] = {0, y, 3*kw/2, keyHeight, "Clr"};
  drawBoxedKey(Key_Master,3*kw, y, 3 * kw, keyHeight, "Space",9);
  keyMap[keyCount++] = {3*kw, y, 3 * kw, keyHeight, "Space"};

  drawBoxedKey(Key_Master,13 * kw/2, y, 3*kw/2, keyHeight, "Del",8);
  keyMap[keyCount++] = {13 * kw/2, y, 3*kw/2, keyHeight, "Del"};
//  drawBoxedKey(Key_Master,8 * kw, y, 2 * kw, keyHeight, "Enter",8);
//  keyMap[keyCount++] = {8 * kw, y, 2 * kw, keyHeight, "Enter"};
}

// Handle a touch at (x, y)
void handleTouch(int tx, int ty) {
  for (int i = 0; i < keyCount; i++) {
    Key& k = keyMap[i];
    if (tx >= k.x && tx < k.x + k.w && ty >= k.y && ty < k.y + k.h) {
      const char* label = k.label;

      if (strcmp(label, "Shift") == 0) {
        currentMode = (KeyboardMode)((currentMode + 1) % 3);
        drawKeyboard(currentMode);
      } else if (strcmp(label, "Clr") == 0) {
        resultBuffer[0] = '\0';
      } else if (strcmp(label, "Del") == 0) {
        int len = strlen(resultBuffer);
        if (len > 0) resultBuffer[len - 1] = '\0';
      } else if (strcmp(label, "Space") == 0) {
        int len = strlen(resultBuffer);
        if (len < 40) {
          resultBuffer[len] = ' ';
          resultBuffer[len + 1] = '\0';
        }
      } else if (strcmp(label, "Enter") == 0) {
        // Submit or process resultBuffer (done differently in Display.. )
      } else {
        int len = strlen(resultBuffer);
        if (len < 40) {
          resultBuffer[len] = label[0];
          resultBuffer[len + 1] = '\0';
        }
      }

      break;
    }
  }
}





//-------------- all old below this line 

// char TOP[] = "qQ1wW2eE3rR4tT5yY6uU7iI8oO9pP0";
// char MIDDLE[] = "aA_sS/dD:fF;gG(hH)jJ$kK&lL@";
// char BOTTOM[] = "^^^zZ.xX,cC?vV!bB'nN\"mM-";

// int caps = 0;
// bool change = false;
// int sz = 3;  //Modulus of key definitions
// int Keyboard_X =0;
// int Keyboard_Y = 200;
// int KBD_size = 2;  //generic size modifier for Kbd 1=small, 2=480 wide
// /*
//               //30 is key V spacing AT SIZE 1
//               //25 IS KEY HEIGHT AT SIZE 1
// */

//   #define hoffset 5  // offsets for text in key boxes
//   #define voffset 35 

// extern int text_height;  //so we can get them if we change heights etc inside functions
// extern const char *Setupfilename;
// extern void SaveConfiguration();

// extern int Display_Page;

// extern TAMC_GT911 ts;
// extern bool Touch_available;

// extern struct _sWiFi_settings_Config Current_Settings;
// extern struct _sDisplay_Config Display_Config;
// extern void TouchCrosshair(int);
// extern int text_offset;


// // void WriteinKey(int h, int v, int size, const char* TEXT) {  //Write WHITE text in filled BLACK box of text height at h,v (using fontoffset to use TOP LEFT of text convention)
// //   page->fillRect(h, v, 480, text_height * size, WHITE);
// //   page->drawTextAt(h, v + (text_offset), TEXT, size,4, NEAR_BLACK);
// //   DEBUG_PORT.printf(" key %i %i ",h,v);
// //   // page->setTextColor(BLACK);
// //   // page->setTextSize(size);
// //   // page->setCursor(h, v + (text_offset));         // offset up/down for GFXFONTS that start at Bottom left. Standard fonts start at TOP LEFT
// //   // page->print(TEXT);
// // }







// void TESTFUNC(){
//  DEBUG_PORT.printf("\n*** debug TEST FUNCTION");
// Serial.printf("\n*** Serial TEST FUNCTION");
// }
// // similar to victron display, as I know that works ! 
// _sButton KDisplayOuterbox;
// _sButton K_Master = { 0, 0, 40, 50, 2, NEAR_BLACK, WHITE, WHITE, 8 };  //WHITE, NEAR_BLACK, BLUE };
//  void Setup_K_Display(int h,int v,char text) {  // setsup the display box , but changes position and height colours  as required
//   char TEXT[2];
//   TEXT[0]=text;
//   TEXT[1]= '\0';
//    KDisplayOuterbox.Font=K_Master.Font;
//   KDisplayOuterbox.bordersize = K_Master.bordersize;
//   KDisplayOuterbox.width = K_Master.width;
//   KDisplayOuterbox.TextColor = K_Master.TextColor;
//   KDisplayOuterbox.BorderColor = K_Master.BorderColor;
//   KDisplayOuterbox.BackColor = K_Master.BackColor;
//   KDisplayOuterbox.height = K_Master.height;
//   KDisplayOuterbox.h = h;
//   KDisplayOuterbox.v = v;
//   page->GFXBorderBoxPrintf(KDisplayOuterbox,"%s",TEXT);  //in gfx , Used to blank the previous stuff! Reset PrintLine lats x etc
// }    

// // void DrawKey(int KBD_size, int x, int rows_down, int width, String text ){
// //  // use Vic_Inst_Master box master struct and ColorSettings for the colours border size etc, etc ...
// //   //pointers so we avoid stack crash?
// //      KDisplayOuterbox.Font=K_Master.Font-1;
// //     KDisplayOuterbox.bordersize = K_Master.bordersize;
// //   KDisplayOuterbox.width = width*KBD_size;
// //   KDisplayOuterbox.TextColor = K_Master.TextColor;
// //   KDisplayOuterbox.BorderColor = RED;
// //   KDisplayOuterbox.BackColor = K_Master.BackColor;
// //   KDisplayOuterbox.height = K_Master.height;
// //   KDisplayOuterbox.h = Keyboard_X+(x*KBD_size)+(2*KBD_size);
// //   KDisplayOuterbox.v = Keyboard_Y + (30*rows_down*KBD_size)+voffset;
// //   page->GFXBorderBoxPrintf(KDisplayOuterbox,"%s",text);  //in gfx , Used to blank the previous stuff! Reset PrintLine lats x etc
// // } 

// // void keyboard(int type) {
// //  static int lasttype;
// //  int oldsize;
// //   TESTFUNC();
// //  //DEBUG_PORT.printf(" setup keyboard %i  was%i \n",type,lasttype);
// //   caps=type;
// //   Serial.printf("\n*** Start keyboard type %i  last type%i \n",type,lasttype);
// //   lasttype=type;
// //  // page->fillRect(Keyboard_X,Keyboard_Y-5,480-Keyboard_X,240,BLUE);
// //   for (int x = 0; x < 10; x++) {
// //     int a = KBD_size*((x * 4) + (20 * x) + 2) + Keyboard_X;
// //     Setup_K_Display(a + hoffset, Keyboard_Y + voffset,TOP[((x * sz) + type)]); 
// //   }

// //   for (int x = 0; x < 9; x++) {
// //     int a = KBD_size*((x * 4) + (20 * x) + 13) + Keyboard_X;
// //     Setup_K_Display(a + hoffset, Keyboard_Y + (30*KBD_size) + voffset, MIDDLE[((x * sz) + type)]);
// //   }

// //   for (int x = 0; x < 8; x++) {
// //     int a = KBD_size*((x * 4) + (20 * x) + 25) + Keyboard_X;
// //     Setup_K_Display(a + hoffset, Keyboard_Y + (60*KBD_size) + voffset, BOTTOM[((x * sz) + type)]);
// //   }
// //    DrawKey(2,50, 3,30,"CLR");
// //    DrawKey(2,155, 3,30,"DEL");
// //    DrawKey(2,190, 3,50,"ENT");
// //    DrawKey(2,5, 3,30,"MEM");
// //    DrawKey(2,90, 3,60,"space");
// // //  page->drawRoundRect((90*KBD_size)+ Keyboard_X, Keyboard_Y + (90*KBD_size), 60*KBD_size, 25*KBD_size, 3, WHITE);
// // }


bool XYinBox(int touchx,int touchy, int h,int v,int width,int height){ //xy position in, xy top left width and height 
   return ((touchx >= h && touchx <= h+width) && (touchy >= v && touchy <= v+height));
}

// bool XYinBox(int x, int y ,int Kx, int Krows_down, int Kwidth){ // use DrawKey type key setting
// //DEBUG_PORT.printf(" Testing accuracy Target is ")

//     int h=Keyboard_X+(Kx*KBD_size);
//     int width=Kwidth*KBD_size;
//     int v=Keyboard_Y + (30*Krows_down*KBD_size); //30 is key V spacing AT SIZE 1
//     int height=25*KBD_size;                      //25 IS KEY HEIGHT AT SIZE 1
//   return XYinBox(x,y,h,v,width,height);
// }


// // bool KeyOver(int x, int y, char* Key, int type){ //char array version
// //   bool Keyfound=false;

// //    if ((y > Keyboard_Y) && (y < (Keyboard_Y+25*KBD_size))) { //a, Keyboard_Y, 20, 25,
// //     //top row  
// //    //DEBUG_PORT.printf(" In TOP ROW  KBD_size;(%i) x%i y%i   looking in space x%i y%i\n",KBD_size,x,y );
// //    /*  for (int x = 0; x < 10; x++) {
// //     int a = KBD_size*((x * 4) + (20 * x) + 2) + Keyboard_X;
// //     page->drawRoundRect(a, Keyboard_Y, 20*KBD_size, 25*KBD_size, 3, WHITE);
// //     page->setCursor(a + hoffset, Keyboard_Y + voffset);
// //     page->print(TOP[((x * sz) + type)]);
// //    */
// //     for (int z = 0; z < 10; z++) {
// //       int a = (KBD_size*((z * 4) + (20 * z) + 2)) + Keyboard_X;
// //       int b = a + (20*KBD_size);
// //       if (x > a && x < b) {strcpy(Key,String(TOP[(z * sz) + type]).c_str());Keyfound=true;}
// //     }
// //   }
 
// //   if ((y > (Keyboard_Y + (30*KBD_size))) && (y < (Keyboard_Y + (30*KBD_size)+(25*KBD_size)))) { //Keyboard_Y + (30*KBD_size), 20, 25, 1, WHITE);
// //     //middle row
// //       // DEBUG_PORT.printf(" In MIDDLE ROW   %i %i",x,y);
// //     for (int z = 0; z < 9; z++) {
// //       int a = KBD_size*((z * 4) + (20 * z) + 13) + Keyboard_X;
// //       int b = a + (20*KBD_size);
// //       if (x > a && x < b) {strcpy(Key,String(MIDDLE[(z * sz) + type]).c_str());Keyfound=true;}
// //     }
// //   }

// //   if ((y > (Keyboard_Y + (60*KBD_size))) && (y < (Keyboard_Y + (60*KBD_size)+(25*KBD_size)))) {
// //     //bottom row
// //     for (int z = 0; z < 8; z++) {
// //       int a = KBD_size*((z * 4) + (20 * z) + 25) + Keyboard_X;
// //       int b = a + (20*KBD_size);
// //       if (x > a && x < b) {strcpy(Key,String(BOTTOM[(z * sz) + type]).c_str());Keyfound=true;}
// //     }
// //   }
// //   if (XYinBox(x,y,55,3,30)){ strcpy(Key,"CLR");Keyfound=true;}
// //   if (XYinBox(x,y,90,3,60)){ strcpy(Key," ");Keyfound=true;}
// //   if (XYinBox(x,y,155,3,30)){strcpy(Key,"DEL");Keyfound=true;}
// //   if (XYinBox(x,y,10,3,30)){strcpy(Key,"MEM");Keyfound=true;}
  
// //   if (XYinBox(x,y,190,3,50)){strcpy(Key,"ENT");Keyfound=true;}
// //  return Keyfound;
// //  }





