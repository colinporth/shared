// iDraw.h -  draw interface
#pragma once
#include <string>

//{{{  colour defines
#define COL_WHITE         0xFFFFFFFF
#define COL_LIGHTGREY     0xFFD3D3D3
#define COL_GREY          0xFF808080
#define COL_DARKGREY      0xFF404040
#define COL_DARKERGREY    0xFF202020
#define COL_BLACK         0xFF000000

#define COL_BLUE          0xFF0000FF
#define COL_LIGHTBLUE     0xFF8080FF
#define COL_DARKBLUE      0xFF000080

#define COL_GREEN         0xFF00FF00
#define COL_LIGHTGREEN    0xFF80FF80
#define COL_DARKGREEN     0xFF008000
#define COL_DARKERGREEN   0xFF004000

#define COL_RED           0xFFFF0000
#define COL_LIGHTRED      0xFFFF8080
#define COL_DARKRED       0xFF800000

#define COL_CYAN          0xFF00FFFF
#define COL_LIGHTCYAN     0xFF80FFFF
#define COL_DARKCYAN      0xFF008080

#define COL_MAGENTA       0xFFFF00FF
#define COL_LIGHTMAGENTA  0xFFFF80FF
#define COL_DARKMAGENTA   0xFF800080

#define COL_YELLOW        0xFFFFFF00
#define COL_LIGHTYELLOW   0xFFFFFF80
#define COL_DARKYELLOW    0xFF808000
#define COL_DARKERYELLOW  0xFF202000

#define COL_BROWN         0xFFA52A2A
#define COL_ORANGE        0xFFFFA500
#define COL_DARKORANGE    0xFFC07800
#define COL_DARKERORANGE  0xFF805000
//}}}
#ifdef USE_NANOVG
  //{{{  nanoVG colours
  const sVgColour kWhite      = { 1.0f, 1.0f, 1.0f, 1.0f };
  const sVgColour kGrey       = { 0.5f, 0.5f, 0.5f, 1.0f };
  const sVgColour kDarkGrey   = { 0.25f, 0.25f, 0.25f, 1.0f };
  const sVgColour kDarkerGrey = { 0.125f, 0.125f, 0.125f, 1.0f };
  const sVgColour kBlack      = { 0.0f, 0.0f, 0.0f, 1.0f };
  const sVgColour kYellow     = { 1.0f, 1.0f, 0.0f, 1.0f };
  //}}}
#endif

class iDraw {
public:
  virtual ~iDraw() {}

  virtual uint16_t getLcdWidthPix() = 0;
  virtual uint16_t getLcdHeightPix() = 0;

  virtual void pixel (uint32_t colour, int16_t x, int16_t y) = 0;
  virtual void drawRect (uint32_t colour, int16_t x, int16_t y, uint16_t width, uint16_t height) = 0;
  virtual int drawText (uint32_t colour, uint16_t fontHeight, std::string str, int16_t x, int16_t y, uint16_t width, uint16_t height) = 0;

  virtual void stamp (uint32_t colour, uint8_t* src, int16_t x, int16_t y, uint16_t width, uint16_t height) {}
  virtual void copy (uint8_t* src, int16_t x, int16_t y, uint16_t width, uint16_t height) {}
  virtual void copy (uint8_t* src, int16_t srcx, int16_t srcy, uint16_t srcWidth, uint16_t srcHeight,
                     int16_t dstx, int16_t dsty, uint16_t dstWidth, uint16_t dstHeight) {}

