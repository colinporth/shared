// cVg.h - based on Mikko Mononen memon@inside.org nanoVg
#pragma once
//{{{  includes
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <string>
#include <algorithm>

#include "../glad/glad.h"
#include "../GLFW/glfw3.h"

#include "../utils/utils.h"
#include "../utils/cLog.h"
//}}}

//{{{  inline math utils
constexpr float kPi = 3.14159265358979323846264338327f;
constexpr float k2Pi = kPi * 2.f;
constexpr float kPiDiv2 = kPi / 2.f;
constexpr float k180Pi = kPi * 180.f;

inline int clampi (int a, int mn, int mx) { return a < mn ? mn : (a > mx ? mx : a); }
inline float clampf (float a, float mn, float mx) { return a < mn ? mn : (a > mx ? mx : a); }
inline float absf (float a) { return a >= 0.0f ? a : -a; }

inline float quantize (float a, float d) { return ((int)(a / d + 0.5f)) * d; }
inline float signf (float a) { return a >= 0.0f ? 1.0f : -1.0f; }
inline float cross (float dx0, float dy0, float dx1, float dy1) { return dx1*dy0 - dx0*dy1; }
//{{{
inline float normalize (float& x, float& y) {

  float d = sqrtf(x*x + y*y);
  if (d > 1e-6f) {
    float id = 1.0f / d;
    x *= id;
    y *= id;
    }

  return d;
  }
//}}}

