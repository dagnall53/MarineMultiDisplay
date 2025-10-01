#include "MarinePageGFX.h"
#include <stdarg.h>
#include <math.h>

#include "..\FontType.h"
#include "..\debug_port.h"

#define DEG_TO_RAD 0.0174532925f
MarinePageGFX* page = nullptr;
// Private variables
static int _ShadowX = 0;
static int _ShadowY = 0;
static bool _ShadowON = 0;

int getShadowY() { return _ShadowY;}
void MarinePageGFX::setShadowY(int value) {_ShadowY = value;}

int getShadowX() {return _ShadowX;}
void MarinePageGFX::setShadowX(int value) {_ShadowX = value;}

bool getShadow_ON() {return _ShadowON;}
void MarinePageGFX::setShadow_ON(bool value) {_ShadowON = value;}






const GFXfont* _currentFont = nullptr;

uint16_t* MarinePageGFX::getBuffer(int index) {
    return (index >= 0 && index < 2) ? _buffer[index] : nullptr;
}





void MarinePageGFX::setTextBound(int x, int y, int w, int h) {
  _textBoundX = x;
  _textBoundY = y;
  _textBoundW = w;
  _textBoundH = h;
}
void MarinePageGFX::setTextWrap(bool wrap) {
    _textCanvas->setTextWrap(wrap);
}




void MarinePageGFX::clearTextCanvas(uint16_t bg) {
  if (!_textCanvas || !isReady()) return;
  _textCanvas->fillScreen(bg);
}

