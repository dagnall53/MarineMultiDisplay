/*******************************************************************************
 * JPEGDEC related function
 *
 * Dependent libraries:
 * JPEGDEC: https://github.com/bitbank2/JPEGDEC.git
 ******************************************************************************/
#ifndef _JPEGFUNC_H_
#define _JPEGFUNC_H_

#include <JPEGDEC.h>
#include "aux_functions.h"
#include <SPIFFS.h>
#include "src/MarinePageGFX.h"  // or wherever GFX is defined

// help me find this again with maruinegfx open clearTextCanvas

//static JPEGDEC _jpeg;
//static File _f;

// static int _x, _y, _x_bound, _y_bound;

// static Arduino_GFX* _jpegTargetCanvas = nullptr; //This will hold the canvas you want to draw to—whe

// static void setJPEGTargetCanvas(Arduino_Canvas* canvas) {
//     _jpegTargetCanvas = canvas;
// }


// // new pixel drawing callback
// static int jpegDrawCallback(JPEGDRAW *pDraw)
// {
//     if (!_jpegTargetCanvas) return 0;

//     _jpegTargetCanvas->draw16bitBeRGBBitmap(
//         pDraw->x, pDraw->y,
//         pDraw->pPixels,
//         pDraw->iWidth, pDraw->iHeight
//     );
//     return 1;
// }



// // static int jpegDrawCallback(JPEGDRAW *pDraw)
// // {
// //   // DEBUG_PORT.printf("Draw pos = %d,%d. size = %d x %d\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
// //   gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
// //   return 1;
// // }

// static void *jpegOpenFile(const char *szFilename, int32_t *pFileSize)
//  {
//     // DEBUG_PORT.println("jpegOpenFile");

//  #if defined(ESP32)
//     // _f = FFat.open(szFilename, "r");
//     //_f = LittleFS.open(szFilename, "r");
//     // _f = SPIFFS.open(szFilename, "r");
//     //SD_CS(LOW);
//     // _f = SD.open(szFilename, "r");
//  #else
//     //_f = SD.open(szFilename, FILE_READ);
//  #endif
//    _f = SPIFFS.open(szFilename, "r");
//     *pFileSize = _f.size();
//     return &_f;
// }

// static void jpegCloseFile(void *pHandle)
// {
//     // DEBUG_PORT.println("jpegCloseFile");
//     File *f = static_cast<File *>(pHandle);
//     f->close();

// }

// static int32_t jpegReadFile(JPEGFILE *pFile, uint8_t *pBuf, int32_t iLen)
// {
//     // DEBUG_PORT.printf("jpegReadFile, iLen: %d\n", iLen);
//     File *f = static_cast<File *>(pFile->fHandle);
//     size_t r = f->read(pBuf, iLen);
//     return r;
// }

// static int32_t jpegSeekFile(JPEGFILE *pFile, int32_t iPosition)
// {
//     // DEBUG_PORT.printf("jpegSeekFile, pFile->iPos: %d, iPosition: %d\n", pFile->iPos, iPosition);
//     File *f = static_cast<File *>(pFile->fHandle);
//     f->seek(iPosition);
//     return iPosition;
// }

// static void jpegDraw(
//     const char *filename, JPEG_DRAW_CALLBACK *jpegDrawCallback, bool useBigEndian,
//     int x, int y, int widthLimit, int heightLimit)
// {
//     _x = x;
//     _y = y;
//     _x_bound = _x + widthLimit - 1;
//     _y_bound = _y + heightLimit - 1;

//     _jpeg.open(filename, jpegOpenFile, jpegCloseFile, jpegReadFile, jpegSeekFile, jpegDrawCallback);

//     // scale to fit height
//     int _scale;
//     int iMaxMCUs;
//     float ratio = (float)_jpeg.getHeight() / heightLimit;
//     if (ratio <= 1)
//     {
//         _scale = 0;
//         iMaxMCUs = widthLimit / 16;
//     }
//     else if (ratio <= 2)
//     {
//         _scale = JPEG_SCALE_HALF;
//         iMaxMCUs = widthLimit / 8;
//     }
//     else if (ratio <= 4)
//     {
//         _scale = JPEG_SCALE_QUARTER;
//         iMaxMCUs = widthLimit / 4;
//     }
//     else
//     {
//         _scale = JPEG_SCALE_EIGHTH;
//         iMaxMCUs = widthLimit / 2;
//     }
//     _jpeg.setMaxOutputSize(iMaxMCUs);
//     if (useBigEndian)
//     {
//         _jpeg.setPixelType(RGB565_BIG_ENDIAN);
//     }
//     _jpeg.decode(x, y, _scale);
//     _jpeg.close();
// }

// void showPicture(const char* name) {
//   jpegDraw(name, jpegDrawCallback, true /* useBigEndian */,
//            0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
// }
#endif // _JPEGFUNC_H_