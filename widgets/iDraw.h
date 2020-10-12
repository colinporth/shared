// iDraw.h -  draw interface
//{{{  includes
#pragma once

#include <string>
#include "iWindow.h"

#include "../nanoVg/sColourF.h"
#include "../nanoVg/cPointF.h"
//}}}

class iDraw : public iWindow {
public:
  virtual ~iDraw() {}

  virtual void drawRect (const sColourF& colour, const cPointF& p, const cPointF& size) = 0;
  virtual void drawPixel (const sColourF& colour, const cPointF& p) { drawRect (colour, p, cPointF(1.f,1.f)); }
  //{{{
  virtual void drawRectOutline (const sColourF& colour, const cPointF& p, const cPointF& size, float thickness) {

    drawRect (colour, p, cPointF(size.x, thickness));
    drawRect (colour, p + cPointF (size.x -thickness, 0.f), cPointF(thickness, size.y));
    drawRect (colour, p + cPointF (0.f, size.y -thickness), cPointF(size.x, thickness));
    drawRect (colour, p, cPointF(thickness, size.y));
    }
  //}}}
  //{{{
  virtual void drawClear (const sColourF& colour) {

    drawRect (colour, cPointF(), getPixSize());
    }
  //}}}

  virtual float drawText (const sColourF& colourr, float fontHeight, std::string str, const cPointF& p, const cPointF& size) = 0;
  virtual float drawTextRight (const sColourF& colour, float fontHeight, std::string str, const cPointF& p, const cPointF& size) = 0;

  virtual void drawCopy (uint8_t* src, const cPointF& p, const cPointF& size) {}
  virtual void drawCopy (uint8_t* src, const cPointF& srcp, const cPointF& srcSize, const cPointF& dstp, const cPointF& dstSize) {}
  virtual void drawStamp (const sColourF& colour, uint8_t* src, const cPointF& p, const cPointF& size) {}

  //{{{
  virtual void drawEllipseSolid (const sColourF& colour, const cPointF& p, const cPointF& radius) {

    if (!radius.x)
      return;
    if (!radius.y)
      return;

    float x1 = 0.f;
    float y1 = -radius.y;
    float err = 2.f - 2.f*radius.x;
    float k = (float)radius.y / radius.x;

    do {
      drawRect (colour, cPointF(p.x-(x1 / k), p.y + y1), cPointF(2*(x1 / k) + 1, 1));
      drawRect (colour, cPointF(p.x-(x1 / k), p.y - y1), cPointF(2*(x1 / k) + 1, 1));

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
  virtual void drawEllipseOutline (const sColourF& colour, const cPointF& p, const cPointF& radius) {

    if (radius.x && radius.y) {
      float x1 = 0;
      float y1 = -radius.y;
      float err = 2 - 2*radius.x;
      float k = (float)radius.y / radius.x;

      do {
        drawRect (colour, cPointF(p.x - (x1 / k), p.y + y1), cPointF(1, 1));
        drawRect (colour, cPointF(p.x + (x1 / k), p.y + y1), cPointF(1, 1));
        drawRect (colour, cPointF(p.x + (x1 / k), p.y - y1), cPointF(1, 1));
        drawRect (colour, cPointF(p.x - (x1 / k), p.y - y1), cPointF(1, 1));

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
  virtual void drawLine (const sColourF& colour, const cPointF& p1, const cPointF& p2) {

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
      drawRect (colour, cPointF (x,y), cPointF(1, 1));   /* Draw the current pixel */
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
