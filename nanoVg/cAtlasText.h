// cAtlasText.h
#pragma once
#include <string>

struct stbtt_fontinfo;
class cAtlasText {
public:
  //{{{  static constexpr
  static constexpr int kHashLutSize = 256;

  static constexpr int kInitFontImageSize = 512;
  static constexpr int kMaxFontImageSize = 2048;

  static constexpr int kScratchBufSize = 64000;

  static constexpr int kInvalid = -1;
  //}}}
  //{{{
  enum eAlign {
    // Horizontal align
    ALIGN_LEFT     = 1<<0, // Default
    ALIGN_CENTER   = 1<<1,
    ALIGN_RIGHT    = 1<<2,

    // Vertical align
    ALIGN_TOP      = 1<<3,
    ALIGN_MIDDLE   = 1<<4,
    ALIGN_BOTTOM   = 1<<5,
    ALIGN_BASELINE = 1<<6, // Default
    };
  //}}}
  //{{{
  enum eErrorCode {
    // Font atlas is full.
    ATLAS_FULL = 1,

    // Scratch memory used to render glyphs is full, requested size reported in 'val', you may need to bump up FONS_SCRATCH_BUF_SIZE.
    SCRATCH_FULL = 2,

    // Calls to fonsPushState has created too large stack, if you need deep state stack bump up FONS_MAX_STATES.
    STATES_OVERFLOW = 3,

    // Trying to pop too many states fonsPopState().
    STATES_UNDERFLOW = 4,
    };
  //}}}
  //{{{
  struct sQuad {
    float x0 = 0.f;
    float y0 = 0.f;
    float s0 = 0.f;
    float t0 = 0.f;

    float x1 = 0.f;
    float y1 = 0.f;
    float s1 = 0.f;
    float t1 = 0.f;
    };
  //}}}
  //{{{
  struct sGlyph {
    unsigned int codepoint = 0;
    int index = 0;
    int next = 0;

    short size = 0;
    short x0 = 0;
    short y0 = 0;
    short x1 = 0;
    short y1 = 0;
    short xadv = 0;
    short xoff = 0;
    short yoff = 0;
    };
  //}}}
  //{{{
  struct cFont {
  public:
    std::string name;

    stbtt_fontinfo* mFontInfo;
    unsigned char* data = nullptr;
    int dataSize = 0;

    float ascender = 0.f;
    float descender = 0.f;
    float lineh = 0.f;

    sGlyph* glyphs = nullptr;
    int cglyphs = 0;
    int nglyphs = 0;

    int lut[kHashLutSize] = { 0 };
    };
  //}}}
  //{{{
  struct sTextIt {
    float x = 0.f;
    float y = 0.f;

    float nextx = 0.f;
    float nexty = 0.f;

    float scale = 0.f;
    float spacing = 0.f;

    unsigned int codepoint = 0;

    short isize = 0;

    cFont* font = nullptr;
    int prevGlyphIndex = 0;

    const char* str = nullptr;
    const char* next = nullptr;
    const char* end = nullptr;

    unsigned int utf8state = 0;
    };
  //}}}

  cAtlasText (int width, int height);
  ~cAtlasText();

  int addFont (const std::string& name, unsigned char* data, int dataSize);
  int resetAtlas (int width, int height);

  int getFontByName (const std::string& name);
  sGlyph* getGlyph (cFont* font, unsigned int codepoint, short isize);
  float getVertAlign (cFont* font, int align, short isize);
  float vertMetrics (float& ascender, float& descender);
  void lineBounds (float y, float* miny, float* maxy);
  float textBounds (float x, float y, const char* str, const char* end, float* bounds);

  void setSize (float size);
  void setColor (unsigned int color);
  void setSpacing (float spacing);
  void setAlign (int align);
  void setFont (int font);

  int textItInit (sTextIt* it, float x, float y, const char* str, const char* end);
  int textItNext (sTextIt* it, sQuad* quad);

  const unsigned char* getTextureData (int* width, int* height);
  int validateTexture (int* dirty);

  // vars
  int mScratchBufSize;
  unsigned char* mScratchBuf;

private:
  //{{{  static constexpr
  static constexpr int kMaxFontStates = 20;

  static constexpr int kInitFonts = 4;
  static constexpr int kInitGlyphs = 256;
  static constexpr int kInitAtlasNodes = 256;
  static constexpr int kVertexCount = 1024;
  //}}}
  //{{{
  class cAtlas {
  public:
    //{{{
    struct sNode {
      short mX;
      short mY;
      short mWidth;
      };
    //}}}

    //{{{
    cAtlas (int w, int h, int maxNumNodes) {

      mWidth = w;
      mHeight = h;

      // Allocate space for skyline nodes
      mNodes = (sNode*)malloc(sizeof(sNode) * maxNumNodes);
      mNumNodes = 0;
      mMaxNumNodes = maxNumNodes;

      // Init root node.
      mNodes[0].mX = 0;
      mNodes[0].mY = 0;
      mNodes[0].mWidth = (short)w;

      mNumNodes++;
      }
    //}}}
    //{{{
    // Atlas based on Skyline Bin Packer by Jukka Jylänki
    ~cAtlas() {

      if (mNodes != NULL)
        free (mNodes);
      }
    //}}}

    //{{{
    void reset (int w, int h) {

      mWidth = w;
      mHeight = h;
      mNumNodes = 0;

      // Init root node.
      mNodes[0].mX = 0;
      mNodes[0].mY = 0;
      mNodes[0].mWidth = (short)w;
      mNumNodes++;
      }
    //}}}
    //{{{
    void expand (int w, int h) {

      // Insert node for empty space
      if (w > mWidth)
        insertNode (mNumNodes, mWidth, 0, w - mWidth);

      mWidth = w;
      mHeight = h;
      }
    //}}}

