// cVg.cpp - based on Mikko Mononen memon@inside.org nanoVg
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include "cVg.h"
#include "cAtlasText.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;
//}}}
//{{{  constexpr
constexpr int kInitPathSize = 16;
constexpr int kInitPointsSize = 128;

constexpr float kAppA90 = 0.5522847493f; // Length proportional to radius of a cubic bezier handle for 90deg arcs.
constexpr float kDistanceTolerance = 0.01f;

constexpr float kLarge = 1e5;
//}}}

//{{{
static const char* kShaderHeader =
  "#version 100\n"
  "#define UNIFORMARRAY_SIZE 11\n"
  "\n";
//}}}
//{{{
static const char* kVertShader =
  "uniform vec2 viewSize;\n"
  "attribute vec2 vertex;\n"
  "attribute vec2 tcoord;\n"
  "varying vec2 ftcoord;\n"
  "varying vec2 fpos;\n"

  "void main() {\n"
    "ftcoord = tcoord;\n"
    "fpos = vertex;\n"
    "gl_Position = vec4(2.0*vertex.x/viewSize.x - 1.0, 1.0 - 2.0*vertex.y/viewSize.y, 0, 1);\n"
    "}\n";
//}}}
//{{{
static const char* kFragShader =
  // vars
  "precision mediump float;\n"
  "uniform vec4 frag[UNIFORMARRAY_SIZE];\n"
  "uniform sampler2D tex;\n"
  "varying vec2 ftcoord;\n"
  "varying vec2 fpos;\n"

  "#define scissorMatrix mat3(frag[0].xyz, frag[1].xyz, frag[2].xyz)\n"
  "#define paintMatrix mat3(frag[3].xyz, frag[4].xyz, frag[5].xyz)\n"
  "#define innerColour frag[6]\n"
  "#define outerColour frag[7]\n"
  "#define scissorExt frag[8].xy\n"
  "#define scissorScale frag[8].zw\n"
  "#define extent frag[9].xy\n"
  "#define radius frag[9].z\n"
  "#define feather frag[9].w\n"
  "#define strokeMult frag[10].x\n"
  "#define strokeThreshold frag[10].y\n"
  "#define texType int(frag[10].z)\n"
  "#define type int(frag[10].w)\n"

  "float sdroundrect(vec2 pt, vec2 ext, float rad) {\n"
    "vec2 ext2 = ext - vec2(rad,rad);\n"
    "vec2 d = abs(pt) - ext2;\n"
    "return min (max (d.x,d.y),0.0) + length(max(d,0.0)) - rad;\n"
    "}\n"

  // Scissoring
  "float scissorMask(vec2 p) {\n"
    "vec2 sc = (abs((scissorMatrix * vec3(p,1.0)).xy) - scissorExt);\n"
    "sc = vec2(0.5,0.5) - sc * scissorScale;\n"
    "return clamp(sc.x,0.0,1.0) * clamp(sc.y,0.0,1.0);\n"
    "}\n"

  // EDGE_AA Stroke - from [0..1] to clipped pyramid, where the slope is 1px
  "float strokeMask() { return min(1.0, (1.0-abs(ftcoord.x*2.0-1.0))*strokeMult) * min(1.0, ftcoord.y); }\n"

  "void main() {\n"
    "vec4 result;\n"
    "float scissor = scissorMask(fpos);\n"
    "float strokeAlpha = strokeMask();\n"

    "if (type == 0) {\n"
      //  SHADER_FILL_GRADIENT - calc grad colour using box grad
      "vec2 pt = (paintMatrix * vec3(fpos,1.0)).xy;\n"
      "float d = clamp((sdroundrect(pt, extent, radius) + feather*0.5) / feather, 0.0, 1.0);\n"
      "vec4 colour = mix(innerColour,outerColour,d);\n"
      // Combine alpha
      "colour *= strokeAlpha * scissor;\n"
      "result = colour;\n"
    "} else if (type == 1) {\n"
      // SHADER_FILL_IMAGE - image calc colour fron texture
      "vec2 pt = (paintMatrix * vec3(fpos,1.0)).xy / extent;\n"
      "vec4 colour = texture2D(tex, pt);\n"
      "if (texType == 1) colour = vec4(colour.xyz*colour.w,colour.w);"
      "if (texType == 2) colour = vec4(colour.x);"
      "colour *= innerColour;\n"            // apply colour tint and alpha
      "colour *= strokeAlpha * scissor;\n" // combine alpha
      "result = colour;\n"
    "} else if (type == 2) {\n"
      //  SHADER_SIMPLE - stencil fill
      "result = vec4(1,1,1,1);\n"
    "} else if (type == 3) {\n"
       // SHADER_IMAGE - textured tris
      "vec4 colour = texture2D(tex, ftcoord);\n"
      "if (texType == 1) colour = vec4(colour.xyz*colour.w,colour.w);"
      "if (texType == 2) colour = vec4(colour.x);"
      "colour *= scissor;\n"
      "result = colour * innerColour;\n"
    "}\n"

  "if (strokeAlpha < strokeThreshold) discard;\n"
  "gl_FragColor = result;\n"
  "}\n";
//}}}

// public
//{{{
cVg::cVg (int flags) : mDrawEdges(!(flags & eNoEdges)), mDrawSolid(!(flags & eNoSolid)), mDrawTriangles(true) {

  saveState();
  resetState();

  setDevicePixelRatio (1.f);
  }
//}}}
//{{{
cVg::~cVg() {

  glDisableVertexAttribArray (0);
  glDisableVertexAttribArray (1);

  if (mVertexBuffer)
    glDeleteBuffers (1, &mVertexBuffer);

  glDisable (GL_CULL_FACE);
  glBindBuffer (GL_ARRAY_BUFFER, 0);
  glUseProgram (0);
  setBindTexture (0);

  for (int i = 0; i < mNumTextures; i++)
    if (mTextures[i].tex && (mTextures[i].flags & eNoDelete) == 0)
      glDeleteTextures (1, &mTextures[i].tex);

  free (mTextures);
  free (mPathVertices);
  free (mFrags);
  free (mDraws);
  }
//}}}

//{{{
void cVg::initialise() {

  mShader.create ("#define EDGE_AA 1\n");
  mShader.getUniforms();

  glGenBuffers (1, &mVertexBuffer);

  // removed because of strange startup time
  //glFinish();

  // initialise cAtlasText and its matching texture
  constexpr int kInitAtlasSize = 512;
  mAtlasText = new cAtlasText (kInitAtlasSize, kInitAtlasSize);
  mFontTextureIds[0] = createTexture (eTextureAlpha, kInitAtlasSize, kInitAtlasSize, 0, NULL, "initFont");
  }
//}}}
//{{{  cVg::cTransform members
//{{{
cVg::cTransform::cTransform() : mIdentity(true) {
  mSx = 1.0f;
  mKy = 0.0f;
  mKx = 0.0f;
  mSy = 1.0f;

  mTx = 0.0f;
  mTy = 0.0f;
  }
//}}}
//{{{
cVg::cTransform::cTransform (float sx, float ky, float kx, float sy, float tx, float ty) {
  mSx = sx;
  mKy = ky;
  mKx = kx;
  mSy = sy;

  mTx = tx;
  mTy = ty;

  mIdentity = isIdentity();
  }
//}}}

// gets
float cVg::cTransform::getAverageScaleX() { return sqrtf (mSx*mSx + mKx*mKx); }
float cVg::cTransform::getAverageScaleY() { return sqrtf (mKy*mKy + mSy*mSy); }
float cVg::cTransform::getAverageScale() { return (getAverageScaleX() + getAverageScaleY()) * 0.5f; }
//{{{
cVg::cTransform cVg::cTransform::getInverse() {

  cTransform inverse;
  double det = (double)mSx * mSy - (double)mKx * mKy;
  if (det > -1e-6 && det < 1e-6)
    inverse.setIdentity();
  else {
    double inverseDet = 1.0 / det;
    inverse.mSx = (float)(mSy * inverseDet);
    inverse.mKx = (float)(-mKx * inverseDet);
    inverse.mTx = (float)(((double)mKx * mTy - (double)mSy * mTx) * inverseDet);
    inverse.mKy = (float)(-mKy * inverseDet);
    inverse.mSy = (float)(mSx * inverseDet);
    inverse.mTy = (float)(((double)mKy * mTx - (double)mSx * mTy) * inverseDet);
    }

  return inverse;
  }
//}}}
//{{{
void cVg::cTransform::getMatrix3x4 (float* matrix3x4) {

  matrix3x4[0] = mSx;
  matrix3x4[1] = mKy;
  matrix3x4[2] = 0.0f;

  matrix3x4[3] = 0.0f;
  matrix3x4[4] = mKx;
  matrix3x4[5] = mSy;

  matrix3x4[6] = 0.0f;
  matrix3x4[7] = 0.0f;
  matrix3x4[8] = mTx;

  matrix3x4[9] = mTy;
  matrix3x4[10] = 1.0f;
  matrix3x4[11] = 0.0f;
  }
//}}}

// sets
//{{{
void cVg::cTransform::setIdentity() {

  mSx = 1.0f;
  mKy = 0.0f;
  mKx = 0.0f;
  mSy = 1.0f;

  mTx = 0.0f;
  mTy = 0.0f;

  mIdentity = true;
  }
//}}}
//{{{
void cVg::cTransform::setTranslate (cPointF t) {

  mSx = 1.0f;
  mKy = 0.0f;
  mKx = 0.0f;
  mSy = 1.0f;

  mTx = t.x;
  mTy = t.y;

  mIdentity = isIdentity();
  }
//}}}
//{{{
void cVg::cTransform::setTranslate (float tx, float ty) {

  mSx = 1.0f;
  mKy = 0.0f;
  mKx = 0.0f;
  mSy = 1.0f;

  mTx = tx;
  mTy = ty;

  mIdentity = isIdentity();
  }
//}}}
//{{{
void cVg::cTransform::setScale (float sx, float sy) {

  mSx = sx;
  mKy = 0.0f;
  mKx = 0.0f;
  mSy = sy;

  mTx = 0.0f;
  mTy = 0.0f;

  mIdentity = isIdentity();
  }
//}}}
//{{{
void cVg::cTransform::setRotate (float angle) {

  float cs = cosf (angle);
  float sn = sinf (angle);

  mSx = cs;
  mKy = sn;
  mKx = -sn;
  mSy = cs;

  mTx = 0.0f;
  mTy = 0.0f;

  mIdentity = isIdentity();
  }
//}}}
//{{{
void cVg::cTransform::setRotateTranslate (float angle, float tx, float ty) {

  float cs = cosf (angle);
  float sn = sinf (angle);

  mSx = cs;
  mKy = sn;
  mKx = -sn;
  mSy = cs;

  mTx = tx;
  mTy = ty;

  mIdentity = isIdentity();
  }
//}}}
//{{{
void cVg::cTransform::setRotateTranslate (float angle, cPointF t) {

  float cs = cosf (angle);
  float sn = sinf (angle);

  mSx = cs;
  mKy = sn;
  mKx = -sn;
  mSy = cs;

  mTx = t.x;
  mTy = t.y;

  mIdentity = isIdentity();
  }
//}}}
//{{{
void cVg::cTransform::set (float sx, float ky, float kx, float sy, float tx, float ty) {

  mSx = sx;
  mKy = ky;
  mKx = kx;
  mSy = sy;

  mTx = tx;
  mTy = ty;

  mIdentity = isIdentity();
  }
//}}}

//{{{
void cVg::cTransform::multiply (cTransform& t) {

  float t0 = mSx * t.mSx + mKy * t.mKx;
  float t2 = mKx * t.mSx + mSy * t.mKx;
  float t4 = mTx * t.mSx + mTy * t.mKx + t.mTx;

  mKy = mSx * t.mKy + mKy * t.mSy;
  mSy = mKx * t.mKy + mSy * t.mSy;
  mTy = mTx * t.mKy + mTy * t.mSy + t.mTy;

  mSx = t0;
  mKx = t2;
  mTx = t4;

  mIdentity = isIdentity();
  }
//}}}
//{{{
void cVg::cTransform::premultiply (cTransform& t) {

  float t0 = t.mSx * mSx + t.mKy * mKx;
  float t2 = t.mKx * mSx + t.mSy * mKx;
  float t4 = t.mTx * mSx + t.mTy * mKx + mTx;

  t.mKy = t.mSx * mKy + t.mKy * mSy;
  t.mSy = t.mKx * mKy + t.mSy * mSy;
  t.mTy = t.mTx * mKy + t.mTy * mSy + mTy;

  t.mSx = t0;
  t.mKx = t2;
  t.mTx = t4;

  mIdentity = isIdentity();
  }
//}}}

//{{{
void cVg::cTransform::point (float& x, float& y) {
// transform point back to itself

  float temp = (x * mSx) + (y * mKx) + mTx;
  y = (x * mKy) + (y * mSy) + mTy;
  x = temp;
  }
//}}}
//{{{
void cVg::cTransform::point (float srcx, float srcy, float& dstx, float& dsty) {
// transform src point to dst point

  dstx = (srcx * mSx) + (srcy * mKx) + mTx;
  dsty = (srcx * mKy) + (srcy * mSy) + mTy;
  }
//}}}
//{{{
void cVg::cTransform::pointScissor (float srcx, float srcy, float& dstx, float& dsty) {

  dstx = srcx * absf (mSx) + srcy * absf (mKx);
  dsty = srcx * absf (mKy) + srcy * absf (mSy);
  }
