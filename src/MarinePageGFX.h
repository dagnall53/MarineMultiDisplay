#pragma once
#include <Arduino_GFX_Library.h>
#include <gfxfont.h>  // Required for GFXfont support
#include "..\CanvasBridge.h"
#define NEAR_BLACK 0x0001  // One bit on


class MarinePageGFX {
public:
  MarinePageGFX(Arduino_GFX* gfx, int16_t width, int16_t height);
  ~MarinePageGFX();

  void begin();
  void swap();     // Switch active buffer
  void push();     // Push active buffer to screen
  bool isReady();  // Check if buffers are initialized

  // Drawing primitives

  void fillScreen(uint16_t color);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color);
  void fillArc(int16_t x, int16_t y, int16_t r, int16_t start_angle, int16_t end_angle, uint16_t color);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
  void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
  void drawText(int16_t x, int16_t y, const char* text, uint8_t size, uint16_t color);
  void drawTextAlign(int16_t x, int16_t y, const char* text, uint8_t size, uint16_t color, uint8_t align);
  void drawTextCentered(int16_t centerX, int16_t centerY, const char* text, uint8_t size, uint16_t color);
  void drawTextOverlay(const char* label, uint16_t color);

  void compositeCanvas(); 
  void clearOutsideRadius(int16_t centerX, int16_t centerY, int16_t radius, uint16_t color);

  // compass
  void drawCompassPointer(int16_t centerX, int16_t centerY, int16_t radius, int16_t tailLength, float angleDeg, uint16_t color,bool shadow = true);

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
  void setCursor(int16_t x, int16_t y);
  void setTextColor(uint16_t color);
  void setTextSize(uint8_t size);
  void printf(const char* fmt, ...);
  void setFont(const GFXfont* font);

  // Buffer access
  uint16_t* getActiveBuffer();
  void clearTextCanvas(uint16_t bg = 0);

private:
  Arduino_GFX* _gfx;
  Arduino_Canvas* _textCanvas = nullptr;

  int16_t _width, _height;
  uint16_t* _buffer[2];
  int _active;
  int16_t _cursorX, _cursorY;
  uint16_t _textColor;
  uint8_t _textSize;
  const GFXfont* _gfxFont = nullptr;
};