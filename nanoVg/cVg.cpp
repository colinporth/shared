// cVg.cpp - based on Mikko Mononen memon@inside.org nanoVg
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include "cVg.h"

#include "fontStash.h"
#include "stb_image.h"

using namespace std;
//}}}
//{{{  defines
#define KAPPA90 0.5522847493f // Length proportional to radius of a cubic bezier handle for 90deg arcs.

#define MAX_FONTIMAGE_SIZE  2048
//}}}
const float kDistanceTolerance = 0.01f;
//{{{  maths
//{{{
float normalize (float& x, float& y) {

  float d = sqrtf(x*x + y*y);
  if (d > 1e-6f) {
    float id = 1.0f / d;
    x *= id;
    y *= id;
    }

  return d;
  }
//}}}

//{{{
int pointEquals (float x1, float y1, float x2, float y2, float tol) {
  float dx = x2 - x1;
  float dy = y2 - y1;
  return dx*dx + dy*dy < tol*tol;
  }
//}}}
//{{{
float distPointSeg (float x, float y, float px, float py, float qx, float qy) {

  float pqx = qx-px;
  float pqy = qy-py;
  float dx = x-px;
  float dy = y-py;

  float d = pqx*pqx + pqy*pqy;
  float t = pqx*dx + pqy*dy;
  if (d > 0)
    t /= d;
  if (t < 0)
    t = 0;
  else if (t > 1)
    t = 1;

  dx = px + t*pqx - x;
  dy = py + t*pqy - y;

  return dx*dx + dy*dy;
  }