MarinePageGFX::MarinePageGFX(Arduino_GFX* gfx, int16_t width, int16_t height)
  : _gfx(gfx), _width(width), _height(height), _active(0),
    _cursorX(0), _cursorY(0), _textColor(0xFFFF), _textSize(1) {

  if (!psramFound()) {
    DEBUG_PORT.println("ERROR: PSRAM not found. Cannot allocate page buffers.");
    _buffer[0] = nullptr;
    _buffer[1] = nullptr;
    return;
  }

  size_t bufSize = width * height * sizeof(uint16_t);
  _buffer[0] = (uint16_t*)heap_caps_malloc(bufSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  _buffer[1] = (uint16_t*)heap_caps_malloc(bufSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  if (!_buffer[0] || !_buffer[1]) {
    DEBUG_PORT.println("ERROR: Failed to allocate page buffers in PSRAM.");
    _buffer[0] = nullptr;
    _buffer[1] = nullptr;
  } else {
    DEBUG_PORT.printf("Page buffers allocated: %d bytes each\n", bufSize);
  }
}

MarinePageGFX::~MarinePageGFX() {
  if (_buffer[0]) free(_buffer[0]);
  if (_buffer[1]) free(_buffer[1]);
}

bool MarinePageGFX::isReady() {
  return _buffer[0] && _buffer[1];
}

void MarinePageGFX::begin() {
  if (isReady()) {
    fillScreen(0x0000);  // Clear to black
  } else {
    DEBUG_PORT.println("WARNING: Buffer not initialized. Skipping fillScreen.");
  }
  _textCanvas = new Arduino_Canvas(_width, _height, nullptr, false);
  _textCanvas->begin();
  _textCanvas->setFont(_gfxFont);  // Sync font
}

void MarinePageGFX::swap() {
  _active = 1 - _active;
}

void MarinePageGFX::push() {
  if (isReady()) {
    _gfx->draw16bitRGBBitmap(0, 0, _buffer[_active], _width, _height);
  } else {
    DEBUG_PORT.println("WARNING: Buffer not initialized. Skipping push.");
  }
}

void MarinePageGFX::fillScreen(uint16_t color) {
  if (!isReady()) return;
  for (int i = 0; i < _width * _height; ++i) {
    _buffer[_active][i] = color;
  }
}

void MarinePageGFX::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  for (int16_t i = 0; i < h; ++i) {
    for (int16_t j = 0; j < w; ++j) {
      drawPixel(x + j, y + i, color);
    }
  }
}


void MarinePageGFX::drawLineToCanvas(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  if (!_textCanvas || !isReady()) return;
  if (x0 >= 0 && x0 < _width && y0 >= 0 && y0 < _height) {
  _textCanvas->drawPixel(x0, y0, color);
}
  int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int16_t err = dx + dy, e2;

  while (true) {
    _textCanvas->drawPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}


void MarinePageGFX::setFontByIndex(int index) {
  if (!_textCanvas) return;
  //DEBUG_PORT,printf("Font set: %d\n", index);
  if (index >= 0 && index < FONT_COUNT) {
    _currentFont = fontTable[index];
    _textCanvas->setFont(_currentFont);
  } else {
    _currentFont = nullptr;
    _textCanvas->setFont(nullptr);
  }
}

int MarinePageGFX::getFontLineHeight(FontID id) {
  if (id >= 0 && id < FONT_COUNT) {
    return fontHeightTable[id];
  }
  return 8; // fallback default
}



void MarinePageGFX::clearOutsideRadius(_sButton& button, uint16_t color) {
  if (!_textCanvas || !isReady()) return;

  // Compute center and radius from button
  int16_t centerX = button.h + button.width / 2;
  int16_t centerY = button.v + button.height / 2;
  int16_t radius = (button.height - 2 * button.bordersize) / 2;
  int32_t r2 = radius * radius;

  uint16_t* canvasBuf = (uint16_t*)_textCanvas->getFramebuffer();

  for (int16_t y = 0; y < _height; y++) {
    for (int16_t x = 0; x < _width; x++) {
      int32_t dx = x - centerX;
      int32_t dy = y - centerY;
      if ((dx * dx + dy * dy) > r2) {
        canvasBuf[y * _width + x] = color;
      }
    }
  }
}


void MarinePageGFX::clearOutsideRadius(int16_t centerX, int16_t centerY, int16_t radius, uint16_t color) {
  if (!_textCanvas || !isReady()) return;

  int32_t r2 = radius * radius;
  uint16_t* canvasBuf = (uint16_t*)_textCanvas->getFramebuffer();

  for (int16_t y = 0; y < _height; y++) {
    for (int16_t x = 0; x < _width; x++) {
      int32_t dx = x - centerX;
      int32_t dy = y - centerY;
      if ((dx * dx + dy * dy) > r2) {
        canvasBuf[y * _width + x] = color;  // Direct write to canvas buffer
      }
    }
  }
}


void MarinePageGFX::DrawScrollingGraph(_sButton& button, const GraphBuffer& buffer, double minVal, double maxVal) {
  if (!_textCanvas || buffer.count == 0) return;

  // Clear textbox area
  _textCanvas->fillRect(button.h, button.v, button.width, button.height, button.BackColor);

  // Optional border
  if (button.bordersize > 0) {
    for (int i = 0; i < button.bordersize; i++) {
      _textCanvas->drawRect(button.h + i, button.v + i,
                            button.width - 2 * i, button.height - 2 * i,
                            button.BorderColor);
    }
  }

  // Graph dimensions
  int16_t graphX = button.h+button.bordersize;
  int16_t graphY = button.v+button.bordersize;
  int16_t graphW = button.width-(2*button.bordersize);
  int16_t graphH = button.height-(2*button.bordersize);

  // Scale factor
  double range = maxVal - minVal;
  if (range <= 0.0001) range = 1.0;

  // Step size
  float xStep = static_cast<float>(graphW) / 199.0;

  // Draw graph line
  for (uint16_t i = 1; i < buffer.count; i++) {
    double v1 = buffer.get(i - 1);
    double v2 = buffer.get(i);

    int16_t x1 = graphX + static_cast<int16_t>((i - 1) * xStep);
    int16_t x2 = graphX + static_cast<int16_t>(i * xStep);

    int16_t y1 = graphY + graphH - static_cast<int16_t>(((v1 - minVal) / range) * graphH);
    int16_t y2 = graphY + graphH - static_cast<int16_t>(((v2 - minVal) / range) * graphH);
    drawWideLineToCanvas(x1, y1, x2, y2, button.TextColor,1);
    //_textCanvas->drawLine(x1, y1, x2, y2, button.TextColor);
  }

  // Optional: draw zero line or threshold
  if (minVal < 0 && maxVal > 0) {
    int16_t zeroY = graphY + graphH - static_cast<int16_t>((0 - minVal) / range * graphH);
    _textCanvas->drawLine(graphX, zeroY, graphX + graphW, zeroY, DARKGREY);
  }

  // Push to display
  //pushCanvas();
}
void MarinePageGFX::GFXBorderBoxPrintf(_sButton& button, const char* fmt, ...) {
  if (!_textCanvas || !fmt) return;

  // Format the string
  char buffer[128];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  //parse value back to double 
  double testValue = 0.0;
  sscanf(buffer, "%lf", &testValue);
// 
  if (testValue == -1e9 || std::isnan(testValue)) {return;}
  // Font selection
  if (button.Font >= 0 && button.Font < FONT_COUNT) {
    setFontByIndex(button.Font);
  }

  // Set text size to 1 (no magnification)
  _textCanvas->setTextSize(1);

  // Measure text bounds
  int16_t x, y;
  uint16_t w, h;
  _textCanvas->getTextBounds(buffer, 0, 0, &x, &y, &w, &h);

  // Calculate centered position
  int16_t tx = button.h + (button.width - w) / 2 - x;
  int16_t ty = button.v + (button.height - h) / 2 - y;

  // Draw background
  _textCanvas->fillRect(button.h, button.v, button.width, button.height, button.BackColor);
  // Draw border
  if (button.bordersize > 0) {
    for (int i = 0; i < button.bordersize; i++) {
      _textCanvas->drawRect(button.h + i, button.v + i,
                            button.width - 2 * i, button.height - 2 * i,
                            button.BorderColor);
    }
  }

  // Set text color and print
  _textCanvas->setTextColor(NEAR_BLACK);
  _textCanvas->setCursor(tx+_ShadowX, ty+_ShadowY);
  _textCanvas->print(buffer);
  _textCanvas->setTextColor(button.TextColor);
  _textCanvas->setCursor(tx, ty);
  _textCanvas->print(buffer);



  // Optional: draw debug box around text bounds
  // _textCanvas->drawRect(tx + x, ty + y, w, h, 0xF800); // red box

  // Reset PrintLine since we're not stacking
  button.PrintLine = 0;
}


void MarinePageGFX::PrintSubshadow(_sButton& button, const char* valueBuffer, int16_t valH,int16_t valV, int chosenFont){
  setFontByIndex(chosenFont);  
  if ((_ShadowX)||(_ShadowY)){
  _textCanvas->setCursor(valH+_ShadowX,valV+_ShadowY);
  _textCanvas->setTextColor(NEAR_BLACK);
  _textCanvas->print(valueBuffer);}
  _textCanvas->setCursor(valH,valV);
  _textCanvas->setTextColor(button.TextColor);
  _textCanvas->print(valueBuffer);
}

void MarinePageGFX::AutoPrint2Size(_sButton& button, const char* reference, const char* fmt, ...) {
    if (!_textCanvas || !fmt) return;
  char valueBuffer[64]; 
  int magnify =1; 
  va_list args;
  va_start(args, fmt);
  vsnprintf(valueBuffer, sizeof(valueBuffer), fmt, args);
  va_end(args);
    //parse value back to double 
  double testValue = 0.0;
  sscanf(valueBuffer, "%lf", &testValue);
// DEBUG_PORT.println(testValue);
// 
  if (testValue == -1e9 || std::isnan(testValue)) {return;}
  int printableWidth = button.width - 2 * button.bordersize;
  int printableHeight = button.height - 2 * button.bordersize;
    // Draw background
  _textCanvas->fillRect(button.h, button.v, button.width, button.height, button.BackColor);
  // Draw border
  if (button.bordersize > 0) {
    for (int i = 0; i < button.bordersize; i++) {
      _textCanvas->drawRect(button.h + i, button.v + i,
                            button.width - 2 * i, button.height - 2 * i,
                            button.BorderColor);
    }
  }
  // Measure parts
  int16_t x1, y1;
  uint16_t w1,w2, h1,h2;
  // Font probing using FontID and fontTable
  _textCanvas->setTextSize(1);
  int chosenFont;
  _textCanvas->setTextBound(0, 0, 1024, 1024);  // artificially large bounds
  int lastw =0;
  for (int fontIndex = 7; fontIndex <=13; fontIndex++) {
    setFontByIndex(fontIndex);
    _textCanvas->getTextBounds(reference, 0, 0, &x1, &y1, &w1, &h1);
    if ((w1 >= printableWidth)||(lastw>=w1)) { break;}   // make sure that if we have an error in the fint table and loop, we still catch the lagest font
    lastw=w1;
    chosenFont = fontIndex;
  }
 //  DEBUG_PORT.printf("** Font selected is font %i, magnification %i \n",chosenFont, magnify );
  // allow possibilty that if biggest font is less than half the printableWidth and then magnify by 2 
  if (lastw <= 2*printableWidth/3 ) {
    magnify=2; 
   _textCanvas->setTextSize(magnify);
    lastw =0;
    for (int fontIndex = 7; fontIndex <=13; fontIndex++) {
     setFontByIndex(fontIndex);
     _textCanvas->getTextBounds(reference, 0, 0, &x1, &y1, &w1, &h1);
      if ((w1 >= printableWidth)||(lastw>=w1)) { break;}   // make sure that if we have an error in the fint table and loop, we still catch the lagest font
      lastw=w1;
     chosenFont = fontIndex;
    }
  } 
  // SET chosen magnify and font for printing 
  _textCanvas->setTextSize(magnify);
  setFontByIndex(chosenFont);  
  _textCanvas->getTextBounds(reference, 0, 0, &x1, &y1, &w1, &h1); // repeat in case we do a magnify before this .. 
  _textCanvas->getTextBounds(valueBuffer, 0, 0, &x1, &y1, &w2, &h2);
   // Compute top  so reference would be vertically centered - fixes vertical alignment so different character heights do not shift position of decimal point
  int16_t refV = button.v + (button.height + h1) / 2; // V centered ?
   // Center actual value horizontally
  int16_t valH = button.h + button.bordersize + (button.width-w2) / 2;
  // set correct text bounds
  _textCanvas->setTextBound(button.h + button.bordersize, button.v+button.bordersize,button.width - 2 * button.bordersize, button.height - 2 * button.bordersize); 
 // DEBUG_PORT.printf("print  %s  at v%i h%i in font %i, magnification %i \n",valueBuffer,refV,valH,chosenFont,magnify );
  PrintSubshadow(button,valueBuffer,valH,refV,chosenFont);

  // setFontByIndex(chosenFont);  
  // _textCanvas->setCursor(valH+_ShadowX,refV+_ShadowY);
  // _textCanvas->setTextColor(NEAR_BLACK);
  // _textCanvas->print(valueBuffer);
  // _textCanvas->setCursor(valH,refV);
  // _textCanvas->setTextColor(button.TextColor);
  // _textCanvas->print(valueBuffer);
    
  _textCanvas->setTextSize(1); // reset magnify for other functions
  button.PrintLine = 0;
 }





void MarinePageGFX::BorderPrintCanvasTwoSize(_sButton& button,int magnify,int bigF,int Smallf,int decimalInset, const char* fmt, ...) {
  if (!_textCanvas || !fmt) return;
  // Format the string
  char buffer[128];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
 // DEBUG_PORT.println("debug ");
 // DEBUG_PORT.println(buffer);
  //parse value back to double 
  double testValue = 0.0;
  sscanf(buffer, "%lf", &testValue);
// DEBUG_PORT.println(testValue);
// 
  if (testValue == -1e9 || std::isnan(testValue)) {return;}

    // Font selection
  FontID mainFont = static_cast<FontID>(bigF);
  FontID smallFont =static_cast<FontID>(Smallf) ;
  // Font selection
  //FontID mainFont = static_cast<FontID>(Font);
  //FontID smallFont = static_cast<FontID>(Font - 1) ;

  _textCanvas->setTextSize(magnify);

  // Split into integer and fractional parts
  const char* dot = strchr(buffer, '.');
  char intPart[64], fracPart[64], intDotPart[66];
  if (dot) {
    size_t intLen = dot - buffer;
    strncpy(intPart, buffer, intLen);
    intPart[intLen] = '\0';
    // Prepend dot to fractional part
    snprintf(fracPart, sizeof(fracPart), ".%s", dot + 1);
    // Just the integer part (no dot)
    strncpy(intDotPart, intPart, sizeof(intDotPart));
    intDotPart[sizeof(intDotPart) - 1] = '\0';
  } else {
    strncpy(intPart, buffer, sizeof(intPart));
    intPart[sizeof(intPart) - 1] = '\0';
    fracPart[0] = '\0';  // No fractional part
    strncpy(intDotPart, intPart, sizeof(intDotPart));
    intDotPart[sizeof(intDotPart) - 1] = '\0';
  }

 // Measure parts
  int16_t x1, y1, x2, y2,y3;
  uint16_t w1, h1, w2, h2,h3,w3,w4,w5;
//   DEBUG_PORT.print(" Debug  before text bounds<");DEBUG_PORT,print(intDotPart);DEBUG_PORT,print(">  <");DEBUG_PORT,print(fracPart);DEBUG_PORT,println(">");
  _textCanvas->setTextBound(0,0,900,500); //Set bigger than we can so that GetText Bound is not limited
  _textCanvas->setFont(fontTable[mainFont]);
  _textCanvas->getTextBounds(intDotPart, 0, 0, &x1, &y1, &w1, &h1);
   _textCanvas->getTextBounds("119", 0, 0, &x1, &y1, &w5, &h3); // I just want h3
  _textCanvas->setFont(fontTable[smallFont]);
  _textCanvas->getTextBounds(fracPart, 0, 0, &x2, &y2, &w2, &h2);
  _textCanvas->getTextBounds(".", 0, 0, &x2, &y2, &w3, &h2);
  _textCanvas->getTextBounds("9", 0, 0, &x2, &y2, &w4, &h2); // get w4 = width of small 9 
  
  uint16_t totalHeight = h3; // fix where 9 is ! do not use max std::max(h1, h2);
 // DEBUG_PORT.print("text<");DEBUG_PORT.print(intDotPart);DEBUG_PORT.print(">  <");DEBUG_PORT.print(fracPart);DEBUG_PORT.println(">");
 // DEBUG_PORT.print("width <");DEBUG_PORT.print(w1);DEBUG_PORT.print(">  <");DEBUG_PORT.print(w2);DEBUG_PORT.print("> <= ? width ");DEBUG_PORT.print(button.width-(2*button.bordersize));DEBUG_PORT.println("");
  uint16_t TotalWidth= w2+w1+w3;
  // Fixed decimal anchor position relative to box 
  // Draw background
  _textCanvas->fillRect(button.h, button.v, button.width, button.height, button.BackColor);
  // Draw border
  if (button.bordersize > 0) {
    for (int i = 0; i < button.bordersize; i++) {
      _textCanvas->drawRect(button.h + i, button.v + i,
                            button.width - 2 * i, button.height - 2 * i,
                            button.BorderColor);
    }
  }

  decimalInset= button.width - (3*button.bordersize) ; 
  int16_t decimalX = button.h + decimalInset-w3;
  int16_t baselineY = button.v + (button.height + h3) / 2; // V centered ?
 if (TotalWidth <= button.width-(2*button.bordersize) ){decimalX = button.h+ button.width - (3*button.bordersize) -(2*w3)-w4;        }
  int16_t intX = decimalX - w1;
  


  //_textCanvas->setTextBound(button.h + button.bordersize, button.v+button.bordersize,button.width - 2 * button.bordersize, button.height - 2 * button.bordersize); //?
  // Draw integer + dot, right-aligned to decimalX

  
  _textCanvas->setFont(fontTable[mainFont]);
    _textCanvas->setTextColor(NEAR_BLACK);
  _textCanvas->setCursor(intX+_ShadowX, baselineY+_ShadowY);
  _textCanvas->print(intDotPart);
  _textCanvas->setTextColor(button.TextColor);
  _textCanvas->setCursor(intX, baselineY);
  _textCanvas->print(intDotPart);
 int16_t fracX = decimalX; // outside for diagnostic ?? add?? subtract ?? allow a bit more w3, for the  size ".""
   if (TotalWidth <= button.width-(2*button.bordersize) ){
  // Draw fractional part, left-aligned after decimal
     _textCanvas->setFont(fontTable[smallFont]);
     _textCanvas->setTextColor(NEAR_BLACK);
  _textCanvas->setCursor(fracX+_ShadowX, baselineY+_ShadowY);
  _textCanvas->print(fracPart);
   _textCanvas->setTextColor(button.TextColor);
  _textCanvas->setCursor(fracX, baselineY);
  _textCanvas->print(fracPart);
   }
  // Optional diagnostics
  //   _textCanvas->drawLine(intX, baselineY, fracX + w2, baselineY, RED); // baseline
  //   _textCanvas->drawLine(decimalX, button.v, decimalX, button.v + button.height, GREEN); // anchor
  //  _textCanvas->drawLine(intX, button.v, intX, button.v + button.height, DARKGREEN); // anchor
 
  button.PrintLine = 0;
}
void MarinePageGFX::DrawCompass(_sButton& button) {
  if (!_textCanvas) return;

  // Center and radius
  int x = button.h + button.width / 2;
  int y = button.v + button.height / 2;
  int rad = (button.height - (2 * button.bordersize)) / 2;

  // Layered radii
  int Rad1 = rad * 0.83;
  int Rad2 = rad * 0.86;
  int Rad3 = rad * 0.91;
  int Rad4 = rad * 0.94;
  int inner = (rad * 28) / 100;

  // Outer box and inner fill
  _textCanvas->fillRect(button.h, button.v, button.width, button.height, button.BorderColor);
  _textCanvas->fillRect(button.h + button.bordersize, button.v + button.bordersize,
                        button.width - (2 * button.bordersize),
                        button.height - (2 * button.bordersize),
                        button.BackColor);

  // Compass rings
  _textCanvas->fillCircle(x, y, rad, button.TextColor);     // outer ring
  _textCanvas->fillCircle(x, y, Rad1, button.BackColor);    // inner fill
  _textCanvas->fillCircle(x, y, inner - 1, button.TextColor);  // center ring
  _textCanvas->fillCircle(x, y, inner - 5, button.BackColor);  // inner core

  // Wind sectors
  _textCanvas->fillArc(x, y, Rad3, Rad1, 225, 270, RED);     // left sector
  _textCanvas->fillArc(x, y, Rad3, Rad1, 270, 315, GREEN);   // right sector

  // Major ticks every 30°
  for (int i = 0; i < 360; i += 30) {
    _textCanvas->fillArc(x, y, rad, Rad1, i, i + 1, BLACK);
  }

  // Minor ticks every 10°
  for (int i = 0; i < 360; i += 10) {
    _textCanvas->fillArc(x, y, rad, Rad4, i, i + 1, BLACK);
  }

}
void MarinePageGFX::drawCompassPointer(_sButton& button, int16_t baseWidth, int16_t tailLength, float angleDeg, uint16_t color, bool shadow) {
  if (!isReady() || !_textCanvas) return;

  // Compute center and radius from button
  int16_t centerX = button.h + button.width / 2;
  int16_t centerY = button.v + button.height / 2;
  int16_t radius = (button.height - 2 * button.bordersize) / 2;

  // Angle math
  float angleRad = (angleDeg-90) * DEG_TO_RAD;  // correct orientation the easy way 
  float perpRad = angleRad + PI / 2;

  // Tip and tail
  int16_t tipX = centerX + cos(angleRad) * radius;
  int16_t tipY = centerY + sin(angleRad) * radius;
  int16_t tailX = centerX - cos(angleRad) * tailLength;
  int16_t tailY = centerY - sin(angleRad) * tailLength;

  // Base triangle points
  int16_t baseX1 = centerX + cos(perpRad) * (baseWidth / 2);
  int16_t baseY1 = centerY + sin(perpRad) * (baseWidth / 2);
  int16_t baseX2 = centerX - cos(perpRad) * (baseWidth / 2);
  int16_t baseY2 = centerY - sin(perpRad) * (baseWidth / 2);

  // Shadow rendering
  if (shadow) {
    int16_t offset = 2;/*_ShadowX, baselineY+_ShadowY*/
    drawTriangleToCanvas(tipX + _ShadowX, tipY + _ShadowY,
                         baseX1 + _ShadowX, baseY1 + _ShadowY,
                         baseX2 + _ShadowX, baseY2 + _ShadowY,
                         NEAR_BLACK);

    drawTriangleToCanvas(tailX + _ShadowX, tailY + _ShadowY,
                         baseX2 + _ShadowX, baseY2 + _ShadowY,
                         baseX1 + _ShadowX, baseY1 + _ShadowY,
                         NEAR_BLACK);
  }

  // Main pointer
  drawTriangleToCanvas(tipX, tipY, baseX1, baseY1, baseX2, baseY2, color);
  drawTriangleToCanvas(tailX, tailY, baseX2, baseY2, baseX1, baseY1, color);
  drawLineToCanvas(tipX, tipY, tailX, tailY, NEAR_BLACK);
}


void MarinePageGFX::Addtitletobutton(_sButton& button, int position, int font, const char* fmt, ...) {
  if (!_textCanvas || !fmt) return;
  // Format the string
  char buffer[128];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  //parse value back to double 
  double testValue = 0.0;
  sscanf(buffer, "%lf", &testValue);
// 
  if (testValue == -1e9 || std::isnan(testValue)) {return;}
  // Font setup
  FontID fontID = static_cast<FontID>(font);
  _textCanvas->setFont(fontTable[fontID]);
  _textCanvas->setTextSize(1);

  // Measure text
  int16_t x, y;
  uint16_t w, h;
  _textCanvas->setTextBound(0, 0, 1024, 1024);  // artificially large bounds
  _textCanvas->getTextBounds(buffer, 0, 0, &x, &y, &w, &h);

  // Positioning logic
  int16_t tx = 0, ty = 0;
  const int16_t margin = 4;  // Padding from edge

  switch (position) {
    case 1: // Top Left
      tx = button.h + margin;
      ty = button.v + margin - y;
      break;

    case 2: // Top Right
      tx = button.h + button.width - margin - w;
      ty = button.v + margin - y;
      break;

    case 3: // Bottom Right
      tx = button.h + button.width - margin - w;
      ty = button.v + button.height - margin - h - y;
      break;

    case 4: // Bottom Left
      tx = button.h + margin;
      ty = button.v + button.height - margin - h - y;
      break;
	case 5: // Bottom Center
      tx = button.h + (button.width - w) / 2 - x;;
      ty = button.v + button.height - margin - h - y;
      break;
	  	case 6: // Top Center
      tx = button.h + (button.width - w) / 2 - x;;
      ty = button.v + margin - y;
      break;

    default:
      return;  // Invalid position
  }

  // Draw text
   _textCanvas->setTextBound(button.h + button.bordersize, button.v+button.bordersize,button.width - 2 * button.bordersize, button.height - 2 * button.bordersize);
  _textCanvas->setTextColor(button.TextColor,button.BorderColor);
  _textCanvas->setCursor(tx, ty);
  _textCanvas->print(buffer);

 
}


void MarinePageGFX::fillCircleToCanvas(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
  if (!_textCanvas || r <= 0) return;

  int16_t x = 0;
  int16_t y = r;
  int16_t dp = 1 - r;

  // Draw center line
  _textCanvas->drawFastHLine(x0 - r, y0, 2 * r + 1, color);

  while (x < y) {
    x++;
    if (dp < 0) {
      dp += 2 * x + 1;
    } else {
      y--;
      dp += 2 * (x - y) + 1;
    }

    // Draw horizontal spans across symmetry axes
    _textCanvas->drawFastHLine(x0 - x, y0 + y, 2 * x + 1, color);
    _textCanvas->drawFastHLine(x0 - x, y0 - y, 2 * x + 1, color);
    _textCanvas->drawFastHLine(x0 - y, y0 + x, 2 * y + 1, color);
    _textCanvas->drawFastHLine(x0 - y, y0 - x, 2 * y + 1, color);
  }
}


void MarinePageGFX::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
  for (int16_t y = -r; y <= r; y++) {
    for (int16_t x = -r; x <= r; x++) {
      if (x * x + y * y <= r * r) {
        drawPixel(x0 + x, y0 + y, color);
      }
    }
  }
}

void MarinePageGFX::fillArc(int16_t x0, int16_t y0, int16_t r, int16_t start_angle, int16_t end_angle, uint16_t color) {
  float start_rad = start_angle * DEG_TO_RAD;
  float end_rad = end_angle * DEG_TO_RAD;

  for (int16_t y = -r; y <= r; y++) {
    for (int16_t x = -r; x <= r; x++) {
      float angle = atan2f(y, x);
      if (angle < 0) angle += 2 * PI;

      float dist = sqrtf(x * x + y * y);
      if (dist <= r && angle >= start_rad && angle <= end_rad) {
        drawPixel(x0 + x, y0 + y, color);
      }
    }
  }
}

void MarinePageGFX::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (!isReady()) return;
  if (x >= 0 && x < _width && y >= 0 && y < _height) {
    _buffer[_active][y * _width + x] = color;
  }
}

