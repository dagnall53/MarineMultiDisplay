 
#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_
#include <Arduino_GFX_Library.h>


// new 
enum KeyboardMode { LC, UC, NUM };
extern char resultBuffer[25]; // same as Password size for simplicity  // Declaration only

void drawBoxedKey(_sButton Key_Master,int x, int y, int w, int h, const char* label,int font);
void drawKeyboard(KeyboardMode mode);
void handleTouch(int tx, int ty);

void TESTFUNC();
void Setup_K_Display(int h,int v,const char text);
void keyboard(int type);
void Use_Keyboard(char* DATA, int sizeof_data);

void DrawKey(int Keysize, int x, int rows_down, int width, String text );

bool KeyOver(int x, int y, char * Key,int type);
bool XYinBox(int x,int y, int h,int v,int width,int height); // also used in CheckButton function 

int KEYBOARD_Y(void);
int KEYBOARD_X(void);


#endif