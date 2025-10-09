#ifndef FONT_TYPES_H
#define FONT_TYPES_H

#include "FONTS/fonts.h"               // Now do not have to use reserved directory name src/ for arduino?
#include "FONTS/FreeSansBold6pt7b.h"   //font 7  9 high
#include "FONTS/FreeSansBold8pt7b.h"   //font 8  11 high
#include "FONTS/FreeSansBold12pt7b.h"  //font 9  18 pixels high
#include "FONTS/FreeSansBold18pt7b.h"  //font 10 27 pixels
#include "FONTS/FreeSansBold27pt7b.h"  //font 11 39 pixels
#include "FONTS/FreeSansBold40pt7b.h"  //font 12 59 pixels
#include "FONTS/FreeSansBold60pt7b.h"  //font 13 88 pixels

struct GlyphMetrics {  //not used yet! 
    int16_t offsetX;    // Horizontal offset from cursor to glyph origin
    int16_t offsetY;    // Vertical offset from baseline to glyph origin
    uint16_t width;     // Glyph bitmap width
    uint16_t height;    // Glyph bitmap height
    int16_t advanceX;   // Cursor advance after rendering this glyph
    bool valid;         // True if glyph exists in font
};
enum FontID {
  FONT_MONO_8,
  FONT_MONO_12,
  FONT_MONO_18,
  FONT_MONO_BOLD_8,
  FONT_MONO_BOLD_12,
  FONT_MONO_BOLD_18,
  FONT_MONO_BOLD_27,
  FONT_FreeSansBold6,
  FONT_FreeSansBold8,
  FONT_FreeSansBold12,
  FONT_FreeSansBold18,
  FONT_FreeSansBold27,
  FONT_FreeSansBold40,
  FONT_FreeSansBold60,
  FONT_COUNT // Optional: total number of fonts
};

// Lookup table for GFXfont pointers
static const GFXfont* const fontTable[FONT_COUNT] = {
  &FreeMono8pt7b,
  &FreeMono12pt7b,
  &FreeMono18pt7b,
  &FreeMonoBold8pt7b,
  &FreeMonoBold12pt7b,
  &FreeMonoBold18pt7b,
  &FreeMonoBold27pt7b,
  &FreeSansBold6pt7b,
  &FreeSansBold8pt7b,
  &FreeSansBold12pt7b,
  &FreeSansBold18pt7b,
  &FreeSansBold27pt7b,
  &FreeSansBold40pt7b,
  &FreeSansBold60pt7b,
  
  
};
// Lookup table for font line heights
static const int fontHeightTable[FONT_COUNT] = {
  11, // FONT_MONO_8              0
  17, // FONT_MONO_12             1
  25, // FONT_MONO_18             2
  11, // FONT_MONO_BOLD_8         3
  17, // FONT_MONO_BOLD_12        4
  25, // FONT_MONO_BOLD_18        5
  36, // FONT_MONO_BOLD_27        6
   9, // FONT_FreeSansBold6       7
  11, // FONT_FreeSansBold8       8
  18, // FONT_FreeSansBold12      9
  27, // FONT_FreeSansBold18      10
  39, // FONT_FreeSansBold27      11
  59, // FONT_FreeSansBold40      12
  88  // FONT_FreeSansBold60      13
};
static const char* const fontNameTable[FONT_COUNT] = {
  "FreeMono8pt7b",         // 0
  "FreeMono12pt7b",        // 1
  "FreeMono18pt7b",        // 2
  "FreeMonoBold8pt7b",     // 3
  "FreeMonoBold12pt7b",    // 4
  "FreeMonoBold18pt7b",    // 5
  "FreeMonoBold27pt7b",    // 6
  "FreeSansBold6pt7b",     // 7
  "FreeSansBold8pt7b",     // 8
  "FreeSansBold12pt7b",    // 9
  "FreeSansBold18pt7b",    // 10
  "FreeSansBold27pt7b",    // 11
  "FreeSansBold40pt7b",    // 12
  "FreeSansBold60pt7b"     // 13
};


#endif  // FONT_TYPES_H
