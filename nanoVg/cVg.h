// cVg.h - based on Mikko Mononen memon@inside.org nanoVg
#pragma once
//{{{  includes
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <math.h>
//}}}
//{{{  defines
#define MAX_STATES            32

#define NVG_MAX_FONTIMAGES         4
#define NVG_INIT_FONTIMAGE_SIZE  512

#define NVG_INIT_POINTS_SIZE     128
#define NVG_INIT_PATHS_SIZE       16
//}}}

//{{{  maths
#define PI 3.14159265358979323846264338327f

inline int mini (int a, int b) { return a < b ? a : b; }
inline int maxi (int a, int b) { return a > b ? a : b; }
inline int clampi (int a, int mn, int mx) { return a < mn ? mn : (a > mx ? mx : a); }

inline float minf (float a, float b) { return a < b ? a : b; }
inline float maxf (float a, float b) { return a > b ? a : b; }
inline float absf (float a) { return a >= 0.0f ? a : -a; }
inline float signf (float a) { return a >= 0.0f ? 1.0f : -1.0f; }
inline float clampf (float a, float mn, float mx) { return a < mn ? mn : (a > mx ? mx : a); }

inline float cross (float dx0, float dy0, float dx1, float dy1) { return dx1*dy0 - dx0*dy1; }

unsigned int nearestPow2 (unsigned int num);

float degToRad (float deg);
float radToDeg (float rad);
//}}}
//{{{  sVgColour
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

typedef struct sVgColour sVgColour;

sVgColour nvgRGB (unsigned char r, unsigned char g, unsigned char b);
sVgColour nvgRGBf (float r, float g, float b);
sVgColour nvgRGBA (unsigned char r, unsigned char g, unsigned char b, unsigned char a);
sVgColour nvgRGBAf (float r, float g, float b, float a);
sVgColour nvgRGB32 (uint32_t colour);

sVgColour nvgLerpRGBA (sVgColour c0, sVgColour c1, float u);

sVgColour nvgTransRGBA (sVgColour c0, unsigned char a);
sVgColour nvgTransRGBAf (sVgColour c0, float a);

// Returns color value specified by hue, saturation and lightness, values range [0..1], alpha set to 255.
sVgColour nvgHSL (float h, float s, float l);

// Returns color value specified by hue, saturation and lightness and alpha, values ange [0..1], alpha [0..255]
sVgColour nvgHSLA (float h, float s, float l, unsigned char a);

sVgColour nvgPremulColor (sVgColour c);
//}}}

class cVg {
public:
  //{{{
  enum eCreateFlags {
    DRAW_NOSOLID   = 0x01,
    DRAW_NOEDGES   = 0x02,
    DRAW_NOVSYNC   = 0x04,
    IMAGE_NODELETE = 0x10,
    };
  //}}}
  cVg (int flags);
  virtual ~cVg();
  void initialise();

  //{{{
  class cTransform {
  // sx,sy define scaling, kx,ky skewing, tx,ty translation.
  // last row is assumed to be 0,0,1 and not stored.
  //   [sx kx tx]
  //   [ky sy ty]
  //   [ 0  0  1]
  public:
    //{{{
    cTransform() : mIdentity(true) {
      mSx = 1.0f;
      mKy = 0.0f;
      mKx = 0.0f;
      mSy = 1.0f;

      mTx = 0.0f;
      mTy = 0.0f;
      }
    //}}}
    //{{{
    cTransform (float sx, float ky, float kx, float sy, float tx, float ty) {
      mSx = sx;
      mKy = ky;
      mKx = kx;
      mSy = sy;

      mTx = tx;
      mTy = ty;

      mIdentity = isIdentity();
      }
    //}}}

