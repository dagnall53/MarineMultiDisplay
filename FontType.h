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
  &FreeSansBold27pt7b,
  &FreeSansBold40pt7b,
  &FreeSansBold60pt7b
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



#endif  // FONT_TYPES_H