void MarinePageGFX::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  if (!isReady() || x < 0 || x >= _width) return;
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (y + h > _height) h = _height - y;
  if (h <= 0) return;

  uint16_t* buf = _buffer[_active] + y * _width + x;
  for (int16_t i = 0; i < h; i++) {
    *buf = color;
    buf += _width;
  }
}

void MarinePageGFX::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  if (!isReady() || y < 0 || y >= _height) return;
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (x + w > _width) w = _width - x;
  if (w <= 0) return;

  uint16_t* buf = _buffer[_active] + y * _width + x;
  for (int16_t i = 0; i < w; i++) buf[i] = color;
}

void MarinePageGFX::drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corner, uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  while (x <= y) {
    if (corner & 0x1) drawPixel(x0 - y, y0 - x, color);  // top-left
    if (corner & 0x2) drawPixel(x0 + y, y0 - x, color);  // top-right
    if (corner & 0x4) drawPixel(x0 + y, y0 + x, color);  // bottom-right
    if (corner & 0x8) drawPixel(x0 - y, y0 + x, color);  // bottom-left

    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
  }
}
void MarinePageGFX::fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corner, uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  while (x <= y) {
    if (corner & 0x1) drawFastVLine(x0 - y, y0 - x, x + 1, color);      // top-left
    if (corner & 0x2) drawFastVLine(x0 + y, y0 - x, x + 1, color);      // top-right
    if (corner & 0x4) drawFastVLine(x0 + y, y0 + x - x, x + 1, color);  // bottom-right
    if (corner & 0x8) drawFastVLine(x0 - y, y0 + x - x, x + 1, color);  // bottom-left

    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
  }
}