  //{{{
  virtual void pixelClipped (uint32_t colour, int16_t x, int16_t y) {

    rectClipped (colour, x, y, 1, 1);
    }
  //}}}
  //{{{
  virtual void stampClipped (uint32_t colour, uint8_t* src, int16_t x, int16_t y, uint16_t width, uint16_t height) {

    if (!width || !height || x < 0)
      return;

    if (y < 0) {
      // top clip
      if (y + height <= 0)
        return;
      height += y;
      src += -y * width;
      y = 0;
      }

    if (y + height > getLcdHeightPix()) {
      // bottom yclip
      if (y >= getLcdHeightPix())
        return;
      height = getLcdHeightPix() - y;
      }

    stamp (colour, src, x, y, width, height);
    }
  //}}}
  //{{{
  virtual void rectClipped (uint32_t colour, int16_t x, int16_t y, uint16_t width, uint16_t height) {

    if (x >= getLcdWidthPix())
      return;
    if (y >= getLcdHeightPix())
      return;

    int xend = x + width;
    if (xend <= 0)
      return;

    int yend = y + height;
    if (yend <= 0)
      return;

    if (x < 0)
      x = 0;
    if (xend > getLcdWidthPix())
      xend = getLcdWidthPix();

    if (y < 0)
      y = 0;
    if (yend > getLcdHeightPix())
      yend = getLcdHeightPix();

    if (!width)
      return;
    if (!height)
      return;

    drawRect (colour, x, y, xend - x, yend - y);
    }
  //}}}
  //{{{
  virtual void rectOutline (uint32_t colour, int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t thickness) {

    rectClipped (colour, x, y, width, thickness);
    rectClipped (colour, x + width-thickness, y, thickness, height);
    rectClipped (colour, x, y + height-thickness, width, thickness);
    rectClipped (colour, x, y, thickness, height);
    }
  //}}}
  //{{{
  virtual void clear (uint32_t colour) {

    drawRect (colour, 0, 0, getLcdWidthPix(), getLcdHeightPix());
    }
  //}}}

  //{{{
  virtual void ellipseSolid (uint32_t colour, int16_t x, int16_t y, uint16_t xradius, uint16_t yradius) {

    if (!xradius)
      return;
    if (!yradius)
      return;

    int x1 = 0;
    int y1 = -yradius;
    int err = 2 - 2*xradius;
    float k = (float)yradius / xradius;

    do {
      rectClipped (colour, (x-(uint16_t)(x1 / k)), y + y1, (2*(uint16_t)(x1 / k) + 1), 1);
      rectClipped (colour, (x-(uint16_t)(x1 / k)), y - y1, (2*(uint16_t)(x1 / k) + 1), 1);

      int e2 = err;
      if (e2 <= x1) {
        err += ++x1 * 2 + 1;
        if (-y1 == x && e2 <= y1)
          e2 = 0;
        }
      if (e2 > y1)
        err += ++y1*2 + 1;
      } while (y1 <= 0);
    }
  //}}}
  //{{{
  virtual void ellipseOutline (uint32_t colour, int16_t x, int16_t y, uint16_t xradius, uint16_t yradius) {

    if (xradius && yradius) {
      int x1 = 0;
      int y1 = -yradius;
      int err = 2 - 2*xradius;
      float k = (float)yradius / xradius;

      do {
        rectClipped (colour, x - (uint16_t)(x1 / k), y + y1, 1, 1);
        rectClipped (colour, x + (uint16_t)(x1 / k), y + y1, 1, 1);
        rectClipped (colour, x + (uint16_t)(x1 / k), y - y1, 1, 1);
        rectClipped (colour, x - (uint16_t)(x1 / k), y - y1, 1, 1);

        int e2 = err;
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
  virtual void line (uint32_t colour, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {

    int16_t deltax = (x2 - x1) > 0 ? (x2 - x1) : -(x2 - x1);        /* The difference between the x's */
    int16_t deltay = (y2 - y1) > 0 ? (y2 - y1) : -(y2 - y1);        /* The difference between the y's */
    int16_t x = x1;                       /* Start x off at the first pixel */
    int16_t y = y1;                       /* Start y off at the first pixel */

    int16_t xinc1;
    int16_t xinc2;
    if (x2 >= x1) {               /* The x-values are increasing */
      xinc1 = 1;
      xinc2 = 1;
      }
    else {                         /* The x-values are decreasing */
      xinc1 = -1;
      xinc2 = -1;
      }

    int yinc1;
    int yinc2;
    if (y2 >= y1) {                 /* The y-values are increasing */
      yinc1 = 1;
      yinc2 = 1;
      }
    else {                         /* The y-values are decreasing */
      yinc1 = -1;
      yinc2 = -1;
      }

    int den = 0;
    int num = 0;
    int num_add = 0;
    int num_pixels = 0;
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
      rectClipped (colour, x, y, 1, 1);   /* Draw the current pixel */
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

  #ifdef USE_NANOVG
    virtual cVg* getContext() = 0;
  #endif
  };
