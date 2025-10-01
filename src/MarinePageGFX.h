#pragma once
#include <Arduino_GFX_Library.h>
#include <gfxfont.h>  // Required for GFXfont support
#include "..\CanvasBridge.h"
#include "..\Structures.h"
#include "..\FontType.h"

#define NEAR_BLACK 0x0001  // One bit on


class MarinePageGFX {
public:
  MarinePageGFX(Arduino_GFX* gfx, int16_t width, int16_t height);
  ~MarinePageGFX();
  
  uint16_t* getBuffer(int index);

  void begin();
  void swap();     // Switch active buffer
  void push();     // Push active buffer to screen
  bool isReady();  // Check if buffers are initialized
  
  int getShadowX();
  void setShadowX(int value);
  int getShadowY();
  void setShadowY(int value);
  bool getShadow_ON();
  void setShadow_ON(bool value);

  
  
  void DrawCompass(_sButton& button);
  
  void DrawScrollingGraph(_sButton& button, const GraphBuffer& buffer, double minVal, double maxVal);
  void Addtitletobutton(_sButton& button, int position, int font, const char* fmt, ...);
  void AutoPrint2Size(_sButton& button, const char* reference, const char* fmt, ...);
  void BorderPrintCanvasTwoSize(_sButton& button,int magnify,int bigF,int Smallf,int decimalInset, const char* fmt, ...);
  void GFXBorderBoxPrintf(_sButton& button, const char* fmt, ...);

  int getFontLineHeight(FontID id);

  // Drawing primitives

  void fillScreen(uint16_t color);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color);
  void fillCircleToCanvas(int16_t x0, int16_t y0, int16_t r, uint16_t color);
  void fillArc(int16_t x, int16_t y, int16_t r, int16_t start_angle, int16_t end_angle, uint16_t color);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  
  void drawLineToCanvas(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
  void drawWideLineToCanvas(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color,int width);
  
  void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
  void drawTriangleToCanvas(int16_t x0, int16_t y0,
                          int16_t x1, int16_t y1,
                          int16_t x2, int16_t y2,
                          uint16_t color);

  void drawText(int16_t x, int16_t y, const char* text, uint8_t size, uint16_t color);
  void drawTextAlign(int16_t x, int16_t y, const char* text, uint8_t size, uint16_t color, uint8_t align);
  void drawTextCentered(int16_t centerX, int16_t centerY, const char* text, uint8_t size, uint16_t color);
  void drawTextOverlay(const char* label, uint16_t color);

  void compositeCanvas(); 
  void clearCanvas(uint16_t color);  // Public method

  void clearOutsideRadius(int16_t centerX, int16_t centerY, int16_t radius, uint16_t color);
  void clearOutsideRadius(_sButton& button, uint16_t color);

  // compass
  void drawCompassPointer(_sButton& button, int16_t baseWidth, int16_t tailLength, float angleDeg, uint16_t color, bool shadow);

  // Rounded rectangle primitives
  void drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);
  void fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);

  // Line drawing helpers
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);

  // Circle helpers for rounded corners
  void drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corner, uint16_t color);
  void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corner, uint16_t color);


  // Text rendering
  void PrintSubshadow(_sButton& button, const char* valueBuffer, int16_t valH,int16_t valV, int chosenFont);

  void setCursor(int16_t x, int16_t y);
  void setTextColor(uint16_t color);
  void setTextColor(uint16_t fg, uint16_t bg);
  void setTextSize(uint8_t size);
  void println(const char* buf);
  void print(const char* buf);
  void printf(const char* fmt, ...);
  void setFont(const GFXfont* font);
  void setFontByIndex(int index);
  void setTextBound(int x, int y, int w, int h);
  void setTextWrap(bool wrap);
  int getCursorX();
  int getCursorY();
  void getTextBounds(const char* msg, int16_t x, int16_t y,int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h);

  
  // Buffer access
  uint16_t* getActiveBuffer();
  void clearTextCanvas(uint16_t bg = 0);

private:
  Arduino_GFX* _gfx;
  Arduino_Canvas* _textCanvas = nullptr;

  int16_t _textBoundX = 0;
  int16_t _textBoundY = 0;
  uint16_t _textBoundW = 0;
  uint16_t _textBoundH = 0;
  bool _textWrap;
  int16_t _width, _height;
  uint16_t* _buffer[2];
  int _active;
  int16_t _cursorX, _cursorY;
  uint16_t _textColor;
  uint16_t _backgroundColor;
  uint8_t _textSize;
  const GFXfont* _gfxFont = nullptr;
};