void MarinePageGFX::drawWideLineToCanvas(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color,int width) {
  if (!_textCanvas || !isReady()) return;
  // Compute perpendicular vector
  float dx = x1 - x0;
  float dy = y1 - y0;
  float length = sqrt(dx * dx + dy * dy);
  if (length == 0) return;

  float nx = -dy / length;  // normal x
  float ny = dx / length;   // normal y
  float halfWidth = width + 0.5;    // line thickness

  // Offset points to form a thin quad
  int16_t x0a = x0 + nx * halfWidth;
  int16_t y0a = y0 + ny * halfWidth;
  int16_t x0b = x0 - nx * halfWidth;
  int16_t y0b = y0 - ny * halfWidth;

  int16_t x1a = x1 + nx * halfWidth;
  int16_t y1a = y1 + ny * halfWidth;
  int16_t x1b = x1 - nx * halfWidth;
  int16_t y1b = y1 - ny * halfWidth;

  // Draw two triangles to form a quad
  _textCanvas->fillTriangle(x0a, y0a, x1a, y1a, x1b, y1b, color);
  _textCanvas->fillTriangle(x0a, y0a, x0b, y0b, x1b, y1b, color);
}



void MarinePageGFX::drawTriangleToCanvas(int16_t x0, int16_t y0,
                                         int16_t x1, int16_t y1,
                                         int16_t x2, int16_t y2,
                                         uint16_t color) {
  if (!_textCanvas || !isReady()) return;
  _textCanvas->fillTriangle(x0, y0, x1, y1, x2, y2, color);
}


