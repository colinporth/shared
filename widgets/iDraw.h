// iDraw.h -  draw interface
//{{{  includes
#pragma once

#include <string>
#include "iWindow.h"
//}}}

//{{{  uint32_t colour
#define COL_WHITE          0xFFFFFFFF
#define COL_LIGHTGREY      0xFFD3D3D3
#define COL_GREY           0xFF808080
#define COL_DARKGREY       0xFF404040
#define COL_DARKERGREY     0xFF202020
#define COL_BLACK          0xFF000000

#define COL_BLUE           0xFF0000FF
#define COL_LIGHTBLUE      0xFF8080FF
#define COL_DARKBLUE       0xFF000080

#define COL_GREEN          0xFF00FF00
#define COL_LIGHTGREEN     0xFF80FF80
#define COL_DARKGREEN      0xFF008000
#define COL_DARKERGREEN    0xFF004000

#define COL_RED            0xFFFF0000
#define COL_LIGHTRED       0xFFFF8080
#define COL_DARKRED        0xFF800000

#define COL_CYAN           0xFF00FFFF
#define COL_LIGHTCYAN      0xFF80FFFF
#define COL_DARKCYAN       0xFF008080

#define COL_MAGENTA        0xFFFF00FF
#define COL_LIGHTMAGENTA   0xFFFF80FF
#define COL_DARKMAGENTA    0xFF800080

#define COL_YELLOW         0xFFFFFF00
#define COL_LIGHTYELLOW    0xFFFFFF80
#define COL_DARKYELLOW     0xFF808000
#define COL_DARKERYELLOW   0xFF202000

#define COL_BROWN          0xFFA52A2A
#define COL_ORANGE         0xFFFFA500
#define COL_DARKORANGE     0xFFC07800
#define COL_DARKERORANGE   0xFF805000
//}}}

class iDraw : public iWindow {
public:
  virtual ~iDraw() {}

  virtual void drawRect (uint32_t colour, cPoint p, cPoint size) = 0;
  virtual float drawText (uint32_t colour, float fontHeight, std::string str, cPoint p, cPoint size) = 0;
  virtual float drawTextRight (uint32_t colour, float fontHeight, std::string str, cPoint p, cPoint size) = 0;

  virtual void drawPixel (uint32_t colour, cPoint p) { drawRect (colour, p, cPoint(1.f,1.f)); }

  virtual void drawStamp (uint32_t colour, uint8_t* src, cPoint p, cPoint size) {}
  virtual void drawCopy (uint8_t* src, cPoint p, cPoint size) {}
  virtual void drawCopy (uint8_t* src, cPoint srcp, cPoint srcSize, cPoint dstp, cPoint dstSize) {}

  //{{{
  virtual void drawPixelClipped (uint32_t colour, cPoint p) {

    drawRectClipped (colour, p, cPoint(1.f, 1.f));
    }
  //}}}
  //{{{
  virtual void drawStampClipped (uint32_t colour, uint8_t* src, cPoint p, cPoint size) {

    if (!size.x || !size.y || p.x < 0)
      return;

    if (p.y < 0) {
      // top clip
      if (p.y + size.y <= 0)
        return;
      size.y += p.y;
      src += int(-p.y * size.x);
      p.y = 0;
      }

    if (p.y + size.y > getHeightPix()) {
      // bottom yclip
      if (p.y >= getHeightPix())
        return;
      size.y = getHeightPix() - p.y;
      }

    drawStamp (colour, src, p, size);
    }
  //}}}
  //{{{
  virtual void drawRectClipped (uint32_t colour, cPoint p, cPoint size) {

    if (p.x >= getWidthPix())
      return;
    if (p.y >= getHeightPix())
      return;

    cPoint end = p + size;
    if (end.x <= 0)
      return;
    if (end.y <= 0)
      return;

    if (p.x < 0)
      p.x = 0;
    if (end.x > getWidthPix())
      end.x = getWidthPix();

    if (p.y < 0)
      p.y = 0;
    if (end.y > getHeightPix())
      end.y = getHeightPix();

    if (!size.x)
      return;
    if (!size.y)
      return;

    drawRect (colour, p, end - p);
    }
  //}}}
  //{{{
  virtual void drawRectOutline (uint32_t colour, cPoint p, cPoint size, float thickness) {

    drawRectClipped (colour, p, cPoint(size.x, thickness));
    drawRectClipped (colour, p + cPoint (size.x -thickness, 0.f), cPoint(thickness, size.y));
    drawRectClipped (colour, p + cPoint (0.f, size.y -thickness), cPoint(size.x, thickness));
    drawRectClipped (colour, p, cPoint(thickness, size.y));
    }
  //}}}
  //{{{
  virtual void drawClear (uint32_t colour) {

    drawRect (colour, cPoint(0.f, 0.f), cPoint(getWidthPix(), getHeightPix()));
    }
  //}}}