    float getAverageScaleX() { return sqrtf (mSx*mSx + mKx*mKx); }
    float getAverageScaleY() { return sqrtf (mKy*mKy + mSy*mSy); }
    float getAverageScale() { return (getAverageScaleX() + getAverageScaleY()) * 0.5f; }
    float getTranslateX() { return mTx; }
    float getTranslateY() { return mTy; }
    //{{{
    bool getInverse (cTransform& inverse) {

      double det = (double)mSx * mSy - (double)mKx * mKy;
      if (det > -1e-6 && det < 1e-6) {
        inverse.setIdentity();
        return false;
        }

      double inverseDet = 1.0 / det;
      inverse.mSx = (float)(mSy * inverseDet);
      inverse.mKx = (float)(-mKx * inverseDet);
      inverse.mTx = (float)(((double)mKx * mTy - (double)mSy * mTx) * inverseDet);
      inverse.mKy = (float)(-mKy * inverseDet);
      inverse.mSy = (float)(mSx * inverseDet);
      inverse.mTy = (float)(((double)mKy * mTx - (double)mSx * mTy) * inverseDet);
      return true;
      }
    //}}}
    //{{{
    void getMatrix3x4 (float* matrix3x4) {

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

    //{{{
    void setIdentity() {
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
    void setTranslate (float tx, float ty) {
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
    void setScale (float sx, float sy) {
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
    void setRotate (float angle) {
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
    void setRotateTranslate (float angle, float tx, float ty) {
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
    void set (float sx, float ky, float kx, float sy, float tx, float ty) {
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
    void multiply (cTransform& t) {

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
    void premultiply (cTransform& t) {
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
    void point (float& x, float& y) {
    // transform point back to itself

      float temp = (x * mSx) + (y * mKx) + mTx;
      y = (x * mKy) + (y * mSy) + mTy;
      x = temp;
      }
    //}}}
    //{{{
    void point (float srcx, float srcy, float& dstx, float& dsty) {
    // transform src point to dst point

      dstx = (srcx * mSx) + (srcy * mKx) + mTx;
      dsty = (srcx * mKy) + (srcy * mSy) + mTy;
      }
    //}}}
    //{{{
    void pointScissor (float srcx, float srcy, float& dstx, float& dsty) {
      dstx = srcx * absf (mSx) + srcy * absf (mKx);
      dsty = srcx * absf (mKy) + srcy * absf (mSy);
      }
    //}}}

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
  class cPaint {
  public:
    //{{{
    void setColor (sVgColour color) {
      mTransform.setIdentity();
      extent[0] = 0.0f;
      extent[1] = 0.0f;
      radius = 0.0f;
      feather = 1.0f;
      innerColor = color;
      outerColor = color;
      image = 0;
      }
    //}}}

    cTransform mTransform;
    float extent[2];
    float radius;
    float feather;

    sVgColour innerColor;
    sVgColour outerColor;
    int image;
    };
  //}}}
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
    NVG_SOURCE_OVER,           NVG_SOURCE_IN,      NVG_SOURCE_OUT,             NVG_ATOP,
    NVG_DESTINATION_OVER, NVG_DESTINATION_IN, NVG_DESTINATION_OUT, NVG_DESTINATION_ATOP,
    NVG_LIGHTER, NVG_COPY, NVG_XOR };
  //}}}
  //{{{
  class cCompositeOpState {
  public:
    eBlendFactor srcRGB;
    eBlendFactor dstRGB;
    eBlendFactor srcAlpha;
    eBlendFactor dstAlpha;
    };
  //}}}

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

  void fillColor (sVgColour color);
  void fillPaint (cPaint paint);
  void globalAlpha (float alpha);

  void strokeColor (sVgColour color);
  void strokePaint (cPaint paint);
  void strokeWidth (float size);

  void fringeWidth (float width);
  float getFringeWidth() { return mFringeWidth; }

  void miterLimit (float limit);
  void lineCap (eLineCap cap);
  void lineJoin (eLineCap join);

  cPaint linearGradient (float sx, float sy, float ex, float ey, sVgColour icol, sVgColour ocol);
  cPaint boxGradient (float x, float y, float w, float h, float r, float f, sVgColour icol, sVgColour ocol);
  cPaint radialGradient (float cx, float cy, float inr, float outr, sVgColour icol, sVgColour ocol);
  cPaint imagePattern (float ox, float oy, float ex, float ey, float angle, int image, float alpha);

  void beginPath();
  void pathWinding (eWinding dir);
  void moveTo (float x, float y);
  void lineTo (float x, float y);
  void bezierTo (float c1x, float c1y, float c2x, float c2y, float x, float y);
  void quadTo (float cx, float cy, float x, float y);
  void arcTo (float x1, float y1, float x2, float y2, float radius);
  void arc (float cx, float cy, float r, float a0, float a1, int dir);
  void rect (float x, float y, float w, float h);
  void roundedRectVarying (float x, float y, float w, float h,
                           float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft);
  void roundedRect (float x, float y, float w, float h, float r);
  void ellipse (float cx, float cy, float rx, float ry);
  void circle (float cx, float cy, float r);
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
  struct NVGtextRow {
    const char* start; // Pointer to the input text where the row starts.
    const char* end;   // Pointer to the input text where the row ends (one past the last character).
    const char* next;  // Pointer to the beginning of the next row.
    float width;       // Logical width of the row.
    float minx, maxx;  // Actual bounds of the row. Logical with and bounds can differ because of kerning and some parts over extending.
    };

  typedef struct NVGtextRow NVGtextRow;
  //}}}
  //{{{
  struct NVGglyphPosition {
    const char* str;  // Position of the glyph in the input string.
    float x;          // The x-coordinate of the logical glyph position.
    float minx, maxx; // The bounds of the glyph shape.
    };

  typedef struct NVGglyphPosition NVGglyphPosition;
  //}}}

  void fontSize (float size);
  void textLetterSpacing (float spacing);
  void textLineHeight (float lineHeight);
  void textAlign (int align);
  void fontFaceId (int font);
  void fontFace (std::string font);

  int createFont (std::string name, std::string filename);
  int createFontMem (std::string name, unsigned char* data, int ndata, int freeData);

  int findFont (std::string);
  int addFallbackFont (std::string baseFont, std::string fallbackFont);
  int addFallbackFontId (int baseFont, int fallbackFont);

  float text (float x, float y, std::string str);
  void textBox (float x, float y, float breakRowWidth, const char* string, const char* end);
  float textBounds (float x, float y, std::string str, float* bounds);
  void textMetrics (float* ascender, float* descender, float* lineh);
  int textGlyphPositions (float x, float y, std::string str, NVGglyphPosition* positions, int maxPositions);

  void textBoxBounds (float x, float y, float breakRowWidth, const char* string, const char* end, float* bounds);
  int textBreakLines (const char* string, const char* end, float breakRowWidth, NVGtextRow* rows, int maxRows);
  //}}}
  //{{{  image
  //{{{
  enum eImageFlags {
    IMAGE_GENERATE_MIPMAPS = 1<<0, // Generate mipmaps during creation of the image.
    IMAGE_REPEATX          = 1<<1, // Repeat image in X direction.
    IMAGE_REPEATY          = 1<<2, // Repeat image in Y direction.
    IMAGE_FLIPY            = 1<<3, // Flips (inverses) image in Y direction when rendered.
    IMAGE_PREMULTIPLIED    = 1<<4, // Image data has premultiplied alpha.
    IMAGE_NEAREST          = 1<<5, // Image interpolation is Nearest instead Linear
    };
  //}}}
  enum eTexture { TEXTURE_ALPHA = 0x01, TEXTURE_RGBA  = 0x02 };

  int createImage (const char* filename, eImageFlags imageFlags);
  int createImageMem (int imageFlags, unsigned char* data, int ndata);
  int createImageRGBA (int w, int h, int imageFlags, const unsigned char* data);

  void updateImage (int image, const unsigned char* data);
  void imageSize (int image, int* w, int* h);

  void deleteImage (int image);
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
  void beginFrame (int windowWidth, int windowHeight, float devicePixelRatio);
  void endFrame();

  std::string getFrameStats();
  void toggleEdges() { mDrawEdges = !mDrawEdges; }
  void toggleSolid() { mDrawSolid = !mDrawSolid; }
  void toggleText()  { mDrawText  = !mDrawText; }
  //}}}

protected:
  //{{{
  class c2dVertex {
  public:
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
  class c2dVertices {
  public:
    //{{{
    c2dVertices() {
      mVertices = (c2dVertex*)malloc (kInitNumVertices * sizeof(c2dVertex));
      mNumAllocatedVertices = kInitNumVertices;
      }
    //}}}
    //{{{
    ~c2dVertices() {
      free (mVertices);
      }
    //}}}

    //{{{
    void reset() {
      mNumVertices = 0;
      }
    //}}}

    //{{{
    int getNumVertices() {
      return mNumVertices;
      }
    //}}}
    //{{{
    c2dVertex* getVertexPtr (int vertexIndex) {
      return mVertices + vertexIndex;
      }
    //}}}

    //{{{
    int alloc (int numVertices) {
    // allocate n vertices and return index of first

      if (mNumVertices + numVertices > mNumAllocatedVertices) {
        mNumAllocatedVertices = maxi (mNumVertices + numVertices, 4096) + mNumAllocatedVertices/2; // 1.5x Overallocate
        mVertices = (c2dVertex*)realloc (mVertices, mNumAllocatedVertices * sizeof(c2dVertex));
        }

      int firstVertexIndex = mNumVertices;
      mNumVertices += numVertices;
      return firstVertexIndex;
      }
    //}}}
    //{{{
    void trim (int numVertices) {
    // trim vertices used to numVertices

      if (numVertices > mNumVertices)
        printf ("trimVertices overflowed %d %d\n", numVertices, mNumVertices);

      mNumVertices = numVertices;
      }
    //}}}

  private:
    const int kInitNumVertices = 4000;

    c2dVertex* mVertices = nullptr;

    int mNumVertices = 0;
    int mNumAllocatedVertices = 0;
    };
  //}}}
  //{{{
  class cPathVertices {
  public:
    int mNumFillVertices = 0;
    int mFirstFillVertexIndex = 0;

    int mNumStrokeVertices = 0;
    int mFirstStrokeVertexIndex = 0;
    };
  //}}}
  //{{{
  class cPath {
  public:
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

    //{{{
    cShape() {
      mPaths = (cPath*)malloc (sizeof(cPath)*NVG_INIT_PATHS_SIZE);
      mNumAllocatedPaths = NVG_INIT_PATHS_SIZE;

      mPoints = (cPoint*)malloc (sizeof(cPoint)*NVG_INIT_POINTS_SIZE);
      mNumAllocatedPoints = NVG_INIT_POINTS_SIZE;

      mNumAllocatedCommands = kInitCommandsSize;
      mCommands = (float*)malloc (sizeof(float) * kInitCommandsSize);
      }
    //}}}
    //{{{
    ~cShape() {
      free (mPoints);
      free (mPaths);
      free (mCommands);
      }
    //}}}

    //{{{

    float getLastX() {
      return mLastX;
      }
    //}}}
    //{{{

    float getLastY() {
      return mLastY;
      }
    //}}}
    //{{{

    int getNumCommands() {
      return mNumCommands;
      }
    //}}}
    //{{{
    void addCommand (float* values, int numValues, cTransform& transform) {

      if (((int)values[0] == eMOVETO) ||
          ((int)values[0] == eLINETO) ||
          ((int)values[0] == eBEZIERTO)) {
        mLastX = values[numValues-2];
        mLastY = values[numValues-1];
        }

      // transform commands
      if (transform.mIdentity) {
        //{{{  state xform nonIdentity, transform points
        for (int i = 0; i < numValues;) {
          switch ((int)values[i++]) {
            case eMOVETO:
            case eLINETO:
              transform.point (values[i], values[i+1]);
              i += 2;
              break;

            case eBEZIERTO:
              transform.point (values[i], values[i+1]);
              transform.point (values[i+2], values[i+3]);
              transform.point (values[i+4], values[i+5]);
              i += 6;
              break;

            case eWINDING:
              i += 1;
              break;

            case eCLOSE:;
            }
          }
        }
        //}}}

      // copy values to shapeCommands
      if (mNumCommands + numValues > mNumAllocatedCommands) {
        // not enough shapeCommands, expand shapeCommands size by 3/2
        mNumAllocatedCommands = mNumCommands + numValues + mNumAllocatedCommands / 2;
        mCommands = (float*)realloc (mCommands, sizeof(float) * mNumAllocatedCommands);
        }
      memcpy (&mCommands[mNumCommands], values, numValues * sizeof(float));
      mNumCommands += numValues;
      }
    //}}}

    //{{{
    int getNumVertices() {

      int numVertices = 0;
      for (int i = 0; i < mNumPaths; i++)
        numVertices += mPaths[i].mNumPoints;
      return numVertices;
      }
    //}}}

    //{{{
    void beginPath() {
      mNumPaths = 0;
      mNumPoints = 0;
      mNumCommands = 0;
      }
    //}}}
    //{{{
    void flattenPaths() {

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
        if (pointEquals (point0->x, point0->y, point1->x, point1->y, kDistanceTolerance)) {
          path->mNumPoints--;
          point0 = &points[path->mNumPoints-1];
          path->mClosed = true;
          }

        // enforce winding direction
        if (path->mNumPoints > 2) {
         float area = polyArea (points, path->mNumPoints);
          if ((path->mWinding == eSOLID) && (area < 0.0f))
            polyReverse (points, path->mNumPoints);
          if ((path->mWinding == eHOLE) && (area > 0.0f))
            polyReverse (points, path->mNumPoints);
          }

        // calc segment direction, length, update shape bounds
        for (int i = 0; i < path->mNumPoints; i++) {
          point0->dx = point1->x - point0->x;
          point0->dy = point1->y - point0->y;
          point0->len = normalize (point0->dx, point0->dy);

          mBounds[0] = minf (mBounds[0], point0->x);
          mBounds[1] = minf (mBounds[1], point0->y);
          mBounds[2] = maxf (mBounds[2], point0->x);
          mBounds[3] = maxf (mBounds[3], point0->y);

          // advance
          point0 = point1++;
          }
        }
      }
    //}}}
    //{{{
    void calculateJoins (float w, int lineJoin, float miterLimit) {

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
          point1->flags = (point1->flags & cPoint::PT_CORNER) ? cPoint::PT_CORNER : 0;

          // Keep track of left turns.
          cross = point1->dx * point0->dy - point0->dx * point1->dy;
          if (cross > 0.0f) {
            nleft++;
            point1->flags |= cPoint::PT_LEFT;
            }

          // Calculate if we should use bevel or miter for inner join.
          limit = maxf (1.01f, minf (point0->len, point1->len) * iw);
          if ((dmr2 * limit * limit) < 1.0f)
            point1->flags |= cPoint::PT_INNERBEVEL;

          // Check to see if the corner needs to be beveled.
          if (point1->flags & cPoint::PT_CORNER) {
            if (((dmr2 * miterLimit * miterLimit) < 1.0f) || (lineJoin == eBEVEL) || (lineJoin == eROUND)) {
              point1->flags |= cPoint::PT_BEVEL;
              }
            }

          if ((point1->flags & (cPoint::PT_BEVEL | cPoint::PT_INNERBEVEL)) != 0)
            path->mNumBevel++;
          point0 = point1++;
          }

        path->mConvex = (nleft == path->mNumPoints);
        }
      }
    //}}}
    //{{{
    void expandFill (c2dVertices& vertices, float w, eLineCap lineJoin, float miterLimit, float fringeWidth) {

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
            if (point1->flags & cPoint::PT_BEVEL) {
              float dlx0 = point0->dy;
              float dly0 = -point0->dx;
              float dlx1 = point1->dy;
              float dly1 = -point1->dx;
              if (point1->flags & cPoint::PT_LEFT) {
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
          cPoint* point0 = &points[path->mNumPoints-1];
          cPoint* point1 = &points[0];
          for (int j = 0; j < path->mNumPoints; ++j) {
            if ((point1->flags & (cPoint::PT_BEVEL | cPoint::PT_INNERBEVEL)) != 0)
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
    void triangleFill (c2dVertices& vertices, int& vertexIndex, int& numVertices) {

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
    void expandStroke (c2dVertices& vertices, float w, eLineCap lineCap, eLineCap lineJoin, float miterLimit, float fringeWidth) {

      calculateJoins (w, lineJoin, miterLimit);

      // Calculate divisions per half circle.
      int ncap = curveDivs (w, PI, 0.25f);
      //{{{  calculate max vertex usage.
      int numVertices = 0;
      for (auto path = mPaths; path < mPaths + mNumPaths; path++) {
        bool loop = path->mClosed;
        if (lineJoin == eROUND)
          numVertices += (path->mNumPoints + path->mNumBevel * (ncap+2) + 1) * 2; // plus one for loop
        else
          numVertices += (path->mNumPoints + path->mNumBevel*5 + 1) * 2; // plus one for loop
        if (!loop) {
          // space for caps
          if (lineCap == eROUND)
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

        cPoint* point0;
        cPoint* point1;
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
          if (lineCap == eBUTT)
            vertexPtr = buttCapStart (vertexPtr, point0, dx, dy, w, -fringeWidth*0.5f, fringeWidth);
          else if (lineCap == eBUTT || lineCap == eSQUARE)
            vertexPtr = buttCapStart (vertexPtr, point0, dx, dy, w, w-fringeWidth, fringeWidth);
          else if (lineCap == eROUND)
            vertexPtr = roundCapStart (vertexPtr, point0, dx, dy, w, ncap, fringeWidth);
          }
          //}}}

        for (int j = s; j < e; ++j) {
          if ((point1->flags & (cPoint::PT_BEVEL | cPoint::PT_INNERBEVEL)) != 0) {
            if (lineJoin == eROUND)
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

          if (lineCap == eBUTT)
            vertexPtr = buttCapEnd (vertexPtr, point1, dx, dy, w, -fringeWidth * 0.5f, fringeWidth);
          else if (lineCap == eBUTT || lineCap == eSQUARE)
            vertexPtr = buttCapEnd (vertexPtr, point1, dx, dy, w, w - fringeWidth, fringeWidth);
          else if (lineCap == eROUND)
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

    // vars
    int mNumPaths = 0;
    cPath* mPaths = nullptr;

    int mNumPoints = 0;
    cPoint* mPoints = nullptr;

    float mBounds[4]; // xmin,ymin,xmax,ymax
    int mBoundsVertexIndex = 0;

  private:
    //{{{  const
    const int kInitCommandsSize = 256;
    const float kDistanceTolerance = 0.01f;
    const float kTesselateTolerance = 0.25f;
    //}}}

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
    int curveDivs (float r, float arc, float tol) {
      float da = acosf (r / (r + tol)) * 2.0f;
      return maxi (2, (int)ceilf(arc / da));
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
    float polyArea (cPoint* points, int mNumPoints) {

      float area = 0;
      for (int i = 2; i < mNumPoints; i++)
        area += (points[i].x - points[0].x) * (points[i-1].y - points[0].y) -
                (points[i-1].x - points[0].x) *  (points[i].y - points[0].y);
      return area * 0.5f;
      }
    //}}}
    //{{{
    void polyReverse (cPoint* points, int numPoints) {

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
    cPath* lastPath() {
      return mNumPaths ? &mPaths[mNumPaths-1] : NULL;
      }
    //}}}
    //{{{
    void lastPathWinding (eWinding winding) {
      auto path = lastPath();
      if (path == NULL)
        return;
      path->mWinding = winding;  }
    //}}}
    //{{{
    void closeLastPath() {
      auto path = lastPath();
      if (path == NULL)
        return;
      path->mClosed = true;
      }
    //}}}
    //{{{
    void addPath() {

      if (mNumPaths+1 > mNumAllocatedPaths) {
        mNumAllocatedPaths = mNumPaths+1 + mNumAllocatedPaths / 2;
        mPaths = (cPath*)realloc (mPaths, sizeof(cPath)* mNumAllocatedPaths);
        }

      auto path = &mPaths[mNumPaths];
      memset (path, 0, sizeof(*path));
      path->mFirstPointIndex = mNumPoints;
      path->mWinding = eSOLID;

      mNumPaths++;
      }
    //}}}

    //{{{
    void tesselateBezier (float x1, float y1, float x2, float y2,
                               float x3, float y3, float x4, float y4, int level, cPoint::eFlags type) {

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

      tesselateBezier (x1,y1, x12,y12, x123,y123, x1234,y1234, level+1, cPoint::PT_NONE);
      tesselateBezier (x1234,y1234, x234,y234, x34,y34, x4,y4, level+1, type);
      }
    //}}}
    //{{{
    cPoint* lastPoint() {
      return mNumPoints ? &mPoints[mNumPoints-1] : NULL;
      }
    //}}}
    //{{{
    void addPoint (float x, float y, cPoint::eFlags flags) {

      auto path = lastPath();
      if (path == NULL)
        return;

      if ((path->mNumPoints > 0) && (mNumPoints > 0)) {
        auto point = lastPoint();
        if (pointEquals (point->x, point->y, x, y, kDistanceTolerance)) {
          point->flags |= flags;
          return;
          }
        }

      // allocate point in pathCache
      if (mNumPoints+1 > mNumAllocatedPoints) {
        mNumAllocatedPoints = mNumPoints+1 + mNumAllocatedPoints / 2;
        mPoints = (cPoint*)realloc (mPoints, sizeof(cPoint)*mNumAllocatedPoints);
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
    void commandsToPaths() {
    // convert shapeCommands to paths,points

      auto command = mCommands;
      while (command < mCommands + mNumCommands) {
        switch ((int)*command++) {
          case eMOVETO:
            addPath();
          case eLINETO:
            addPoint (*command, *(command+1), cPoint::PT_CORNER);
            command += 2;
            break;

          case eBEZIERTO: {
            auto last = lastPoint();
            if (last != NULL)
              tesselateBezier (last->x, last->y,
                               *command, *(command+1),
                               *(command+2), *(command+3),
                               *(command+4), *(command+5),
                               0, cPoint::PT_CORNER);
            command += 6;
            break;
            }

          case eWINDING:
            lastPathWinding ((eWinding)(int)*command++);
            break;

          case eCLOSE:
            closeLastPath();
          }
        }
      }
    //}}}

    //{{{
    void chooseBevel (int bevel, cPoint* p0, cPoint* p1, float w, float* x0, float* y0, float* x1, float* y1) {

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
    c2dVertex* roundJoin (c2dVertex* vertexPtr, cPoint* point0, cPoint* point1,
                        float lw, float rw, float lu, float ru, int ncap, float fringe) {
      int i, n;
      float dlx0 = point0->dy;
      float dly0 = -point0->dx;
      float dlx1 = point1->dy;
      float dly1 = -point1->dx;

      if (point1->flags & cPoint::PT_LEFT) {
        float lx0,ly0,lx1,ly1,a0,a1;
        chooseBevel (point1->flags & cPoint::PT_INNERBEVEL, point0, point1, lw, &lx0,&ly0, &lx1,&ly1);
        a0 = atan2f (-dly0, -dlx0);
        a1 = atan2f (-dly1, -dlx1);
        if (a1 > a0)
          a1 -= PI*2;

        vertexPtr++->set (lx0, ly0, lu,1);
        vertexPtr++->set (point1->x - dlx0*rw, point1->y - dly0*rw, ru,1);

        n = clampi ((int)ceilf (((a0 - a1) / PI) * ncap), 2, ncap);
        for (i = 0; i < n; i++) {
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
        chooseBevel (point1->flags & cPoint::PT_INNERBEVEL, point0, point1, -rw, &rx0,&ry0, &rx1,&ry1);
        a0 = atan2f (dly0, dlx0);
        a1 = atan2f (dly1, dlx1);
        if (a1 < a0)
          a1 += PI *2;

        vertexPtr++->set (point1->x + dlx0*rw, point1->y + dly0*rw, lu,1);
        vertexPtr++->set (rx0, ry0, ru,1);

        n = clampi ((int)ceilf(((a1 - a0) / PI) * ncap), 2, ncap);
        for (i = 0; i < n; i++) {
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
    c2dVertex* bevelJoin (c2dVertex* vertexPtr, cPoint* point0, cPoint* point1,
                        float lw, float rw, float lu, float ru, float fringe) {

      float rx0, ry0, rx1, ry1;
      float lx0, ly0, lx1 ,ly1;

      float dlx0 = point0->dy;
      float dly0 = -point0->dx;
      float dlx1 = point1->dy;
      float dly1 = -point1->dx;

      if (point1->flags & cPoint::PT_LEFT) {
        chooseBevel (point1->flags & cPoint::PT_INNERBEVEL, point0, point1, lw, &lx0, &ly0, &lx1, &ly1);
        vertexPtr++->set (lx0, ly0, lu,1);
        vertexPtr++->set (point1->x - dlx0*rw, point1->y - dly0*rw, ru,1);

        if (point1->flags & cPoint::PT_BEVEL) {
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
        chooseBevel (point1->flags & cPoint::PT_INNERBEVEL, point0, point1, -rw, &rx0,&ry0, &rx1,&ry1);
        vertexPtr++->set (point1->x + dlx0*lw, point1->y + dly0*lw, lu,1);
        vertexPtr++->set (rx0, ry0, ru,1);

        if (point1->flags & cPoint::PT_BEVEL) {
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
    c2dVertex* buttCapStart (c2dVertex* vertexPtr, cPoint* point, float dx, float dy, float w, float d, float aa) {

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
    c2dVertex* buttCapEnd (c2dVertex* vertexPtr, cPoint* point, float dx, float dy, float w, float d, float aa) {

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
    c2dVertex* roundCapStart (c2dVertex* vertexPtr, cPoint* point, float dx, float dy, float w, int ncap, float aa) {

      float px = point->x;
      float py = point->y;
      float dlx = dy;
      float dly = -dx;

      for (int i = 0; i < ncap; i++) {
        float a = i / (float)(ncap-1) * PI;
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
    c2dVertex* roundCapEnd (c2dVertex* vertexPtr, cPoint* point, float dx, float dy, float w, int ncap, float aa) {

      float px = point->x;
      float py = point->y;
      float dlx = dy;
      float dly = -dx;

      vertexPtr++->set (px + dlx * w, py + dly * w, 0, 1);
      vertexPtr++->set (px - dlx * w, py - dly * w, 1, 1);
      for (int i = 0; i < ncap; i++) {
        float a = i / (float)(ncap-1) * PI;
        float ax = cosf(a) * w;
        float ay = sinf(a) * w;
        vertexPtr++->set (px, py, 0.5f, 1);
        vertexPtr++->set (px - dlx * ax + dx * ay, py - dly * ax + dy * ay, 0, 1);
        }

      return vertexPtr;
      }
    //}}}

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
  //{{{
  class cScissor {
  public:
    cTransform mTransform;
    float extent[2];
    };
  //}}}

  virtual void renderViewport (int width, int height, float devicePixelRatio) = 0;
  virtual void renderText (int firstVertexIndex, int numVertices, cPaint& paint, cScissor& scissor) = 0;
  virtual void renderTriangles (int firstVertexIndex, int numVertices, cPaint& paint, cScissor& scissor) = 0;
  virtual void renderFill (cShape& shape, cPaint& paint, cScissor& scissor, float fringe) = 0;
  virtual void renderStroke (cShape& shape, cPaint& paint, cScissor& scissor, float fringe, float strokeWidth) = 0;
  virtual void renderFrame (c2dVertices& vertices, cCompositeOpState compositeOp) = 0;

  virtual int renderCreateTexture (int type, int w, int h, int imageFlags, const unsigned char* data) = 0;
  virtual bool renderDeleteTexture (int image) = 0;
  virtual bool renderUpdateTexture (int image, int x, int y, int w, int h, const unsigned char* data) = 0;
  virtual bool renderGetTextureSize (int image, int* w, int* h) = 0;

  bool mDrawEdges = false;
  bool mDrawSolid = false;
  bool mDrawText = false;
  int mDrawArrays = 0;

private:
  //{{{
  class cState {
  public:
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

  void setDevicePixelRatio (float ratio);
  cCompositeOpState compositeOpState (eCompositeOp op);
  //{{{  text
  float getFontScale (cState* state);

  void flushTextTexture();
  int allocTextAtlas();
  //}}}

  //{{{  vars
  int mNumStates = 0;
  cState mStates[MAX_STATES];

  cShape mShape;
  c2dVertices mVertices;

  float mFringeWidth = 1.0f;
  float devicePixelRatio = 1.0f;

  struct FONScontext* fonsContext = NULL;
  int fontImageIdx = 0;
  int fontImages[NVG_MAX_FONTIMAGES];
  //}}}
  };