void MarinePageGFX::fillTriangle(int16_t x0, int16_t y0,
                                 int16_t x1, int16_t y1,
                                 int16_t x2, int16_t y2,
                                 uint16_t color) {
  auto swap = [](int16_t& a, int16_t& b) {
    int16_t t = a;
    a = b;
    b = t;
  };

  // Sort by y
  if (y0 > y1) {
    swap(y0, y1);
    swap(x0, x1);
  }
  if (y1 > y2) {
    swap(y1, y2);
    swap(x1, x2);
  }
  if (y0 > y1) {
    swap(y0, y1);
    swap(x0, x1);
  }

  auto drawScanline = [&](int16_t y, int16_t xa, int16_t xb) {
    if (xa > xb) swap(xa, xb);
    for (int16_t x = xa; x <= xb; x++) drawPixel(x, y, color);
  };

  int16_t totalHeight = y2 - y0;
  for (int16_t i = 0; i < totalHeight; i++) {
    bool secondHalf = i > y1 - y0 || y1 == y0;
    int16_t segmentHeight = secondHalf ? y2 - y1 : y1 - y0;
    float alpha = (float)i / totalHeight;
    float beta = (float)(i - (secondHalf ? y1 - y0 : 0)) / segmentHeight;

    int16_t ax = x0 + (x2 - x0) * alpha;
    int16_t bx = secondHalf
                   ? x1 + (x2 - x1) * beta
                   : x0 + (x1 - x0) * beta;

    int16_t y = y0 + i;
    drawScanline(y, ax, bx);
  }
}