//}}}
//}}}

//{{{  state
//{{{
void cVg::resetState() {

  auto state = &mStates[mNumStates-1];

  state->composite = compositeState (NVG_SOURCE_OVER);
  state->fillPaint.set (sColourF (255, 255, 255, 255));
  state->strokePaint.set (sColourF (0, 0, 0, 255));

  state->strokeWidth = 1.0f;
  state->miterLimit = 10.0f;
  state->lineCap = eButt;
  state->lineJoin = eMiter;
  state->alpha = 1.0f;

  state->mTransform.setIdentity();

  state->scissor.extent[0] = -1.0f;
  state->scissor.extent[1] = -1.0f;

  state->fontSize = 16.0f;
  state->letterSpacing = 0.0f;
  state->lineHeight = 1.0f;
  state->textAlign = eAlignLeft | eAlignBaseline;
  state->fontId = 0;
  }
//}}}
//{{{
void cVg::saveState() {

  if (mNumStates >= kMaxStates)
    return;
  if (mNumStates > 0)
    memcpy (&mStates[mNumStates], &mStates[mNumStates-1], sizeof (sState));
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

//{{{
void cVg::setCompositeState (eCompositeOp compositeOp) {
  mStates[mNumStates-1].composite = compositeState (compositeOp);
  }
//}}}
//{{{
void cVg::setCompositeBlendFuncSeparate (eBlendFactor srcRGB, eBlendFactor dstRGB,
                                         eBlendFactor srcAlpha, eBlendFactor dstAlpha) {

  sCompositeState compositeState;
  compositeState.srcRGB = srcRGB;
  compositeState.dstRGB = dstRGB;
  compositeState.srcAlpha = srcAlpha;
  compositeState.dstAlpha = dstAlpha;

  mStates[mNumStates-1].composite = compositeState;
  }
//}}}
//{{{
void cVg::setCompositeBlendFunc (eBlendFactor sfactor, eBlendFactor dfactor) {
  setCompositeBlendFuncSeparate (sfactor, dfactor, sfactor, dfactor);
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
void cVg::setTranslate (cPointF p) {

  cTransform transform;
  transform.setTranslate (p);
  mStates[mNumStates - 1].mTransform.premultiply (transform);
  }
//}}}
//{{{
void cVg::setTranslate (float x, float y) {

  cTransform transform;
  transform.setTranslate (x, y);
  mStates[mNumStates - 1].mTransform.premultiply (transform);
  }
//}}}
//{{{
void cVg::setScale (float x, float y) {

  cTransform transform;
  transform.setScale (x, y);
  mStates[mNumStates - 1].mTransform.premultiply (transform);
  }
//}}}
//{{{
void cVg::setRotate (float angle) {

  cTransform transform;
  transform.setRotate (angle);
  mStates[mNumStates - 1].mTransform.premultiply (transform);
  }
//}}}

//{{{
void cVg::setTransform (float a, float b, float c, float d, float e, float f) {

  cTransform transform (a, b, c, d, e, f);
  mStates[mNumStates - 1].mTransform.premultiply (transform);
  }
//}}}
//}}}
//{{{  scissor
//{{{
void cVg::scissor (cPointF p, cPointF size) {

  auto state = &mStates[mNumStates-1];
  float width = max (0.0f, size.x);
  float height = max (0.0f, size.y);

  state->scissor.mTransform.setTranslate (p + cPointF(width * 0.5f, height * 0.5f));
  state->scissor.mTransform.multiply (state->mTransform);

  state->scissor.extent[0] = width * 0.5f;
  state->scissor.extent[1] = height * 0.5f;
  }
//}}}
//{{{
void cVg::intersectScissor (cPointF p, cPointF size) {

  // if no previous scissor has been set, set the scissor as current scissor.
  auto state = &mStates[mNumStates-1];
  if (state->scissor.extent[0] < 0) {
    scissor (p, size);
    return;
    }

  // transform the current scissor rect into current transform space, if diff in rotation, approximation.
  cTransform transform (state->scissor.mTransform);
  float ex = state->scissor.extent[0];
  float ey = state->scissor.extent[1];

  cTransform inverse = state->mTransform.getInverse();
  transform.multiply (inverse);

  // Intersect rects
  float tex;
  float tey;
  transform.pointScissor (ex, ey, tex, tey);
  float rect[4];
  intersectRects (rect, transform.getTranslateX() - tex, transform.getTranslateY() - tey, tex*2, tey*2,
                  p.x, p.y, size.x, size.y);
  scissor (cPointF(rect[0], rect[1]), cPointF(rect[2], rect[3]));
  }
//}}}
//{{{
void cVg::resetScissor() {

  auto state = &mStates[mNumStates-1];
  state->scissor.mTransform.setIdentity();
  state->scissor.extent[0] = -1.0f;
  state->scissor.extent[1] = -1.0f;
  }
//}}}
//}}}
//{{{  text
enum eVgCodepointType { NVG_SPACE, NVG_NEWLINE, NVG_CHAR, NVG_CJK_CHAR };
//{{{
int cVg::createFont (const string& fontName, uint8_t* data, int dataSize) {
  return mAtlasText->addFont (fontName, data, dataSize);
  }
//}}}

//{{{
float cVg::getTextBounds (cPointF p, const string& str, float* bounds) {

  auto state = &mStates[mNumStates-1];
  if (state->fontId == cAtlasText::kInvalid)
    return 0.f;

  float scale = getFontScale (state) * devicePixelRatio;
  float inverseScale = 1.0f / scale;

  mAtlasText->setFontSizeSpacingAlign (state->fontId, state->fontSize * scale, state->letterSpacing * scale, state->textAlign);

  float width = mAtlasText->getTextBounds (p*scale, str, bounds);

  // Use line bounds for height.
  mAtlasText->getLineBounds (p.y * scale, bounds[1], bounds[3]);
  bounds[0] *= inverseScale;
  bounds[1] *= inverseScale;
  bounds[2] *= inverseScale;
  bounds[3] *= inverseScale;

  return width * inverseScale;
  }
//}}}
//{{{
float cVg::getTextMetrics (float& ascender, float& descender) {

  auto state = &mStates[mNumStates-1];
  if (state->fontId == cAtlasText::kInvalid)
    return 0.f;

  float scale = getFontScale(state) * devicePixelRatio;
  float inverseScale = 1.0f / scale;

  mAtlasText->setFontSizeSpacingAlign (
    state->fontId, state->fontSize * scale, state->letterSpacing * scale, state->textAlign);

  float lineh = mAtlasText->getVertMetrics (ascender, descender);

  ascender *= inverseScale;
  descender *= inverseScale;
  lineh *= inverseScale;

  return lineh;
  }
//}}}
//{{{
int cVg::getTextGlyphPositions (cPointF p, const string& str, sGlyphPosition* positions, int maxPositions) {

  if (str.empty())
    return 0;

  auto state = &mStates[mNumStates-1];
  if (state->fontId == cAtlasText::kInvalid)
    return 0;

  float scale = getFontScale (state) * devicePixelRatio;
  float inverseScale = 1.0f / scale;

  mAtlasText->setFontSizeSpacingAlign (
    state->fontId, state->fontSize * scale, state->letterSpacing * scale, state->textAlign);

  cAtlasText::sTextIt it;
  mAtlasText->textIt (&it, p*scale, str);
  cAtlasText::sTextIt prevIt = it;

  int npos = 0;
  cAtlasText::sQuad quad;
  while (mAtlasText->textItNext (&it, &quad)) {
    if ((it.prevGlyphIndex < 0) && allocAtlas()) {
      // can not retrieve glyph, try again
      it = prevIt;
      mAtlasText->textItNext (&it, &quad);
      }
    prevIt = it;

    positions[npos].str = it.str;
    positions[npos].x = it.x * inverseScale;
    positions[npos].minx = min (it.x, quad.x0) * inverseScale;
    positions[npos].maxx = max (it.nextx, quad.x1) * inverseScale;
    npos++;
    if (npos >= maxPositions)
      break;
    }

  return npos;
  }
//}}}

//{{{
void cVg::setFontById (int font) {
  mStates[mNumStates-1].fontId = font;
  }
//}}}
//{{{
void cVg::setFontByName (const string& fontName) {
  mStates[mNumStates-1].fontId = mAtlasText->getFontByName (fontName);
  }
//}}}
//{{{
void cVg::setFontSize (float size) {
  mStates[mNumStates-1].fontSize = size;
  }
//}}}
//{{{
void cVg::setTextAlign (int align) {
  mStates[mNumStates-1].textAlign = align;
  }
//}}}
//{{{
void cVg::setTextLineHeight (float lineHeight) {
  mStates[mNumStates-1].lineHeight = lineHeight;
  }
//}}}
//{{{
void cVg::setTextLetterSpacing (float spacing) {
  mStates[mNumStates-1].letterSpacing = spacing;
  }
//}}}

//{{{
float cVg::text (cPointF p, const string& str) {

  auto state = &mStates[mNumStates-1];
  if (state->fontId == cAtlasText::kInvalid)
    return p.x;

  float scale = getFontScale (state) * devicePixelRatio;
  float inverseScale = 1.f / scale;
  mAtlasText->setFontSizeSpacingAlign (state->fontId, state->fontSize * scale, state->letterSpacing * scale, state->textAlign);

  // allocate 6 vertices per glyph
  int numVertices = max (2, (int)str.size()) * 6;
  int vertexIndex = mVertices.alloc (numVertices);
  auto vertices = mVertices.getVertexPtr (vertexIndex);
  auto firstVertex = vertices;

  cAtlasText::sTextIt it;
  mAtlasText->textIt (&it, p*scale, str);
  cAtlasText::sTextIt prevIt = it;

  cAtlasText::sQuad quad;
  while (mAtlasText->textItNext (&it, &quad)) {
    if (it.prevGlyphIndex == -1) {
      // can not retrieve glyph?
      if (!allocAtlas())
        break; // no memory
      it = prevIt;
      mAtlasText->textItNext (&it, &quad); // try again
      if (it.prevGlyphIndex == -1) // still can not find glyph?
        break;
      }
    prevIt = it;

    // set triangle vertices from quad
    if (state->mTransform.mIdentity) {
      //{{{  identity transform, copy
      vertices++->set (quad.x0 * inverseScale, quad.y0 * inverseScale, quad.s0, quad.t0);
      vertices++->set (quad.x1 * inverseScale, quad.y1 * inverseScale, quad.s1, quad.t1);
      vertices++->set (quad.x1 * inverseScale, quad.y0 * inverseScale, quad.s1, quad.t0);
      vertices++->set (quad.x0 * inverseScale, quad.y0 * inverseScale, quad.s0, quad.t0);  // 3 = 0
      vertices++->set (quad.x0 * inverseScale, quad.y1 * inverseScale, quad.s0, quad.t1);
      vertices++->set (quad.x1 * inverseScale, quad.y1 * inverseScale, quad.s1, quad.t1);  // 5 = 1
      }
      //}}}
    else {
      //{{{  transform
      float x;
      float y;
      state->mTransform.point (quad.x0 * inverseScale, quad.y0 * inverseScale, x, y);
      vertices++->set (x, y, quad.s0, quad.t0);

      state->mTransform.point (quad.x1 * inverseScale, quad.y1 * inverseScale, x, y);
      vertices++->set (x, y, quad.s1, quad.t1);

      state->mTransform.point (quad.x1 * inverseScale, quad.y0 * inverseScale, x, y);
      vertices++->set (x, y, quad.s1, quad.t0);

      state->mTransform.point (quad.x0 * inverseScale, quad.y0 * inverseScale, x, y);  // 3 = 0
      vertices++->set (x, y, quad.s0, quad.t0);

      state->mTransform.point (quad.x0 * inverseScale, quad.y1 * inverseScale, x, y);
      vertices++->set (x, y, quad.s0, quad.t1);

      state->mTransform.point (quad.x1 * inverseScale, quad.y1 * inverseScale, x, y);  // 5 = 1
      vertices++->set (x, y, quad.s1, quad.t1);
      }
      //}}}
    }

  // calc vertices used
  numVertices = int(vertices - firstVertex);
  mVertices.trim (vertexIndex + numVertices);
  if (numVertices) {
    auto fillPaint = mStates[mNumStates - 1].fillPaint;
    fillPaint.innerColour.a *= mStates[mNumStates - 1].alpha;
    fillPaint.outerColour.a *= mStates[mNumStates - 1].alpha;
    fillPaint.mImageId = mFontTextureIds[mFontTextureIndex];
    renderText (vertexIndex, numVertices, fillPaint, mStates[mNumStates - 1].scissor);
    }
  flushAtlasTexture();

  return it.x;
  }
//}}}
//}}}
//{{{  image
//{{{
int cVg::createImageRGBA (int width, int height, int imageFlags, const uint8_t* data) {
  return createTexture (eTextureRgba, width, height, imageFlags, data, "rgba");
  }
//}}}
//{{{
int cVg::createImage (int imageFlags, uint8_t* data, int dataSize) {

  int width;
  int height;
  int numBytes;
  uint8_t* imageData = stbi_load_from_memory (data, dataSize, &width, &height, &numBytes, 4);
  if (imageData) {
    int imageId = createImageRGBA (width, height, imageFlags, imageData);
    stbi_image_free (imageData);
    return imageId;
    }
  else {
    printf ("Failed to load %s\n", stbi_failure_reason());
    return 0;
    }
  }
//}}}

//{{{
void cVg::updateImage (int image, const uint8_t* data) {

  int width;
  int height;
  getTextureSize (image, width, height);
  updateTexture (image, 0,0, width, height, data);
  }
//}}}

//{{{
bool cVg::deleteImage (int image) {

  for (int i = 0; i < mNumTextures; i++) {
    if (mTextures[i].id == image) {
      if ((mTextures[i].tex != 0) && ((mTextures[i].flags & eNoDelete) == 0))
        glDeleteTextures (1, &mTextures[i].tex);
      mTextures[i].reset();
      return true;
      }
    }

  return false;
  }
//}}}
//}}}
//{{{  shape
void cVg::setAlpha (float alpha) { mStates[mNumStates-1].alpha = alpha; }

void cVg::setFillColour (const sColourF& color) { mStates[mNumStates-1].fillPaint.set (color); }
void cVg::setStrokeColour (const sColourF& color) { mStates[mNumStates-1].strokePaint.set (color); }
void cVg::setStrokeWidth (float width) { mStates[mNumStates-1].strokeWidth = width; }
void cVg::setFringeWidth (float width) { mFringeWidth = max (0.f, min (10.f, width)); }

void cVg::setLineCap (eLineCap cap) { mStates[mNumStates-1].lineCap = cap; }
void cVg::setLineJoin (eLineCap join) { mStates[mNumStates-1].lineJoin = join; }
void cVg::setMiterLimit (float limit) { mStates[mNumStates-1].miterLimit = limit; }

//{{{
void cVg::setFillPaint (const sPaint& paint) {

  mStates[mNumStates-1].fillPaint = paint;
  mStates[mNumStates-1].fillPaint.mTransform.multiply (mStates[mNumStates-1].mTransform);
  }
//}}}
//{{{
void cVg::setStrokePaint (const sPaint& paint) {

  mStates[mNumStates-1].strokePaint = paint;
  mStates[mNumStates-1].strokePaint.mTransform.multiply (mStates[mNumStates-1].mTransform);
  }
//}}}

//{{{
cVg::sPaint cVg::setBoxGradient (cPointF p, cPointF size, float radius, float feather,
                              const sColourF& innerColour, const sColourF& outerColour) {

  sPaint paint;
  paint.mTransform.setTranslate (p + (size / 2.f));
  paint.extent[0] = 0.f;
  paint.extent[1] = 0.f;
  paint.radius = radius;
  paint.feather = max (1.f, feather);
  paint.innerColour = innerColour;
  paint.outerColour = outerColour;
  paint.mImageId = 0;

  return paint;
  }
//}}}
//{{{
cVg::sPaint cVg::setRadialGradient (cPointF centre, float innerRadius, float outerRadius,
                                 const sColourF& innerColour, const sColourF& outerColour) {

  sPaint paint;
  paint.mTransform.setTranslate (centre);
  float radius = (innerRadius + outerRadius) / 2.f;
  paint.extent[0] = radius;
  paint.extent[1] = radius;
  paint.radius = radius;
  paint.feather = max (1.0f, outerRadius - innerRadius);
  paint.innerColour = innerColour;
  paint.outerColour = outerColour;
  paint.mImageId = 0;

  return paint;
  }
//}}}
//{{{
cVg::sPaint cVg::setLinearGradient (cPointF start, cPointF end,
                                    const sColourF& innerColor, const sColourF& outerColor) {

  // Calculate transform aligned to the line
  cPointF diff = end - start;
  float distance = diff.magnitude();
  if (distance > 0.0001f)
    diff /= distance;
  else
    diff = { 0.f, 1.f };

  sPaint paint;
  paint.mTransform.set (diff.y, -diff.x, diff.x, diff.y, start.x - diff.x * kLarge, start.y - diff.y * kLarge);
  paint.extent[0] = kLarge;
  paint.extent[1] = kLarge + distance * 0.5f;
  paint.radius = 0.f;
  paint.feather = max (1.0f, distance);
  paint.innerColour = innerColor;
  paint.outerColour = outerColor;
  paint.mImageId = 0;

  return paint;
  }
//}}}
//{{{
cVg::sPaint cVg::setImagePattern (cPointF centre, cPointF size, float angle, int imageId, float alpha) {

  sPaint paint;
  paint.mTransform.setRotateTranslate (angle, centre);
  paint.extent[0] = size.x;
  paint.extent[1] = size.y;
  paint.radius = 0.f;
  paint.feather = 0.f;
  paint.innerColour = sColourF (1.f, 1.f, 1.f, alpha);
  paint.outerColour = sColourF (1.f, 1.f, 1.f, alpha);
  paint.mImageId = imageId;

  return paint;
  }
//}}}

void cVg::beginPath() { mShape.beginPath(); }
//{{{
void cVg::pathWinding (eWinding dir) {

  float values[] = { eWindingDirection, (float)dir };
  mShape.addCommand (values, sizeof(values)/4, mStates[mNumStates-1].mTransform);
  }
//}}}

//{{{
void cVg::moveTo (cPointF p) {

  float values[] = { eMoveTo, p.x, p.y };
  mShape.addCommand (values, sizeof(values)/4, mStates[mNumStates-1].mTransform);
  }
//}}}
//{{{
void cVg::lineTo (cPointF p) {

  float values[] = { eLineTo, p.x, p.y };
  mShape.addCommand (values, sizeof(values)/4, mStates[mNumStates-1].mTransform);
  }
//}}}
//{{{
void cVg::bezierTo (cPointF c1, cPointF c2, cPointF p) {

  float values[] = { eBezierTo, c1.x,c1.y, c2.x,c2.y, p.x,p.y };
  mShape.addCommand (values, sizeof(values)/4, mStates[mNumStates-1].mTransform);
  }
//}}}
//{{{
void cVg::quadTo (cPointF centre, cPointF p) {

  float x0 = mShape.getLastX();
  float y0 = mShape.getLastY();

  float values[] = { eBezierTo,
                     x0 + 2.0f / 3.0f * (centre.x - x0), y0 + 2.0f / 3.0f * (centre.y - y0),
                     p.x + 2.0f / 3.0f * (centre.x - p.x), p.y + 2.0f / 3.0f * (centre.y - p.y),
                     p.x, p.y };
  mShape.addCommand (values, sizeof(values)/4, mStates[mNumStates-1].mTransform);
  }
//}}}
//{{{
void cVg::arcTo (cPointF p1, cPointF p2, float radius) {

  float x0 = mShape.getLastX();
  float y0 = mShape.getLastY();

  if (!mShape.getNumCommands())
    return;

  // handle degenerate cases
  if (equal (x0,y0, p1.x,p1.y, kDistanceTolerance) ||
      equal (p1.x,p1.y, p2.x,p2.y, kDistanceTolerance) ||
      distPointSeg (p1.x,p1.y, x0,y0, p2.x,p2.y) < kDistanceTolerance*kDistanceTolerance ||
      radius < kDistanceTolerance) {
    lineTo (p1);
    return;
    }

  // calculate tangential circle to lines (x0,y0)-(x1,y1) and (x1,y1)-(x2,y2).
  float dx0 = x0 - p1.x;
  float dy0 = y0 - p1.y;
  float dx1 = p2.x - p1.x;
  float dy1 = p2.y - p1.y;
  normalize (dx0, dy0);
  normalize (dx1, dy1);
  float a = acosf (dx0 * dx1 + dy0 * dy1);
  float d = radius / tanf (a / 2.0f);

  if (d > 10000.0f) {
    lineTo (p1);
    return;
    }

  int dir;
  cPointF centre;
  float a0,a1;
  if (cross (dx0,dy0, dx1,dy1) > 0.0f) {
    dir = eClockWise;
    centre.x = p1.x + dx0 * d + dy0 * radius;
    centre.y = p1.y + dy0 * d - dx0 * radius;
    a0 = atan2f (dx0, -dy0);
    a1 = atan2f (-dx1, dy1);
    }
  else {
    dir = eCounterClockWise;
    centre.x = p1.x + dx0 * d - dy0 * radius;
    centre.y = p1.y + dy0 * d + dx0 * radius;
    a0 = atan2f (-dx0, dy0);
    a1 = atan2f (dx1, -dy1);
    }

  arc (centre, radius, a0, a1, dir);
  }
//}}}
//{{{
void cVg::arc (cPointF centre, float r, float a0, float a1, int dir) {

  int move = mShape.getNumCommands() ? eLineTo : eMoveTo;

  // clamp angles
  float da = a1 - a0;
  if (dir == eClockWise) {
    if (absf(da) >= k2Pi)
      da = k2Pi;
    else
      while (da < 0.0f)
        da += k2Pi;
    }
  else if (absf(da) >= k2Pi)
    da = -k2Pi;
  else
    while (da > 0.0f)
      da -= k2Pi;

  // split arc into max 90 degree segments
  int ndivs = max (1, min((int)(absf(da) / kPiDiv2 + 0.5f), 5));
  float hda = (da / (float)ndivs) / 2.0f;
  float kappa = absf (4.0f / 3.0f * (1.0f - cosf (hda)) / sinf (hda));

  if (dir == eCounterClockWise)
    kappa = -kappa;

  float px = 0;
  float py = 0;
  float ptanx = 0;
  float ptany = 0;
  int numValues = 0;
  float values[3 + 5*7 + 100];
  for (int i = 0; i <= ndivs; i++) {
    float a = a0 + da * (i / (float)ndivs);
    float dx = cosf (a);
    float dy = sinf (a);
    float x = centre.x + dx * r;
    float y = centre.y + dy * r;
    float tanx = -dy * r * kappa;
    float tany = dx * r * kappa;

    if (i == 0) {
      values[numValues++] = (float)move;
      values[numValues++] = x;
      values[numValues++] = y;
      }
    else {
      values[numValues++] = eBezierTo;
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
void cVg::rect (cPointF p, cPointF size) {

  float values[] = { eMoveTo, p.x, p.y,
                     eLineTo, p.x, p.y + size.y,
                     eLineTo, p.x + size.x, p.y + size.y,
                     eLineTo, p.x + size.x, p.y,
                     eClose };
  mShape.addCommand (values, sizeof(values)/4, mStates[mNumStates-1].mTransform);
  }
//}}}
//{{{
void cVg::roundedRectVarying (cPointF p, cPointF size, float tlRadius, float trRadius, float brRadius, float blRadius) {

  if ((tlRadius < 0.1f) && (trRadius < 0.1f) && (brRadius < 0.1f) && (blRadius < 0.1f))
    rect (p, size);

  else {
    float halfw = absf (size.x) * 0.5f;
    float halfh = absf (size.y) * 0.5f;

    float rxTL = min (tlRadius, halfw) * signf (size.x);
    float ryTL = min (tlRadius, halfh) * signf (size.y);
    float rxTR = min (trRadius, halfw) * signf (size.x);
    float ryTR = min (trRadius, halfh) * signf (size.y);
    float rxBR = min (brRadius, halfw) * signf (size.x);
    float ryBR = min (brRadius, halfh) * signf (size.y);
    float rxBL = min (blRadius, halfw) * signf (size.x);
    float ryBL = min (blRadius, halfh) * signf (size.y);

    float values[] = { eMoveTo, p.x, p.y + ryTL,
                       eLineTo, p.x, p.y + size.y - ryBL,
                       eBezierTo, p.x, p.y + size.y - ryBL*(1 - kAppA90),
                                  p.x + rxBL*(1 - kAppA90), p.y + size.y,
                                  p.x + rxBL, p.y + size.y,
                       eLineTo, p.x + size.x - rxBR, p.y + size.y,
                       eBezierTo, p.x + size.x - rxBR*(1 - kAppA90), p.y + size.y,
                                  p.x + size.x,p.y + size.y - ryBR*(1 - kAppA90),
                                  p.x + size.x, p.y + size.y - ryBR,
                       eLineTo, p.x + size.x, p.y + ryTR,
                       eBezierTo, p.x + size.x, p.y + ryTR*(1 - kAppA90),
                                  p.x + size.x - rxTR*(1 - kAppA90), p.y,
                                  p.x + size.x - rxTR, p.y,
                       eLineTo, p.x + rxTL, p.y,
                       eBezierTo, p.x + rxTL*(1 - kAppA90), p.y,
                                  p.x, p.y + ryTL*(1 - kAppA90),
                                  p.x, p.y + ryTL,
                       eClose };
    mShape.addCommand (values, sizeof(values)/4, mStates[mNumStates-1].mTransform);
    }
  }
//}}}
//{{{
void cVg::roundedRect (cPointF p, cPointF size, float radius) {
  roundedRectVarying (p, size, radius, radius, radius, radius);
  }
//}}}
//{{{
void cVg::ellipse (cPointF centre, cPointF radius) {

  float values[] = { eMoveTo, centre.x - radius.x, centre.y,
                     eBezierTo, centre.x - radius.x, centre.y + radius.y*kAppA90, centre.x - radius.x*kAppA90,
                                centre.y + radius.y, centre.x, centre.y + radius.y,
                     eBezierTo, centre.x + radius.x*kAppA90, centre.y + radius.y, centre.x + radius.x,
                                centre.y + radius.y*kAppA90, centre.x + radius.x, centre.y,
                     eBezierTo, centre.x + radius.x, centre.y - radius.y*kAppA90, centre.x + radius.x*kAppA90,
                                centre.y - radius.y, centre.x, centre.y - radius.y,
                     eBezierTo, centre.x - radius.x*kAppA90, centre.y - radius.y, centre.x - radius.x,
                                centre.y - radius.y*kAppA90, centre.x - radius.x, centre.y,
                     eClose };
  mShape.addCommand (values, sizeof(values)/4, mStates[mNumStates-1].mTransform);
  }
//}}}
//{{{
void cVg::circle (cPointF centre, float radius) {
  ellipse (centre, cPointF(radius,radius));
  }
//}}}

//{{{
void cVg::closePath() {
  float values[] = { eClose };
  mShape.addCommand (values, sizeof(values)/4, mStates[mNumStates-1].mTransform);
  }
//}}}

//{{{
void cVg::fill() {

  mShape.flattenPaths();
  mShape.expandFill (mVertices, mDrawEdges ? mFringeWidth : 0.0f, eMiter, 2.4f, mFringeWidth);

  sPaint fillPaint = mStates[mNumStates-1].fillPaint;
  fillPaint.innerColour.a *= mStates[mNumStates-1].alpha;
  fillPaint.outerColour.a *= mStates[mNumStates-1].alpha;
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
    strokePaint.innerColour.a *= alpha * alpha;
    strokePaint.outerColour.a *= alpha * alpha;
    strokeWidth = mFringeWidth;
    }

  mShape.expandStroke (mVertices, mDrawEdges ? (strokeWidth + mFringeWidth) * 0.5f : strokeWidth * 0.5f,
                       state->lineCap, state->lineJoin, state->miterLimit, mFringeWidth);

  strokePaint.innerColour.a *= state->alpha;
  strokePaint.outerColour.a *= state->alpha;
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
  sPaint fillPaint = mStates[mNumStates-1].fillPaint;
  fillPaint.innerColour.a *= mStates[mNumStates-1].alpha;
  fillPaint.outerColour.a *= mStates[mNumStates-1].alpha;
  renderTriangles (vertexIndex, numVertices, fillPaint, mStates[mNumStates-1].scissor);
  }
//}}}
//}}}
//{{{  frame
//{{{
string cVg::getFrameStats() {
  return "vertices:" + dec (mVertices.getNumVertices()) +
         " drawArrays:" + dec (mDrawArrays);
  }
//}}}

//{{{
void cVg::beginFrame (int width, int height, float devicePixelRatio) {

  mNumStates = 0;
  saveState();
  resetState();

  setDevicePixelRatio (devicePixelRatio);

  mViewport[0] = (float)width;
  mViewport[1] = (float)height;
  }
//}}}
//{{{
void cVg::endFrame() {

  auto state = &mStates[mNumStates-1];
  renderFrame (mVertices, state->composite);

  if (mFontTextureIndex) {
    // delete fontImages smaller than current one
    int fontTextureId = mFontTextureIds[mFontTextureIndex];
    if (fontTextureId == 0)
      return;

    int width = 0;
    int height = 0;
    getTextureSize (fontTextureId, width, height);

    int j = 0;
    for (int i = 0; i < mFontTextureIndex; i++) {
      if (mFontTextureIds[i] != 0) {
        int nWidth = 0;
        int nHeight = 0;
        getTextureSize (mFontTextureIds[i], nWidth, nHeight);
        if (nWidth < width || nHeight < height)
          deleteImage (mFontTextureIds[i]);
        else
          mFontTextureIds[j++] = mFontTextureIds[i];
        }
      }

    // make current fontImage first
    mFontTextureIds[j++] = mFontTextureIds[0];
    mFontTextureIds[0] = fontTextureId;
    mFontTextureIndex = 0;

    // clear fontImage after j
    for (int i = j; i < kMaxFontTextures; i++)
      mFontTextureIds[i] = 0;
    }

  mVertices.reset();
  }
//}}}
//}}}

// private
//{{{  cVg::cVertices
//{{{
cVg::cVertices::cVertices() {
  mVertices = (sVertex*)malloc (kInitNumVertices * sizeof(sVertex));
  mNumAllocatedVertices = kInitNumVertices;
  }
//}}}
//{{{
cVg::cVertices::~cVertices() {
  free (mVertices);
  }
//}}}

//{{{
void cVg::cVertices::reset() {
  mNumVertices = 0;
  }
//}}}

//{{{
int cVg::cVertices::alloc (int numVertices) {
// allocate n vertices and return index of first

  if (mNumVertices + numVertices > mNumAllocatedVertices) {
    mNumAllocatedVertices = max (mNumVertices + numVertices, 4096) + mNumAllocatedVertices/2; // 1.5x Overallocate
    cLog::log (LOGINFO2, "realloc vertices " + dec(mNumAllocatedVertices));
    mVertices = (sVertex*)realloc (mVertices, mNumAllocatedVertices * sizeof(sVertex));
    }

  int firstVertexIndex = mNumVertices;
  mNumVertices += numVertices;
  return firstVertexIndex;
  }
//}}}
//{{{
void cVg::cVertices::trim (int numVertices) {
// trim vertices used to numVertices

  if (numVertices > mNumVertices)
    printf ("trimVertices overflowed %d %d\n", numVertices, mNumVertices);

  mNumVertices = numVertices;
  }
//}}}
//}}}
//{{{  cVg::cShape
//{{{
cVg::cShape::cShape() {
  mPaths = (sPath*)malloc (sizeof(sPath) * kInitPathSize);
  mNumAllocatedPaths = kInitPathSize;

  mPoints = (sShapePoint*)malloc (sizeof(sShapePoint) * kInitPointsSize);
  mNumAllocatedPoints = kInitPointsSize;

  mNumAllocatedCommands = kInitCommandsSize;
  mCommands = (float*)malloc (sizeof(float) * kInitCommandsSize);
  }
//}}}
//{{{
cVg::cShape::~cShape() {
  free (mPoints);
  free (mPaths);
  free (mCommands);
  }
//}}}

//{{{
void cVg::cShape::addCommand (float* values, int numValues, cTransform& transform) {

  if (((int)values[0] == eMoveTo) ||
      ((int)values[0] == eLineTo) ||
      ((int)values[0] == eBezierTo)) {
    mLastX = values[numValues-2];
    mLastY = values[numValues-1];
    }

  // transform commands
  if (!transform.mIdentity) {
    //{{{  transform points
    for (int i = 0; i < numValues;) {
      switch ((int)values[i++]) {
        case eMoveTo:
        case eLineTo:
          if (i+1 < numValues) // ensure enough values supplied for command type
            transform.point (values[i], values[i+1]);
          else
            cLog::log (LOGERROR, "not enough values in addCommand");
          i += 2;
          break;

        case eBezierTo:
          if (i+5 < numValues) { // ensure enough values supplied for command type
            transform.point (values[i], values[i+1]);
            transform.point (values[i+2], values[i+3]);
            transform.point (values[i+4], values[i+5]);
            }
          else
            cLog::log (LOGERROR, "not enough values in addCommand");

          i += 6;
          break;

        case eWindingDirection:
          i += 1;
          break;

        case eClose:;
        }
      }
    }
    //}}}

  // copy values to shapeCommands
  if (mNumCommands + numValues > mNumAllocatedCommands) {
    // not enough shapeCommands, expand shapeCommands size by 3/2
    mNumAllocatedCommands = mNumCommands + numValues + mNumAllocatedCommands / 2;
    mCommands = (float*)realloc (mCommands, sizeof(float) * mNumAllocatedCommands);
    cLog::log (LOGINFO2, "realloc commmands " + dec(mNumAllocatedCommands));
    }

  memcpy (&mCommands[mNumCommands], values, numValues * sizeof(float));
  mNumCommands += numValues;
  }
//}}}

//{{{
int cVg::cShape::getNumVertices() {

  int numVertices = 0;
  for (int i = 0; i < mNumPaths; i++)
    numVertices += mPaths[i].mNumPoints;

  return numVertices;
  }
//}}}

//{{{
void cVg::cShape::beginPath() {

  mNumPaths = 0;
  mNumPoints = 0;
  mNumCommands = 0;
  }
//}}}
//{{{
void cVg::cShape::flattenPaths() {

  commandsToPaths();

  // Calculate the direction and length of line segments.
  mBounds[0] = 1e6f;
  mBounds[1] = 1e6f;
  mBounds[2] = -1e6f;
  mBounds[3] = -1e6f;

  for (auto path = mPaths; path < mPaths + mNumPaths; path++) {
    // if firstPoint = lastPoint, remove the last, mark as closed
    auto points = &mPoints[path->mFirstPointIndex];
    auto point0 = &points[path->mNumPoints-1];
    auto point1 = &points[0];
    if (equal (point0->x, point0->y, point1->x, point1->y, kDistanceTolerance)) {
      path->mNumPoints--;
      point0 = &points[path->mNumPoints-1];
      path->mClosed = true;
      }

    // enforce winding direction
    if (path->mNumPoints > 2) {
     float area = polyArea (points, path->mNumPoints);
      if ((path->mWinding == eCounterClockWise) && (area < 0.0f))
        polyReverse (points, path->mNumPoints);
      if ((path->mWinding == eClockWise) && (area > 0.0f))
        polyReverse (points, path->mNumPoints);
      }

    // calc segment direction, length, update shape bounds
    for (int i = 0; i < path->mNumPoints; i++) {
      point0->dx = point1->x - point0->x;
      point0->dy = point1->y - point0->y;
      point0->len = normalize (point0->dx, point0->dy);

      mBounds[0] = min (mBounds[0], point0->x);
      mBounds[1] = min (mBounds[1], point0->y);
      mBounds[2] = max (mBounds[2], point0->x);
      mBounds[3] = max (mBounds[3], point0->y);

      // advance
      point0 = point1++;
      }
    }
  }
//}}}
//{{{
void cVg::cShape::calculateJoins (float w, int lineJoin, float miterLimit) {

  float iw = 0.0f;
  if (w > 0.0f)
    iw = 1.0f / w;

  // Calculate which joins needs extra vertices to append, and gather vertex count.
  for (auto path = mPaths; path < mPaths + mNumPaths; path++) {
    auto points = &mPoints[path->mFirstPointIndex];
    auto point0 = &points[path->mNumPoints-1];
    auto point1 = &points[0];
    int nleft = 0;
    path->mNumBevel = 0;

    for (int j = 0; j < path->mNumPoints; j++) {
      float dlx0, dly0, dlx1, dly1, dmr2, cross, limit;
      dlx0 = point0->dy;
      dly0 = -point0->dx;
      dlx1 = point1->dy;
      dly1 = -point1->dx;

      // Calculate extrusions
      point1->dmx = (dlx0 + dlx1) * 0.5f;
      point1->dmy = (dly0 + dly1) * 0.5f;
      dmr2 = point1->dmx*point1->dmx + point1->dmy*point1->dmy;
      if (dmr2 > 0.000001f) {
        float scale = 1.0f / dmr2;
        if (scale > 600.0f) {
          scale = 600.0f;
          }
        point1->dmx *= scale;
        point1->dmy *= scale;
        }

      // Clear flags, but keep the corner.
      point1->flags = (point1->flags & sShapePoint::ePtCORNER) ? sShapePoint::ePtCORNER : 0;

      // Keep track of left turns.
      cross = point1->dx * point0->dy - point0->dx * point1->dy;
      if (cross > 0.0f) {
        nleft++;
        point1->flags |= sShapePoint::ePtLEFT;
        }

      // Calculate if we should use bevel or miter for inner join.
      limit = max (1.01f, min (point0->len, point1->len) * iw);
      if ((dmr2 * limit * limit) < 1.0f)
        point1->flags |= sShapePoint::ePtINNERBEVEL;

      // Check to see if the corner needs to be beveled.
      if (point1->flags & sShapePoint::ePtCORNER) {
        if (((dmr2 * miterLimit * miterLimit) < 1.0f) || (lineJoin == eBevel) || (lineJoin == eRound)) {
          point1->flags |= sShapePoint::ePtBEVEL;
          }
        }

      if ((point1->flags & (sShapePoint::ePtBEVEL | sShapePoint::ePtINNERBEVEL)) != 0)
        path->mNumBevel++;
      point0 = point1++;
      }

    path->mConvex = (nleft == path->mNumPoints);
    }
  }
//}}}
//{{{
void cVg::cShape::expandFill (cVertices& vertices, float w, eLineCap lineJoin, float miterLimit, float fringeWidth) {

  bool fringe = w > 0.0f;

  calculateJoins (w, lineJoin, miterLimit);

  // calc max numVertices
  int numVertices = 0;
  for (auto path = mPaths; path < mPaths + mNumPaths; path++) {
    numVertices += path->mNumPoints + path->mNumBevel + 1;
    if (fringe)
      numVertices += (path->mNumPoints + path->mNumBevel * 5 + 1) * 2; // plus one for loop
    }

  bool convex = (mNumPaths == 1) && mPaths[0].mConvex;
  numVertices += 4;

  int vertexIndex = vertices.alloc (numVertices);
  auto vertexPtr = vertices.getVertexPtr (vertexIndex);

  // setup bounds vertices
  mBoundsVertexIndex = vertexIndex;
  vertexPtr++->set (mBounds[0], mBounds[1], 0.5f, 1.0f);
  vertexPtr++->set (mBounds[0], mBounds[3], 0.5f, 1.0f);
  vertexPtr++->set (mBounds[2], mBounds[1], 0.5f, 1.0f);
  vertexPtr++->set (mBounds[2], mBounds[3], 0.5f, 1.0f);
  vertexIndex += 4;

  for (auto path = mPaths; path < mPaths + mNumPaths; path++) {
    auto points = &mPoints[path->mFirstPointIndex];

    // Calculate shape vertices.
    auto firstVertexPtr = vertexPtr;
    path->mPathVertices.mFirstFillVertexIndex = vertexIndex;

    float woff = 0.5f * fringeWidth;
    if (fringe) {
      //{{{  looping
      auto point0 = &points[path->mNumPoints-1];
      auto point1 = &points[0];

      for (int j = 0; j < path->mNumPoints; ++j) {
        if (point1->flags & sShapePoint::ePtBEVEL) {
          float dlx0 = point0->dy;
          float dly0 = -point0->dx;
          float dlx1 = point1->dy;
          float dly1 = -point1->dx;
          if (point1->flags & sShapePoint::ePtLEFT) {
            float lx = point1->x + point1->dmx * woff;
            float ly = point1->y + point1->dmy * woff;
            vertexPtr++->set (lx, ly, 0.5f,1);
            }
          else {
            float lx0 = point1->x + dlx0 * woff;
            float ly0 = point1->y + dly0 * woff;
            float lx1 = point1->x + dlx1 * woff;
            float ly1 = point1->y + dly1 * woff;
            vertexPtr++->set (lx0, ly0, 0.5f,1);
            vertexPtr++->set (lx1, ly1, 0.5f,1);
            }
          }
        else
          vertexPtr++->set (point1->x + (point1->dmx * woff), point1->y + (point1->dmy * woff), 0.5f, 1);

        point0 = point1++;
        }
      }
      //}}}
    else
      for (int j = 0; j < path->mNumPoints; ++j)
        vertexPtr++->set (points[j].x, points[j].y, 0.5f, 1);

    int numVertices = (int)(vertexPtr - firstVertexPtr);
    path->mPathVertices.mNumFillVertices = numVertices;
    vertexIndex += numVertices;

    if (fringe) {
      //{{{  calculate fringe
      float lw = w + woff;
      float rw = w - woff;
      float lu = 0;
      float ru = 1;

      firstVertexPtr = vertexPtr;
      path->mPathVertices.mFirstStrokeVertexIndex = vertexIndex;

      // Create only half a fringe for convex shapes so that the shape can be rendered without stenciling.
      if (convex) {
        lw = woff;  // This should generate the same vertex as fill inset above.
        lu = 0.5f;  // Set outline fade at middle.
        }

      // Looping
      sShapePoint* point0 = &points[path->mNumPoints-1];
      sShapePoint* point1 = &points[0];
      for (int j = 0; j < path->mNumPoints; ++j) {
        if ((point1->flags & (sShapePoint::ePtBEVEL | sShapePoint::ePtINNERBEVEL)) != 0)
          vertexPtr = bevelJoin (vertexPtr, point0, point1, lw, rw, lu, ru, fringeWidth);
        else {
          vertexPtr++->set (point1->x + (point1->dmx * lw), point1->y + (point1->dmy * lw), lu, 1);
          vertexPtr++->set (point1->x - (point1->dmx * rw), point1->y - (point1->dmy * rw), ru, 1);
          }
        point0 = point1++;
        }

      // Loop it
      vertexPtr++->set (firstVertexPtr->mX, firstVertexPtr->mY, lu, 1);
      vertexPtr++->set ((firstVertexPtr+1)->mX, (firstVertexPtr+1)->mY, ru, 1);

      int numVertices = (int)(vertexPtr - firstVertexPtr);
      path->mPathVertices.mNumStrokeVertices = numVertices;
      vertexIndex += numVertices;
      }
      //}}}
    else {
      path->mPathVertices.mFirstStrokeVertexIndex = 0;
      path->mPathVertices.mNumStrokeVertices = 0;
      }
    }

  vertices.trim (vertexIndex);
  }
//}}}
//{{{
void cVg::cShape::triangleFill (cVertices& vertices, int& vertexIndex, int& numVertices) {

  commandsToPaths();

  // estimate numVertices, 6 vertices per quad path
  numVertices = mNumPaths * 6;

  // alloc and set vertices
  vertexIndex = vertices.alloc (numVertices);
  auto vertexPtr = vertices.getVertexPtr (vertexIndex);
  auto firstVertexPtr = vertexPtr;
  for (int i = 0; i < mNumPaths; i++)
    if (mPaths[i].mNumPoints == 4) {
      auto point = &mPoints[mPaths[i].mFirstPointIndex];
      vertexPtr++->set (point[0].x, point[0].y, 0.5f, 1.0f);
      vertexPtr++->set (point[1].x, point[1].y, 0.5f, 1.0f);
      vertexPtr++->set (point[2].x, point[2].y, 0.5f, 1.0f);
      vertexPtr++->set (point[0].x, point[0].y, 0.5f, 1.0f); // 3 = 0
      vertexPtr++->set (point[2].x, point[2].y, 0.5f, 1.0f); // 5 = 2
      vertexPtr++->set (point[3].x, point[3].y, 0.5f, 1.0f);
      }

  numVertices = (int)(vertexPtr - firstVertexPtr);
  vertices.trim (vertexIndex + numVertices);
  }
//}}}
//{{{
void cVg::cShape::expandStroke (cVertices& vertices, float w, eLineCap lineCap, eLineCap lineJoin, float miterLimit, float fringeWidth) {

  calculateJoins (w, lineJoin, miterLimit);

  // Calculate divisions per half circle.
  int ncap = curveDivs (w, kPi, 0.25f);
  //{{{  calculate max vertex usage.
  int numVertices = 0;
  for (auto path = mPaths; path < mPaths + mNumPaths; path++) {
    bool loop = path->mClosed;
    if (lineJoin == eRound)
      numVertices += (path->mNumPoints + path->mNumBevel * (ncap+2) + 1) * 2; // plus one for loop
    else
      numVertices += (path->mNumPoints + path->mNumBevel*5 + 1) * 2; // plus one for loop
    if (!loop) {
      // space for caps
      if (lineCap == eRound)
        numVertices += (ncap*2 + 2)*2;
      else
        numVertices += (3+3)*2;
      }
    }
  //}}}

  int vertexIndex = vertices.alloc (numVertices);
  auto vertexPtr = vertices.getVertexPtr (vertexIndex);

  for (auto path = mPaths; path < mPaths + mNumPaths; path++) {
    auto points = &mPoints[path->mFirstPointIndex];

    path->mPathVertices.mFirstFillVertexIndex = 0;
    path->mPathVertices.mNumFillVertices = 0;

    // Calculate fringe or stroke
    bool loop = path->mClosed;

    auto firstVertexPtr = vertexPtr;
    path->mPathVertices.mFirstStrokeVertexIndex = vertexIndex;

    sShapePoint* point0;
    sShapePoint* point1;
    int s, e;
    if (loop) {
      //{{{  looping
      point0 = &points[path->mNumPoints-1];
      point1 = &points[0];
      s = 0;
      e = path->mNumPoints;
      }
      //}}}
    else {
      //{{{  add cap
      point0 = &points[0];
      point1 = &points[1];
      s = 1;
      e = path->mNumPoints-1;
      }
      //}}}

    float dx, dy;
    if (!loop) {
      //{{{  add cap
      dx = point1->x - point0->x;
      dy = point1->y - point0->y;
      normalize (dx, dy);
      if (lineCap == eButt)
        vertexPtr = buttCapStart (vertexPtr, point0, dx, dy, w, -fringeWidth*0.5f, fringeWidth);
      else if (lineCap == eButt || lineCap == eSquare)
        vertexPtr = buttCapStart (vertexPtr, point0, dx, dy, w, w-fringeWidth, fringeWidth);
      else if (lineCap == eRound)
        vertexPtr = roundCapStart (vertexPtr, point0, dx, dy, w, ncap, fringeWidth);
      }
      //}}}

    for (int j = s; j < e; ++j) {
      if ((point1->flags & (sShapePoint::ePtBEVEL | sShapePoint::ePtINNERBEVEL)) != 0) {
        if (lineJoin == eRound)
          vertexPtr = roundJoin (vertexPtr, point0, point1, w, w, 0, 1, ncap, fringeWidth);
        else
          vertexPtr = bevelJoin (vertexPtr, point0, point1, w, w, 0, 1, fringeWidth);
        }
      else {
        vertexPtr++->set (point1->x + (point1->dmx * w), point1->y + (point1->dmy * w), 0, 1);
        vertexPtr++->set (point1->x - (point1->dmx * w), point1->y - (point1->dmy * w), 1, 1);
        }
      point0 = point1++;
      }

    if (loop) {
      //{{{  loop it
      vertexPtr++->set (firstVertexPtr->mX, firstVertexPtr->mY, 0, 1);
      vertexPtr++->set ((firstVertexPtr+1)->mX, (firstVertexPtr+1)->mY, 1, 1);
      }
      //}}}
    else {
      //{{{  add cap
      dx = point1->x - point0->x;
      dy = point1->y - point0->y;
      normalize (dx, dy);

      if (lineCap == eButt)
        vertexPtr = buttCapEnd (vertexPtr, point1, dx, dy, w, -fringeWidth * 0.5f, fringeWidth);
      else if (lineCap == eButt || lineCap == eSquare)
        vertexPtr = buttCapEnd (vertexPtr, point1, dx, dy, w, w - fringeWidth, fringeWidth);
      else if (lineCap == eRound)
        vertexPtr = roundCapEnd (vertexPtr, point1, dx, dy, w, ncap, fringeWidth);
      }
      //}}}

    int numVertices = (int)(vertexPtr - firstVertexPtr);
    path->mPathVertices.mNumStrokeVertices = numVertices;
    vertexIndex += numVertices;
    }

  vertices.trim (vertexIndex);
  }
//}}}

//{{{
float cVg::cShape::normalize (float& x, float& y) {

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
int cVg::cShape::curveDivs (float r, float arc, float tol) {

  float da = acosf (r / (r + tol)) * 2.0f;
  return max (2, (int)ceilf(arc / da));
  }
//}}}
//{{{
float cVg::cShape::polyArea (sShapePoint* points, int mNumPoints) {

  float area = 0;
  for (int i = 2; i < mNumPoints; i++)
    area += (points[i].x - points[0].x) * (points[i-1].y - points[0].y) -
            (points[i-1].x - points[0].x) *  (points[i].y - points[0].y);

  return area * 0.5f;
  }
//}}}
//{{{
void cVg::cShape::polyReverse (sShapePoint* points, int numPoints) {

  int i = 0;
  int j = numPoints - 1;
  while (i < j) {
    auto tempPoint = points[i];
    points[i] = points[j];
    points[j] = tempPoint;
    i++;
    j--;
    }
  }
//}}}

//{{{
cVg::sPath* cVg::cShape::lastPath() {
  return mNumPaths ? &mPaths[mNumPaths-1] : NULL;
  }
//}}}
//{{{
void cVg::cShape::lastPathWinding (eWinding winding) {

  auto path = lastPath();
  if (path == NULL)
    return;
  path->mWinding = winding;  }
//}}}
//{{{
void cVg::cShape::closeLastPath() {

  auto path = lastPath();
  if (path == NULL)
    return;
  path->mClosed = true;
  }
//}}}
//{{{
void cVg::cShape::addPath() {

  if (mNumPaths+1 > mNumAllocatedPaths) {
    mNumAllocatedPaths = mNumPaths+1 + mNumAllocatedPaths / 2;
    cLog::log (LOGINFO2, "realloc paths " + dec(mNumAllocatedPaths));
    mPaths = (sPath*)realloc (mPaths, sizeof(sPath)* mNumAllocatedPaths);
    }

  auto path = &mPaths[mNumPaths];
  path->mNumPoints = 0;
  path->mFirstPointIndex = mNumPoints;
  path->mWinding = eCounterClockWise;
  path->mConvex = false;
  path->mClosed = false;
  path->mNumBevel = 0;
  path->mPathVertices.mNumFillVertices = 0;
  path->mPathVertices.mFirstFillVertexIndex = 0;
  path->mPathVertices.mNumStrokeVertices = 0;
  path->mPathVertices.mFirstStrokeVertexIndex = 0;;

  mNumPaths++;
  }
//}}}

//{{{
void cVg::cShape::tesselateBezier (float x1, float y1, float x2, float y2,
                                   float x3, float y3, float x4, float y4,
                                   int level, sShapePoint::eFlags type) {

  if (level > 10)
    return;

  float x12 = (x1+x2) * 0.5f;
  float y12 = (y1+y2) * 0.5f;
  float x23 = (x2+x3) * 0.5f;
  float y23 = (y2+y3) * 0.5f;
  float x34 = (x3+x4) * 0.5f;
  float y34 = (y3+y4) * 0.5f;
  float x123 = (x12+x23) * 0.5f;
  float y123 = (y12+y23) * 0.5f;

  float dx = x4 - x1;
  float dy = y4 - y1;
  float d2 = absf (((x2 - x4) * dy - (y2 - y4) * dx));
  float d3 = absf (((x3 - x4) * dy - (y3 - y4) * dx));

  if ((d2 + d3)*(d2 + d3) < kTesselateTolerance * (dx*dx + dy*dy)) {
    addPoint (x4, y4, type);
    return;
    }

  //if (absf(x1+x3-x2-x2) + absf(y1+y3-y2-y2) + absf(x2+x4-x3-x3) + absf(y2+y4-y3-y3) < tessTol) {
  //  addPoint(context, x4, y4, type);
  //  return;
  //  }

  float x234 = (x23+x34)*0.5f;
  float y234 = (y23+y34)*0.5f;
  float x1234 = (x123+x234)*0.5f;
  float y1234 = (y123+y234)*0.5f;

  tesselateBezier (x1,y1, x12,y12, x123,y123, x1234,y1234, level+1, sShapePoint::ePtNONE);
  tesselateBezier (x1234,y1234, x234,y234, x34,y34, x4,y4, level+1, type);
  }
//}}}

//{{{
cVg::cShape::sShapePoint* cVg::cShape::lastPoint() {
  return mNumPoints ? &mPoints[mNumPoints-1] : NULL;
  }
//}}}
//{{{
void cVg::cShape::addPoint (float x, float y, sShapePoint::eFlags flags) {

  auto path = lastPath();
  if (path == NULL)
    return;

  if ((path->mNumPoints > 0) && (mNumPoints > 0)) {
    auto point = lastPoint();
    if (equal (point->x, point->y, x, y, kDistanceTolerance)) {
      point->flags |= flags;
      return;
      }
    }

  // allocate point in pathCache
  if (mNumPoints+1 > mNumAllocatedPoints) {
    mNumAllocatedPoints = mNumPoints+1 + mNumAllocatedPoints / 2;
    cLog::log (LOGINFO2, "realloc points " + dec(mNumAllocatedPoints));
    mPoints = (sShapePoint*)realloc (mPoints, sizeof(sShapePoint)*mNumAllocatedPoints);
    }

  // set point
  auto point = &mPoints[mNumPoints];
  point->x = x;
  point->y = y;
  point->flags = flags;

  mNumPoints++;
  path->mNumPoints++;
  }
//}}}

//{{{
void cVg::cShape::commandsToPaths() {
// convert shapeCommands to paths,points

  auto command = mCommands;
  while (command < mCommands + mNumCommands) {
    switch ((int)*command++) {
      case eMoveTo:
        addPath();

      case eLineTo:
        addPoint (*command, *(command+1), sShapePoint::ePtCORNER);
        command += 2;
        break;

      case eBezierTo: {
        auto last = lastPoint();
        if (last != NULL)
          tesselateBezier (last->x, last->y, *command, *(command+1), *(command+2), *(command+3), *(command+4), *(command+5), 0, sShapePoint::ePtCORNER);
        command += 6;
        break;
        }

      case eWindingDirection:
        lastPathWinding ((eWinding)(int)*command++);
        break;

      case eClose:
        closeLastPath();
      }
    }
  }
//}}}

//{{{
void cVg::cShape::chooseBevel (int bevel, sShapePoint* p0, sShapePoint* p1, float w, float* x0, float* y0, float* x1, float* y1) {

  if (bevel) {
    *x0 = p1->x + p0->dy * w;
    *y0 = p1->y - p0->dx * w;
    *x1 = p1->x + p1->dy * w;
    *y1 = p1->y - p1->dx * w;
    }
  else {
    *x0 = p1->x + p1->dmx * w;
    *y0 = p1->y + p1->dmy * w;
    *x1 = p1->x + p1->dmx * w;
    *y1 = p1->y + p1->dmy * w;
    }
  }
//}}}
//{{{
cVg::sVertex* cVg::cShape::roundJoin (sVertex* vertexPtr, sShapePoint* point0, sShapePoint* point1,
                                      float lw, float rw, float lu, float ru, int ncap, float fringe) {

  float dlx0 = point0->dy;
  float dly0 = -point0->dx;
  float dlx1 = point1->dy;
  float dly1 = -point1->dx;

  if (point1->flags & sShapePoint::ePtLEFT) {
    float lx0,ly0,lx1,ly1,a0,a1;
    chooseBevel (point1->flags & sShapePoint::ePtINNERBEVEL, point0, point1, lw, &lx0,&ly0, &lx1,&ly1);
    a0 = atan2f (-dly0, -dlx0);
    a1 = atan2f (-dly1, -dlx1);
    if (a1 > a0)
      a1 -= k2Pi;

    vertexPtr++->set (lx0, ly0, lu,1);
    vertexPtr++->set (point1->x - dlx0*rw, point1->y - dly0*rw, ru,1);

    int n = clampi ((int)ceilf (((a0 - a1) / kPi) * ncap), 2, ncap);
    for (int i = 0; i < n; i++) {
      float u = i/(float)(n-1);
      float a = a0 + u*(a1-a0);
      float rx = point1->x + cosf(a) * rw;
      float ry = point1->y + sinf(a) * rw;
      vertexPtr++->set (point1->x, point1->y, 0.5f,1);
      vertexPtr++->set (rx, ry, ru,1);
      }

    vertexPtr++->set (lx1, ly1, lu,1);
    vertexPtr++->set (point1->x - dlx1*rw, point1->y - dly1*rw, ru,1);
    }

  else {
    float rx0,ry0,rx1,ry1,a0,a1;
    chooseBevel (point1->flags & sShapePoint::ePtINNERBEVEL, point0, point1, -rw, &rx0,&ry0, &rx1,&ry1);
    a0 = atan2f (dly0, dlx0);
    a1 = atan2f (dly1, dlx1);
    if (a1 < a0)
      a1 += k2Pi;

    vertexPtr++->set (point1->x + dlx0*rw, point1->y + dly0*rw, lu,1);
    vertexPtr++->set (rx0, ry0, ru,1);

    int n = clampi ((int)ceilf(((a1 - a0) / kPi) * ncap), 2, ncap);
    for (int i = 0; i < n; i++) {
      float u = i/(float)(n-1);
      float a = a0 + u*(a1-a0);
      float lx = point1->x + cosf(a) * lw;
      float ly = point1->y + sinf(a) * lw;
      vertexPtr++->set (lx, ly, lu,1);
      vertexPtr++->set (point1->x, point1->y, 0.5f,1);
      }

    vertexPtr++->set (point1->x + dlx1*rw, point1->y + dly1*rw, lu,1);
    vertexPtr++->set (rx1, ry1, ru,1);
    }

  return vertexPtr;
  }
//}}}
//{{{
cVg::sVertex* cVg::cShape::bevelJoin (sVertex* vertexPtr, sShapePoint* point0, sShapePoint* point1,
                                        float lw, float rw, float lu, float ru, float fringe) {

  float rx0, ry0, rx1, ry1;
  float lx0, ly0, lx1 ,ly1;

  float dlx0 = point0->dy;
  float dly0 = -point0->dx;
  float dlx1 = point1->dy;
  float dly1 = -point1->dx;

  if (point1->flags & sShapePoint::ePtLEFT) {
    chooseBevel (point1->flags & sShapePoint::ePtINNERBEVEL, point0, point1, lw, &lx0, &ly0, &lx1, &ly1);
    vertexPtr++->set (lx0, ly0, lu,1);
    vertexPtr++->set (point1->x - dlx0*rw, point1->y - dly0*rw, ru,1);

    if (point1->flags & sShapePoint::ePtBEVEL) {
      vertexPtr++->set (lx0, ly0, lu,1);
      vertexPtr++->set (point1->x - dlx0*rw, point1->y - dly0*rw, ru,1);
      vertexPtr++->set (lx1, ly1, lu,1);
      vertexPtr++->set (point1->x - dlx1*rw, point1->y - dly1*rw, ru,1);
      }
    else {
      rx0 = point1->x - point1->dmx * rw;
      ry0 = point1->y - point1->dmy * rw;
      vertexPtr++->set (point1->x, point1->y, 0.5f,1);
      vertexPtr++->set (point1->x - dlx0*rw, point1->y - dly0*rw, ru,1);
      vertexPtr++->set (rx0, ry0, ru,1);
      vertexPtr++->set (rx0, ry0, ru,1);
      vertexPtr++->set (point1->x, point1->y, 0.5f,1);
      vertexPtr++->set (point1->x - dlx1*rw, point1->y - dly1*rw, ru,1);
      }

    vertexPtr++->set (lx1, ly1, lu,1);
    vertexPtr++->set (point1->x - dlx1*rw, point1->y - dly1*rw, ru,1);
    }
  else {
    chooseBevel (point1->flags & sShapePoint::ePtINNERBEVEL, point0, point1, -rw, &rx0,&ry0, &rx1,&ry1);
    vertexPtr++->set (point1->x + dlx0*lw, point1->y + dly0*lw, lu,1);
    vertexPtr++->set (rx0, ry0, ru,1);

    if (point1->flags & sShapePoint::ePtBEVEL) {
      vertexPtr++->set (point1->x + dlx0*lw, point1->y + dly0*lw, lu,1);
      vertexPtr++->set (rx0, ry0, ru,1);
      vertexPtr++->set (point1->x + dlx1*lw, point1->y + dly1*lw, lu,1);
      vertexPtr++->set (rx1, ry1, ru,1);
      }

    else {
      lx0 = point1->x + point1->dmx * lw;
      ly0 = point1->y + point1->dmy * lw;
      vertexPtr++->set (point1->x + dlx0*lw, point1->y + dly0*lw, lu,1);
      vertexPtr++->set (point1->x, point1->y, 0.5f,1);
      vertexPtr++->set (lx0, ly0, lu,1);
      vertexPtr++->set (lx0, ly0, lu,1);
      vertexPtr++->set (point1->x + dlx1*lw, point1->y + dly1*lw, lu,1);
      vertexPtr++->set (point1->x, point1->y, 0.5f,1);
      }

    vertexPtr++->set (point1->x + dlx1*lw, point1->y + dly1*lw, lu,1);
    vertexPtr++->set (rx1, ry1, ru,1);
    }

  return vertexPtr;
  }
//}}}

//{{{
cVg::sVertex* cVg::cShape::buttCapStart (sVertex* vertexPtr, sShapePoint* point, float dx, float dy, float w, float d, float aa) {

  float px = point->x - dx*d;
  float py = point->y - dy*d;
  float dlx = dy;
  float dly = -dx;

  vertexPtr++->set (px + dlx * w - dx * aa, py + dly * w - dy * aa, 0, 0);
  vertexPtr++->set (px - dlx * w - dx * aa, py - dly * w - dy * aa, 1, 0);
  vertexPtr++->set (px + dlx * w, py + dly * w, 0, 1);
  vertexPtr++->set (px - dlx * w, py - dly * w, 1, 1);

  return vertexPtr;
  }
//}}}
//{{{
cVg::sVertex* cVg::cShape::buttCapEnd (sVertex* vertexPtr, sShapePoint* point, float dx, float dy, float w, float d, float aa) {

  float px = point->x + dx * d;
  float py = point->y + dy * d;
  float dlx = dy;
  float dly = -dx;

  vertexPtr++->set (px + dlx * w, py + dly * w, 0,1);
  vertexPtr++->set (px - dlx * w, py - dly * w, 1,1);
  vertexPtr++->set (px + dlx * w + dx * aa, py + dly * w + dy * aa, 0, 0);
  vertexPtr++->set (px - dlx * w + dx * aa, py - dly * w + dy * aa, 1, 0);

  return vertexPtr;
  }
//}}}
//{{{
cVg::sVertex* cVg::cShape::roundCapStart (sVertex* vertexPtr, sShapePoint* point, float dx, float dy, float w, int ncap, float aa) {

  float px = point->x;
  float py = point->y;
  float dlx = dy;
  float dly = -dx;

  for (int i = 0; i < ncap; i++) {
    float a = i / (float)(ncap-1) * kPi;
    float ax = cosf(a) * w;
    float ay = sinf(a) * w;
    vertexPtr++->set (px - dlx * ax - dx * ay, py - dly * ax - dy * ay, 0, 1);
    vertexPtr++->set (px, py, 0.5f, 1);
    }

  vertexPtr++->set (px + dlx * w, py + dly * w, 0, 1);
  vertexPtr++->set (px - dlx * w, py - dly * w, 1, 1);

  return vertexPtr;
  }
//}}}
//{{{
cVg::sVertex* cVg::cShape::roundCapEnd (sVertex* vertexPtr, sShapePoint* point, float dx, float dy, float w, int ncap, float aa) {

  float px = point->x;
  float py = point->y;
  float dlx = dy;
  float dly = -dx;

  vertexPtr++->set (px + dlx * w, py + dly * w, 0, 1);
  vertexPtr++->set (px - dlx * w, py - dly * w, 1, 1);
  for (int i = 0; i < ncap; i++) {
    float a = i / (float)(ncap-1) * kPi;
    float ax = cosf(a) * w;
    float ay = sinf(a) * w;
    vertexPtr++->set (px, py, 0.5f, 1);
    vertexPtr++->set (px - dlx * ax + dx * ay, py - dly * ax + dy * ay, 0, 1);
    }

  return vertexPtr;
  }
//}}}
//}}}
//{{{  cVg::cShader
//{{{
cVg::cShader::~cShader() {
  if (prog)
    glDeleteProgram (prog);
  if (vert)
    glDeleteShader (vert);
  if (frag)
    glDeleteShader (frag);
  }
//}}}
//{{{
bool cVg::cShader::create (const char* opts) {

  const char* str[3];
  str[0] = kShaderHeader;
  str[1] = opts != nullptr ? opts : "";

  prog = glCreateProgram();
  vert = glCreateShader (GL_VERTEX_SHADER);
  str[2] = kVertShader;
  glShaderSource (vert, 3, str, 0);

  frag = glCreateShader (GL_FRAGMENT_SHADER);
  str[2] = kFragShader;
  glShaderSource (frag, 3, str, 0);

  glCompileShader (vert);
  GLint status;
  glGetShaderiv (vert, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    //{{{  error return
    dumpShaderError (vert, "shader", "vert");
    return false;
    }
    //}}}

  glCompileShader (frag);
  glGetShaderiv (frag, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    //{{{  error return
    dumpShaderError (frag, "shader", "frag");
    return false;
    }
    //}}}

  glAttachShader (prog, vert);
  glAttachShader (prog, frag);

  glBindAttribLocation (prog, 0, "vertex");
  glBindAttribLocation (prog, 1, "tcoord");

  glLinkProgram (prog);
  glGetProgramiv (prog, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    //{{{  error return
    dumpProgramError (prog, "shader");
    return false;
    }
    //}}}

  glUseProgram (prog);

  return true;
  }
//}}}

//{{{
void cVg::cShader::getUniforms() {
  location[LOCATION_VIEWSIZE] = glGetUniformLocation (prog, "viewSize");
  location[LOCATION_TEX] = glGetUniformLocation (prog, "tex");
  location[LOCATION_FRAG] = glGetUniformLocation (prog, "frag");
  }
//}}}

//{{{
void cVg::cShader::dumpShaderError (GLuint shader, const char* name, const char* type) {

  GLchar str[512+1];
  GLsizei len = 0;
  glGetShaderInfoLog (shader, 512, &len, str);
  if (len > 512)
    len = 512;
  str[len] = '\0';

  printf ("Shader %s/%s error:%s\n", name, type, str);
  }
//}}}
//{{{
void cVg::cShader::dumpProgramError (GLuint prog, const char* name) {

  GLchar str[512+1];
  GLsizei len = 0;
  glGetProgramInfoLog (prog, 512, &len, str);
  if (len > 512)
    len = 512;
  str[len] = '\0';

  printf ("Program %s error:%s\n", name, str);
  }
//}}}
//}}}

// converts
//{{{
GLenum cVg::convertBlendFuncFactor (eBlendFactor factor) {

  switch (factor) {
    case NVG_ZERO:
      return GL_ZERO;
    case NVG_ONE:
      return GL_ONE;
    case NVG_SRC_COLOR:
      return GL_SRC_COLOR;
    case NVG_ONE_MINUS_SRC_COLOR:
      return GL_ONE_MINUS_SRC_COLOR;
    case NVG_DST_COLOR:
      return GL_DST_COLOR;
    case NVG_ONE_MINUS_DST_COLOR:
      return GL_ONE_MINUS_DST_COLOR;
    case NVG_SRC_ALPHA:
      return GL_SRC_ALPHA;
    case  NVG_ONE_MINUS_SRC_ALPHA:
      return GL_ONE_MINUS_SRC_ALPHA;
    case NVG_DST_ALPHA:
      return GL_DST_ALPHA;
    case NVG_ONE_MINUS_DST_ALPHA:
      return GL_ONE_MINUS_DST_ALPHA;
    case NVG_SRC_ALPHA_SATURATE:
      return GL_SRC_ALPHA_SATURATE;
    default:
      return GL_INVALID_ENUM;
    }
  }
//}}}
//{{{
cVg::sCompositeState cVg::compositeState (eCompositeOp compositeOp) {

  sCompositeState compositeState;
  switch (compositeOp) {
    case NVG_SOURCE_OVER :
      compositeState.srcRGB = NVG_ONE;
      compositeState.dstRGB = NVG_ONE_MINUS_SRC_ALPHA;
      break;
    case NVG_SOURCE_IN:
      compositeState.srcRGB = NVG_DST_ALPHA;
      compositeState.dstRGB = NVG_ZERO;
      break;
    case NVG_SOURCE_OUT:
      compositeState.srcRGB = NVG_ONE_MINUS_DST_ALPHA;
      compositeState.dstRGB = NVG_ZERO;
      break;
    case NVG_ATOP:
      compositeState.srcRGB = NVG_DST_ALPHA;
      compositeState.dstRGB = NVG_ONE_MINUS_SRC_ALPHA;
      break;
    case NVG_DESTINATION_OVER:
      compositeState.srcRGB = NVG_ONE_MINUS_DST_ALPHA;
      compositeState.dstRGB = NVG_ONE;
      break;
    case NVG_DESTINATION_IN:
      compositeState.srcRGB = NVG_ZERO;
      compositeState.dstRGB = NVG_SRC_ALPHA;
      break;
    case NVG_DESTINATION_OUT:
      compositeState.srcRGB = NVG_ZERO;
      compositeState.dstRGB = NVG_ONE_MINUS_SRC_ALPHA;
      break;
    case NVG_DESTINATION_ATOP:
      compositeState.srcRGB = NVG_ONE_MINUS_DST_ALPHA;
      compositeState.dstRGB = NVG_SRC_ALPHA;
      break;
    case NVG_LIGHTER:
      compositeState.srcRGB = NVG_ONE;
      compositeState.dstRGB = NVG_ONE;
      break;
    case NVG_COPY:
      compositeState.srcRGB = NVG_ONE;
      compositeState.dstRGB = NVG_ZERO;
      break;
    case NVG_XOR:
      compositeState.srcRGB = NVG_ONE_MINUS_DST_ALPHA;
      compositeState.dstRGB = NVG_ONE_MINUS_SRC_ALPHA;
      break;
    default:
      compositeState.srcRGB = NVG_ONE;
      compositeState.dstRGB = NVG_ZERO;
      break;
    }

  compositeState.srcAlpha = compositeState.srcRGB;
  compositeState.dstAlpha = compositeState.dstRGB;
  return compositeState;
  }
//}}}

// sets
//{{{
void cVg::setStencilMask (GLuint mask) {

  if (mStencilMask != mask) {
    mStencilMask = mask;
    glStencilMask (mask);
    }
  }
//}}}
//{{{
void cVg::setStencilFunc (GLenum func, GLint ref, GLuint mask) {

  if ((mStencilFunc != func) || (mStencilFuncRef != ref) || (mStencilFuncMask != mask)) {
    mStencilFunc = func;
    mStencilFuncRef = ref;
    mStencilFuncMask = mask;
    glStencilFunc (func, ref, mask);
    }
  }
//}}}
//{{{
void cVg::setBindTexture (GLuint texture) {

  if (mBindTexture != texture) {
    mBindTexture = texture;
    glBindTexture (GL_TEXTURE_2D, texture);
    }
  }
//}}}
//{{{
void cVg::setUniforms (int firstFragIndex, int id) {

  mShader.setFrags ((float*)(&mFrags[firstFragIndex]));

  if (id) {
    auto texture = findTextureById (id);
    setBindTexture (texture ? texture->tex : 0);
    }
  else
    setBindTexture (0);
  }
//}}}

// allocs
//{{{
cVg::sDraw* cVg::allocDraw() {
// allocate a draw, return pointer to it

  if (mNumDraws + 1 > mNumAllocatedDraws) {
    // 1.5x Overallocate
    mNumAllocatedDraws = max (mNumDraws + 1, 128) + mNumAllocatedDraws / 2;
    mDraws = (sDraw*)realloc (mDraws, sizeof(sDraw) * mNumAllocatedDraws);
    }

  return &mDraws[mNumDraws++];
  }
//}}}
//{{{
int cVg::allocFrags (int numFrags) {
// allocate numFrags, return index of first

  if (mNumFrags + numFrags > mNumAllocatedFrags) {
    // 1.5x Overallocate
    mNumAllocatedFrags = max (mNumFrags + numFrags, 128) + mNumAllocatedFrags / 2;
    mFrags = (sFrag*)realloc (mFrags, mNumAllocatedFrags * sizeof(sFrag));
    }

  int firstFragIndex = mNumFrags;
  mNumFrags += numFrags;

  return firstFragIndex;
  }
//}}}
//{{{
int cVg::allocPathVertices (int numPaths) {
// allocate numPaths pathVertices, return index of first

  if (mNumPathVertices + numPaths > mNumAllocatedPathVertices) {
    // 1.5x Overallocate
    mNumAllocatedPathVertices = max (mNumPathVertices + numPaths, 128) + mNumAllocatedPathVertices / 2;
    mPathVertices = (sPathVertices*)realloc (mPathVertices, mNumAllocatedPathVertices * sizeof(sPathVertices));
    }

  int firstPathVerticeIndex = mNumPathVertices;
  mNumPathVertices += numPaths;

  return firstPathVerticeIndex;
  }
//}}}

// texture
//{{{
int cVg::createTexture (int type, int width, int height, int imageFlags, const uint8_t* data, const string& debug) {

  cLog::log (LOGINFO, "createTexture - " + debug + " " + dec(width) + "x" + dec(height));

  sTexture* texture = nullptr;
  for (int i = 0; i < mNumTextures; i++) {
    if (mTextures[i].id == 0) {
      texture = &mTextures[i];
      break;
      }
    }

  if (texture == nullptr) {
    if (mNumTextures + 1 > mNumAllocatedTextures) {
      mNumAllocatedTextures = max (mNumTextures + 1, 4) +  mNumAllocatedTextures / 2; // 1.5x Overallocate
      mTextures = (sTexture*)realloc (mTextures, mNumAllocatedTextures * sizeof(sTexture));
      if (mTextures == nullptr)
        return 0;
      }
    texture = &mTextures[mNumTextures++];
    }

  // init texture
  texture->reset();
  texture->id = ++mTextureId;
  glGenTextures (1, &texture->tex);
  texture->type = type;
  if (nearestPow2 (width) != (unsigned int)width || nearestPow2(height) != (unsigned int)height) {
    //{{{  check non-power of 2 restrictions
    if ((imageFlags & eImageRepeatX) != 0 || (imageFlags & eImageRepeatY) != 0) {
      cLog::log (LOGINFO, "no repeat x,y for non powerOfTwo textures %dx%d", width, height);
      imageFlags &= ~(eImageRepeatX | eImageRepeatY);
      }

    if (imageFlags & eImageGenerateMipmaps) {
      cLog::log (LOGINFO, "no mipmap for non powerOfTwo textures %dx%d", width, height);
      imageFlags &= ~eImageGenerateMipmaps;
      }
    }
    //}}}
  texture->flags = imageFlags;
  texture->width = width;
  texture->height = height;
  setBindTexture (texture->tex);

  glPixelStorei (GL_UNPACK_ALIGNMENT,1);

  if (type == eTextureRgba)
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  else // alpha
    glTexImage2D (GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);

  if (imageFlags & cVg::eImageGenerateMipmaps)
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                     (imageFlags & cVg::eImageNearestPixel) ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR);

  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (imageFlags & cVg::eImageNearestPixel) ? GL_NEAREST : GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (imageFlags & cVg::eImageNearestPixel) ? GL_NEAREST : GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (imageFlags & cVg::eImageRepeatX) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (imageFlags & cVg::eImageRepeatY) ? GL_REPEAT : GL_CLAMP_TO_EDGE);

  glPixelStorei (GL_UNPACK_ALIGNMENT, 4);

  if (imageFlags & cVg::eImageGenerateMipmaps)
    glGenerateMipmap (GL_TEXTURE_2D);

  setBindTexture (0);

  return texture->id;
  }
//}}}
//{{{
cVg::sTexture* cVg::findTextureById (int id) {

  for (int i = 0; i < mNumTextures; i++)
    if (mTextures[i].id == id)
      return &mTextures[i];

  return nullptr;
  }
//}}}
//{{{
bool cVg::updateTexture (int id, int x, int y, int width, int height, const uint8_t* data) {

  //cLog::log (LOGINFO, "updateTexture");
  sTexture* texture = findTextureById (id);
  if (texture == nullptr) {
    cLog::log (LOGERROR, "updateTexture - no id:" + dec(id) + " " + dec(width) + "x" + dec(height));
    return false;
    }
  setBindTexture (texture->tex);

  glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

  // no support for all of skip, need to update a whole row at a time.
  if (texture->type == eTextureRgba)
    data += y * texture->width * 4;
  else // alpha
    data += y * texture->width;

  x = 0;
  width = texture->width;

  if (texture->type == eTextureRgba)
    glTexSubImage2D (GL_TEXTURE_2D, 0, x,y, width,height, GL_RGBA, GL_UNSIGNED_BYTE, data);
  else // alpha
    glTexSubImage2D (GL_TEXTURE_2D, 0, x,y, width,height, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);

  glPixelStorei (GL_UNPACK_ALIGNMENT, 4);

  setBindTexture (0);
  return true;
  }
//}}}
//{{{
bool cVg::getTextureSize (int id, int& width, int& height) {

  auto texture = findTextureById (id);
  if (texture == nullptr) {
    width = 0;
    height = 0;
    return false;
    }

  width = texture->width;
  height = texture->height;
  return true;
  }
//}}}

// render
//{{{
void cVg::renderStroke (cShape& shape, sPaint& paint, cScissor& scissor, float fringe, float strokeWidth) {
// only uses toPathVertices firstStrokeVertexIndex, strokeVertexCount, no fill

  auto draw = allocDraw();
  draw->set (sDraw::eStroke, paint.mImageId, allocPathVertices (shape.mNumPaths), shape.mNumPaths, allocFrags (2), 0,0);
  mFrags[draw->mFirstFragIndex].setFill (paint, scissor, strokeWidth, fringe, -1.0f, findTextureById (paint.mImageId));
  mFrags[draw->mFirstFragIndex+1].setFill (paint, scissor, strokeWidth, fringe, 1.0f - 0.5f/255.0f, findTextureById (paint.mImageId));

  auto fromPath = shape.mPaths;
  auto toPathVertices = &mPathVertices[draw->mFirstPathVerticesIndex];
  while (fromPath < shape.mPaths + shape.mNumPaths)
    *toPathVertices++ = fromPath++->mPathVertices;
  }
//}}}
//{{{
void cVg::renderFill (cShape& shape, sPaint& paint, cScissor& scissor, float fringe) {

  auto draw = allocDraw();
  if ((shape.mNumPaths == 1) && shape.mPaths[0].mConvex) {
    // convex
    draw->set (sDraw::eConvexFill, paint.mImageId, allocPathVertices (shape.mNumPaths), shape.mNumPaths,
               allocFrags (1), 0,0);
    mFrags[draw->mFirstFragIndex].setFill (paint, scissor, fringe, fringe, -1.0f, findTextureById (paint.mImageId));
    }
  else {
    // stencil
    draw->set (sDraw::eStencilFill, paint.mImageId, allocPathVertices (shape.mNumPaths), shape.mNumPaths,
               allocFrags (2), shape.mBoundsVertexIndex, 4);
    mFrags[draw->mFirstFragIndex].setSimple();
    mFrags[draw->mFirstFragIndex+1].setFill (paint, scissor, fringe, fringe, -1.0f, findTextureById(paint.mImageId));
    }

  auto fromPath = shape.mPaths;
  auto toPathVertices = &mPathVertices[draw->mFirstPathVerticesIndex];
  while (fromPath < shape.mPaths + shape.mNumPaths)
    *toPathVertices++ = fromPath++->mPathVertices;
  }
//}}}
//{{{
void cVg::renderText (int firstVertexIndex, int numVertices, sPaint& paint, cScissor& scissor) {

  //cLog::log (LOGINFO, "renderText " + dec(firstVertexIndex) + " " + dec(numVertices) + " " + dec(paint.id));
  auto draw = allocDraw();
  draw->set (sDraw::eText, paint.mImageId, 0, 0, allocFrags (1), firstVertexIndex, numVertices);
  mFrags[draw->mFirstFragIndex].setImage (paint, scissor, findTextureById (paint.mImageId));
  }
//}}}
//{{{
void cVg::renderTriangles (int firstVertexIndex, int numVertices, sPaint& paint, cScissor& scissor) {

  auto draw = allocDraw();
  draw->set (sDraw::eTriangle, paint.mImageId, 0, 0, allocFrags (1), firstVertexIndex, numVertices);
  mFrags[draw->mFirstFragIndex].setFill (paint, scissor, 1.0f, 1.0f, -1.0f, findTextureById (paint.mImageId));
  }
//}}}
//{{{
void cVg::renderFrame (cVertices& vertices, sCompositeState composite) {

  mDrawArrays = 0;
  //{{{  init gl blendFunc
  GLenum srcRGB = convertBlendFuncFactor (composite.srcRGB);
  GLenum dstRGB = convertBlendFuncFactor (composite.dstRGB);
  GLenum srcAlpha = convertBlendFuncFactor (composite.srcAlpha);
  GLenum dstAlpha = convertBlendFuncFactor (composite.dstAlpha);

  if (srcRGB == GL_INVALID_ENUM || dstRGB == GL_INVALID_ENUM ||
      srcAlpha == GL_INVALID_ENUM || dstAlpha == GL_INVALID_ENUM)
    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  else
    glBlendFuncSeparate (srcRGB, dstRGB, srcAlpha, dstAlpha);
  //}}}
  //{{{  init gl draw style
  glEnable (GL_CULL_FACE);
  glCullFace (GL_BACK);

  glFrontFace (GL_CCW);
  glEnable (GL_BLEND);

  glDisable (GL_DEPTH_TEST);
  glDisable (GL_SCISSOR_TEST);
  //}}}
  //{{{  init gl stencil buffer
  mStencilMask = 0xFF;
  glStencilMask (mStencilMask);

  mStencilFunc = GL_ALWAYS;
  mStencilFuncRef = 0;
  mStencilFuncMask = 0xFF;
  glStencilFunc (mStencilFunc, mStencilFuncRef, mStencilFuncMask);

  glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);
  glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  //}}}
  //{{{  init gl texture
  glActiveTexture (GL_TEXTURE0);
  glBindTexture (GL_TEXTURE_2D, 0);
  mBindTexture = 0;
  //}}}
  //{{{  init gl uniforms
  mShader.setTex (0);
  mShader.setViewport (mViewport);
  //}}}
  //{{{  init gl vertices
  glBindBuffer (GL_ARRAY_BUFFER, mVertexBuffer);
  glBufferData (GL_ARRAY_BUFFER, vertices.getNumVertices() * sizeof(sVertex), vertices.getVertexPtr(0), GL_STREAM_DRAW);

  glEnableVertexAttribArray (0);
  glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, sizeof(sVertex), (const GLvoid*)(size_t)0);

  glEnableVertexAttribArray (1);
  glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, sizeof(sVertex), (const GLvoid*)(0 + 2*sizeof(float)));
  //}}}

  for (auto draw = mDraws; draw < mDraws + mNumDraws; draw++)
    switch (draw->mType) {
      case sDraw::eStroke: {
        //{{{  stroke
        glEnable (GL_STENCIL_TEST);

        auto pathVertices = &mPathVertices[draw->mFirstPathVerticesIndex];

        // fill stroke base without overlap
        if (mDrawSolid) {
          glStencilOp (GL_KEEP, GL_KEEP, GL_INCR);
          setStencilFunc (GL_EQUAL, 0x00, 0xFF);
          setUniforms (draw->mFirstFragIndex + 1, draw->mId);
          for (int i = 0; i < draw->mNumPaths; i++) {
            glDrawArrays (GL_TRIANGLE_STRIP, pathVertices[i].mFirstStrokeVertexIndex, pathVertices[i].mNumStrokeVertices);
            mDrawArrays++;
            }
          }

        setUniforms (draw->mFirstFragIndex, draw->mId);
        if (mDrawEdges) {
          glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);
          setStencilFunc (GL_EQUAL, 0x00, 0xFF);
          for (int i = 0; i < draw->mNumPaths; i++)  {
            glDrawArrays (GL_TRIANGLE_STRIP, pathVertices[i].mFirstStrokeVertexIndex, pathVertices[i].mNumStrokeVertices);
            mDrawArrays++;
            }
          }

        // clear stencilBuffer
        if (mDrawSolid || mDrawEdges) {
          glColorMask (GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
          glStencilOp (GL_ZERO, GL_ZERO, GL_ZERO);
          setStencilFunc (GL_ALWAYS, 0x00, 0xFF);
          for (int i = 0; i < draw->mNumPaths; i++) {
            glDrawArrays (GL_TRIANGLE_STRIP, pathVertices[i].mFirstStrokeVertexIndex, pathVertices[i].mNumStrokeVertices);
            mDrawArrays++;
            }
          glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
          }

        glDisable (GL_STENCIL_TEST);
        break;
        }
        //}}}
      case sDraw::eConvexFill: {
        //{{{  convexFill
        setUniforms (draw->mFirstFragIndex, draw->mId);

        auto pathVertices = &mPathVertices[draw->mFirstPathVerticesIndex];

        if (mDrawSolid)
          for (int i = 0; i < draw->mNumPaths; i++) {
            glDrawArrays (GL_TRIANGLE_FAN, pathVertices[i].mFirstFillVertexIndex, pathVertices[i].mNumFillVertices);
            mDrawArrays++;
            }

        if (mDrawEdges)
          for (int i = 0; i < draw->mNumPaths; i++)
            if (pathVertices[i].mNumStrokeVertices) {
              glDrawArrays (GL_TRIANGLE_STRIP, pathVertices[i].mFirstStrokeVertexIndex, pathVertices[i].mNumStrokeVertices);
              mDrawArrays++;
              }

        break;
        }
        //}}}
      case sDraw::eStencilFill: {
        //{{{  stencilFill
        glEnable (GL_STENCIL_TEST);

        auto pathVertices = &mPathVertices[draw->mFirstPathVerticesIndex];

        if (mDrawSolid) {
          glDisable (GL_CULL_FACE);

          glStencilOpSeparate (GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
          glStencilOpSeparate (GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
          glColorMask (GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

          setStencilFunc (GL_ALWAYS, 0x00, 0xFF);
          setUniforms (draw->mFirstFragIndex, 0);
          for (int i = 0; i < draw->mNumPaths; i++) {
            glDrawArrays (GL_TRIANGLE_FAN, pathVertices[i].mFirstFillVertexIndex, pathVertices[i].mNumFillVertices);
            mDrawArrays++;
            }

          glEnable (GL_CULL_FACE);
          glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
          }

        setUniforms (draw->mFirstFragIndex + 1, draw->mId);
        if (mDrawEdges) {
          glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);
          setStencilFunc (GL_EQUAL, 0x00, 0xFF);
          for (int i = 0; i < draw->mNumPaths; i++)
            if (pathVertices[i].mNumStrokeVertices) {
              glDrawArrays (GL_TRIANGLE_STRIP, pathVertices[i].mFirstStrokeVertexIndex, pathVertices[i].mNumStrokeVertices);
              mDrawArrays++;
              }
          }

        // draw bounding rect as triangleStrip
        if (mDrawSolid || mDrawEdges) {
          glStencilOp (GL_ZERO, GL_ZERO, GL_ZERO);
          setStencilFunc (GL_NOTEQUAL, 0x00, 0xFF);
          glDrawArrays (GL_TRIANGLE_STRIP, draw->mTriangleFirstVertexIndex, draw->mNumTriangleVertices);
          mDrawArrays++;
          }

        glDisable (GL_STENCIL_TEST);
        break;
        }
        //}}}
      case sDraw::eText:
        //{{{  text triangles
        if (mDrawTriangles) {
          setUniforms (draw->mFirstFragIndex, draw->mId);
          glDrawArrays (GL_TRIANGLES, draw->mTriangleFirstVertexIndex, draw->mNumTriangleVertices);
          mDrawArrays++;
          }
        break;
        //}}}
      case sDraw::eTriangle:
        //{{{  fill triangles
        if (mDrawSolid) {
          setUniforms (draw->mFirstFragIndex, draw->mId);
          glDrawArrays (GL_TRIANGLES, draw->mTriangleFirstVertexIndex, draw->mNumTriangleVertices);
          mDrawArrays++;
          }
        break;
        //}}}
      }

  // reset counts
  mNumDraws = 0;
  mNumPathVertices = 0;
  mNumFrags = 0;
  }
//}}}

// font
//{{{
float cVg::getFontScale (sState* state) {
  return min (quantize (state->mTransform.getAverageScale(), 0.01f), 4.f);
  }
//}}}
//{{{
bool cVg::allocAtlas() {

  flushAtlasTexture();
  if (mFontTextureIndex >= kMaxFontTextures-1) {
    cLog::log (LOGERROR, "allocAtlas - no more fontTextures");
    return false;
    }

  cLog::log (LOGINFO, "allocAtlas");

  // if next fontImage already have a texture
  int width;
  int height;
  if (mFontTextureIds[mFontTextureIndex+1] != 0)
    getTextureSize (mFontTextureIds[mFontTextureIndex+1], width, height);
  else {
    // calculate the new font image size and create it
    getTextureSize (mFontTextureIds[mFontTextureIndex], width, height);
    if (width > height)
      height *= 2;
    else
      width *= 2;

    if ((width > 2048) || (height > 2048)) {
      width = 2048;
      height = 2048;
      }

    mFontTextureIds[mFontTextureIndex +1] = createTexture (eTextureAlpha, width, height, 0, NULL, "allocAtlas");
    }
  ++mFontTextureIndex;

  mAtlasText->resetAtlas (width, height);
  return true;
  }
//}}}
//{{{
void cVg::flushAtlasTexture() {

  int dirty[4];
  if (mAtlasText->getAtlasDirty (dirty)) {
    // atlasDirty, update texture
    int fontTextureId = mFontTextureIds[mFontTextureIndex];
    if (fontTextureId != 0) {
      int width;
      int height;
      const uint8_t* data = mAtlasText->getAtlasTextureData (width, height);

      int dirtyX = dirty[0];
      int dirtyY = dirty[1];
      int dirtyWidth = dirty[2] - dirty[0];
      int dirtyHeight = dirty[3] - dirty[1];
      //cLog::log (LOGINFO, "flushAtlasTexture mFontTextureIndex:%d fontTextureId:%d %dx%d dirty:%d,%d:%dx%d",
      //                    mFontTextureIndex, fontTextureId, width, height,
      //                    dirtyX, dirtyY, dirtyWidth, dirtyHeight);
      updateTexture (fontTextureId, dirtyX, dirtyY, dirtyWidth, dirtyHeight, data);
      }
    else
      cLog::log (LOGINFO, "flushTextTexture - dirty");
    }
  }
//}}}