    //{{{
    int addRect (int rw, int rh, int* rx, int* ry) {

      int besth = mHeight, bestw = mWidth, besti = -1;
      int bestx = -1, besty = -1, i;

      // Bottom left fit heuristic.
      for (i = 0; i < mNumNodes; i++) {
        int y = rectFits (i, rw, rh);
        if (y != -1) {
          if (y + rh < besth || (y + rh == besth && mNodes[i].mWidth < bestw)) {
            besti = i;
            bestw = mNodes[i].mWidth;
            besth = y + rh;
            bestx = mNodes[i].mX;
            besty = y;
            }
          }
        }

      if (besti == -1)
        return 0;

      // Perform the actual packing.
      if (addSkylineLevel (besti, bestx, besty, rw, rh) == 0)
        return 0;

      *rx = bestx;
      *ry = besty;

      return 1;
      }
    //}}}

    int mNumNodes = 0;
    int mMaxNumNodes = 0;
    sNode* mNodes = nullptr;

  private:
    //{{{
    int insertNode (int index, int x, int y, int w) {

      // Insert node
      if (mNumNodes+1 > mMaxNumNodes) {
        mMaxNumNodes = mMaxNumNodes == 0 ? 8 : mMaxNumNodes * 2;
        mNodes = (sNode*)realloc (mNodes, sizeof(sNode) * mMaxNumNodes);
        }

      for (int i = mNumNodes; i > index; i--)
        mNodes[i] = mNodes[i-1];
      mNodes[index].mX = (short)x;
      mNodes[index].mY = (short)y;
      mNodes[index].mWidth = (short)w;
      mNumNodes++;

      return 1;
      }
    //}}}
    //{{{
    void removeNode (int index) {

      if (mNumNodes == 0)
        return;

      for (int i = index; i < mNumNodes-1; i++)
        mNodes[i] = mNodes[i+1];

      mNumNodes--;
      }
    //}}}

    //{{{
    int rectFits (int i, int w, int h) {
    // Checks if there is enough space at the location of skyline span 'i',
    // and return the max mHeight of all skyline spans under that at that location,
    // (think tetris block being dropped at that position). Or -1 if no space found.

      int x = mNodes[i].mX;
      int y = mNodes[i].mY;

      if (x + w > mWidth)
        return -1;

      int spaceLeft = w;
      while (spaceLeft > 0) {
        if (i == mNumNodes)
         return -1;
        y = maxi (y, mNodes[i].mY);
        if (y + h > mHeight)
          return -1;
        spaceLeft -= mNodes[i].mWidth;
        ++i;
        }
      return y;
      }
    //}}}
    //{{{
    int addSkylineLevel (int index, int x, int y, int w, int h) {

      // Insert new node
      if (insertNode (index, x, y+h, w) == 0)
        return 0;

      // Delete skyline segments that fall under the shadow of the new segment.
      for (int i = index+1; i < mNumNodes; i++) {
        if (mNodes[i].mX < mNodes[i-1].mX + mNodes[i-1].mWidth) {
          int shrink = mNodes[i-1].mX + mNodes[i-1].mWidth - mNodes[i].mX;
          mNodes[i].mX += (short)shrink;
          mNodes[i].mWidth -= (short)shrink;
          if (mNodes[i].mWidth <= 0) {
            removeNode (i);
            i--;
            }
          else
            break;
          }
        else
          break;
        }

      // Merge same mHeight skyline segments that are next to each other.
      for (int i = 0; i < mNumNodes-1; i++) {
        if (mNodes[i].mY == mNodes[i+1].mY) {
          mNodes[i].mWidth += mNodes[i+1].mWidth;
          removeNode (i+1);
          i--;
          }
        }

      return 1;
      }
    //}}}

    int mWidth = 0;
    int mHeight = 0;
    };
  //}}}

  //{{{
  struct sFontState {
    int font;
    int align;
    float size;
    unsigned int color;
    float spacing;
    };
  //}}}
  sFontState* getState();
  void pushState();
  void popState();
  void clearState();

  void ttGetFontVMetrics (stbtt_fontinfo* fontInfo, int* ascent, int* descent, int* lineGap);
  float ttGetPixelHeightScale (stbtt_fontinfo* fontInfo, float size);
  int ttGetGlyphIndex (stbtt_fontinfo* fontInfo, int codepoint);
  int ttGetGlyphKernAdvance (stbtt_fontinfo* fontInfo, int glyph1, int glyph2);
  int ttBuildGlyphBitmap (stbtt_fontinfo* fontInfo, int glyph, float size, float scale,
                          int *advance, int *lsb, int *x0, int *y0, int *x1, int *y1);
  void ttRenderGlyphBitmap (stbtt_fontinfo* fontInfo, unsigned char* output,
                            int outWidth, int outHeight, int outStride,
                            float scaleX, float scaleY, int glyph);

  int allocFont();
  void freeFont (cFont* font);

  void getQuad (cFont* font, int prevGlyphIndex, sGlyph* glyph,
                float scale, float spacing, float* x, float* y, sQuad* q);

  sGlyph* allocGlyph (cFont* font);
  void getAtlasSize (int* width, int* height);
  int expandAtlas (int width, int height);

  void flush();
  //{{{  vars
  int mWidth = 0;
  int mHeight = 0;
  float itw = 0.f;
  float ith = 0.f;

  int mNumStates = 0;
  sFontState states[kMaxFontStates];

  unsigned char* texData = nullptr;
  int dirtyRect[4] = { 0 };

  int mNumAllocatedFonts = 0;
  int mNumFonts = 0;
  cAtlas* mAtlas = nullptr;
  cFont** mFonts = { nullptr };
  //}}}
  };