// Returns the current X position of the text cursor
int MarinePageGFX::getCursorX() {
    return _textCanvas->getCursorX();  // assuming _textcanvas is a pointer
}

int MarinePageGFX::getCursorY() {
    return _textCanvas->getCursorY();
}

void MarinePageGFX::setCursor(int16_t x, int16_t y) {
  _textCanvas->setCursor(x, y);

}

void MarinePageGFX::setTextColor(uint16_t fg, uint16_t bg) {
	_gfx->setTextColor(fg,bg);
  _textColor = fg;
  _backgroundColor = bg;
}
void MarinePageGFX::setTextColor(uint16_t color) {
  _textColor = color;
}

void MarinePageGFX::setTextSize(uint8_t size) {
  _textSize = size;
}

void MarinePageGFX::getTextBounds(const char* msg, int16_t x, int16_t y,
                                   int16_t* x1, int16_t* y1,
                                   uint16_t* w, uint16_t* h) {
    if (!msg ) return;
     _textCanvas->getTextBounds(msg, x, y, x1, y1, w, h);

}

void MarinePageGFX::println(const char* buf) {
  _gfx->setCursor(_cursorX, _cursorY);
  _gfx->setTextColor(_textColor);
  _gfx->setTextSize(_textSize);

  // Apply bounding box and wrap settings
  if (_textBoundW > 0 && _textBoundH > 0) {
    _gfx->setTextBound(_textBoundX, _textBoundY, _textBoundW, _textBoundH);
    _gfx->setTextWrap(_textWrap);
  }

  _gfx->println(buf);
}