  //{{{
  virtual void drawEllipseSolid (uint32_t colour, cPoint p, cPoint radius) {

    if (!radius.x)
      return;
    if (!radius.y)
      return;

    float x1 = 0.f;
    float y1 = -radius.y;
    float err = 2.f - 2.f*radius.x;
    float k = (float)radius.y / radius.x;

    do {
      drawRectClipped (colour, cPoint(p.x-(x1 / k), p.y + y1), cPoint(2*(x1 / k) + 1, 1));
      drawRectClipped (colour, cPoint(p.x-(x1 / k), p.y - y1), cPoint(2*(x1 / k) + 1, 1));

      float e2 = err;
      if (e2 <= x1) {
        err += ++x1 * 2 + 1;
        if (-y1 == p.x && e2 <= y1)
          e2 = 0;
        }
      if (e2 > y1)
        err += ++y1*2 + 1;
      } while (y1 <= 0);
    }
  //}}}
  //{{{
  virtual void drawEllipseOutline (uint32_t colour, cPoint p, cPoint radius) {

    if (radius.x && radius.y) {
      float x1 = 0;
      float y1 = -radius.y;
      float err = 2 - 2*radius.x;
      float k = (float)radius.y / radius.x;

      do {
        drawRectClipped (colour, cPoint(p.x - (x1 / k), p.y + y1), cPoint(1, 1));
        drawRectClipped (colour, cPoint(p.x + (x1 / k), p.y + y1), cPoint(1, 1));
        drawRectClipped (colour, cPoint(p.x + (x1 / k), p.y - y1), cPoint(1, 1));
        drawRectClipped (colour, cPoint(p.x - (x1 / k), p.y - y1), cPoint(1, 1));

        float e2 = err;
        if (e2 <= x1) {
          err += ++x1*2 + 1;
          if (-y1 == x1 && e2 <= y1)
            e2 = 0;
          }
        if (e2 > y1)
          err += ++y1*2 + 1;
        } while (y1 <= 0);
      }
    }
  //}}}

  //{{{
  virtual void drawLine (uint32_t colour, cPoint p1, cPoint p2) {

    float deltax = (p2.x - p1.x) > 0 ? (p2.x - p1.x) : -(p2.x - p2.x);        /* The difference between the x's */
    float deltay = (p2.y - p1.y) > 0 ? (p2.y - p1.y) : -(p2.y - p1.y);        /* The difference between the y's */
    float x = p1.x;                       /* Start x off at the first pixel */
    float y = p1.y;                       /* Start y off at the first pixel */

    float xinc1;
    float xinc2;
    if (p2.x >= p1.x) {               /* The x-values are increasing */
      xinc1 = 1;
      xinc2 = 1;
      }
    else {                         /* The x-values are decreasing */
      xinc1 = -1;
      xinc2 = -1;
      }

    int yinc1;
    int yinc2;
    if (p2.y >= p1.y) {                 /* The y-values are increasing */
      yinc1 = 1;
      yinc2 = 1;
      }
    else {                         /* The y-values are decreasing */
      yinc1 = -1;
      yinc2 = -1;
      }

    float den = 0;
    float num = 0;
    float num_add = 0;
    float num_pixels = 0;
    if (deltax >= deltay) {        /* There is at least one x-value for every y-value */
      xinc1 = 0;                  /* Don't change the x when numerator >= denominator */
      yinc2 = 0;                  /* Don't change the y for every iteration */
      den = deltax;
      num = deltax / 2;
      num_add = deltay;
      num_pixels = deltax;         /* There are more x-values than y-values */
      }
    else {                         /* There is at least one y-value for every x-value */
      xinc2 = 0;                  /* Don't change the x for every iteration */
      yinc1 = 0;                  /* Don't change the y when numerator >= denominator */
      den = deltay;
      num = deltay / 2;
      num_add = deltax;
      num_pixels = deltay;         /* There are more y-values than x-values */
    }

    for (int curpixel = 0; curpixel <= num_pixels; curpixel++) {
      drawRectClipped (colour, cPoint (x,y), cPoint(1, 1));   /* Draw the current pixel */
      num += num_add;                            /* Increase the numerator by the top of the fraction */
      if (num >= den) {                          /* Check if numerator >= denominator */
        num -= den;                             /* Calculate the new numerator value */
        x += xinc1;                             /* Change the x as appropriate */
        y += yinc1;                             /* Change the y as appropriate */
        }
      x += xinc2;                               /* Change the x as appropriate */
      y += yinc2;                               /* Change the y as appropriate */
      }
    }
  //}}}
  };