inline float degToRad (float deg) { return deg / k180Pi; }
inline float radToDeg (float rad) { return rad / k180Pi; }
//{{{
inline unsigned int nearestPow2 (unsigned int num) {

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
inline int pointEquals (float x1, float y1, float x2, float y2, float tol) {
  float dx = x2 - x1;
  float dy = y2 - y1;
  return dx*dx + dy*dy < tol*tol;
  }
//}}}
//{{{
inline float distPointSeg (float x, float y, float px, float py, float qx, float qy) {

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
inline void intersectRects (float* dst, float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {

  float minx = std::max (ax, bx);
  float miny = std::max (ay, by);
  float maxx = std::min (ax+aw, bx+bw);
  float maxy = std::min (ay+ah, by+bh);

  dst[0] = minx;
  dst[1] = miny;
  dst[2] = std::max (0.0f, maxx - minx);
  dst[3] = std::max (0.0f, maxy - miny);
  }
//}}}

//{{{
inline float hue (float h, float m1, float m2) {

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
//}}}
//{{{  inline sVgColour
struct sVgColour {
  union {
    float rgba[4];
    struct {
      float r;
      float g;
      float b;
      float a;
      };
    };
  };

//{{{
inline sVgColour nvgRGBA32 (uint32_t colour) {
  sVgColour color;
  color.r = ((colour & 0xFF0000) >> 16) / 255.0f;
  color.g = ((colour & 0xFF00) >> 8) / 255.0f;
  color.b = (colour & 0xFF) / 255.0f;
  color.a = (colour >> 24) / 255.0f;
  return color;
  }
//}}}
//{{{
inline sVgColour nvgRGBA (unsigned char r, unsigned char g, unsigned char b, unsigned char a) {

  sVgColour color;
  color.r = r / 255.0f;
  color.g = g / 255.0f;
  color.b = b / 255.0f;
  color.a = a / 255.0f;
  return color;
  }
//}}}
//{{{
inline sVgColour nvgRGB (unsigned char r, unsigned char g, unsigned char b) {
  return nvgRGBA (r,g,b,255);
  }
//}}}
//{{{
inline sVgColour nvgRGBAf (float r, float g, float b, float a) {
  sVgColour color;
  color.r = r;
  color.g = g;
  color.b = b;
  color.a = a;
  return color;
  }
//}}}
//{{{
inline sVgColour nvgRGBf (float r, float g, float b) {
  return nvgRGBAf (r,g,b,1.0f);
  }
//}}}
//{{{
inline sVgColour nvgPremulColor (sVgColour c) {
  c.r *= c.a;
  c.g *= c.a;
  c.b *= c.a;
  return c;
  }
//}}}
//}}}

class cAtlasText;
class cVg {
public:
  enum eCreateFlags { eNoSolid = 0x01, eNoEdges = 0x02,  eNoDelete = 0x40 };
  //{{{
  enum eBlendFactor {
    NVG_ZERO                = 1<<0,
    NVG_ONE                 = 1<<1,
    NVG_SRC_COLOR           = 1<<2,
    NVG_ONE_MINUS_SRC_COLOR = 1<<3,
    NVG_DST_COLOR           = 1<<4,
    NVG_ONE_MINUS_DST_COLOR = 1<<5,
    NVG_SRC_ALPHA           = 1<<6,
    NVG_ONE_MINUS_SRC_ALPHA = 1<<7,
    NVG_DST_ALPHA           = 1<<8,
    NVG_ONE_MINUS_DST_ALPHA = 1<<9,
    NVG_SRC_ALPHA_SATURATE  = 1<<10
    };
  //}}}
  //{{{
  enum eCompositeOp {
    NVG_SOURCE_OVER,
    NVG_SOURCE_IN,
    NVG_SOURCE_OUT,
    NVG_ATOP,
    NVG_DESTINATION_OVER,
    NVG_DESTINATION_IN,
    NVG_DESTINATION_OUT,
    NVG_DESTINATION_ATOP,
    NVG_LIGHTER,
    NVG_COPY,
    NVG_XOR };
  //}}}
  //{{{
  class cTransform {
  // sx,sy define scaling, kx,ky skewing, tx,ty translation.
  // last row is assumed to be 0,0,1 and not stored.
  //   [sx kx tx]
  //   [ky sy ty]
  //   [ 0  0  1]
  public:
    cTransform();
    cTransform (float sx, float ky, float kx, float sy, float tx, float ty);

    float getAverageScaleX();
    float getAverageScaleY();
    float getAverageScale();
    float getTranslateX() { return mTx; }
    float getTranslateY() { return mTy; }
    bool getInverse (cTransform& inverse);
    void getMatrix3x4 (float* matrix3x4);

    void setIdentity();
    void setTranslate (float tx, float ty);
    void setScale (float sx, float sy);
    void setRotate (float angle);
    void setRotateTranslate (float angle, float tx, float ty);
    void set (float sx, float ky, float kx, float sy, float tx, float ty);

    void multiply (cTransform& t);
    void premultiply (cTransform& t);

    void point (float& x, float& y);
    void point (float srcx, float srcy, float& dstx, float& dsty);
    void pointScissor (float srcx, float srcy, float& dstx, float& dsty);

    // vars
    float mSx;
    float mKy;
    float mKx;
    float mSy;

    float mTx;
    float mTy;

    bool mIdentity;

  private:
    //{{{
    bool isIdentity() {
      return mSx == 1.0f && mKy == 0.0f && mKx == 0.0f && mSy == 1.0f && mTx == 0.0f && mTy == 0.0f;
      }
    //}}}
    };
  //}}}
  //{{{
  struct cPaint {
    //{{{
    void set (const sVgColour& color) {

      mTransform.setIdentity();

      extent[0] = 0.0f;
      extent[1] = 0.0f;

      radius = 0.0f;
      feather = 1.0f;
      innerColor = color;
      outerColor = color;

      mImageId = 0;
      }
    //}}}

    cTransform mTransform;
    float extent[2];
    float radius = 0.f;
    float feather = 0.f;

    sVgColour innerColor;
    sVgColour outerColor;

    int mImageId = 0;
    };
  //}}}
  //{{{
  struct cCompositeOpState {
    eBlendFactor srcRGB;
    eBlendFactor dstRGB;
    eBlendFactor srcAlpha;
    eBlendFactor dstAlpha;
    };
  //}}}

  cVg() : cVg(0) {}
  cVg (int flags);
  virtual ~cVg();
  void initialise();

  //{{{  state
  void saveState();
  void restoreState();
  void resetState();
  //}}}
  //{{{  transform
  void resetTransform();
  void transform (float a, float b, float c, float d, float e, float f);

  void translate (float x, float y);
  void scale (float x, float y);
  void rotate (float angle);
  //}}}
  //{{{  shape
  enum eWinding { eSOLID = 1, eHOLE = 2 };  // CCW, CW
  enum eLineCap { eBUTT, eROUND, eSQUARE, eBEVEL, eMITER };
  enum eShapeCommands { eMOVETO, eLINETO, eBEZIERTO, eWINDING, eCLOSE };

  float getFringeWidth() { return mFringeWidth; }

  void fillColor (const sVgColour& color);
  void strokeColor (const sVgColour& color);
  void strokeWidth (float size);
  void globalAlpha (float alpha);
  void fringeWidth (float width);

  void miterLimit (float limit);
  void lineCap (eLineCap cap);
  void lineJoin (eLineCap join);

  cPaint linearGradient (float startx, float starty, float endx, float endy,
                         const sVgColour& innerColor, const sVgColour& outerColor);
  cPaint boxGradient (float x, float y, float width, float height, float radius, float feather,
                      const sVgColour& innerColor, const sVgColour& outerColor);
  cPaint radialGradient (float centrex, float centrey, float innerRadius, float outerRadius,
                         const sVgColour& innerColor, const sVgColour& outerColor);
  cPaint imagePattern (float cx, float cy, float width, float height, float angle, int imageId, float alpha);

  void fillPaint (const cPaint& paint);
  void strokePaint (const cPaint& paint);

  void beginPath();
  void pathWinding (eWinding dir);

  void moveTo (float x, float y);
  void lineTo (float x, float y);
  void bezierTo (float c1x, float c1y, float c2x, float c2y, float x, float y);
  void quadTo (float cx, float cy, float x, float y);
  void arcTo (float x1, float y1, float x2, float y2, float radius);
  void arc (float cx, float cy, float r, float a0, float a1, int dir);

  void rect (float x, float y, float width, float height);
  void roundedRectVarying (float x, float y, float width, float height,
                           float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft);
  void roundedRect (float x, float y, float w, float h, float radius);
  void ellipse (float cx, float cy, float rx, float ry);
  void circle (float cx, float cy, float radius);

  void closePath();

  void fill();
  void stroke();
  void triangleFill();
  //}}}
  //{{{  text
  //{{{
  enum eAlign {
    // Horizontal align
    ALIGN_LEFT    = 1<<0, // Default, align text horizontally to left.
    ALIGN_CENTER  = 1<<1, // Align text horizontally to center.
    ALIGN_RIGHT   = 1<<2, // Align text horizontally to right.

    // Vertical align
    ALIGN_TOP     = 1<<3, // Align text vertically to top.
    ALIGN_MIDDLE  = 1<<4, // Align text vertically to middle.
    ALIGN_BOTTOM  = 1<<5, // Align text vertically to bottom.
    ALIGN_BASELINE  = 1<<6, // Default, align text vertically to baseline.
    };
  //}}}
  //{{{
  struct sGlyphPosition {
    const char* str;  // Position of the glyph in the input string.
    float x;          // The x-coordinate of the logical glyph position.
    float minx, maxx; // The bounds of the glyph shape.
    };
  //}}}

  int createFont (const std::string& fontName, unsigned char* data, int dataSize);

  float getTextBounds (float x, float y, const std::string& str, float* bounds);
  float getTextMetrics (float& ascender, float& descender);
  int getTextGlyphPositions (float x, float y, const std::string& str, sGlyphPosition* positions, int maxPositions);

  void setFontById (int font);
  void setFontByName (const std::string& fontName);
  void setFontSize (float size);
  void setTextAlign (int align);
  void setTextLetterSpacing (float spacing);
  void setTextLineHeight (float lineHeight);

  float text (float x, float y, const std::string& str);
  //}}}
  //{{{  image
  enum eImageFlags {
    IMAGE_GENERATE_MIPMAPS = 1<<0, // Generate mipmaps during creation of the image.
    IMAGE_REPEATX          = 1<<1, // Repeat image in X direction.
    IMAGE_REPEATY          = 1<<2, // Repeat image in Y direction.
    IMAGE_FLIPY            = 1<<3, // Flips (inverses) image in Y direction when rendered.
    IMAGE_PREMULTIPLIED    = 1<<4, // Image data has premultiplied alpha.
    IMAGE_NEAREST          = 1<<5, // Image interpolation is Nearest instead Linear
    };

  enum eTexture {
    TEXTURE_ALPHA = 0x01,
    TEXTURE_RGBA  = 0x02
    };

  int createImageMem (int imageFlags, unsigned char* data, int ndata);
  int createImageRGBA (int width, int height, int imageFlags, const unsigned char* data);
  void updateImage (int id, const unsigned char* data);
  bool deleteImage (int id);
  //}}}
  //{{{  scissor
  void scissor (float x, float y, float w, float h);
  void intersectScissor (float x, float y, float w, float h);

  void resetScissor();
  //}}}
  //{{{  composite
  void globalCompositeOp (eCompositeOp op);
  void globalCompositeBlendFunc (eBlendFactor sfactor, eBlendFactor dfactor);
  void globalCompositeBlendFuncSeparate (eBlendFactor srcRGB, eBlendFactor dstRGB,
                                         eBlendFactor srcAlpha, eBlendFactor dstAlpha);
  //}}}
  //{{{  frame
  void toggleEdges() { mDrawEdges = !mDrawEdges; }
  void toggleSolid() { mDrawSolid = !mDrawSolid; }
  void toggleTriangles() { mDrawTriangles = !mDrawTriangles; }

  std::string getFrameStats();

  void beginFrame (int windowWidth, int windowHeight, float devicePixelRatio);
  void endFrame();
  //}}}

private:
  //{{{  static constexpr
  static constexpr int kMaxStates = 32;
  static constexpr int kMaxFontTextures = 4;
  //}}}
  enum eUniformBindings { FRAG_BINDING };
  enum eUniformLocation { LOCATION_VIEWSIZE, LOCATION_TEX, LOCATION_FRAG, MAX_LOCATIONS };
  enum eShaderType { SHADER_FILL_GRADIENT = 0, SHADER_FILL_IMAGE, SHADER_SIMPLE, SHADER_IMAGE };
  //{{{
  struct cScissor {
    cTransform mTransform;
    float extent[2];
    };
  //}}}
  //{{{
  struct c2dVertex {
    //{{{
    void set (float x, float y, float u, float v) {
      mX = x;
      mY = y;
      mU = u;
      mV = v;
      }
    //}}}

    float mX = 0.0f;
    float mY = 0.0f;

    float mU = 0.0f;
    float mV = 0.0f;
    };
  //}}}
  //{{{
  struct cPathVertices {
    int mNumFillVertices = 0;
    int mFirstFillVertexIndex = 0;

    int mNumStrokeVertices = 0;
    int mFirstStrokeVertexIndex = 0;
    };
  //}}}
  //{{{
  struct cPath {
    int mNumPoints = 0;
    int mFirstPointIndex = 0;

    eWinding mWinding = eSOLID;
    bool mConvex = false;
    bool mClosed = false;
    int mNumBevel = 0;

    cPathVertices mPathVertices;
    };
  //}}}
  //{{{
  struct cDraw {
    enum eType { TEXT, TRIANGLE, CONVEX_FILL, STENCIL_FILL, STROKE };
    //{{{
    void set (eType type, int id, int firstPathVerticesIndex, int numPaths, int firstFragIndex,
              int firstVertexIndex, int numVertices) {
      mType = type;
      mId = id;

      mFirstPathVerticesIndex = firstPathVerticesIndex;
      mNumPaths = numPaths;

      mFirstFragIndex = firstFragIndex;

      mTriangleFirstVertexIndex = firstVertexIndex;
      mNumTriangleVertices = numVertices;
      }
    //}}}

    eType mType;
    int mId = 0;

    int mFirstPathVerticesIndex = 0;
    int mNumPaths = 0;

    int mFirstFragIndex = 0;

    int mTriangleFirstVertexIndex = 0;
    int mNumTriangleVertices = 0;
    };
  //}}}
  //{{{
  struct cTexture {
    //{{{
    void reset() {
      id = 0;

      type = 0;
      flags = 0;
      tex = 0;

      width = 0;
      height = 0;
      }
    //}}}

    int id = 0;

    int type = 0;
    int flags = 0;
    GLuint tex = 0;

    int width = 0;
    int height = 0;
    };
  //}}}
  //{{{
  struct cFrag {
    //{{{
    void setSimple() {
      type = SHADER_SIMPLE;
      strokeThreshold = -1.0f;
      }
    //}}}
    //{{{
    void setImage (cPaint* paint, cScissor* scissor, cTexture* tex) {

      innerColor = nvgPremulColor (paint->innerColor);
      outerColor = nvgPremulColor (paint->outerColor);

      if ((scissor->extent[0] < -0.5f) || (scissor->extent[1] < -0.5f)) {
        memset (scissorMatrix, 0, sizeof(scissorMatrix));
        scissorExt[0] = 1.0f;
        scissorExt[1] = 1.0f;
        scissorScale[0] = 1.0f;
        scissorScale[1] = 1.0f;
        }
      else {
        cTransform inverse;
        scissor->mTransform.getInverse (inverse);
        inverse.getMatrix3x4 (scissorMatrix);
        scissorExt[0] = scissor->extent[0];
        scissorExt[1] = scissor->extent[1];
        scissorScale[0] = scissor->mTransform.getAverageScaleX();
        scissorScale[1] = scissor->mTransform.getAverageScaleY();
        }

      memcpy (extent, paint->extent, sizeof(extent));
      strokeMult = 1.0f;
      strokeThreshold = -1.0f;

      type = SHADER_IMAGE;
      if (tex->type == TEXTURE_RGBA)
        texType = (tex->flags & cVg::IMAGE_PREMULTIPLIED) ? 0.f : 1.f;
      else
        texType = 2.f;

      cTransform inverse;
      paint->mTransform.getInverse (inverse);
      inverse.getMatrix3x4 (paintMatrix);
      }
    //}}}
    //{{{
    void setFill (cPaint* paint, cScissor* scissor, float width, float fringe, float strokeThreshold, cTexture* tex) {

      innerColor = nvgPremulColor (paint->innerColor);
      outerColor = nvgPremulColor (paint->outerColor);

      if ((scissor->extent[0] < -0.5f) || (scissor->extent[1] < -0.5f)) {
        memset (scissorMatrix, 0, sizeof(scissorMatrix));
        scissorExt[0] = 1.0f;
        scissorExt[1] = 1.0f;
        scissorScale[0] = 1.0f;
        scissorScale[1] = 1.0f;
        }
      else {
        cTransform inverse;
        scissor->mTransform.getInverse (inverse);
        inverse.getMatrix3x4 (scissorMatrix);
        scissorExt[0] = scissor->extent[0];
        scissorExt[1] = scissor->extent[1];
        scissorScale[0] = scissor->mTransform.getAverageScaleX() / fringe;
        scissorScale[1] = scissor->mTransform.getAverageScaleY() / fringe;
        }

      memcpy (extent, paint->extent, sizeof(extent));
      strokeMult = (width * 0.5f + fringe * 0.5f) / fringe;
      this->strokeThreshold = strokeThreshold;

      cTransform inverse;
      if (paint->mImageId) {
        type = SHADER_FILL_IMAGE;
        if ((tex->flags & cVg::IMAGE_FLIPY) != 0) {
          //{{{  flipY
          cTransform m1;
          m1.setTranslate (0.0f, extent[1] * 0.5f);
          m1.multiply (paint->mTransform);

          cTransform m2;
          m2.setScale (1.0f, -1.0f);
          m2.multiply (m1);
          m1.setTranslate (0.0f, -extent[1] * 0.5f);
          m1.multiply (m2);
          m1.getInverse (inverse);
          }
          //}}}
        else
          paint->mTransform.getInverse (inverse);

        if (tex->type == TEXTURE_RGBA)
          texType = (tex->flags & cVg::IMAGE_PREMULTIPLIED) ? 0.f : 1.f;
        else
          texType = 2.f;
        }
      else {
        type = SHADER_FILL_GRADIENT;
        radius = paint->radius;
        feather = paint->feather;
        paint->mTransform.getInverse (inverse);
        }
      inverse.getMatrix3x4 (paintMatrix);
      }
    //}}}

    union {
      struct {
        float scissorMatrix[12]; // 3vec4's
        float paintMatrix[12];   // 3vec4's
        sVgColour innerColor;
        sVgColour outerColor;
        float scissorExt[2];
        float scissorScale[2];
        float extent[2];
        float radius;
        float feather;
        float strokeMult;
        float strokeThreshold;
        float texType;
        float type;
        };
      #define NANOVG_GL_UNIFORMARRAY_SIZE 11
      float uniformArray[NANOVG_GL_UNIFORMARRAY_SIZE][4];
      };
    };
  //}}}
  //{{{
  struct cState {
    cCompositeOpState compositeOp;
    cPaint fillPaint;
    cPaint strokePaint;

    float strokeWidth;
    float miterLimit;
    eLineCap lineJoin;
    eLineCap lineCap;
    float alpha;

    cTransform mTransform;

    cScissor scissor;
    float fontSize;
    float letterSpacing;
    float lineHeight;
    int textAlign;
    int fontId;
    };
  //}}}
  //{{{
  class cShader {
  public:
    ~cShader();

    bool create (const char* opts);
    void getUniforms();

    void setTex (int tex) { glUniform1i (location[LOCATION_TEX], tex); }
    void setViewport (float* viewport) { glUniform2fv (location[LOCATION_VIEWSIZE], 1, viewport); }
    void setFrags (float* frags) { glUniform4fv (location[LOCATION_FRAG], NANOVG_GL_UNIFORMARRAY_SIZE, frags); }

  private:
    void dumpShaderError (GLuint shader, const char* name, const char* type);
    void dumpProgramError (GLuint prog, const char* name);

    // vars
    GLuint prog = 0;
    GLuint frag = 0;
    GLuint vert = 0;

    GLint location[MAX_LOCATIONS];
    };
  //}}}
  //{{{
  class c2dVertices {
  public:
    c2dVertices();
    ~c2dVertices();

    void reset();

    int getNumVertices() { return mNumVertices; }
    c2dVertex* getVertexPtr (int vertexIndex) { return mVertices + vertexIndex; }

    int alloc (int numVertices);
    void trim (int numVertices);

  private:
    static constexpr int kInitNumVertices = 4000;

    c2dVertex* mVertices = nullptr;

    int mNumVertices = 0;
    int mNumAllocatedVertices = 0;
    };
  //}}}
  //{{{
  class cShape {
  public:
    //{{{
    class cPoint {
    public:
      enum eFlags { PT_NONE = 0, PT_CORNER = 0x01, PT_LEFT = 0x02, PT_BEVEL = 0x04, PT_INNERBEVEL = 0x08 };

      float x,y;

      float dx, dy;
      float len;
      float dmx, dmy;

      uint8_t flags;     // set of eFlags
      };
    //}}}

    cShape();
    ~cShape();

    float getLastX() { return mLastX; }
    float getLastY() { return mLastY; }
    int getNumCommands() { return mNumCommands; }

    void addCommand (float* values, int numValues, cTransform& transform);

    int getNumVertices();

    void beginPath();
    void flattenPaths();
    void calculateJoins (float w, int lineJoin, float miterLimit);
    void expandFill (c2dVertices& vertices, float w, eLineCap lineJoin, float miterLimit, float fringeWidth);
    void triangleFill (c2dVertices& vertices, int& vertexIndex, int& numVertices);
    void expandStroke (c2dVertices& vertices, float w, eLineCap lineCap, eLineCap lineJoin, float miterLimit, float fringeWidth);

    // vars
    int mNumPaths = 0;
    cPath* mPaths = nullptr;

    int mNumPoints = 0;
    cPoint* mPoints = nullptr;

    float mBounds[4]; // xmin,ymin,xmax,ymax
    int mBoundsVertexIndex = 0;

  private:
    //{{{  const
    static constexpr int kInitCommandsSize = 256;
    static constexpr float kDistanceTolerance = 0.01f;
    static constexpr float kTesselateTolerance = 0.25f;
    //}}}

    float normalize (float& x, float& y);
    int curveDivs (float r, float arc, float tol);
    int pointEquals (float x1, float y1, float x2, float y2, float tol);
    float polyArea (cPoint* points, int mNumPoints);
    void polyReverse (cPoint* points, int numPoints);

    cPath* lastPath();
    void lastPathWinding (eWinding winding);
    void closeLastPath();
    void addPath();

    void tesselateBezier (float x1, float y1, float x2, float y2,
                               float x3, float y3, float x4, float y4, int level, cPoint::eFlags type);
    cPoint* lastPoint();
    void addPoint (float x, float y, cPoint::eFlags flags);

    void commandsToPaths();

    void chooseBevel (int bevel, cPoint* p0, cPoint* p1, float w, float* x0, float* y0, float* x1, float* y1);
    c2dVertex* roundJoin (c2dVertex* vertexPtr, cPoint* point0, cPoint* point1,
                        float lw, float rw, float lu, float ru, int ncap, float fringe);
    c2dVertex* bevelJoin (c2dVertex* vertexPtr, cPoint* point0, cPoint* point1,
                        float lw, float rw, float lu, float ru, float fringe);

    c2dVertex* buttCapStart (c2dVertex* vertexPtr, cPoint* point, float dx, float dy, float w, float d, float aa);
    c2dVertex* buttCapEnd (c2dVertex* vertexPtr, cPoint* point, float dx, float dy, float w, float d, float aa);
    c2dVertex* roundCapStart (c2dVertex* vertexPtr, cPoint* point, float dx, float dy, float w, int ncap, float aa);
    c2dVertex* roundCapEnd (c2dVertex* vertexPtr, cPoint* point, float dx, float dy, float w, int ncap, float aa);

    // private vars
    int mNumCommands = 0;
    float* mCommands = nullptr;

    float mLastX = 0;
    float mLastY = 0;

    int mNumAllocatedPaths = 0;
    int mNumAllocatedPoints = 0;
    int mNumAllocatedCommands = 0;
    };
  //}}}

  // converts
  cCompositeOpState compositeOpState (eCompositeOp op);
  GLenum convertBlendFuncFactor (eBlendFactor factor);

  // sets
  void setStencilMask (GLuint mask);
  void setStencilFunc (GLenum func, GLint ref, GLuint mask);
  void setBindTexture (GLuint texture);
  void setUniforms (int firstFragIndex, int id);
  void setDevicePixelRatio (float ratio) { devicePixelRatio = ratio; }

  // allocs
  cDraw* allocDraw();
  int allocFrags (int numFrags);
  int allocPathVertices (int numPaths);

  // texture
  int createTexture (int type, int width, int height, int imageFlags, const unsigned char* data, const std::string& debug);
  cTexture* findTextureById (int id);
  bool updateTexture (int id, int x, int y, int width, int height, const unsigned char* data);
  bool getTextureSize (int id, int& width, int& height);

  // render
  void renderText (int firstVertexIndex, int numVertices, cPaint& paint, cScissor& scissor);
  void renderTriangles (int firstVertexIndex, int numVertices, cPaint& paint, cScissor& scissor);
  void renderFill (cShape& shape, cPaint& paint, cScissor& scissor, float fringe);
  void renderStroke (cShape& shape, cPaint& paint, cScissor& scissor, float fringe, float strokeWidth);
  void renderFrame (c2dVertices& vertices, cCompositeOpState compositeOp);

  // font
  float getFontScale (cState* state);
  void flushAtlasTexture();
  bool allocFontAtlas();

  //{{{  vars
  bool mDrawEdges = false;
  bool mDrawSolid = false;
  bool mDrawTriangles = false;
  int mDrawArrays = 0;

  float mViewport[2];
  cShader mShader;

  GLuint mStencilMask = 0;
  GLenum mStencilFunc = 0;
  GLint mStencilFuncRef = 0;
  GLuint mStencilFuncMask = 0;
  GLuint mBindTexture = 0;

  int mTextureId = 0;
  int mNumTextures = 0;
  int mNumAllocatedTextures = 0;
  cTexture* mTextures = nullptr;

  GLuint mVertexBuffer = 0;
  GLuint mVertexArray = 0;
  GLuint mFragBuffer = 0;

  // per frame buffers
  int mNumDraws = 0;
  int mNumAllocatedDraws = 0;
  cDraw* mDraws = nullptr;

  int mNumFrags = 0;
  int mNumAllocatedFrags = 0;
  cFrag* mFrags = nullptr;

  int mNumPathVertices = 0;
  int mNumAllocatedPathVertices = 0;
  cPathVertices* mPathVertices = nullptr;

  int mNumStates = 0;
  cState mStates[kMaxStates];

  cShape mShape;
  c2dVertices mVertices;

  float mFringeWidth = 1.0f;
  float devicePixelRatio = 1.0f;

  cAtlasText* mAtlasText = nullptr;

  // !!! should vector this !!!
  int mFontTextureIndex = 0;
  int mFontTextureIds[kMaxFontTextures] = { 0 };
  //}}}
  };