//}}}
//{{{
void intersectRects (float* dst, float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {

  float minx = maxf (ax, bx);
  float miny = maxf (ay, by);
  float maxx = minf (ax+aw, bx+bw);
  float maxy = minf (ay+ah, by+bh);

  dst[0] = minx;
  dst[1] = miny;
  dst[2] = maxf (0.0f, maxx - minx);
  dst[3] = maxf (0.0f, maxy - miny);
  }
//}}}

float quantize (float a, float d) { return ((int)(a / d + 0.5f)) * d; }
//{{{
unsigned int nearestPow2 (unsigned int num) {

  unsigned n = num > 0 ? num - 1 : 0;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n++;

  return n;
  }
//}}}

//{{{
float hue (float h, float m1, float m2) {

  if (h < 0)
    h += 1;
  if (h > 1)
    h -= 1;

  if (h < 1.0f/6.0f)
    return m1 + (m2 - m1) * h * 6.0f;
  else if (h < 3.0f/6.0f)
    return m2;
  else if (h < 4.0f/6.0f)
    return m1 + (m2 - m1) * (2.0f/3.0f - h) * 6.0f;

  return m1;
  }
//}}}

float degToRad (float deg) { return deg / 180.0f * PI; }
float radToDeg (float rad) { return rad / PI * 180.0f; }
//}}}
//{{{  NVGcolour
//{{{
sVgColour nvgRGB (unsigned char r, unsigned char g, unsigned char b) {
  return nvgRGBA (r,g,b,255);
  }
//}}}
//{{{
sVgColour nvgRGBf (float r, float g, float b) {
  return nvgRGBAf (r,g,b,1.0f);
  }
//}}}
//{{{
sVgColour nvgRGBA (unsigned char r, unsigned char g, unsigned char b, unsigned char a) {

  sVgColour color;
  color.r = r / 255.0f;
  color.g = g / 255.0f;
  color.b = b / 255.0f;
  color.a = a / 255.0f;
  return color;
  }
//}}}
//{{{
sVgColour nvgRGBAf (float r, float g, float b, float a) {
  sVgColour color;
  color.r = r;
  color.g = g;
  color.b = b;
  color.a = a;
  return color;
  }
//}}}
//{{{
sVgColour nvgRGB32 (uint32_t colour) {
  sVgColour color;
  color.r = ((colour & 0xFF0000) >> 16) / 255.0f;
  color.g = ((colour & 0xFF00) >> 8) / 255.0f;
  color.b = (colour & 0xFF) / 255.0f;
  color.a = (colour >> 24) / 255.0f;
  return color;
  }
//}}}

//{{{
sVgColour nvgTransRGBA (sVgColour c, unsigned char a) {
  c.a = a / 255.0f;
  return c;
  }
//}}}
//{{{
sVgColour nvgTransRGBAf (sVgColour c, float a) {
  c.a = a;
  return c;
  }
//}}}
//{{{
sVgColour nvgLerpRGBA (sVgColour c0, sVgColour c1, float u) {

  sVgColour cint = {{{0}}};
  u = clampf(u, 0.0f, 1.0f);
  float oneminu = 1.0f - u;
  for (int i = 0; i <4; i++ )
    cint.rgba[i] = c0.rgba[i] * oneminu + c1.rgba[i] * u;
  return cint;
  }
//}}}

//{{{
sVgColour nvgHSL (float h, float s, float l) {
  return nvgHSLA (h,s,l,255);
  }
//}}}
//{{{
sVgColour nvgHSLA (float h, float s, float l, unsigned char a) {

  h = fmodf (h, 1.0f);
  if (h < 0.0f)
    h += 1.0f;
  s = clampf (s, 0.0f, 1.0f);
  l = clampf (l, 0.0f, 1.0f);

  float m2 = l <= 0.5f ? (l * (1 + s)) : (l + s - l * s);
  float m1 = 2 * l - m2;

  sVgColour col;
  col.r = clampf (hue (h + 1.0f/3.0f, m1, m2), 0.0f, 1.0f);
  col.g = clampf (hue (h, m1, m2), 0.0f, 1.0f);
  col.b = clampf (hue (h - 1.0f/3.0f, m1, m2), 0.0f, 1.0f);
  col.a = a / 255.0f;
  return col;
  }
//}}}

//{{{
sVgColour nvgPremulColor (sVgColour c) {
  c.r *= c.a;
  c.g *= c.a;
  c.b *= c.a;
  return c;
  }
//}}}
//}}}

//{{{
cVg::cVg (int flags) :
    mDrawEdges(!(flags & DRAW_NOEDGES)), mDrawSolid(!(flags & DRAW_NOSOLID)), mDrawTriangles(true) {

  saveState();
  resetState();

  setDevicePixelRatio (1.f);

  // init font rendering
  FONSparams fontParams;
  memset (&fontParams, 0, sizeof(fontParams));

  fontParams.width = NVG_INIT_FONTIMAGE_SIZE;
  fontParams.height = NVG_INIT_FONTIMAGE_SIZE;
  fontParams.flags = FONS_ZERO_TOPLEFT;

  fontParams.renderCreate = nullptr;
  fontParams.renderUpdate = nullptr;
  fontParams.renderDraw = nullptr;
  fontParams.renderDelete = nullptr;
  fontParams.userPtr = nullptr;

  fonsContext = fonsCreateInternal (&fontParams);
  }
//}}}
//{{{
cVg::~cVg() {

  if (fonsContext)
    fonsDeleteInternal (fonsContext);
  }
//}}}
//{{{
void cVg::initialise() {
  fontImages[0] = renderCreateTexture (TEXTURE_ALPHA,
                                       NVG_INIT_FONTIMAGE_SIZE, NVG_INIT_FONTIMAGE_SIZE, 0, NULL);
  }
//}}}

//{{{  state
//{{{
void cVg::resetState() {

  auto state = &mStates[mNumStates-1];

  state->compositeOp = compositeOpState (NVG_SOURCE_OVER);
  state->fillPaint.setColor (nvgRGBA (255, 255, 255, 255));
  state->strokePaint.setColor (nvgRGBA (0, 0, 0, 255));

  state->strokeWidth = 1.0f;
  state->miterLimit = 10.0f;
  state->lineCap = eBUTT;
  state->lineJoin = eMITER;
  state->alpha = 1.0f;

  state->mTransform.setIdentity();

  state->scissor.extent[0] = -1.0f;
  state->scissor.extent[1] = -1.0f;

  state->fontSize = 16.0f;
  state->letterSpacing = 0.0f;
  state->lineHeight = 1.0f;
  state->textAlign = ALIGN_LEFT | ALIGN_BASELINE;
  state->fontId = 0;
  }
//}}}

//{{{
void cVg::saveState() {
  if (mNumStates >= MAX_STATES)
    return;
  if (mNumStates > 0)
    memcpy (&mStates[mNumStates], &mStates[mNumStates-1], sizeof (cState));
  mNumStates++;
  }
//}}}
//{{{
void cVg::restoreState() {
  if (mNumStates <= 1)
    return;
  mNumStates--;
  }
//}}}
//}}}
//{{{  transform
//{{{
void cVg::resetTransform() {
  mStates[mNumStates - 1].mTransform.setIdentity();
  }
//}}}
//{{{
void cVg::transform (float a, float b, float c, float d, float e, float f) {
  cTransform transform (a, b, c, d, e, f);
  mStates[mNumStates - 1].mTransform.premultiply (transform);
  }
//}}}

//{{{
void cVg::translate (float x, float y) {
  cTransform transform;
  transform.setTranslate (x, y);
  mStates[mNumStates - 1].mTransform.premultiply (transform);
  }
//}}}
//{{{
void cVg::scale (float x, float y) {
  cTransform transform;
  transform.setScale (x, y);
  mStates[mNumStates - 1].mTransform.premultiply (transform);
  }
//}}}
//{{{
void cVg::rotate (float angle) {
  cTransform transform;
  transform.setRotate (angle);
  mStates[mNumStates - 1].mTransform.premultiply (transform);
  }
//}}}
//}}}
//{{{  shape

void cVg::fillColor (sVgColour color) { mStates[mNumStates-1].fillPaint.setColor (color); }
void cVg::strokeColor (sVgColour color) { mStates[mNumStates-1].strokePaint.setColor (color); }
void cVg::globalAlpha (float alpha) { mStates[mNumStates-1].alpha = alpha; }

void cVg::strokeWidth (float width) { mStates[mNumStates-1].strokeWidth = width; }
//{{{
void cVg::fringeWidth (float width) {

  mFringeWidth = width;
  if (mFringeWidth < 0.0f)
    mFringeWidth = 0.0f;
  else if (mFringeWidth > 10.0f)
   mFringeWidth = 10.0f;
  }
//}}}

void cVg::miterLimit (float limit) { mStates[mNumStates-1].miterLimit = limit; }
void cVg::lineJoin (eLineCap join) { mStates[mNumStates-1].lineJoin = join; }
void cVg::lineCap (eLineCap cap) { mStates[mNumStates-1].lineCap = cap; }

//{{{
cVg::cPaint cVg::linearGradient (float sx, float sy, float ex, float ey, sVgColour icol, sVgColour ocol) {

  // Calculate transform aligned to the line
  float dx = ex - sx;
  float dy = ey - sy;
  float d = sqrtf (dx * dx + dy * dy);
  if (d > 0.0001f) {
    dx /= d;
    dy /= d;
    }
  else {
    dx = 0;
    dy = 1;
    }

  cPaint p;
  memset (&p, 0, sizeof(p));

  const float large = 1e5;
  p.mTransform.set (dy, -dx, dx, dy, sx - dx * large, sy - dy * large);
  p.extent[0] = large;
  p.extent[1] = large + d * 0.5f;
  p.radius = 0.0f;
  p.feather = maxf (1.0f, d);
  p.innerColor = icol;
  p.outerColor = ocol;

  return p;
  }
//}}}
//{{{
cVg::cPaint cVg::radialGradient (float cx, float cy, float inr, float outr, sVgColour icol, sVgColour ocol) {

  float r = (inr + outr) * 0.5f;
  float f = (outr - inr);

  cPaint p;
  memset (&p, 0, sizeof (p));

  p.mTransform.setTranslate (cx, cy);

  p.extent[0] = r;
  p.extent[1] = r;

  p.radius = r;
  p.feather = maxf (1.0f, f);
  p.innerColor = icol;
  p.outerColor = ocol;

  return p;
  }
//}}}
//{{{
cVg::cPaint cVg::boxGradient (float x, float y, float w, float h, float r, float f, sVgColour icol, sVgColour ocol) {

  cPaint p;
  memset (&p, 0, sizeof (p));

  p.mTransform.setTranslate (x + w * 0.5f, y + h * 0.5f);

  p.radius = r;
  p.feather = maxf (1.0f, f);
  p.innerColor = icol;
  p.outerColor = ocol;

  return p;
  }
//}}}
//{{{
cVg::cPaint cVg::imagePattern (float cx, float cy, float w, float h, float angle, int image, float alpha) {

  cPaint p;
  memset (&p, 0, sizeof(p));

  p.mTransform.setRotateTranslate (angle, cx, cy);
  p.extent[0] = w;
  p.extent[1] = h;
  p.image = image;
  p.innerColor = p.outerColor = nvgRGBAf (1, 1 , 1, alpha);

  return p;
  }
//}}}

//{{{
void cVg::fillPaint (cPaint paint) {
  mStates[mNumStates-1].fillPaint = paint;
  mStates[mNumStates - 1].fillPaint.mTransform.multiply (mStates[mNumStates-1].mTransform);
  }
//}}}
//{{{
void cVg::strokePaint (cPaint paint) {
  mStates[mNumStates-1].strokePaint = paint;
  mStates[mNumStates - 1].strokePaint.mTransform.multiply (mStates[mNumStates-1].mTransform);
  }
//}}}

//{{{
void cVg::beginPath() {
  mShape.beginPath();
  }
//}}}
//{{{
void cVg::pathWinding (eWinding dir) {
  float values[] = { eWINDING, (float)dir };
  mShape.addCommand (values, 2, mStates[mNumStates-1].mTransform);
  }
//}}}
//{{{
void cVg::moveTo (float x, float y) {
  float values[] = { eMOVETO, x, y };
  mShape.addCommand (values, 3, mStates[mNumStates-1].mTransform);
  }
//}}}
//{{{
void cVg::lineTo (float x, float y) {
  float values[] = { eLINETO, x, y };
  mShape.addCommand (values, 3, mStates[mNumStates-1].mTransform);
  }
//}}}
//{{{
void cVg::bezierTo (float c1x, float c1y, float c2x, float c2y, float x, float y) {
  float values[] = { eBEZIERTO,
                     c1x, c1y,
                     c2x, c2y,
                       x,   y };
  mShape.addCommand (values, 7, mStates[mNumStates-1].mTransform);
  }
//}}}
//{{{
void cVg::quadTo (float cx, float cy, float x, float y) {

  float x0 = mShape.getLastX();
  float y0 = mShape.getLastY();

  float values[] = { eBEZIERTO,
                     x0 + 2.0f / 3.0f * (cx - x0), y0 + 2.0f / 3.0f * (cy - y0),
                       x + 2.0f / 3.0f * (cx - x),   y + 2.0f / 3.0f * (cy - y),
                     x, y };
  mShape.addCommand (values, 7, mStates[mNumStates-1].mTransform);
  }
//}}}
//{{{
void cVg::arcTo (float x1, float y1, float x2, float y2, float radius) {

  float x0 = mShape.getLastX();
  float y0 = mShape.getLastY();
  float dx0,dy0, dx1,dy1, a, d, cx,cy, a0,a1;
  int dir;

  if (!mShape.getNumCommands())
    return;

  // Handle degenerate cases.
  if (pointEquals (x0,y0, x1,y1, kDistanceTolerance) ||
      pointEquals (x1,y1, x2,y2, kDistanceTolerance) ||
      distPointSeg (x1,y1, x0,y0, x2,y2) < kDistanceTolerance*kDistanceTolerance ||
      radius < kDistanceTolerance) {
    lineTo (x1,y1);
    return;
    }

  // Calculate tangential circle to lines (x0,y0)-(x1,y1) and (x1,y1)-(x2,y2).
  dx0 = x0 - x1;
  dy0 = y0 - y1;
  dx1 = x2 - x1;
  dy1 = y2 - y1;
  normalize (dx0, dy0);
  normalize (dx1, dy1);
  a = acosf (dx0 * dx1 + dy0 * dy1);
  d = radius / tanf (a / 2.0f);

  if (d > 10000.0f) {
    lineTo (x1, y1);
    return;
    }

  if (cross (dx0,dy0, dx1,dy1) > 0.0f) {
    cx = x1 + dx0 * d + dy0 * radius;
    cy = y1 + dy0 * d - dx0 * radius;
    a0 = atan2f (dx0, -dy0);
    a1 = atan2f (-dx1, dy1);
    dir = eHOLE;
    }
  else {
    cx = x1 + dx0 * d - dy0 * radius;
    cy = y1 + dy0 * d + dx0 * radius;
    a0 = atan2f (-dx0, dy0);
    a1 = atan2f (dx1, -dy1);
    dir = eSOLID;
    }

  arc (cx, cy, radius, a0, a1, dir);
  }
//}}}
//{{{
void cVg::arc (float cx, float cy, float r, float a0, float a1, int dir) {

  float a = 0, da = 0, hda = 0, kappa = 0;
  float dx = 0, dy = 0, x = 0, y = 0, tanx = 0, tany = 0;
  float px = 0, py = 0, ptanx = 0, ptany = 0;
  float values[3 + 5*7 + 100];

  int i, ndivs, numValues;
  int move = mShape.getNumCommands() ? eLINETO : eMOVETO;

  // Clamp angles
  da = a1 - a0;
  if (dir == eHOLE) {
    if (absf(da) >= PI*2)
      da = PI*2;
    else
      while (da < 0.0f) da += PI*2;
    }
  else {
    if (absf(da) >= PI*2)
      da = -PI*2;
    else
      while (da > 0.0f) da -= PI*2;
    }

  // Split arc into max 90 degree segments.
  ndivs = maxi (1, mini((int)(absf(da) / (PI*0.5f) + 0.5f), 5));
  hda = (da / (float)ndivs) / 2.0f;
  kappa = absf (4.0f / 3.0f * (1.0f - cosf (hda)) / sinf (hda));

  if (dir == eSOLID)
    kappa = -kappa;

  numValues = 0;
  for (i = 0; i <= ndivs; i++) {
    a = a0 + da * (i / (float)ndivs);
    dx = cosf (a);
    dy = sinf (a);
    x = cx + dx * r;
    y = cy + dy * r;
    tanx = -dy * r * kappa;
    tany = dx * r * kappa;

    if (i == 0) {
      values[numValues++] = (float)move;
      values[numValues++] = x;
      values[numValues++] = y;
      }
    else {
      values[numValues++] = eBEZIERTO;
      values[numValues++] = px + ptanx;
      values[numValues++] = py + ptany;
      values[numValues++] = x - tanx;
      values[numValues++] = y - tany;
      values[numValues++] = x;
      values[numValues++] = y;
      }

    px = x;
    py = y;
    ptanx = tanx;
    ptany = tany;
    }
  mShape.addCommand (values, numValues, mStates[mNumStates-1].mTransform);
  }
//}}}
//{{{
void cVg::rect (float x, float y, float w, float h) {

  float values[] = { eMOVETO,   x,   y,
                     eLINETO,   x, y+h,
                     eLINETO, x+w, y+h,
                     eLINETO, x+w,   y,
                     eCLOSE };
  mShape.addCommand (values, 13, mStates[mNumStates-1].mTransform);
  }
//}}}
//{{{
void cVg::roundedRectVarying (float x, float y, float w, float h,
                              float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft) {

  if ((radTopLeft < 0.1f) && (radTopRight < 0.1f) && (radBottomRight < 0.1f) && (radBottomLeft < 0.1f))
    rect (x, y, w, h);
  else {
    float halfw = absf (w) * 0.5f;
    float halfh = absf (h) * 0.5f;

    float rxBL = minf (radBottomLeft, halfw) * signf (w);
    float ryBL = minf(radBottomLeft, halfh) * signf(h);
    float rxBR = minf (radBottomRight, halfw) * signf (w);
    float ryBR = minf(radBottomRight, halfh) * signf(h);
    float rxTR = minf (radTopRight, halfw) * signf (w);
    float ryTR = minf(radTopRight, halfh) * signf(h);
    float rxTL = minf (radTopLeft, halfw) * signf (w);
    float ryTL = minf(radTopLeft, halfh) * signf(h);

    float values[] = {
      eMOVETO, x, y + ryTL,
      eLINETO, x, y + h - ryBL,
      eBEZIERTO, x, y + h - ryBL*(1 - KAPPA90), x + rxBL*(1 - KAPPA90), y + h, x + rxBL, y + h,
      eLINETO, x + w - rxBR, y + h,
      eBEZIERTO, x + w - rxBR*(1 - KAPPA90), y + h, x + w, y + h - ryBR*(1 - KAPPA90), x + w, y + h - ryBR,
      eLINETO, x + w, y + ryTR,
      eBEZIERTO, x + w, y + ryTR*(1 - KAPPA90), x + w - rxTR*(1 - KAPPA90), y, x + w - rxTR, y,
      eLINETO, x + rxTL, y,
      eBEZIERTO, x + rxTL*(1 - KAPPA90), y, x, y + ryTL*(1 - KAPPA90), x, y + ryTL,
      eCLOSE
      };
    mShape.addCommand (values, 44, mStates[mNumStates-1].mTransform);
    }
  }
//}}}
//{{{
void cVg::roundedRect (float x, float y, float w, float h, float r) {
  roundedRectVarying (x, y, w, h, r, r, r, r);
  }
//}}}
//{{{
void cVg::ellipse (float cx, float cy, float rx, float ry) {

  float values[] = {
    eMOVETO, cx-rx, cy,
    eBEZIERTO, cx-rx, cy+ry*KAPPA90, cx-rx*KAPPA90, cy+ry, cx, cy+ry,
    eBEZIERTO, cx+rx*KAPPA90, cy+ry, cx+rx, cy+ry*KAPPA90, cx+rx, cy,
    eBEZIERTO, cx+rx, cy-ry*KAPPA90, cx+rx*KAPPA90, cy-ry, cx, cy-ry,
    eBEZIERTO, cx-rx*KAPPA90, cy-ry, cx-rx, cy-ry*KAPPA90, cx-rx, cy,
    eCLOSE
    };
  mShape.addCommand (values, 32, mStates[mNumStates-1].mTransform);
  }
//}}}
//{{{
void cVg::circle (float cx, float cy, float r) {
  ellipse (cx,cy, r,r);
  }
//}}}
//{{{
void cVg::closePath() {
  float values[] = { eCLOSE };
  mShape.addCommand (values, 1, mStates[mNumStates-1].mTransform);
  }
//}}}

//{{{
void cVg::fill() {

  mShape.flattenPaths();
  mShape.expandFill (mVertices, mDrawEdges ? mFringeWidth : 0.0f, eMITER, 2.4f, mFringeWidth);

  auto fillPaint = mStates[mNumStates-1].fillPaint;
  fillPaint.innerColor.a *= mStates[mNumStates-1].alpha;
  fillPaint.outerColor.a *= mStates[mNumStates-1].alpha;
  renderFill (mShape, fillPaint, mStates[mNumStates-1].scissor, mFringeWidth);
  }
//}}}
//{{{
void cVg::stroke() {

  mShape.flattenPaths();

  auto state = &mStates[mNumStates-1];
  float scale = state->mTransform.getAverageScale();
  float strokeWidth = clampf (state->strokeWidth * scale, 0.0f, 200.0f);
  auto strokePaint = state->strokePaint;
  if (strokeWidth < mFringeWidth) {
    // strokeWidth < pixel, use alpha to emulate coverage, scale by alpha*alpha.
    float alpha = clampf (strokeWidth / mFringeWidth, 0.0f, 1.0f);
    strokePaint.innerColor.a *= alpha * alpha;
    strokePaint.outerColor.a *= alpha * alpha;
    strokeWidth = mFringeWidth;
    }

  mShape.expandStroke (mVertices, mDrawEdges ? (strokeWidth + mFringeWidth) * 0.5f : strokeWidth * 0.5f,
                       state->lineCap, state->lineJoin, state->miterLimit, mFringeWidth);

  strokePaint.innerColor.a *= state->alpha;
  strokePaint.outerColor.a *= state->alpha;
  renderStroke (mShape, strokePaint, state->scissor, mFringeWidth, strokeWidth);
  }
//}}}
//{{{
void cVg::triangleFill() {
// only rects, turn multiple rect paths into single path of triangles, no antiAlias fringe

  int vertexIndex;
  int numVertices;
  mShape.triangleFill (mVertices, vertexIndex, numVertices);

  // set up opengl call
  auto fillPaint = mStates[mNumStates-1].fillPaint;
  fillPaint.innerColor.a *= mStates[mNumStates-1].alpha;
  fillPaint.outerColor.a *= mStates[mNumStates-1].alpha;
  renderTriangles (vertexIndex, numVertices, fillPaint, mStates[mNumStates-1].scissor);
  }
//}}}
//}}}
//{{{  text
void cVg::fontSize (float size) { mStates[mNumStates-1].fontSize = size; }
void cVg::textAlign (int align) { mStates[mNumStates-1].textAlign = align; }
void cVg::textLineHeight (float lineHeight) { mStates[mNumStates-1].lineHeight = lineHeight; }
void cVg::textLetterSpacing (float spacing) { mStates[mNumStates-1].letterSpacing = spacing; }
void cVg::fontFaceId (int font) { mStates[mNumStates-1].fontId = font; }
void cVg::fontFace (string font) { mStates[mNumStates-1].fontId = fonsGetFontByName (fonsContext, font.c_str()); }

//{{{
int cVg::createFont (string name, string path) {
  return fonsAddFont (fonsContext, name.c_str(), path.c_str());
  }
//}}}
//{{{
int cVg::createFontMem (string name, unsigned char* data, int ndata, int freeData) {
  return fonsAddFontMem (fonsContext, name.c_str(), data, ndata, freeData);
  }
//}}}
//{{{
int cVg::findFont (string name) {
  if (name.empty())
    return -1;
  return fonsGetFontByName (fonsContext, name.c_str());
  }
//}}}
//{{{
int cVg::addFallbackFont (string baseFont, string fallbackFont) {
  return addFallbackFontId (findFont (baseFont), findFont (fallbackFont));
  }
//}}}
//{{{
int cVg::addFallbackFontId (int baseFont, int fallbackFont) {
  if (baseFont == -1 || fallbackFont == -1)
    return 0;
  return fonsAddFallbackFont (fonsContext, baseFont, fallbackFont);
  }
//}}}

enum eVgCodepointType { NVG_SPACE, NVG_NEWLINE, NVG_CHAR, NVG_CJK_CHAR };
//{{{
float cVg::text (float x, float y, string str) {

  auto state = &mStates[mNumStates-1];
  if (state->fontId == FONS_INVALID)
    return x;

  float scale = getFontScale (state) * devicePixelRatio;
  float inverseScale = 1.0f / scale;

  fonsSetSize (fonsContext, state->fontSize * scale);
  fonsSetSpacing (fonsContext, state->letterSpacing * scale);
  fonsSetAlign (fonsContext, state->textAlign);
  fonsSetFont (fonsContext, state->fontId);

  // allocate 6 vertices per glyph
  int numVertices = maxi (2, (int)str.size()) * 6;
  int vertexIndex = mVertices.alloc (numVertices);
  auto vertices = mVertices.getVertexPtr (vertexIndex);
  auto firstVertex = vertices;

  FONStextIter iter;
  fonsTextIterInit (fonsContext, &iter, x*scale, y*scale, str.c_str(), str.c_str() + str.size());
  FONStextIter prevIter = iter;
  FONSquad q;
  while (fonsTextIterNext (fonsContext, &iter, &q)) {
    if (iter.prevGlyphIndex == -1) {
      // can not retrieve glyph?
      if (!allocTextAtlas())
        break; // no memory
      iter = prevIter;
      fonsTextIterNext (fonsContext, &iter, &q); // try again
      if (iter.prevGlyphIndex == -1) // still can not find glyph?
        break;
      }
    prevIter = iter;
    //{{{  set vertex triangles
    if (state->mTransform.mIdentity) {
      vertices++->set (q.x0 * inverseScale, q.y0 * inverseScale, q.s0, q.t0);
      vertices++->set (q.x1 * inverseScale, q.y1 * inverseScale, q.s1, q.t1);
      vertices++->set (q.x1 * inverseScale, q.y0 * inverseScale, q.s1, q.t0);
      vertices++->set (q.x0 * inverseScale, q.y0 * inverseScale, q.s0, q.t0);  // 3 = 0
      vertices++->set (q.x0 * inverseScale, q.y1 * inverseScale, q.s0, q.t1);
      vertices++->set (q.x1 * inverseScale, q.y1 * inverseScale, q.s1, q.t1);  // 5 = 1
      }

    else {
      float x;
      float y;
      state->mTransform.point (q.x0 * inverseScale, q.y0 * inverseScale, x, y);
      vertices++->set (x, y, q.s0, q.t0);

      state->mTransform.point (q.x1 * inverseScale, q.y1 * inverseScale, x, y);
      vertices++->set (x, y, q.s1, q.t1);

      state->mTransform.point (q.x1 * inverseScale, q.y0 * inverseScale, x, y);
      vertices++->set (x, y, q.s1, q.t0);

      state->mTransform.point (q.x0 * inverseScale, q.y0 * inverseScale, x, y);  // 3 = 0
      vertices++->set (x, y, q.s0, q.t0);

      state->mTransform.point (q.x0 * inverseScale, q.y1 * inverseScale, x, y);
      vertices++->set (x, y, q.s0, q.t1);

      state->mTransform.point (q.x1 * inverseScale, q.y1 * inverseScale, x, y);  // 5 = 1
      vertices++->set (x, y, q.s1, q.t1);
      }
    //}}}
    }

  // calc vertices used
  numVertices = int(vertices - firstVertex);
  mVertices.trim (vertexIndex + numVertices);

  if (numVertices) {
    // set up opengl call
    auto fillPaint = mStates[mNumStates - 1].fillPaint;
    fillPaint.image = fontImages[fontImageIdx];
    fillPaint.innerColor.a *= mStates[mNumStates - 1].alpha;
    fillPaint.outerColor.a *= mStates[mNumStates - 1].alpha;
    renderText (vertexIndex, numVertices, fillPaint, mStates[mNumStates - 1].scissor);
    }
  flushTextTexture();

  return iter.x;
  }
//}}}
//{{{
void cVg::textBox (float x, float y, float breakRowWidth, const char* string, const char* end) {

  auto state = &mStates[mNumStates-1];
  if (state->fontId == FONS_INVALID)
    return;

  int oldAlign = state->textAlign;
  int hAlign = state->textAlign & (ALIGN_LEFT | ALIGN_CENTER | ALIGN_RIGHT);
  int vAlign = state->textAlign & (ALIGN_TOP | ALIGN_MIDDLE | ALIGN_BOTTOM | ALIGN_BASELINE);
  float lineh = 0;

  textMetrics (NULL, NULL, &lineh);
  state->textAlign = vAlign | ALIGN_LEFT;

  NVGtextRow rows[2];
  int nrows = 0;
  while ((nrows = textBreakLines (string, end, breakRowWidth, rows, 2))) {
    for (int i = 0; i < nrows; i++) {
      auto row = &rows[i];
      if (hAlign & ALIGN_LEFT)
        text (x, y, std::string (row->start, row->end));
      else if (hAlign & ALIGN_CENTER)
        text (x + breakRowWidth * 0.5f - row->width * 0.5f, y, std::string(row->start, row->end));
      else if (hAlign & ALIGN_RIGHT)
        text (x + breakRowWidth - row->width, y, std::string(row->start, row->end));
      y += lineh * state->lineHeight;
      }
    string = rows[nrows-1].next;
    }

  state->textAlign = oldAlign;
  }
//}}}
//{{{
float cVg::textBounds (float x, float y, string str, float* bounds) {

  auto state = &mStates[mNumStates-1];
  if (state->fontId == FONS_INVALID)
    return 0;

  float scale = getFontScale (state) * devicePixelRatio;
  float inverseScale = 1.0f / scale;
  float width;

  fonsSetSize (fonsContext, state->fontSize*scale);
  fonsSetSpacing (fonsContext, state->letterSpacing*scale);
  fonsSetAlign (fonsContext, state->textAlign);
  fonsSetFont (fonsContext, state->fontId);

  width = fonsTextBounds (fonsContext, x*scale, y*scale, str.c_str(), str.c_str() + str.size(), bounds);
  if (bounds != NULL) {
    // Use line bounds for height.
    fonsLineBounds (fonsContext, y*scale, &bounds[1], &bounds[3]);
    bounds[0] *= inverseScale;
    bounds[1] *= inverseScale;
    bounds[2] *= inverseScale;
    bounds[3] *= inverseScale;
    }

  return width * inverseScale;
  }
//}}}
//{{{
void cVg::textMetrics (float* ascender, float* descender, float* lineh) {

  auto state = &mStates[mNumStates-1];
  if (state->fontId == FONS_INVALID)
    return;

  float scale = getFontScale(state) * devicePixelRatio;
  float inverseScale = 1.0f / scale;

  fonsSetSize (fonsContext, state->fontSize*scale);
  fonsSetSpacing (fonsContext, state->letterSpacing*scale);
  fonsSetAlign (fonsContext, state->textAlign);
  fonsSetFont (fonsContext, state->fontId);

  fonsVertMetrics (fonsContext, ascender, descender, lineh);
  if (ascender != NULL)
    *ascender *= inverseScale;
  if (descender != NULL)
    *descender *= inverseScale;
  if (lineh != NULL)
    *lineh *= inverseScale;
  }
//}}}
//{{{
int cVg::textGlyphPositions (float x, float y, string str, NVGglyphPosition* positions, int maxPositions) {

  auto state = &mStates[mNumStates-1];
  if (state->fontId == FONS_INVALID)
    return 0;

  if (str.empty())
    return 0;

  float scale = getFontScale (state) * devicePixelRatio;
  float inverseScale = 1.0f / scale;

  fonsSetSize (fonsContext, state->fontSize * scale);
  fonsSetSpacing (fonsContext, state->letterSpacing * scale);
  fonsSetAlign (fonsContext, state->textAlign);
  fonsSetFont (fonsContext, state->fontId);

  FONStextIter iter;
  fonsTextIterInit (fonsContext, &iter, x*scale, y*scale, str.c_str(), str.c_str() + str.size());
  FONStextIter prevIter = iter;

  int npos = 0;
  FONSquad q;
  while (fonsTextIterNext (fonsContext, &iter, &q)) {
    if (iter.prevGlyphIndex < 0 && allocTextAtlas()) {
      // can not retrieve glyph, try again
      iter = prevIter;
      fonsTextIterNext(fonsContext, &iter, &q);
      }
    prevIter = iter;

    positions[npos].str = iter.str;
    positions[npos].x = iter.x * inverseScale;
    positions[npos].minx = minf(iter.x, q.x0) * inverseScale;
    positions[npos].maxx = maxf(iter.nextx, q.x1) * inverseScale;
    npos++;
    if (npos >= maxPositions)
      break;
    }

  return npos;
  }
//}}}

//{{{
void cVg::textBoxBounds (float x, float y, float breakRowWidth, const char* string, const char* end, float* bounds) {

  auto state = &mStates[mNumStates-1];
  if (state->fontId == FONS_INVALID) {
    if (bounds != NULL) {
      bounds[0] = 0.0f;
      bounds[1] = 0.0f;
      bounds[2] = 0.0f;
      bounds[3] = 0.0f;
      }
    return;
    }

  float scale = getFontScale(state) * devicePixelRatio;
  float inverseScale = 1.0f / scale;
  int oldAlign = state->textAlign;
  int haling = state->textAlign & (ALIGN_LEFT | ALIGN_CENTER | ALIGN_RIGHT);
  int valign = state->textAlign & (ALIGN_TOP | ALIGN_MIDDLE | ALIGN_BOTTOM | ALIGN_BASELINE);
  float lineh = 0, rminy = 0, rmaxy = 0;
  float minx, miny, maxx, maxy;

  textMetrics ( NULL, NULL, &lineh);

  state->textAlign = ALIGN_LEFT | valign;

  minx = maxx = x;
  miny = maxy = y;

  fonsSetSize (fonsContext, state->fontSize*scale);
  fonsSetSpacing (fonsContext, state->letterSpacing*scale);
  fonsSetAlign (fonsContext, state->textAlign);
  fonsSetFont (fonsContext, state->fontId);
  fonsLineBounds (fonsContext, 0, &rminy, &rmaxy);
  rminy *= inverseScale;
  rmaxy *= inverseScale;

  int nrows = 0;
  NVGtextRow rows[2];
  while ((nrows = textBreakLines (string, end, breakRowWidth, rows, 2))) {
    for (int i = 0; i < nrows; i++) {
      auto row = &rows[i];
      float rminx, rmaxx, dx = 0;
      // Horizontal bounds
      if (haling & ALIGN_LEFT)
        dx = 0;
      else if (haling & ALIGN_CENTER)
        dx = breakRowWidth*0.5f - row->width*0.5f;
      else if (haling & ALIGN_RIGHT)
        dx = breakRowWidth - row->width;
      rminx = x + row->minx + dx;
      rmaxx = x + row->maxx + dx;
      minx = minf (minx, rminx);
      maxx = maxf (maxx, rmaxx);
      // Vertical bounds.
      miny = minf (miny, y + rminy);
      maxy = maxf (maxy, y + rmaxy);

      y += lineh * state->lineHeight;
      }
    string = rows[nrows-1].next;
    }

  state->textAlign = oldAlign;
  if (bounds != NULL) {
    bounds[0] = minx;
    bounds[1] = miny;
    bounds[2] = maxx;
    bounds[3] = maxy;
    }
  }
//}}}
//{{{
int cVg::textBreakLines (const char* string, const char* end, float breakRowWidth, NVGtextRow* rows, int maxRows) {

  if (maxRows == 0)
    return 0;
  if (end == NULL)
    end = string + strlen (string);
  if (string == end)
    return 0;

  auto state = &mStates[mNumStates-1];
  if (state->fontId == FONS_INVALID)
    return 0;

  float scale = getFontScale(state) * devicePixelRatio;
  float inverseScale = 1.0f / scale;
  int nrows = 0;
  float rowStartX = 0;
  float rowWidth = 0;
  float rowMinX = 0;
  float rowMaxX = 0;
  const char* rowStart = NULL;
  const char* rowEnd = NULL;
  const char* wordStart = NULL;
  float wordStartX = 0;
  float wordMinX = 0;
  const char* breakEnd = NULL;
  float breakWidth = 0;
  float breakMaxX = 0;
  int type = NVG_SPACE;
  int ptype = NVG_SPACE;
  unsigned int pcodepoint = 0;

  fonsSetSize (fonsContext, state->fontSize*scale);
  fonsSetSpacing (fonsContext, state->letterSpacing*scale);
  fonsSetAlign (fonsContext, state->textAlign);
  fonsSetFont (fonsContext, state->fontId);

  breakRowWidth *= scale;

  FONStextIter iter;
  fonsTextIterInit (fonsContext, &iter, 0, 0, string, end);
  FONStextIter prevIter = iter;

  FONSquad q;
  while (fonsTextIterNext (fonsContext, &iter, &q)) {
    if (iter.prevGlyphIndex < 0 && allocTextAtlas()) { // can not retrieve glyph?
      iter = prevIter;
      fonsTextIterNext (fonsContext, &iter, &q); // try again
      }
    prevIter = iter;

    switch (iter.codepoint) {
      case 9:     // \t
      case 11:    // \v
      case 12:    // \f
      case 32:    // space
      case 0x00a0:  // NBSP
        type = NVG_SPACE;
        break;

      case 10:    // \n
        type = pcodepoint == 13 ? NVG_SPACE : NVG_NEWLINE;
        break;

      case 13:    // \r
        type = pcodepoint == 10 ? NVG_SPACE : NVG_NEWLINE;
        break;

      case 0x0085:  // NEL
        type = NVG_NEWLINE;
        break;

      default:
        if ((iter.codepoint >= 0x4E00 && iter.codepoint <= 0x9FFF) ||
            (iter.codepoint >= 0x3000 && iter.codepoint <= 0x30FF) ||
            (iter.codepoint >= 0xFF00 && iter.codepoint <= 0xFFEF) ||
            (iter.codepoint >= 0x1100 && iter.codepoint <= 0x11FF) ||
            (iter.codepoint >= 0x3130 && iter.codepoint <= 0x318F) ||
            (iter.codepoint >= 0xAC00 && iter.codepoint <= 0xD7AF))
          type = NVG_CJK_CHAR;
        else
          type = NVG_CHAR;
        break;
      }

    if (type == NVG_NEWLINE) {
     //{{{  Always handle new lines.
     rows[nrows].start = rowStart != NULL ? rowStart : iter.str;
     rows[nrows].end = rowEnd != NULL ? rowEnd : iter.str;
     rows[nrows].width = rowWidth * inverseScale;
     rows[nrows].minx = rowMinX * inverseScale;
     rows[nrows].maxx = rowMaxX * inverseScale;
     rows[nrows].next = iter.next;
     nrows++;
     if (nrows >= maxRows)
       return nrows;

     // Set null break point
     breakEnd = rowStart;
     breakWidth = 0.0;
     breakMaxX = 0.0;

     // Indicate to skip the white space at the beginning of the row.
     rowStart = NULL;
     rowEnd = NULL;
     rowWidth = 0;
     rowMinX = rowMaxX = 0;
     }
     //}}}
   else {
     if (rowStart == NULL) {
       //{{{  Skip white space until the beginning of the line
       if (type == NVG_CHAR || type == NVG_CJK_CHAR) {
         // The current char is the row so far
         rowStartX = iter.x;
         rowStart = iter.str;
         rowEnd = iter.next;
         rowWidth = iter.nextx - rowStartX; // q.x1 - rowStartX;
         rowMinX = q.x0 - rowStartX;
         rowMaxX = q.x1 - rowStartX;
         wordStart = iter.str;
         wordStartX = iter.x;
         wordMinX = q.x0 - rowStartX;
         // Set null break point
         breakEnd = rowStart;
         breakWidth = 0.0;
         breakMaxX = 0.0;
         }
       }
       //}}}
     else {
       float nextWidth = iter.nextx - rowStartX;
       //{{{  track last non-white space character
       if (type == NVG_CHAR || type == NVG_CJK_CHAR) {
         rowEnd = iter.next;
         rowWidth = iter.nextx - rowStartX;
         rowMaxX = q.x1 - rowStartX;
         }
       //}}}
       //{{{  track last end of a word
       if (((ptype == NVG_CHAR || ptype == NVG_CJK_CHAR) && type == NVG_SPACE) || type == NVG_CJK_CHAR) {
         breakEnd = iter.str;
         breakWidth = rowWidth;
         breakMaxX = rowMaxX;
         }
       //}}}
       //{{{  track last beginning of a word
       if ((ptype == NVG_SPACE && (type == NVG_CHAR || type == NVG_CJK_CHAR)) || type == NVG_CJK_CHAR) {
         wordStart = iter.str;
         wordStartX = iter.x;
         wordMinX = q.x0 - rowStartX;
         }
       //}}}

       // Break to new line when a character is beyond break width.
       if ((type == NVG_CHAR || type == NVG_CJK_CHAR) && nextWidth > breakRowWidth) {
         // The run length is too long, need to break to new line.
         if (breakEnd == rowStart) {
           //{{{  current word longer than row length, just break it from here
           rows[nrows].start = rowStart;
           rows[nrows].end = iter.str;
           rows[nrows].width = rowWidth * inverseScale;
           rows[nrows].minx = rowMinX * inverseScale;
           rows[nrows].maxx = rowMaxX * inverseScale;
           rows[nrows].next = iter.str;
           nrows++;
           if (nrows >= maxRows)
             return nrows;
           rowStartX = iter.x;
           rowStart = iter.str;
           rowEnd = iter.next;
           rowWidth = iter.nextx - rowStartX;
           rowMinX = q.x0 - rowStartX;
           rowMaxX = q.x1 - rowStartX;
           wordStart = iter.str;
           wordStartX = iter.x;
           wordMinX = q.x0 - rowStartX;
           }
           //}}}
         else {
           //{{{  break line from end of last word, start new line from beginning of new.
           rows[nrows].start = rowStart;
           rows[nrows].end = breakEnd;
           rows[nrows].width = breakWidth * inverseScale;
           rows[nrows].minx = rowMinX * inverseScale;
           rows[nrows].maxx = breakMaxX * inverseScale;
           rows[nrows].next = wordStart;
           nrows++;
           if (nrows >= maxRows)
             return nrows;

           rowStartX = wordStartX;
           rowStart = wordStart;
           rowEnd = iter.next;
           rowWidth = iter.nextx - rowStartX;
           rowMinX = wordMinX;
           rowMaxX = q.x1 - rowStartX;
           // No change to the word start
           }
           //}}}

         // Set null break point
         breakEnd = rowStart;
         breakWidth = 0.0;
         breakMaxX = 0.0;
         }
       }
     }

    pcodepoint = iter.codepoint;
    ptype = type;
    }

  // Break the line from the end of the last word, and start new line from the beginning of the new.
  if (rowStart != NULL) {
    rows[nrows].start = rowStart;
    rows[nrows].end = rowEnd;
    rows[nrows].width = rowWidth * inverseScale;
    rows[nrows].minx = rowMinX * inverseScale;
    rows[nrows].maxx = rowMaxX * inverseScale;
    rows[nrows].next = end;
    nrows++;
    }

  return nrows;
  }
//}}}
//}}}
//{{{  image
//{{{
int cVg::createImage (const char* filename, eImageFlags imageFlags) {

  stbi_set_unpremultiply_on_load (1);
  stbi_convert_iphone_png_to_rgb (1);

  int w, h, n;
  unsigned char* img = stbi_load (filename, &w, &h, &n, 4);
  if (img == NULL) {
    printf ("Failed to load %s - %s\n", filename, stbi_failure_reason());
    return 0;
    }

  int image = createImageRGBA (w, h, imageFlags, img);
  stbi_image_free (img);
  return image;
  }
//}}}
//{{{
int cVg::createImageMem (int imageFlags, unsigned char* data, int ndata) {

  int w, h, n;
  unsigned char* img = stbi_load_from_memory (data, ndata, &w, &h, &n, 4);
  if (img == NULL) {
    printf ("Failed to load %s\n", stbi_failure_reason());
    return 0;
    }

  int image = createImageRGBA (w, h, imageFlags, img);
  stbi_image_free (img);
  return image;
  }
//}}}
//{{{
int cVg::createImageRGBA (int w, int h, int imageFlags, const unsigned char* data) {
  return renderCreateTexture (TEXTURE_RGBA, w, h, imageFlags, data);
  }
//}}}

//{{{
void cVg::updateImage (int image, const unsigned char* data) {
  int w, h;
  renderGetTextureSize (image, &w, &h);
  renderUpdateTexture (image, 0,0, w,h, data);
  }
//}}}
//{{{
void cVg::imageSize (int image, int* w, int* h) {
  renderGetTextureSize (image, w, h);
  }
//}}}

//{{{
void cVg::deleteImage (int image) {
  renderDeleteTexture (image);
  }
//}}}
//}}}
//{{{  scissor
//{{{
void cVg::scissor (float x, float y, float w, float h) {

  auto state = &mStates[mNumStates-1];
  w = maxf (0.0f, w);
  h = maxf (0.0f, h);

  state->scissor.mTransform.setTranslate (x + w * 0.5f, y + h* 0.5f);
  state->scissor.mTransform.multiply (state->mTransform);

  state->scissor.extent[0] = w * 0.5f;
  state->scissor.extent[1] = h * 0.5f;
  }
//}}}
//{{{
void cVg::intersectScissor (float x, float y, float w, float h) {

  // If no previous scissor has been set, set the scissor as current scissor.
  auto state = &mStates[mNumStates-1];
  if (state->scissor.extent[0] < 0) {
    scissor (x, y, w, h);
    return;
    }

  // Transform the current scissor rect into current transform space, if diff in rotation, approximation.
  cTransform transform (state->scissor.mTransform);
  float ex = state->scissor.extent[0];
  float ey = state->scissor.extent[1];

  cTransform inverse;
  state->mTransform.getInverse (inverse);
  transform.multiply (inverse);

  // Intersect rects
  float tex;
  float tey;
  transform.pointScissor (ex, ey, tex, tey);
  float rect[4];
  intersectRects (rect, transform.getTranslateX() - tex, transform.getTranslateY() - tey, tex*2, tey*2, x, y, w, h);
  scissor (rect[0], rect[1], rect[2], rect[3]);
  }
//}}}
//{{{
void cVg::resetScissor() {

  auto state = &mStates[mNumStates-1];
  memset (&state->scissor.mTransform, 0, sizeof(cTransform));
  state->scissor.extent[0] = -1.0f;
  state->scissor.extent[1] = -1.0f;
  }
//}}}
//}}}
//{{{  composite
//{{{
void cVg::globalCompositeOp (eCompositeOp op) {
  mStates[mNumStates-1].compositeOp = compositeOpState (op);
  }
//}}}
//{{{
void cVg::globalCompositeBlendFuncSeparate (eBlendFactor srcRGB, eBlendFactor dstRGB,
                                            eBlendFactor srcAlpha, eBlendFactor dstAlpha) {

  cCompositeOpState compositeOpState;
  compositeOpState.srcRGB = srcRGB;
  compositeOpState.dstRGB = dstRGB;
  compositeOpState.srcAlpha = srcAlpha;
  compositeOpState.dstAlpha = dstAlpha;

  mStates[mNumStates-1].compositeOp = compositeOpState;
  }
//}}}
//{{{
void cVg::globalCompositeBlendFunc (eBlendFactor sfactor, eBlendFactor dfactor) {
  globalCompositeBlendFuncSeparate (sfactor, dfactor, sfactor, dfactor);
  }
//}}}
//}}}
//{{{  frame
//{{{
void cVg::beginFrame (int windowWidth, int windowHeight, float devicePixelRatio) {

  mNumStates = 0;
  saveState();
  resetState();

  setDevicePixelRatio (devicePixelRatio);
  renderViewport (windowWidth, windowHeight, devicePixelRatio);
  }
//}}}
//{{{
void cVg::endFrame() {

  auto state = &mStates[mNumStates-1];
  renderFrame (mVertices, state->compositeOp);

  if (fontImageIdx) {
    int fontImage = fontImages[fontImageIdx];
    int i, j, iw, ih;
    // delete images that smaller than current one
    if (fontImage == 0)
      return;

    imageSize (fontImage, &iw, &ih);
    for (i = j = 0; i < fontImageIdx; i++) {
      if (fontImages[i] != 0) {
        int nw, nh;
        imageSize (fontImages[i], &nw, &nh);
        if (nw < iw || nh < ih)
          renderDeleteTexture (fontImages[i]);
        else
          fontImages[j++] = fontImages[i];
        }
      }

    // make current font image to first
    fontImages[j++] = fontImages[0];
    fontImages[0] = fontImage;
    fontImageIdx = 0;

    // clear all images after j
    for (i = j; i < NVG_MAX_FONTIMAGES; i++)
      fontImages[i] = 0;
    }

  mVertices.reset();
  }
//}}}

//{{{
string cVg::getFrameStats() {
  return "v:" + to_string(mVertices.getNumVertices()) +
         " d:" + to_string (mDrawArrays);
  }
//}}}
//}}}

// private
//{{{
void cVg::setDevicePixelRatio (float ratio) {

  devicePixelRatio = ratio;
  }
//}}}
//{{{
cVg::cCompositeOpState cVg::compositeOpState (eCompositeOp op) {

  cCompositeOpState compositeOpState;
  switch (op) {
    case NVG_SOURCE_OVER :
      compositeOpState.srcRGB = NVG_ONE;
      compositeOpState.dstRGB = NVG_ONE_MINUS_SRC_ALPHA;
      break;
    case NVG_SOURCE_IN:
      compositeOpState.srcRGB = NVG_DST_ALPHA;
      compositeOpState.dstRGB = NVG_ZERO;
      break;
    case NVG_SOURCE_OUT:
      compositeOpState.srcRGB = NVG_ONE_MINUS_DST_ALPHA;
      compositeOpState.dstRGB = NVG_ZERO;
      break;
    case NVG_ATOP:
      compositeOpState.srcRGB = NVG_DST_ALPHA;
      compositeOpState.dstRGB = NVG_ONE_MINUS_SRC_ALPHA;
      break;
    case NVG_DESTINATION_OVER:
      compositeOpState.srcRGB = NVG_ONE_MINUS_DST_ALPHA;
      compositeOpState.dstRGB = NVG_ONE;
      break;
    case NVG_DESTINATION_IN:
      compositeOpState.srcRGB = NVG_ZERO;
      compositeOpState.dstRGB = NVG_SRC_ALPHA;
      break;
    case NVG_DESTINATION_OUT:
      compositeOpState.srcRGB = NVG_ZERO;
      compositeOpState.dstRGB = NVG_ONE_MINUS_SRC_ALPHA;
      break;
    case NVG_DESTINATION_ATOP:
      compositeOpState.srcRGB = NVG_ONE_MINUS_DST_ALPHA;
      compositeOpState.dstRGB = NVG_SRC_ALPHA;
      break;
    case NVG_LIGHTER:
      compositeOpState.srcRGB = NVG_ONE;
      compositeOpState.dstRGB = NVG_ONE;
      break;
    case NVG_COPY:
      compositeOpState.srcRGB = NVG_ONE;
      compositeOpState.dstRGB = NVG_ZERO;
      break;
    case NVG_XOR:
      compositeOpState.srcRGB = NVG_ONE_MINUS_DST_ALPHA;
      compositeOpState.dstRGB = NVG_ONE_MINUS_SRC_ALPHA;
      break;
    default:
      compositeOpState.srcRGB = NVG_ONE;
      compositeOpState.dstRGB = NVG_ZERO;
      break;
    }

  compositeOpState.srcAlpha = compositeOpState.srcRGB;
  compositeOpState.dstAlpha = compositeOpState.dstRGB;
  return compositeOpState;
  }
//}}}

//{{{  text
//{{{
float cVg::getFontScale (cState* state) {
  return minf (quantize (state->mTransform.getAverageScale(), 0.01f), 4.0f);
  }
//}}}
//{{{
void cVg::flushTextTexture() {

  int dirty[4];
  if (fonsValidateTexture (fonsContext, dirty)) {
    int fontImage = fontImages[fontImageIdx];
    // Update texture
    if (fontImage != 0) {
      int iw, ih;
      const unsigned char* data = fonsGetTextureData (fonsContext, &iw, &ih);
      int x = dirty[0];
      int y = dirty[1];
      int w = dirty[2] - dirty[0];
      int h = dirty[3] - dirty[1];
      renderUpdateTexture (fontImage, x,y, w,h, data);
      }
    }
  }
//}}}
//{{{
int cVg::allocTextAtlas() {

  flushTextTexture();
  if (fontImageIdx >= NVG_MAX_FONTIMAGES-1)
    return 0;

  // if next fontImage already have a texture
  int iw, ih;
  if (fontImages[fontImageIdx+1] != 0)
    imageSize (fontImages[fontImageIdx+1], &iw, &ih);
  else {
    // calculate the new font image size and create it.
    imageSize (fontImages[fontImageIdx], &iw, &ih);
    if (iw > ih)
      ih *= 2;
    else
      iw *= 2;
    if (iw > MAX_FONTIMAGE_SIZE || ih > MAX_FONTIMAGE_SIZE)
      iw = ih = MAX_FONTIMAGE_SIZE;
    fontImages[fontImageIdx+1] = renderCreateTexture (TEXTURE_ALPHA, iw, ih, 0, NULL);
    }

  ++fontImageIdx;
  fonsResetAtlas (fonsContext, iw, ih);
  return 1;
  }
//}}}
//}}}
