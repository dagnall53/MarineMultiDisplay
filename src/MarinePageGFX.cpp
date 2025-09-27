#include "MarinePageGFX.h"
#include <stdarg.h>
#include <math.h>

#define DEG_TO_RAD 0.0174532925f

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
void MarinePageGFX::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  if (!isReady()) return;

  int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int16_t err = dx + dy, e2;

  while (true) {
    drawPixel(x0, y0, color);
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

void MarinePageGFX::setTextColor(uint16_t color) {
  _textColor = color;
}

void MarinePageGFX::setTextSize(uint8_t size) {
  _textSize = size;
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

void MarinePageGFX::drawCompassPointer(int16_t centerX, int16_t centerY, int16_t radius, int16_t tailLength, float angleDeg, uint16_t color, bool shadow) {
  if (!isReady()) return;

  float angleRad = angleDeg * DEG_TO_RAD;
  float perpRad = angleRad + PI / 2;

  int16_t tipX = centerX + cos(angleRad) * radius;
  int16_t tipY = centerY + sin(angleRad) * radius;

  //int16_t tailLength = radius / 2;
  int16_t tailX = centerX - cos(angleRad) * tailLength;
  int16_t tailY = centerY - sin(angleRad) * tailLength;

  int16_t baseWidth = 12;
  int16_t baseX1 = centerX + cos(perpRad) * (baseWidth / 2);
  int16_t baseY1 = centerY + sin(perpRad) * (baseWidth / 2);
  int16_t baseX2 = centerX - cos(perpRad) * (baseWidth / 2);
  int16_t baseY2 = centerY - sin(perpRad) * (baseWidth / 2);

  if (shadow) {
    int16_t offset = 2;
    fillTriangle(tipX + offset, tipY + offset,
                 baseX1 + offset, baseY1 + offset,
                 baseX2 + offset, baseY2 + offset,
                 NEAR_BLACK);

    fillTriangle(tailX + offset, tailY + offset,
                 baseX2 + offset, baseY2 + offset,
                 baseX1 + offset, baseY1 + offset,
                 NEAR_BLACK);
  }

  fillTriangle(tipX, tipY, baseX1, baseY1, baseX2, baseY2, color);
  fillTriangle(tailX, tailY, baseX2, baseY2, baseX1, baseY1, color);
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

  // Composite canvas into active buffer
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
