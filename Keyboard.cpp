

#include <Arduino_GFX_Library.h> 
// also includes Arduino etc, so variable names are understood
#include <Arduino.h>       
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

extern int Screen_Width;

char resultBuffer[25] = {0};  
const int keyHeight = 40;
//Externally supplied.. Screen_Width = TOUCH_WIDTH;  //480 or 800
const int maxKeys = 32;

struct Key {
  int x, y, w, h;
  const char* label;
};

Key keyMap[maxKeys];
int keyCount = 0;
_sButton Key_Master = { 0, 0, keyHeight, Screen_Width/10, 2, WHITE, NEAR_BLACK,NEAR_BLACK, 9 };  //WHITE, NEAR_BLACK, BLUE };
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
  int kw = Screen_Width / 10;
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
    //page->drawTextAt((i + 0.5) * kw, y,  mid[mode][i], 1,9, RED); // test to check drawTextAt
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






