#include "MarinePageGFX.h"
#include <stdarg.h>
#include <math.h>

#include "..\FontType.h"


#define DEG_TO_RAD 0.0174532925f

const GFXfont* _currentFont = nullptr;

// Private variables
static int _ShadowX = 0;
static int _ShadowY = 0;
static bool _ShadowON = 0;
extern int MasterFont;
extern int Screen_Width;

int getShadowY() { return _ShadowY;}
void MarinePageGFX::setShadowY(int value) {_ShadowY = value;}

int getShadowX() {return _ShadowX;}
void MarinePageGFX::setShadowX(int value) {_ShadowX = value;}

bool getShadow_ON() {return _ShadowON;}
void MarinePageGFX::setShadow_ON(bool value) {_ShadowON = value;}



void MarinePageGFX::clearTextCanvas(uint16_t bg) {
  if (!_textCanvas || !isReady()) return;
  _textCanvas->fillScreen(bg);
}

 MarinePageGFX::MarinePageGFX(Arduino_GFX* gfx, int16_t width, int16_t height)
  : _gfx(gfx), _width(width), _height(height), _active(0),
    _cursorX(0), _cursorY(0), _textColor(0xFFFF), _textSize(1) {

  if (!psramFound()) {
    Serial0.println("ERROR: PSRAM not found. Cannot allocate page buffers.");
    _buffer[0] = nullptr;
    _buffer[1] = nullptr;
    return;
  }

  size_t bufSize = width * height * sizeof(uint16_t);
  _buffer[0] = (uint16_t*)heap_caps_malloc(bufSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  _buffer[1] = (uint16_t*)heap_caps_malloc(bufSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  if (!_buffer[0] || !_buffer[1]) {
    Serial0.println("ERROR: Failed to allocate page buffers in PSRAM.");
    _buffer[0] = nullptr;
    _buffer[1] = nullptr;
  } else {
    Serial0.printf("Page buffers allocated: %d bytes each\n", bufSize);
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
    Serial0.println("WARNING: Buffer not initialized. Skipping fillScreen.");
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
    Serial0.println("WARNING: Buffer not initialized. Skipping push.");
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
  //Serial0.printf("Font set: %d\n", index);


  if (index >= 0 && index < FONT_COUNT) {
    _currentFont = fontTable[index];
    _textCanvas->setFont(_currentFont);
  } else {
    _currentFont = nullptr;
    _textCanvas->setFont(nullptr);
  }
}

int MarinePageGFX::getFontByIndex(void) {
  if (!_currentFont) return -1; // No font set

  for (int i = 0; i < FONT_COUNT; ++i) {
    if (_currentFont == fontTable[i]) {
      return i;
    }
  }

  return -1; // Font not found in table
}



int MarinePageGFX::getFontLineHeight(FontID id) {
  if (id >= 0 && id < FONT_COUNT) {
    return fontHeightTable[id];
  }
  return 8; // fallback default
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
  
  float angleRad = (angleDeg-90) * DEG_TO_RAD; // align with convention up is 0/360
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
    int16_t offset = 2;
    drawTriangleToCanvas(tipX + offset, tipY + offset,
                         baseX1 + offset, baseY1 + offset,
                         baseX2 + offset, baseY2 + offset,
                         NEAR_BLACK);

    drawTriangleToCanvas(tailX + offset, tailY + offset,
                         baseX2 + offset, baseY2 + offset,
                         baseX1 + offset, baseY1 + offset,
                         NEAR_BLACK);
  }

  // Main pointer
  drawTriangleToCanvas(tipX, tipY, baseX1, baseY1, baseX2, baseY2, color);
  drawTriangleToCanvas(tailX, tailY, baseX2, baseY2, baseX1, baseY1, color);
  drawLineToCanvas(tipX, tipY, tailX, tailY, NEAR_BLACK);
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
  if (range == 0) range = 1.0;
  // Step size
  float xStep = static_cast<float>(graphW) / 199.0;
  
  // Inline clamp helper
  auto clampY = [&](int16_t y) -> int16_t {
    if (y < graphY) return graphY;
    if (y > graphY + graphH) return graphY + graphH;
    return y;
  };
  // Draw graph line
  for (uint16_t i = 1; i < buffer.count; i++) {
    double v1 = buffer.get(i - 1);
    double v2 = buffer.get(i);

    int16_t x1 = graphX + static_cast<int16_t>((i - 1) * xStep);
    int16_t x2 = graphX + static_cast<int16_t>(i * xStep);

    int16_t y1 = graphY + graphH - static_cast<int16_t>(((v1 - minVal) / range) * graphH);
    int16_t y2 = graphY + graphH - static_cast<int16_t>(((v2 - minVal) / range) * graphH);
	    // Clamp to graph box so drawing never escapes the button
    y1 = clampY(y1);
    y2 = clampY(y2);


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
  if (testValue == -1e9  ) {return;}
  int printableWidth = button.width - 2 * button.bordersize;
  int printableHeight = button.height - 2 * button.bordersize;
  DrawBox(button);
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

  _textCanvas->setTextSize(1); // reset magnify for other functions
  button.PrintLine = 0;button.lastY = button.v+button.bordersize;
 }


void MarinePageGFX::BorderPrintCanvasTwoSize(_sButton& button, int decimalInset, const char* fmt, ...) {
  if (!_textCanvas || !fmt) return;

  // Format the string
  char buffer[128];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  // Font selection
  FontID mainFont = static_cast<FontID>(button.Font);
  FontID smallFont = (mainFont > 0) ? static_cast<FontID>(button.Font - 1) : mainFont;

  _textCanvas->setTextSize(1);

  // Split into integer and fractional parts
  const char* dot = strchr(buffer, '.');
  char intPart[64], fracPart[64], intDotPart[66];

  if (dot) {
    size_t intLen = dot - buffer;
    strncpy(intPart, buffer, intLen);
    intPart[intLen] = '\0';

    strncpy(fracPart, dot + 1, sizeof(fracPart));
    fracPart[sizeof(fracPart) - 1] = '\0';
    snprintf(intDotPart, sizeof(intDotPart), "%s.", intPart);  // int + dot
  } else {
    strncpy(intPart, buffer, sizeof(intPart));
    intPart[sizeof(intPart) - 1] = '\0';
    fracPart[0] = '\0';
    strncpy(intDotPart, intPart, sizeof(intDotPart));
    intDotPart[sizeof(intDotPart) - 1] = '\0';
  }

  // Measure parts
  int16_t x1, y1, x2, y2,y3;
  uint16_t w1, h1, w2, h2,w3;

  _textCanvas->setFont(fontTable[mainFont]);
  _textCanvas->getTextBounds(intDotPart, 0, 0, &x1, &y1, &w1, &h1);

  _textCanvas->setFont(fontTable[smallFont]);
  _textCanvas->getTextBounds(fracPart, 0, 0, &x2, &y2, &w2, &h2);
  _textCanvas->getTextBounds("1", 0, 0, &x2, &y2, &w3, &h2);
  
  uint16_t totalHeight = std::max(h1, h2);

  // Fixed decimal anchor position
  int16_t decimalX = button.h + decimalInset;
  int16_t baselineY = button.v + (button.height - totalHeight) / 2 - y1; 

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

  // Draw integer + dot, right-aligned to decimalX
  int16_t intX = decimalX - w1;
  _textCanvas->setFont(fontTable[mainFont]);
    _textCanvas->setTextColor(NEAR_BLACK);
  _textCanvas->setCursor(intX+2, baselineY+2);
  _textCanvas->print(intDotPart);
  _textCanvas->setTextColor(button.TextColor);
  _textCanvas->setCursor(intX, baselineY);
  _textCanvas->print(intDotPart);

  // Draw fractional part, left-aligned after decimal
  int16_t fracX = decimalX+(w3/2); // allow a bit more for the decimal size half the width of a "1"
  
  _textCanvas->setFont(fontTable[smallFont]);
     _textCanvas->setTextColor(NEAR_BLACK);
  _textCanvas->setCursor(fracX+2, baselineY+2);
  _textCanvas->print(fracPart);
   _textCanvas->setTextColor(button.TextColor);
  _textCanvas->setCursor(fracX, baselineY);
  _textCanvas->print(fracPart);

  // Optional diagnostics
  //if (diagnosticsEnabled()) {
  //  _textCanvas->drawLine(intX, baselineY, fracX + w2, baselineY, TFT_RED); // baseline
  //  _textCanvas->drawLine(decimalX, button.v, decimalX, button.v + button.height, TFT_CYAN); // anchor
  //}

  button.PrintLine = 0;button.lastY = button.v+button.bordersize;
}

//Pos 1 2 3 4 for top right, botom right etc. add a top left title to the box -1 outside 
void MarinePageGFX::AddTitleInsideBox(_sButton& button, int position, int font, const char* fmt, ...) {
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
  if (testValue == -1e9  ) {return;}
  // Font setup
  setFontByIndex(font);
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
	  	  
	case -1: // outside title top left
      tx = button.h + margin;
      ty = button.v - margin - y;
      break;
 	  
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

void MarinePageGFX::Addtitletobutton(_sButton& button, int position, int font, const char* fmt, ...) {
  if (!_textCanvas || !fmt) return;

  // Format the string
  char buffer[128];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  // Font setup
  setFontByIndex(font);
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
      ty = button.v + margin - h;
      break;

    case 2: // Top Right
      tx = button.h + button.width - margin - w;
      ty = button.v + margin - h;
      break;

    case 3: // Bottom Right
      tx = button.h + button.width - margin - w;
      ty = button.v + button.height + margin ;
      break;

    case 4: // Bottom Left
      tx = button.h + margin;
      ty = button.v + button.height + margin  ;
      break;
	case 5: // Bottom Center
      tx = button.h + (button.width - w) / 2 - x;;
      ty = button.v + button.height + margin - h ;
      break;
	case 6: // Top Center
      tx = button.h + (button.width - w) / 2 - x;;
      ty = button.v + margin - y;
      break;

    default:
	  tx = button.h + margin;
      ty = button.v + margin - h;
    return;  
  }
  if (ty<=0){ty=0;}if (ty>=480){ty=480-h-margin;}
  if (tx<=0){tx=0;}

  // Draw text
   if (button.BackColor!=button.BorderColor){
  _textCanvas->setTextColor(button.BackColor,button.BorderColor);}
  else{
  _textCanvas->setTextColor(WHITE,NEAR_BLACK); }
  _textCanvas->setCursor(tx, ty);
  _textCanvas->print(buffer);

 
}

void MarinePageGFX::GFXBorderBoxPrintf(_sButton& button, const char* fmt, ...) {
  if (!_textCanvas || !fmt) return;
  // Format the string
  char buffer[128];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  // Font selection
    setFontByIndex(button.Font);
  // Set text size to 1 (no magnification)
  _textCanvas->setTextSize(1);

  // Measure text bounds
  int16_t x, y;
  uint16_t w, h;
    _textCanvas->setTextBound(0, 0, 1024, 1024);  // artificially large bounds
  _textCanvas->getTextBounds(buffer, 0, 0, &x, &y, &w, &h);

  // Calculate centered position
  int16_t tx = button.h + (button.width - w) / 2 - x;
  int16_t ty = button.v + (button.height - h) / 2 - y;

  // Draw background
  DrawBox(button);
  PrintSubshadow(button,buffer,tx,ty,button.Font);

  // Optional: draw debug box around text bounds
  // _textCanvas->drawRect(tx + x, ty + y, w, h, 0xF800); // red box

  // Reset PrintLine since we're not stacking
  button.PrintLine = 0;button.lastY = button.v;
}

void MarinePageGFX::DrawBox(_sButton& button){
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

void MarinePageGFX::SplitInterDecimal(const char* buffer, char* Integer, char* Fraction, char* DotPart) {
  if (!buffer || !Integer || !Fraction ) return;
  const char* dot = strchr(buffer, '.');
  if (dot) {
    size_t intLen = dot - buffer;
    strncpy(Integer, buffer, intLen);
    Integer[intLen] = '\0';
    // Prepend dot to fractional part
    snprintf(Fraction, sizeof(Fraction), ".%s", dot + 1);
    // Just the integer part (no dot)
    strncpy(DotPart, Integer, sizeof(DotPart));
    DotPart[sizeof(DotPart) - 1] = '\0';
  } else {
    strncpy(Integer, buffer, sizeof(Integer));
    Integer[sizeof(Integer) - 1] = '\0';
    Fraction[0] = '\0';  // No fractional part
    strncpy(DotPart, Integer, sizeof(DotPart));
    DotPart[sizeof(DotPart) - 1] = '\0';
  }
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

void MarinePageGFX::setCursor(int16_t x, int16_t y) {
  _cursorX = x;
  _cursorY = y;
}
int16_t MarinePageGFX::getCursorX() {
  return _cursorX;
}
int16_t MarinePageGFX::getCursorY() {
  return _cursorY;
}

void MarinePageGFX::setTextColor(uint16_t color) {
  _textColor = color;
}

void MarinePageGFX::setTextSize(uint8_t size) {
  _textSize = size;
}
void MarinePageGFX::setFont(const GFXfont* font) {
  _gfxFont = font;
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


// this overload has no option for centered..
void MarinePageGFX::CommonSub_UpdateLine(uint16_t color, int font, _sButton &button, const char *msg) {
  CommonSub_UpdateLine(true, false, color, font, button, msg);
}
void MarinePageGFX::CommonSub_UpdateLine(bool horizCenter, bool vertCenter, uint16_t color, int font, _sButton &button, const char *msg) {
  int LinesOfType;
  int16_t x, y, TBx1, TBy1;
  uint16_t TBw1, TBh1,TextW,TextH;
  int typingspaceH, typingspaceW;
  int local;
  int MasterFont = getFontByIndex();
  // can now change font inside this function
  setFontByIndex(font);
  typingspaceH = button.height - (2 * button.bordersize);
  typingspaceW = button.width - (2 * button.bordersize);
                                                    
  if (horizCenter || vertCenter) {
    _textCanvas->setTextWrap(false);
  } else {
    _textCanvas->setTextWrap(true);
  }
  // get bounds as would be printed at top of box..
  // set text bounds first so that can be taken into account ! Use same zero starts as in Sub_for_UpdateTwoSize
  _textCanvas->setTextBound(0, 0, 1024, 500);   
  _textCanvas->getTextBounds("9", 0, 0, &TBx1, &TBy1, &TextW, &TextH);  // so that TBx1 can be simply obtained and used in better h centering
  _textCanvas->getTextBounds(msg, 0, 0, &TBx1, &TBy1, &TBw1, &TBh1);  // do not forget '&' using pointers not values!!!
  _textCanvas->setTextBound(button.h + button.bordersize, button.v + button.bordersize, typingspaceW, typingspaceH);  //
  //test _textCanvas->fillRect(button.h + button.bordersize, button.v + button.bordersize, typingspaceW, typingspaceH,RED);
   if(button.lastY<=button.v+button.bordersize){DrawBox(button);button.lastY=  button.v+button.bordersize;}
  y = button.lastY+ TextH;
  x = button.h + button.bordersize;
  if (horizCenter) { x = x + ((typingspaceW - (TBw1)) / 2) - TBx1; }                                   // subtract any start text offset
  if (vertCenter) { y = TextH + button.bordersize + button.v + ((typingspaceH - (TBh1)) / 2); }  // vertical centering
  //page->fillRect(x,y-TextH,TBw1,TBh1, button.BackColor); // Background exactly the text - needed for STATUS to make flash work in status!
  
  _textCanvas->setCursor(x, y);
  _textCanvas->setTextColor(color, button.BackColor);  // Background colour the text
  _textCanvas->print(msg); 
  button.lastY = _textCanvas->getCursorY()+2-TextH;// top left?
  //NOTE TEXT WRAP uses the last variable in the GFXFont setting, which should be roughly twice the character height.
  //But often seems to be set larger,
  // which gives a text wrap of TWO lines..

  if ((button.lastY + TextH) >= (button.v+typingspaceH-button.bordersize)) {
    button.screenfull = true;
    if (!button.debugpause) {
	  button.lastY = button.v+button.bordersize;
	  DrawBox(button);
      button.screenfull = false;
    }
  }

  setFontByIndex(local);
}

void MarinePageGFX:: UpdateLinef(uint16_t color, int font, _sButton &button, const char *fmt, ...) {  // Types sequential lines in the button space '&' for button to store printline?
  if (button.screenfull && button.debugpause) { return; }
  //DEBUG_PORT.printf(" lines  TypingspaceH =%i  number of lines=%i printing line <%i>\n",typingspaceH,LinesOfType,button.PrintLine);
  static char msg[600] = { '\0' };
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 600, fmt, args);
  va_end(args);
  int len = strlen(msg);
  CommonSub_UpdateLine(false, false, color, font, button, msg);
}
void MarinePageGFX:: UpdateLinef(int font, _sButton &button, const char *fmt, ...) {  // Types sequential lines in the button space '&' for button to store printline?
  if (button.screenfull && button.debugpause) { return; }
  static char msg[500] = { '\0' };
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 500, fmt, args);
  va_end(args);
  int len = strlen(msg);
  CommonSub_UpdateLine(false, false, button.TextColor, font, button, msg);
}

void MarinePageGFX:: UpdateTwoSize_MultiLine(int magnify, bool horizCenter, bool erase, int bigfont, int smallfont, _sButton &button, const char *fmt, ...) {  // TWO font print. separates at decimal point Centers text in space GREYS if data is OLD
  //Serial.print(" UD2S: ");Serial.println(data); this version does not use the border for the height evaluation ! 
  static char msg[300] = { '\0' };
  char digits[30];
  char decimal[30];
  static char *token;
  const char delimiter[2] = ".";  // Or space, etc.
  int16_t x, y, TBx1, TBy1, TBx2, TBy2, TBx3, TBy3;
  uint16_t TBw1, TBh1, TBw2, TBh2, TBw3, TBh3,TextW,TextH;
  int typingspaceH, typingspaceW;
  ////// buttton width and height are for the OVERALL box. subtract border! for sides only as this function may be used in lines..
  typingspaceH = button.height -2;// (2 * button.bordersize);
  typingspaceW = button.width - 2- (2 * button.bordersize);  // small one pixel inset either side
  setFontByIndex(bigfont);  _textCanvas->setTextSize(magnify);//is now set in used in message buildup AND TEXT SIZING 
   //_textCanvas->setTextBound(button.h + button.bordersize+1, button.v + button.bordersize+1, typingspaceW-2, typingspaceH-2);
  _textCanvas->setTextBound(0, 0, 1024, 1024);  // test.. set a full (width) text bound to be certain that the get does not take into account any 'wrap'
  _textCanvas->getTextBounds("9", 0, 0, &TBx1, &TBy1, &TextW, &TextH);  // so that TextH can be simply obtained and used in better h centering
  if (horizCenter ) {
    _textCanvas->setTextWrap(false);
  } else {
    _textCanvas->setTextWrap(true);
  }

  va_list args;  // extract the fmt..
  va_start(args, fmt);
  vsnprintf(msg, 300, fmt, args);
  va_end(args);
  int len = strlen(msg);
  // split msg at the decimal point .. so must have decimal point!
  // if (typingspaceW >=300){
  // Serial.printf("** Debug Msg is <%s> typingspacew=%i \n",msg,typingspaceW);

  if (strcspn(msg, delimiter) != strlen(msg)) {
    token = strtok(msg, delimiter);
    strcpy(digits, token);
    token = strtok(NULL, delimiter);
    strcpy(decimal, delimiter);
    decimal[1] = 0;          // add dp to the decimal delimiter and the critical null so the strcat works !! (not re0uqired now const char delimiter[2] = "."; )
    strcat(decimal, token);  // Concatenate (add) the decimals to the dp..
  } else {
    strcpy(digits, msg);  // missing dp, so just put the whole message in 'digits'.
    decimal[0] = 0;
  }
                                                 // here so the text_offset is correct for bigger font
  x = button.h + button.bordersize + 1;                            //starting point left..
  //  if(button.lastY<=button.v+button.bordersize){DrawBox(button); //USE TO AUTO DRAW BOX FIRST TIME?
  //     button.lastY=  button.v+button.bordersize;}
  y = button.lastY+ TextH;
 //y = button.v + button.bordersize +(magnify * TextH)+ button.PrintLine;  // Printline here will be GFX pixels down inside.. not LINES starting bpoint 'down' allow for magnify !! bigger font for front half and use printline to set start
  //_textCanvas->setTextBound(button.h + button.bordersize+1, button.v + button.bordersize+1, typingspaceW-2, typingspaceH-2);
  _textCanvas->setTextBound(0, 0, 1024, 1024);  // test.. set a full (width) text bound to be certain that the get does not take into account any 'wrap'
  _textCanvas->getTextBounds(digits, 0, 0, &TBx1, &TBy1, &TBw1, &TBh1);  // get text bound for digits use 0,0 for start to ensure we get a usable TBx1 and TBx2 later
   //button.PrintLine=button.PrintLine+(magnify * TextH)+button.bordersize; // do here before text offset gets set for smaller font! 
  setFontByIndex(smallfont);
  _textCanvas->getTextBounds(decimal, 0, 0, &TBx2, &TBy2, &TBw2, &TBh2);    // get text bounds for decimal
                                                                    // if (typingspaceW >=300){
                                                                    //   Serial.printf("digits<%s>:decimal<%s> Total %i tbx1: %i tbx2: %i   TBW1: %i TBW2: %i  ",digits,decimal,TBw1+TBw2,TBx1,TBx2, TBw1, TBw2);
                                                                    //   }
  // if (((TBw1 + TBw2) >= typingspaceW) || (TBh1 >= typingspaceH)) {  // too big!!
  //   if ((TBw1 <= typingspaceW) && (TBh1 <= typingspaceH)) {         //just print digits not decimals
  //     TBw2 = 0;
  //     decimal[0] = 0;
  //     decimal[1] = 0;
  //   } else {  // Serial.print("***DEBUG <"); Serial.print(msg);Serial.print("> became <");Serial.print(digits);
  //             // Serial.print(decimal);Serial.println("> and was too big to print in box");
  //     _textCanvas->setTextBound(0, 0, 480, 480);
  //     return;
  //   }
  // }
  setFontByIndex(bigfont);                                                                                                // Reset to big font for Digits..
  if (horizCenter) { x = button.h + button.bordersize + ((typingspaceW - (TBw1 + TBw2)) / 2); }                    //offset to horizontal center
 // if (vertCenter) { y = button.v + button.bordersize + (magnify * text_offset) + ((typingspaceH - (TBh1)) / 2); }  // vertical centering
  if (erase) {
    _textCanvas->setTextColor(button.BackColor);
  } else {
    _textCanvas->setTextColor(button.TextColor);
  }
  _textCanvas->setTextBound(button.h + button.bordersize, button.v + button.bordersize, typingspaceW, typingspaceH);
  x = x - TBx1;  // NOTE TBx1 is normally zero for most fonts, but some print with offsets that will be corrected by TBx1.
  _textCanvas->setCursor(x, y);
  _textCanvas->print(digits);
  button.lastY = _textCanvas->getCursorY()-TextH;// top left?
  x = _textCanvas->getCursorX();
  if (TBw2 != 0) {
    setFontByIndex(smallfont);
    _textCanvas->setCursor((x - TBx2), y);  // Set decimals start position based on where Digits ended and allow for any font start offset TBx2
    _textCanvas->print(decimal);
  }
   button.lastY = _textCanvas->getCursorY()-TextH;// top left?
  _textCanvas->setTextColor(button.TextColor);
  //MUST reset it for other functions that do not set it themselves!?
  _textCanvas->setTextSize(1);
}

