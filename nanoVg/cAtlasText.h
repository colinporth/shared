// cAtlasText.h
#pragma once
//{{{  includes
#include <cstdint>
#include <vector>
#include <string>
//}}}

struct stbtt_fontinfo;
class cAtlasText {
public:
  //{{{  static constexpr
  static constexpr int kHashLutSize = 256;
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
  public:
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
  public:
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

    stbtt_fontinfo* mFontInfo = nullptr;
    uint8_t* mData = nullptr;
    int mDataSize = 0;

    float mLineh = 0.f;
    float mAscender = 0.f;
    float mDescender = 0.f;

    int mNumGlyphs = 0;
    int mNumAllocatedGlyphs = 0;
    sGlyph* mGlyphs = nullptr;

    int mHashLut[kHashLutSize] = { -1 };
    };
  //}}}
  //{{{
  struct sTextIt {
  public:
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

  int addFont (const std::string& name, uint8_t* data, int dataSize);
  int resetAtlas (int width, int height);

  // gets
  int getFontByName (const std::string& name);
  sGlyph* getGlyph (cFont* font, unsigned int codepoint, short isize);
  float getVertAlign (cFont* font, int align, short isize);
  float getVertMetrics (float& ascender, float& descender);
  void getLineBounds (float y, float& miny, float& maxy);
  float getTextBounds (float x, float y, const char* str, const char* end, float* bounds);

  int getAtlasDirty (int* dirty);
  const uint8_t* getAtlasTextureData (int& width, int& height);

  // sets
  void setColor (unsigned int color);
  void setFont (int font);
  void setSize (float size);
  void setSpacing (float spacing);
  void setAlign (int align);
  void setFontSizeSpacingAlign (int font, float size, float spacing, int align);

  int textIt (sTextIt* it, float x, float y, const char* str, const char* end);
  int textItNext (sTextIt* it, sQuad* quad);

  // vars
  int mScratchBufSize;
  uint8_t* mScratchBuf;

private:
  //{{{  static constexpr
  static constexpr int kInitFonts = 4;
  static constexpr int kInitGlyphs = 256;
  static constexpr int kInitAtlasNodes = 256;
  static constexpr int kMaxFontStates = 20;
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

    cAtlas (int w, int h, int maxNumNodes);
    ~cAtlas();

    void reset (int w, int h);
    void expand (int w, int h);
    int addRect (int rw, int rh, int* rx, int* ry);

    int mNumNodes = 0;
    int mMaxNumNodes = 0;
    sNode* mNodes = nullptr;

  private:
    int insertNode (int index, int x, int y, int w);
    void removeNode (int index);

    int rectFits (int i, int w, int h);
    int addSkylineLevel (int index, int x, int y, int w, int h);

    int mWidth = 0;
    int mHeight = 0;
    };
  //}}}
  //{{{
  struct sFontState {
    int font = 0;
    int align = 0;
    float size = 0.f;
    float spacing = 0.f;
    unsigned int color = 0;
    };
  //}}}

  sFontState* getState();
  void clearState();
  void pushState();
  void popState();

  void ttGetFontVMetrics (stbtt_fontinfo* fontInfo, int* ascent, int* descent, int* lineGap);
  float ttGetPixelHeightScale (stbtt_fontinfo* fontInfo, float size);
  int ttGetGlyphIndex (stbtt_fontinfo* fontInfo, int codepoint);
  int ttGetGlyphKernAdvance (stbtt_fontinfo* fontInfo, int glyph1, int glyph2);
  int ttBuildGlyphBitmap (stbtt_fontinfo* fontInfo, int glyph, float size, float scale,
                          int *advance, int *lsb, int *x0, int *y0, int *x1, int *y1);
  void ttRenderGlyphBitmap (stbtt_fontinfo* fontInfo, uint8_t* output,
                            int outWidth, int outHeight, int outStride,
                            float scaleX, float scaleY, int glyph);

  int allocFont();
  void freeFont (cFont* font);

  void getQuad (cFont* font, int prevGlyphIndex, sGlyph* glyph,
                float scale, float spacing, float* x, float* y, sQuad* q);

  sGlyph* allocGlyph (cFont* font);
  int expandAtlas (int width, int height);
  void flushPendingGlyphs();

  //{{{  vars
  int mWidth = 0;
  int mHeight = 0;
  float itw = 0.f;
  float ith = 0.f;

  int mNumStates = 0;
  sFontState states[kMaxFontStates];

  uint8_t* texData = nullptr;
  int dirtyRect[4] = { 0 };

  cAtlas* mAtlas = nullptr;
  std::vector <cFont*> mFonts;
  //}}}
  };
