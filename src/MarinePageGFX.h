#pragma once
#include <Arduino_GFX_Library.h>
#include <gfxfont.h>  // Required for GFXfont support
#include "..\CanvasBridge.h"
#include "..\Structures.h"
#include "..\FontType.h"
/*******************************************************************************
 * Adding JPEGDEC related functions
 *
 * Dependent libraries:
 * JPEGDEC: https://github.com/bitbank2/JPEGDEC.git
 ******************************************************************************/
#include <JPEGDEC.h>
#define NEAR_BLACK 0x0001  // One bit on
#define SILVER_GRAY 0xBEEF


class MarinePageGFX {
public:
  MarinePageGFX(Arduino_GFX* gfx, int16_t width, int16_t height);
  ~MarinePageGFX();

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

  
  void showPicture(const char* name);
  
  void drawBoatOutline(int x, int y, int size);
  void DrawCompass(_sButton& button);
  
  void DrawScrollingGraph(_sButton& button, const GraphBuffer& buffer, double minVal, double maxVal);
  void AddTitleInsideBox(_sButton& button, int position, int font, const char* fmt, ...);
  void Addtitletobutton(_sButton& button, int position, int font, const char* fmt, ...);
  void BorderPrintCanvasTwoSize(_sButton& button, int decimalInset, const char* fmt, ...);
  void GFXBorderBoxPrintf(_sButton& button, const char* fmt, ...);

  
  void AutoPrint2Size(_sButton& button,_sInstData &data , const char* reference, const char* fmt, ...) ;
  int getFontLineHeight(FontID id);
  
  void CommonSub_UpdateLine(uint16_t color, int font, _sButton &button, const char *msg);
  void CommonSub_UpdateLine(bool horizCenter, bool vertCenter, uint16_t color, int font, _sButton &button, const char *msg);
  void UpdateLinef(uint16_t color, int font, _sButton &button, const char *fmt, ...);
  void UpdateLinef(int font, _sButton &button, const char *fmt, ...);
  void UpdateTwoSize_MultiLine(int magnify, bool horizCenter, bool vertCenter, int bigfont, int smallfont, _sButton &button, const char *fmt, ...);
  
  // Drawing primitives

  void fillScreen(uint16_t color);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color); //? defective??
  void fillArc(int16_t x, int16_t y, int16_t r1, int16_t r2, float start, float end, uint16_t color);
  void drawArc(int16_t x, int16_t y, int16_t r1, int16_t r2, float start, float end, uint16_t color);
  void fillArc(int16_t x, int16_t y, int16_t r, int16_t start_angle, int16_t end_angle, uint16_t color);
  void drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  
  void drawLineToCanvas(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
  void drawWideLineToCanvas(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color,int width);
  
  void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
  void drawTriangle(int16_t x0, int16_t y0,
                          int16_t x1, int16_t y1,
                          int16_t x2, int16_t y2,
                          uint16_t color);

  //void drawText(int16_t x, int16_t y, const char* text, uint8_t size, uint16_t color);
  void drawTextAlign(int16_t x, int16_t y, const char* text, uint8_t size, uint16_t color, uint8_t align);
  void drawTextCentered(int16_t centerX, int16_t centerY, const char* text, uint8_t size, uint16_t color);
  void drawTextOverlay(const char* label, uint16_t color);

  void compositeCanvas(); 
  void clearCanvas(uint16_t color);  // Public method

  void clearOutsideRadius(int16_t centerX, int16_t centerY, int16_t radius, uint16_t color);
  void clearOutsideRadius(_sButton& button, uint16_t color);

  // compass
  void drawCompassPointer(_sButton& button, int16_t baseWidth, int16_t tailLength, _sInstData &data, uint16_t color, bool shadow);

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
    // Text rendering
  void  drawTextAt(int16_t x, int16_t y, const char* text, uint8_t size,int font, uint16_t color);
  void PrintSubshadow(_sButton& button, const char* valueBuffer, int16_t valH,int16_t valV, int chosenFont);
  void SplitInterDecimal(const char* buffer, char* Integer, char* Fraction, char* Dot);
  void DrawBox(_sButton& button);
  void setCursor(int16_t x, int16_t y);
  int16_t getCursorX();
  int16_t getCursorY();
  void setTextColor(uint16_t color);
  void setTextSize(uint8_t size);
  void printf(const char* fmt, ...);
  void setFontByIndex(int index);
  void setFont(const GFXfont* font);
  int getFontByIndex(void);

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

  static int jpegDrawCallback(JPEGDRAW* pDraw);
  static void* jpegOpenFile(const char* szFilename, int32_t* pFileSize);
  static void jpegCloseFile(void* pHandle);
  static int32_t jpegReadFile(JPEGFILE* pFile, uint8_t* pBuf, int32_t iLen);
  static int32_t jpegSeekFile(JPEGFILE* pFile, int32_t iPosition);



};