void MarinePageGFX::print(const char* buf) {
  _gfx->setCursor(_cursorX, _cursorY);
  _gfx->setTextColor(_textColor);
  _gfx->setTextSize(_textSize);

  // Apply bounding box and wrap settings
  if (_textBoundW > 0 && _textBoundH > 0) {
    _gfx->setTextBound(_textBoundX, _textBoundY, _textBoundW, _textBoundH);
    _gfx->setTextWrap(_textWrap);
  }

  _gfx->print(buf);
}




void MarinePageGFX::printf(const char* fmt, ...) {
  char buf[128];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  _gfx->setCursor(_cursorX, _cursorY);
  _gfx->setTextColor(_textColor);
  _gfx->setTextSize(_textSize);
   // Apply bounding box and wrap settings
  if (_textBoundW > 0 && _textBoundH > 0) {
    _gfx->setTextBound(_textBoundX, _textBoundY, _textBoundW, _textBoundH);
    _gfx->setTextWrap(_textWrap);
  }
  _gfx->print(buf);
}

uint16_t* MarinePageGFX::getActiveBuffer() {
  return _buffer[_active];
}
void MarinePageGFX::clearCanvas(uint16_t color) {
  if (_textCanvas && isReady()) {
    _textCanvas->fillScreen(color);
  }
}

void MarinePageGFX::drawText(int16_t x, int16_t y, const char* text, uint8_t size, uint16_t color) {
  if (!_textCanvas || !isReady()) return;
  _textCanvas->setFont(_gfxFont);
  _textCanvas->setTextSize(size);
  _textCanvas->setTextColor(color);
  _textCanvas->setCursor(x, y);
  _textCanvas->print(text);
  // Copy canvas pixels into active buffer
  uint16_t* canvasBuf = (uint16_t*)_textCanvas->getFramebuffer();
  //Replace  with pixel-wise blending  memcpy(_buffer[_active], canvasBuf, _width * _height * sizeof(uint16_t));
  for (int16_t y = 0; y < _height; y++) {
    for (int16_t x = 0; x < _width; x++) {
      uint16_t pixel = canvasBuf[y * _width + x];
      if (pixel != 0) {  // Only draw non-black pixels (assuming black = transparent)
        _buffer[_active][y * _width + x] = pixel;
      }
    }
  }
}


void MarinePageGFX::compositeCanvas() {
  uint16_t* canvasBuf = (uint16_t*)_textCanvas->getFramebuffer();
  for (int16_t y = 0; y < _height; y++) {
    for (int16_t x = 0; x < _width; x++) {
      uint16_t pixel = canvasBuf[y * _width + x];
      if (pixel != 0) { //for black is transparent
        _buffer[_active][y * _width + x] = pixel;
      }
    

    }
  }
}
//Left center Right
void MarinePageGFX::drawTextAlign(int16_t x, int16_t y, const char* text, uint8_t size, uint16_t color, uint8_t align) {
  if (!_textCanvas || !isReady()) return;

  _textCanvas->setFont(_gfxFont);
  _textCanvas->setTextSize(size);

  int16_t x1, y1;
  uint16_t w, h;
  _textCanvas->getTextBounds(text, x, y, &x1, &y1, &w, &h);

  if (align == 1) x -= w / 2;      // Center
  else if (align == 2) x -= w;     // Right

  _textCanvas->setTextColor(color);
  _textCanvas->setCursor(x, y);
  _textCanvas->print(text);

  // Composite into active buffer
  uint16_t* canvasBuf = (uint16_t*)_textCanvas->getFramebuffer();
  for (int16_t yy = 0; yy < _height; yy++) {
    for (int16_t xx = 0; xx < _width; xx++) {
      uint16_t pixel = canvasBuf[yy * _width + xx];
      if (pixel != 0) {
        _buffer[_active][yy * _width + xx] = pixel;
      }
    }
  }
}
//center on x and y 
void MarinePageGFX::drawTextCentered(int16_t centerX, int16_t centerY, const char* text, uint8_t size, uint16_t color) {
  if (!_textCanvas || !isReady()) return;

  _textCanvas->setFont(_gfxFont);
  _textCanvas->setTextSize(size);

  int16_t x1, y1;
  uint16_t w, h;
  _textCanvas->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

  int16_t x = centerX - (w / 2);
  int16_t y = centerY - (h / 2) - y1;

  _textCanvas->setTextColor(color);
  _textCanvas->setCursor(x, y);
  _textCanvas->print(text);

  // ❌ Removed compositing — let caller decide when to composite
}

void MarinePageGFX::drawTextOverlay(const char* label, uint16_t color) {
  if (!_textCanvas || !isReady()) return;

  _textCanvas->setFont(_gfxFont);
  _textCanvas->setTextSize(1);
  _textCanvas->setTextColor(color);
  _textCanvas->setCursor(4, 4);
  _textCanvas->print(label);

  uint16_t* canvasBuf = (uint16_t*)_textCanvas->getFramebuffer();
  for (int16_t yy = 0; yy < _height; yy++) {
    for (int16_t xx = 0; xx < _width; xx++) {
      uint16_t pixel = canvasBuf[yy * _width + xx];
      if (pixel != 0) {
        _buffer[_active][yy * _width + xx] = pixel;
      }
    }
  }
}
void MarinePageGFX::fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t r, uint16_t color) {
  if (!isReady()) return;

  // Fill center rectangle
  fillRect(x0 + r, y0, w - 2 * r, h, color);
  fillRect(x0, y0 + r, r, h - 2 * r, color);
  fillRect(x0 + w - r, y0 + r, r, h - 2 * r, color);

  // Fill corner circles
  fillCircleHelper(x0 + r, y0 + r, r, 1, color);                  // Top-left
  fillCircleHelper(x0 + w - r - 1, y0 + r, r, 2, color);          // Top-right
  fillCircleHelper(x0 + r, y0 + h - r - 1, r, 8, color);          // Bottom-left
  fillCircleHelper(x0 + w - r - 1, y0 + h - r - 1, r, 4, color);  // Bottom-right
}
void MarinePageGFX::drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t r, uint16_t color) {
  if (!isReady()) return;

  // Draw straight edges
  drawFastHLine(x0 + r, y0, w - 2 * r, color);          // Top
  drawFastHLine(x0 + r, y0 + h - 1, w - 2 * r, color);  // Bottom
  drawFastVLine(x0, y0 + r, h - 2 * r, color);          // Left
  drawFastVLine(x0 + w - 1, y0 + r, h - 2 * r, color);  // Right

  // Draw corner arcs
  drawCircleHelper(x0 + r, y0 + r, r, 1, color);                  // Top-left
  drawCircleHelper(x0 + w - r - 1, y0 + r, r, 2, color);          // Top-right
  drawCircleHelper(x0 + r, y0 + h - r - 1, r, 8, color);          // Bottom-left
  drawCircleHelper(x0 + w - r - 1, y0 + h - r - 1, r, 4, color);  // Bottom-right
}


void MarinePageGFX::setFont(const GFXfont* font) {
  _gfxFont = font;
}
