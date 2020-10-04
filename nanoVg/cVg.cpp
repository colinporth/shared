// cVg.cpp - based on Mikko Mononen memon@inside.org nanoVg
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include "cVg.h"
#include "stb_image.h"

using namespace std;
//}}}
//{{{  constexpr
constexpr float kDistanceTolerance = 0.01f;
constexpr int kInitPathSize = 16;
constexpr int kInitPointsSize = 128;
constexpr float KAPPA90 = 0.5522847493f; // Length proportional to radius of a cubic bezier handle for 90deg arcs.
//}}}

//{{{  cFontContext, stbtt
static void* fontAlloc (size_t size, void* up);
#define STBTT_malloc(x,u) fontAlloc (x,u)

static void fontFree (void* ptr, void* up) {}
#define STBTT_free(x,u) fontFree (x,u)
#include "stb_truetype.h"

//{{{
class cFontContext {
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
    float x0;
    float y0;
    float s0;
    float t0;

    float x1;
    float y1;
    float s1;
    float t1;
    };
  //}}}
  //{{{
  struct sGlyph {
    unsigned int codepoint;
    int index;
    int next;

    short size;
    short x0;
    short y0;
    short x1;
    short y1;
    short xadv;
    short xoff;
    short yoff;
    };
  //}}}
  //{{{
  struct sFont {
    string name;

    stbtt_fontinfo fontInfo;
    unsigned char* data;
    int dataSize;

    float ascender;
    float descender;
    float lineh;

    sGlyph* glyphs;
    int cglyphs;
    int nglyphs;

    int lut[kHashLutSize];
    };
  //}}}
  //{{{
  struct sTextIt {
    float x;
    float y;

    float nextx;
    float nexty;

    float scale;
    float spacing;

    unsigned int codepoint;

    short isize;

    struct sFont* font;
    int prevGlyphIndex;

    const char* str;
    const char* next;
    const char* end;

    unsigned int utf8state;
    };
  //}}}

  //{{{
  cFontContext (int width, int height) {

    // allocate scratch buffer.
    mScratchBuf = (unsigned char*)malloc (kScratchBufSize);

    mAtlas = new cAtlas (width, height, kInitAtlasNodes);

    // Allocate space for fonts.
    mFonts = (sFont**)malloc (sizeof(sFont*) * kInitFonts);
    memset (mFonts, 0, sizeof(sFont*) * kInitFonts);
    mNumAllocatedFonts = kInitFonts;
    mNumFonts = 0;

    // Create texture for the cache.
    itw = 1.0f / width;
    ith = 1.0f / height;
    texData = (unsigned char*)malloc (width * height);
    memset (texData, 0, width * height);

    dirtyRect[0] = width;
    dirtyRect[1] = height;
    dirtyRect[2] = 0;
    dirtyRect[3] = 0;

    // Add white rect at 0,0 for debug drawing.
    addWhiteRect (2,2);
    pushState();
    clearState();
    }
  //}}}
  //{{{
  ~cFontContext() {

    for (int i = 0; i < mNumFonts; ++i)
      freeFont (mFonts[i]);
    free (mFonts);

    delete mAtlas;

    free (texData);
    free (mScratchBuf);
    }
  //}}}

  //{{{
  int addFont (const string& name, unsigned char* data, int dataSize) {

    int idx = allocFont();
    if (idx == kInvalid)
      return kInvalid;

    auto font = mFonts[idx];
    font->name = name;

    // Init hash lookup.
    for (int i = 0; i < kHashLutSize; ++i)
      font->lut[i] = -1;

    // Read in the font data, point to us for alloc context
    font->data = data;
    font->dataSize = dataSize;
    font->fontInfo.userdata = this;

    // Init font
    mScratchBufSize = 0;
    if (!stbtt_InitFont (&font->fontInfo, data, 0)) {
      freeFont (font);
      mNumFonts--;
      return kInvalid;
      }

    // Store normalized line height. The real line height is got
    // by multiplying the lineh by font size.
    int ascent, descent, lineGap;
    ttGetFontVMetrics (&font->fontInfo, &ascent, &descent, &lineGap);
    int fh = ascent - descent;

    font->ascender = (float)ascent / (float)fh;
    font->descender = (float)descent / (float)fh;
    font->lineh = (float)(fh + lineGap) / (float)fh;

    return idx;
    }
  //}}}
  //{{{
  int getFontByName (const string& name) {

    for (int i = 0; i < mNumFonts; i++)
      if (mFonts[i]->name == name)
        return i;

    return kInvalid;
    }
  //}}}

  //{{{
  int resetAtlas (int width, int height) {

    // Flush pending glyphs.
    flush();

    // Reset atlas
    mAtlas->reset (width, height);

    // Clear texture data
    texData = (unsigned char*)realloc(texData, width * height);
    if (texData == NULL)
      return 0;
    memset (texData, 0, width * height);

    // Reset dirty rect
    dirtyRect[0] = width;
    dirtyRect[1] = height;
    dirtyRect[2] = 0;
    dirtyRect[3] = 0;

    // Reset cached glyphs
    int i, j;
    for (i = 0; i < mNumFonts; i++) {
      sFont* font = mFonts[i];
      font->nglyphs = 0;
      for (j = 0; j < kHashLutSize; j++)
        font->lut[j] = -1;
      }

    mWidth = width;
    mHeight = height;
    itw = 1.0f / mWidth;
    ith = 1.0f / mHeight;

    // Add white rect at 0,0 for debug drawing.
    addWhiteRect (2,2);

    return 1;
    }
  //}}}

  void setSize (float size) { getState()->size = size; }
  void setColor (unsigned int color) { getState()->color = color; }
  void setSpacing (float spacing) { getState()->spacing = spacing; }
  void setAlign (int align) { getState()->align = align; }
  void setFont (int font) { getState()->font = font; }

  //{{{
  sGlyph* getGlyph (sFont* font, unsigned int codepoint, short isize) {

    int i, g, advance, lsb, x0, y0, x1, y1, gw, gh, gx, gy, x, y;
    float scale;
    sGlyph* glyph = NULL;
    unsigned int h;
    float size = isize/10.0f;
    int pad, added;
    unsigned char* dst;
    sFont* renderFont = font;

    if (isize < 2)
      return NULL;
    pad = 2;

    // Reset allocator.
    mScratchBufSize = 0;

    // Find code point and size.
    h = hashint (codepoint) & (kHashLutSize -1);
    i = font->lut[h];
    while (i != -1) {
      if (font->glyphs[i].codepoint == codepoint && font->glyphs[i].size == isize)
        return &font->glyphs[i];
      i = font->glyphs[i].next;
      }

    // Could not find glyph, create it.
    g = ttGetGlyphIndex (&font->fontInfo, codepoint);

    scale = ttGetPixelHeightScale (&renderFont->fontInfo, size);
    ttBuildGlyphBitmap (&renderFont->fontInfo, g, size, scale, &advance, &lsb, &x0, &y0, &x1, &y1);
    gw = x1-x0 + pad*2;
    gh = y1-y0 + pad*2;

    // Find free spot for the rect in the atlas
    added = mAtlas->addRect (gw, gh, &gx, &gy);
    if (added == 0)
      return NULL;

    // Init glyph.
    glyph = allocGlyph (font);
    glyph->codepoint = codepoint;
    glyph->size = isize;
    glyph->index = g;
    glyph->x0 = (short)gx;
    glyph->y0 = (short)gy;
    glyph->x1 = (short)(glyph->x0+gw);
    glyph->y1 = (short)(glyph->y0+gh);
    glyph->xadv = (short)(scale * advance * 10.0f);
    glyph->xoff = (short)(x0 - pad);
    glyph->yoff = (short)(y0 - pad);
    glyph->next = 0;

    // Insert char to hash lookup.
    glyph->next = font->lut[h];
    font->lut[h] = font->nglyphs-1;

    // Rasterize
    dst = &texData[(glyph->x0+pad) + (glyph->y0+pad) * mWidth];
    ttRenderGlyphBitmap (&renderFont->fontInfo, dst, gw-pad*2,gh-pad*2, mWidth, scale,scale, g);

    // Make sure there is one pixel empty border.
    dst = &texData[glyph->x0 + glyph->y0 * mWidth];
    for (y = 0; y < gh; y++) {
      dst[y*mWidth] = 0;
      dst[gw-1 + y*mWidth] = 0;
      }
    for (x = 0; x < gw; x++) {
      dst[x] = 0;
      dst[x + (gh-1)*mWidth] = 0;
      }

    dirtyRect[0] = mini (dirtyRect[0], glyph->x0);
    dirtyRect[1] = mini (dirtyRect[1], glyph->y0);
    dirtyRect[2] = maxi (dirtyRect[2], glyph->x1);
    dirtyRect[3] = maxi (dirtyRect[3], glyph->y1);

    return glyph;
    }
  //}}}

  //{{{
  float getVertAlign (sFont* font, int align, short isize) {

    if (align & ALIGN_TOP)
      return font->ascender * (float)isize/10.0f;
    else if (align & ALIGN_MIDDLE)
      return (font->ascender + font->descender) / 2.0f * (float)isize/10.0f;
    else if (align & ALIGN_BASELINE)
      return 0.0f;
    else if (align & ALIGN_BOTTOM)
      return font->descender * (float)isize/10.0f;

    return 0.0;
    }
  //}}}
  //{{{
  void vertMetrics (float* ascender, float* descender, float* lineh) {

    auto state = getState();

    if (state->font < 0 || state->font >= mNumFonts)
      return;

    auto font = mFonts[state->font];
    short isize = (short)(state->size*10.0f);
    if (font->data == NULL)
      return;

    if (ascender)
      *ascender = font->ascender*isize/10.0f;
    if (descender)
      *descender = font->descender*isize/10.0f;
    if (lineh)
      *lineh = font->lineh*isize/10.0f;
    }
  //}}}
  //{{{
  void lineBounds (float y, float* miny, float* maxy) {

    auto state = getState();
    short isize;

    if (state->font < 0 || state->font >= mNumFonts)
      return;

    auto font = mFonts[state->font];
    isize = (short)(state->size*10.0f);
    if (font->data == NULL)
      return;

    y += getVertAlign (font, state->align, isize);

    *miny = y - font->ascender * (float)isize/10.0f;
    *maxy = *miny + font->lineh*isize/10.0f;
    }
  //}}}
  //{{{
  float textBounds (float x, float y, const char* str, const char* end, float* bounds) {

    auto state = getState();

    unsigned int codepoint;
    unsigned int utf8state = 0;

    int prevGlyphIndex = -1;
    short isize = (short)(state->size * 10.0f);

    float startx, advance;
    float minx, miny, maxx, maxy;

    if (state->font < 0 || state->font >= mNumFonts)
      return 0;

    auto font = mFonts[state->font];
    if (font->data == NULL)
      return 0;

    float scale = ttGetPixelHeightScale (&font->fontInfo, (float)isize / 10.0f);

    // Align vertically.
    y += getVertAlign (font, state->align, isize);

    minx = maxx = x;
    miny = maxy = y;
    startx = x;

    if (end == NULL)
      end = str + strlen(str);

    for (; str != end; ++str) {
      if (decutf8(&utf8state, &codepoint, *(const unsigned char*)str))
        continue;
      auto glyph = getGlyph (font, codepoint, isize);
      if (glyph != NULL) {
        sQuad q;
        getQuad (font, prevGlyphIndex, glyph, scale, state->spacing, &x, &y, &q);
        if (q.x0 < minx)
          minx = q.x0;
        if (q.x1 > maxx)
          maxx = q.x1;
        if (q.y0 < miny)
          miny = q.y0;
        if (q.y1 > maxy)
          maxy = q.y1;
        }
      prevGlyphIndex = glyph != NULL ? glyph->index : -1;
      }

    advance = x - startx;

    // Align horizontally
    if (state->align & ALIGN_LEFT) {
      // empty
      }
    else if (state->align & ALIGN_RIGHT) {
      minx -= advance;
      maxx -= advance;
      }
   else if (state->align & ALIGN_CENTER) {
      minx -= advance * 0.5f;
      maxx -= advance * 0.5f;
      }

    if (bounds) {
      bounds[0] = minx;
      bounds[1] = miny;
      bounds[2] = maxx;
      bounds[3] = maxy;
      }

    return advance;
    }
  //}}}

  //{{{
  int textItInit (sTextIt* it, float x, float y, const char* str, const char* end) {

    memset (it, 0, sizeof(*it));

    auto state = getState();
    if ((state->font < 0) || (state->font >= mNumFonts))
      return 0;

    it->font = mFonts[state->font];
    if (it->font->data == NULL)
      return 0;

    it->isize = (short)(state->size * 10.f);
    it->scale = ttGetPixelHeightScale (&it->font->fontInfo, (float)it->isize / 10.f);

    // Align horizontally
    if (state->align & ALIGN_RIGHT) {
      float width = textBounds (x,y, str,end, NULL);
      x -= width;
      }
    else if (state->align & ALIGN_CENTER) {
      float width = textBounds (x,y, str,end, NULL);
      x -= width * 0.5f;
      }

    // Align vertically.
    y += getVertAlign (it->font, state->align, it->isize);

    if (end == NULL)
      end = str + strlen (str);

    it->x = x;
    it->nextx = x;
    it->y = y;
    it->nexty = y;
    it->spacing = state->spacing;
    it->str = str;
    it->next = str;
    it->end = end;
    it->codepoint = 0;
    it->prevGlyphIndex = -1;

    return 1;
    }
  //}}}
  //{{{
  int textItNext (sTextIt* it, sQuad* quad) {

    const char* str = it->next;

    it->str = it->next;
    if (str == it->end)
      return 0;

    for (; str != it->end; str++) {
      if (decutf8 (&it->utf8state, &it->codepoint, *(const unsigned char*)str))
        continue;

      str++;
      // Get glyph and quad
      it->x = it->nextx;
      it->y = it->nexty;
      sGlyph* glyph = getGlyph (it->font, it->codepoint, it->isize);
      if (glyph != NULL)
        getQuad (it->font, it->prevGlyphIndex, glyph, it->scale, it->spacing, &it->nextx, &it->nexty, quad);

      it->prevGlyphIndex = glyph != NULL ? glyph->index : -1;
      break;
      }

    it->next = str;
    return 1;
    }
  //}}}

  //{{{
  const unsigned char* getTextureData (int* width, int* height) {

    if (width != NULL)
      *width = mWidth;

    if (height != NULL)
      *height = mHeight;

    return texData;
    }
  //}}}
  //{{{
  int validateTexture (int* dirty) {

    if ((dirtyRect[0] < dirtyRect[2]) && (dirtyRect[1] < dirtyRect[3])) {
      dirty[0] = dirtyRect[0];
      dirty[1] = dirtyRect[1];
      dirty[2] = dirtyRect[2];
      dirty[3] = dirtyRect[3];

      // Reset dirty rect
      dirtyRect[0] = mWidth;
      dirtyRect[1] = mHeight;
      dirtyRect[2] = 0;
      dirtyRect[3] = 0;
      return 1;
      }

    return 0;
    }
  //}}}

  // vars
  int mScratchBufSize;
  unsigned char* mScratchBuf;

private:
  //{{{  static constexpr
  static constexpr int kMaxFontStates = 20;

  static constexpr int kUtf8Accept = 0;
  static constexpr int kUtf8Reject = 12;

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
      memset (mNodes, 0, sizeof(sNode) * maxNumNodes);

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
    int insertNode (int idx, int x, int y, int w) {

      // Insert node
      if (mNumNodes+1 > mMaxNumNodes) {
        mMaxNumNodes = mMaxNumNodes == 0 ? 8 : mMaxNumNodes * 2;
        mNodes = (sNode*)realloc (mNodes, sizeof(sNode) * mMaxNumNodes);
        if (mNodes == NULL)
          return 0;
        }

      for (int i = mNumNodes; i > idx; i--)
        mNodes[i] = mNodes[i-1];
      mNodes[idx].mX = (short)x;
      mNodes[idx].mY = (short)y;
      mNodes[idx].mWidth = (short)w;
      mNumNodes++;

      return 1;
      }
    //}}}
    //{{{
    void removeNode (int idx) {

      if (mNumNodes == 0)
        return;

      for (int i = idx; i < mNumNodes-1; i++)
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
    int addSkylineLevel (int idx, int x, int y, int w, int h) {

      // Insert new node
      if (insertNode (idx, x, y+h, w) == 0)
        return 0;

      // Delete skyline segments that fall under the shadow of the new segment.
      for (int i = idx+1; i < mNumNodes; i++) {
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
  //{{{
  sFontState* getState() {
    return &states[mNumStates-1];
    }
  //}}}
  //{{{
  void pushState() {

    if (mNumStates >= kMaxFontStates)
      return;

    if (mNumStates > 0)
      memcpy(&states[mNumStates], &states[mNumStates-1], sizeof(sFontState));

    mNumStates++;
    }
  //}}}
  //{{{
  void popState() {

    if (mNumStates <= 1)
      return;

    mNumStates--;
    }
  //}}}
  //{{{
  void clearState() {

    auto state = getState();
    state->size = 12.0f;
    state->color = 0xffffffff;
    state->font = 0;
    state->spacing = 0;
    state->align = ALIGN_LEFT | ALIGN_BASELINE;
    }
  //}}}

  //{{{
  static unsigned int decutf8 (unsigned int* state, unsigned int* codep, unsigned int byte) {
  // Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
  // See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

    const unsigned char utf8d[] = {
      // The first part of the table maps bytes to character classes that
      // to reduce the size of the transition table and create bitmasks.
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
      7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
      8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
      10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

      // The second part is a transition table that maps a combination
      // of a state of the automaton and a character class to a state.
      0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
      12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
      12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
      12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
      12,36,12,12,12,12,12,12,12,12,12,12,
      };

    unsigned int type = utf8d[byte];
    *codep = (*state != kUtf8Accept) ? (byte & 0x3fu) | (*codep << 6) : (0xff >> type) & (byte);
    *state = utf8d[256 + *state + type];
    return *state;
    }
  //}}}
  //{{{
  static unsigned int hashint (unsigned int a) {

    a += ~(a<<15);
    a ^=  (a>>10);
    a +=  (a<<3);
    a ^=  (a>>6);
    a += ~(a<<11);
    a ^=  (a>>16);

    return a;
    }
  //}}}

  //{{{
  void ttGetFontVMetrics (stbtt_fontinfo* fontInfo, int* ascent, int* descent, int* lineGap) {
    stbtt_GetFontVMetrics (fontInfo, ascent, descent, lineGap);
    }
  //}}}
  //{{{
  float ttGetPixelHeightScale (stbtt_fontinfo* fontInfo, float size) {
    return stbtt_ScaleForPixelHeight (fontInfo, size);
    }
  //}}}
  //{{{
  int ttGetGlyphIndex (stbtt_fontinfo* fontInfo, int codepoint) {
    return stbtt_FindGlyphIndex (fontInfo, codepoint);
    }
  //}}}
  //{{{
  int ttGetGlyphKernAdvance (stbtt_fontinfo* fontInfo, int glyph1, int glyph2) {
    return stbtt_GetGlyphKernAdvance (fontInfo, glyph1, glyph2);
    }
  //}}}
  //{{{
  int ttBuildGlyphBitmap (stbtt_fontinfo* fontInfo, int glyph, float size, float scale,
                          int *advance, int *lsb, int *x0, int *y0, int *x1, int *y1) {

    stbtt_GetGlyphHMetrics (fontInfo, glyph, advance, lsb);
    stbtt_GetGlyphBitmapBox (fontInfo, glyph, scale, scale, x0, y0, x1, y1);
    return 1;
    }
  //}}}
  //{{{
  void ttRenderGlyphBitmap (stbtt_fontinfo* fontInfo, unsigned char* output,
                            int outWidth, int outHeight, int outStride,
                            float scaleX, float scaleY, int glyph) {
    stbtt_MakeGlyphBitmap (fontInfo, output, outWidth, outHeight, outStride, scaleX, scaleY, glyph);
    }
  //}}}

  //{{{
  int allocFont() {

    if (mNumFonts + 1 > mNumAllocatedFonts) {
      mNumAllocatedFonts = mNumAllocatedFonts == 0 ? 8 : mNumAllocatedFonts * 2;
      mFonts = (sFont**)realloc (mFonts, sizeof(sFont*) * mNumAllocatedFonts);
      if (mFonts == NULL)
        return -1;
      }

    auto font = (sFont*)malloc(sizeof(sFont));
    if (font == NULL)
      goto error;
    memset (font, 0, sizeof(sFont));

    font->glyphs = (sGlyph*)malloc(sizeof(sGlyph) * kInitGlyphs);
    if (font->glyphs == NULL)
      goto error;
    font->cglyphs = kInitGlyphs;
    font->nglyphs = 0;

    mFonts[mNumFonts++] = font;
    return mNumFonts-1;

  error:
    freeFont (font);
    return kInvalid;
    }
  //}}}
  //{{{
  void freeFont (sFont* font) {

    if (font == NULL)
      free (font->glyphs);
    free (font);
    }
  //}}}

  //{{{
  void getQuad (sFont* font,
                int prevGlyphIndex, sGlyph* glyph,
                float scale, float spacing, float* x, float* y, sQuad* q) {

    if (prevGlyphIndex != -1) {
      float adv = ttGetGlyphKernAdvance(&font->fontInfo, prevGlyphIndex, glyph->index) * scale;
      *x += (int)(adv + spacing + 0.5f);
      }

    // Each glyph has 2px border to allow good interpolation,
    // one pixel to prevent leaking, and one to allow good interpolation for rendering.
    // Inset the texture region by one pixel for correct interpolation.
    float xoff = (short)(glyph->xoff+1);
    float yoff = (short)(glyph->yoff+1);
    float x0 = (float)(glyph->x0+1);
    float y0 = (float)(glyph->y0+1);
    float x1 = (float)(glyph->x1-1);
    float y1 = (float)(glyph->y1-1);

    float rx = (float)(int)(*x + xoff);
    float ry = (float)(int)(*y + yoff);

    q->x0 = rx;
    q->y0 = ry;
    q->x1 = rx + x1 - x0;
    q->y1 = ry + y1 - y0;

    q->s0 = x0 * itw;
    q->t0 = y0 * ith;
    q->s1 = x1 * itw;
    q->t1 = y1 * ith;

    *x += (int)(glyph->xadv / 10.0f + 0.5f);
    }
  //}}}

  //{{{
  void addWhiteRect (int w, int h) {

    int x, y, gx, gy;
    unsigned char* dst;
    if (mAtlas->addRect (w, h, &gx, &gy) == 0)
      return;

    // Rasterize
    dst = &texData[gx + gy * mWidth];
    for (y = 0; y < h; y++) {
      for (x = 0; x < w; x++)
        dst[x] = 0xff;
      dst += mWidth;
      }

    dirtyRect[0] = mini(dirtyRect[0], gx);
    dirtyRect[1] = mini(dirtyRect[1], gy);
    dirtyRect[2] = maxi(dirtyRect[2], gx+w);
    dirtyRect[3] = maxi(dirtyRect[3], gy+h);
    }
  //}}}
  //{{{
  sGlyph* allocGlyph (sFont* font) {

    if (font->nglyphs+1 > font->cglyphs) {
      font->cglyphs = font->cglyphs == 0 ? 8 : font->cglyphs * 2;
      font->glyphs = (sGlyph*)realloc (font->glyphs, sizeof(sGlyph) * font->cglyphs);
      if (font->glyphs == NULL)
        return NULL;
      }

    font->nglyphs++;
    return &font->glyphs[font->nglyphs-1];
    }
  //}}}
  //{{{
  void getAtlasSize (int* width, int* height) {

    *width = mWidth;
    *height = mHeight;
    }
  //}}}
  //{{{
  int expandAtlas (int width, int height) {

    width = maxi (width, mWidth);
    height = maxi (height, mHeight);
    if ((width == mWidth) && (height == mHeight))
      return 1;

    // flush pending glyphs.
    flush();

    // copy old texture data
    unsigned char* data = (unsigned char*)malloc(width * height);
    for (int i = 0; i < mHeight; i++) {
      unsigned char* dst = &data[i*width];
      unsigned char* src = &texData[i*mWidth];
      memcpy(dst, src, mWidth);
      if (width > mWidth)
        memset (dst+mWidth, 0, width - mWidth);
      }
    if (height > mHeight)
      memset (&data[mHeight * width], 0, (height - mHeight) * width);

    free (texData);

    texData = data;

    // increase atlas size
    mAtlas->expand (width, height);

    // add existing data as dirty
    int maxy = 0;
    for (int i = 0; i < mAtlas->mNumNodes; i++)
      maxy = maxi (maxy, mAtlas->mNodes[i].mY);

    dirtyRect[0] = 0;
    dirtyRect[1] = 0;
    dirtyRect[2] = mWidth;
    dirtyRect[3] = maxy;

    mWidth = width;
    mHeight = height;
    itw = 1.0f / mWidth;
    ith = 1.0f / mHeight;

    return 1;
    }
  //}}}
  //{{{
  void flush() {
  // Flush texture

    if (dirtyRect[0] < dirtyRect[2] && dirtyRect[1] < dirtyRect[3]) {
      // Reset dirty rect
      dirtyRect[0] = mWidth;
      dirtyRect[1] = mHeight;
      dirtyRect[2] = 0;
      dirtyRect[3] = 0;
      }
    }
  //}}}

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
  sFont** mFonts = { nullptr };
  //}}}
  };
//}}}

//{{{
static void* fontAlloc (size_t size, void* up) {

  // 16-byte align the returned pointer
  size = (size + 0xf) & ~0xf;
  auto fontContext = (cFontContext*)up;
  if (fontContext->mScratchBufSize + (int)size > cFontContext::kScratchBufSize)
    return NULL;

  unsigned char* ptr = fontContext->mScratchBuf + fontContext->mScratchBufSize;
  fontContext->mScratchBufSize += (int)size;

  return ptr;
  }
//}}}
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
  "#define innerColor frag[6]\n"
  "#define outerColor frag[7]\n"
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
      //  SHADER_FILL_GRADIENT - calc grad color using box grad
      "vec2 pt = (paintMatrix * vec3(fpos,1.0)).xy;\n"
      "float d = clamp((sdroundrect(pt, extent, radius) + feather*0.5) / feather, 0.0, 1.0);\n"
      "vec4 color = mix(innerColor,outerColor,d);\n"
      // Combine alpha
      "color *= strokeAlpha * scissor;\n"
      "result = color;\n"
    "} else if (type == 1) {\n"
      // SHADER_FILL_IMAGE - image calc color fron texture
      "vec2 pt = (paintMatrix * vec3(fpos,1.0)).xy / extent;\n"
      "vec4 color = texture2D(tex, pt);\n"
      "if (texType == 1) color = vec4(color.xyz*color.w,color.w);"
      "if (texType == 2) color = vec4(color.x);"
      "color *= innerColor;\n"            // apply color tint and alpha
      "color *= strokeAlpha * scissor;\n" // combine alpha
      "result = color;\n"
    "} else if (type == 2) {\n"
      //  SHADER_SIMPLE - stencil fill
      "result = vec4(1,1,1,1);\n"
    "} else if (type == 3) {\n"
       // SHADER_IMAGE - textured tris
      "vec4 color = texture2D(tex, ftcoord);\n"
      "if (texType == 1) color = vec4(color.xyz*color.w,color.w);"
      "if (texType == 2) color = vec4(color.x);"
      "color *= scissor;\n"
      "result = color * innerColor;\n"
    "}\n"

  "if (strokeAlpha < strokeThreshold) discard;\n"
  "gl_FragColor = result;\n"
  "}\n";
//}}}

// cVg::cTransform
//{{{  cVg::cTransform
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
float cVg::cTransform::getTranslateX() { return mTx; }
float cVg::cTransform::getTranslateY() { return mTy; }
float cVg::cTransform::getAverageScaleX() { return sqrtf (mSx*mSx + mKx*mKx); }
float cVg::cTransform::getAverageScaleY() { return sqrtf (mKy*mKy + mSy*mSy); }
float cVg::cTransform::getAverageScale() { return (getAverageScaleX() + getAverageScaleY()) * 0.5f; }
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
//{{{
bool cVg::cTransform::getInverse (cTransform& inverse) {

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

// private
//{{{
bool cVg::cTransform::isIdentity() {
  return mSx == 1.0f && mKy == 0.0f && mKx == 0.0f && mSy == 1.0f && mTx == 0.0f && mTy == 0.0f;
  }
//}}}
//}}}

// public
//{{{
cVg::cVg (int flags)
   : mDrawEdges(!(flags & DRAW_NOEDGES)), mDrawSolid(!(flags & DRAW_NOSOLID)), mDrawTriangles(true) {

  saveState();
  resetState();
  setDevicePixelRatio (1.f);

  mFontContext = new cFontContext (cFontContext::kInitFontImageSize, cFontContext::kInitFontImageSize);
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
    if (mTextures[i].tex && (mTextures[i].flags & IMAGE_NODELETE) == 0)
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

  fontImages[0] = createTexture (TEXTURE_ALPHA, cFontContext::kInitFontImageSize, cFontContext::kInitFontImageSize, 0, NULL);
  }
//}}}

//{{{  state
//{{{
void cVg::resetState() {

  auto state = &mStates[mNumStates-1];

  state->compositeOp = compositeOpState (NVG_SOURCE_OVER);
  state->fillPaint.set (nvgRGBA (255, 255, 255, 255));
  state->strokePaint.set (nvgRGBA (0, 0, 0, 255));

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

  if (mNumStates >= kMaxStates)
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
void cVg::fillColor (const sVgColour& color) { mStates[mNumStates-1].fillPaint.set (color); }
void cVg::strokeColor (const sVgColour& color) { mStates[mNumStates-1].strokePaint.set (color); }
void cVg::strokeWidth (float width) { mStates[mNumStates-1].strokeWidth = width; }
void cVg::globalAlpha (float alpha) { mStates[mNumStates-1].alpha = alpha; }
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
cVg::cPaint cVg::linearGradient (float sx, float sy, float ex, float ey, const sVgColour& icol, const sVgColour& ocol) {

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
  const float large = 1e5;
  p.mTransform.set (dy, -dx, dx, dy, sx - dx * large, sy - dy * large);
  p.extent[0] = large;
  p.extent[1] = large + d * 0.5f;
  p.radius = 0.0f;
  p.feather = maxf (1.0f, d);
  p.innerColor = icol;
  p.outerColor = ocol;
  p.imageId = 0;

  return p;
  }
//}}}
//{{{
cVg::cPaint cVg::radialGradient (float cx, float cy, float inr, float outr, const sVgColour& icol, const sVgColour& ocol) {

  float r = (inr + outr) * 0.5f;
  float f = (outr - inr);

  cPaint p;
  p.mTransform.setTranslate (cx, cy);
  p.extent[0] = r;
  p.extent[1] = r;
  p.radius = r;
  p.feather = maxf (1.0f, f);
  p.innerColor = icol;
  p.outerColor = ocol;
  p.imageId = 0;

  return p;
  }
//}}}
//{{{
cVg::cPaint cVg::boxGradient (float x, float y, float w, float h, float r, float f, const sVgColour& icol, const sVgColour& ocol) {

  cPaint p;
  p.mTransform.setTranslate (x + w * 0.5f, y + h * 0.5f);
  p.extent[0] = 0.f;
  p.extent[1] = 0.f;
  p.radius = r;
  p.feather = maxf (1.0f, f);
  p.innerColor = icol;
  p.outerColor = ocol;
  p.imageId = 0;

  return p;
  }
//}}}
//{{{
cVg::cPaint cVg::imagePattern (float cx, float cy, float w, float h, float angle, int imageId, float alpha) {

  cPaint paint;

  paint.mTransform.setRotateTranslate (angle, cx, cy);

  paint.extent[0] = w;
  paint.extent[1] = h;

  paint.radius = 0.f;
  paint.feather = 0.f;

  paint.innerColor = nvgRGBAf (1, 1, 1, alpha);
  paint.outerColor = nvgRGBAf (1, 1, 1, alpha);
  paint.imageId = imageId;

  return paint;
  }
//}}}

//{{{
void cVg::fillPaint (const cPaint& paint) {

  mStates[mNumStates-1].fillPaint = paint;
  mStates[mNumStates-1].fillPaint.mTransform.multiply (mStates[mNumStates-1].mTransform);
  }
//}}}
//{{{
void cVg::strokePaint (const cPaint& paint) {

  mStates[mNumStates-1].strokePaint = paint;
  mStates[mNumStates-1].strokePaint.mTransform.multiply (mStates[mNumStates-1].mTransform);
  }
//}}}

void cVg::beginPath() { mShape.beginPath(); }
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

  float values[] = { eBEZIERTO, c1x, c1y, c2x, c2y, x,   y };
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

  float values[] = { eMOVETO, x, y, eLINETO, x, y+h, eLINETO, x+w, y+h, eLINETO, x+w, y, eCLOSE };
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
void cVg::roundedRect (float x, float y, float w, float h, float r) { roundedRectVarying (x, y, w, h, r, r, r, r); }

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
void cVg::circle (float cx, float cy, float r) { ellipse (cx,cy, r,r); }

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
void cVg::setFontById (int font) {
  mStates[mNumStates-1].fontId = font;
  }
//}}}
//{{{
void cVg::setFontByName (const string& fontName) {
  mStates[mNumStates-1].fontId = mFontContext->getFontByName (fontName);
  }
//}}}

//{{{
int cVg::createFont (const string& fontName, unsigned char* data, int dataSize) {
  return mFontContext->addFont (fontName, data, dataSize);
  }
//}}}

enum eVgCodepointType { NVG_SPACE, NVG_NEWLINE, NVG_CHAR, NVG_CJK_CHAR };
//{{{
float cVg::text (float x, float y, const string& str) {

  auto state = &mStates[mNumStates-1];
  if (state->fontId == cFontContext::kInvalid)
    return x;

  float scale = getFontScale (state) * devicePixelRatio;
  float inverseScale = 1.0f / scale;

  mFontContext->setSize (state->fontSize * scale);
  mFontContext->setSpacing (state->letterSpacing * scale);
  mFontContext->setAlign (state->textAlign);
  mFontContext->setFont ( state->fontId);

  // allocate 6 vertices per glyph
  int numVertices = maxi (2, (int)str.size()) * 6;
  int vertexIndex = mVertices.alloc (numVertices);
  auto vertices = mVertices.getVertexPtr (vertexIndex);
  auto firstVertex = vertices;

  cFontContext::sTextIt it;
  mFontContext->textItInit (&it, x*scale, y*scale, str.c_str(), str.c_str() + str.size());
  cFontContext::sTextIt prevIt = it;

  cFontContext::sQuad quad;
  while (mFontContext->textItNext (&it, &quad)) {
    if (it.prevGlyphIndex == -1) {
      // can not retrieve glyph?
      if (!allocTextAtlas())
        break; // no memory
      it = prevIt;
      mFontContext->textItNext (&it, &quad); // try again
      if (it.prevGlyphIndex == -1) // still can not find glyph?
        break;
      }
    prevIt = it;

    // set vertex triangles from quad
    if (state->mTransform.mIdentity) {
      //{{{  simple
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
    fillPaint.innerColor.a *= mStates[mNumStates - 1].alpha;
    fillPaint.outerColor.a *= mStates[mNumStates - 1].alpha;
    fillPaint.imageId = fontImages[fontImageIdx];
    renderText (vertexIndex, numVertices, fillPaint, mStates[mNumStates - 1].scissor);
    }
  flushTextTexture();

  return it.x;
  }
//}}}
//{{{
float cVg::textBounds (float x, float y, const string& str, float* bounds) {

  auto state = &mStates[mNumStates-1];
  if (state->fontId == cFontContext::kInvalid)
    return 0;

  float scale = getFontScale (state) * devicePixelRatio;
  float inverseScale = 1.0f / scale;
  float width;

  mFontContext->setSize (state->fontSize*scale);
  mFontContext->setSpacing (state->letterSpacing*scale);
  mFontContext->setAlign (state->textAlign);
  mFontContext->setFont (state->fontId);

  width = mFontContext->textBounds (x*scale, y*scale, str.c_str(), str.c_str() + str.size(), bounds);
  if (bounds != NULL) {
    // Use line bounds for height.
    mFontContext->lineBounds (y*scale, &bounds[1], &bounds[3]);
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
  if (state->fontId == cFontContext::kInvalid)
    return;

  float scale = getFontScale(state) * devicePixelRatio;
  float inverseScale = 1.0f / scale;

  mFontContext->setSize (state->fontSize*scale);
  mFontContext->setSpacing (state->letterSpacing*scale);
  mFontContext->setAlign (state->textAlign);
  mFontContext->setFont (state->fontId);

  mFontContext->vertMetrics (ascender, descender, lineh);
  if (ascender != NULL)
    *ascender *= inverseScale;
  if (descender != NULL)
    *descender *= inverseScale;
  if (lineh != NULL)
    *lineh *= inverseScale;
  }
//}}}
//{{{
int cVg::textGlyphPositions (float x, float y, const string& str, NVGglyphPosition* positions, int maxPositions) {

  auto state = &mStates[mNumStates-1];
  if (state->fontId == cFontContext::kInvalid)
    return 0;

  if (str.empty())
    return 0;

  float scale = getFontScale (state) * devicePixelRatio;
  float inverseScale = 1.0f / scale;

  mFontContext->setSize (state->fontSize * scale);
  mFontContext->setSpacing (state->letterSpacing * scale);
  mFontContext->setAlign (state->textAlign);
  mFontContext->setFont (state->fontId);

  cFontContext::sTextIt it;
  mFontContext->textItInit (&it, x*scale, y*scale, str.c_str(), str.c_str() + str.size());
  cFontContext::sTextIt prevIt = it;

  int npos = 0;
  cFontContext::sQuad quad;
  while (mFontContext->textItNext (&it, &quad)) {
    if (it.prevGlyphIndex < 0 && allocTextAtlas()) {
      // can not retrieve glyph, try again
      it = prevIt;
      mFontContext->textItNext (&it, &quad);
      }
    prevIt = it;

    positions[npos].str = it.str;
    positions[npos].x = it.x * inverseScale;
    positions[npos].minx = minf (it.x, quad.x0) * inverseScale;
    positions[npos].maxx = maxf (it.nextx, quad.x1) * inverseScale;
    npos++;
    if (npos >= maxPositions)
      break;
    }

  return npos;
  }
//}}}

//{{{
void cVg::textBox (float x, float y, float breakRowWidth, const char* string, const char* end) {

  auto state = &mStates[mNumStates-1];
  if (state->fontId == cFontContext::kInvalid)
    return;

  int oldAlign = state->textAlign;
  int hAlign = state->textAlign & (ALIGN_LEFT | ALIGN_CENTER | ALIGN_RIGHT);
  int vAlign = state->textAlign & (ALIGN_TOP | ALIGN_MIDDLE | ALIGN_BOTTOM | ALIGN_BASELINE);
  float lineh = 0;

  textMetrics (NULL, NULL, &lineh);
  state->textAlign = vAlign | ALIGN_LEFT;

  sVgTextRow rows[2];
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
void cVg::textBoxBounds (float x, float y, float breakRowWidth, const char* string, const char* end, float* bounds) {

  auto state = &mStates[mNumStates-1];
  if (state->fontId == cFontContext::kInvalid) {
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

  mFontContext->setSize (state->fontSize*scale);
  mFontContext->setSpacing (state->letterSpacing*scale);
  mFontContext->setAlign (state->textAlign);
  mFontContext->setFont (state->fontId);
  mFontContext->lineBounds (0, &rminy, &rmaxy);
  rminy *= inverseScale;
  rmaxy *= inverseScale;

  int nrows = 0;
  sVgTextRow rows[2];
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
int cVg::textBreakLines (const char* string, const char* end, float breakRowWidth, sVgTextRow* rows, int maxRows) {

  if (maxRows == 0)
    return 0;
  if (end == NULL)
    end = string + strlen (string);
  if (string == end)
    return 0;

  auto state = &mStates[mNumStates-1];
  if (state->fontId == cFontContext::kInvalid)
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

  mFontContext->setSize (state->fontSize*scale);
  mFontContext->setSpacing (state->letterSpacing*scale);
  mFontContext->setAlign (state->textAlign);
  mFontContext->setFont (state->fontId);

  breakRowWidth *= scale;

  cFontContext::sTextIt it;
  mFontContext->textItInit (&it, 0, 0, string, end);
  cFontContext::sTextIt prevIt = it;

  cFontContext::sQuad quad;
  while (mFontContext->textItNext (&it, &quad)) {
    if ((it.prevGlyphIndex < 0) && allocTextAtlas()) {
      // can not retrieve glyph?
      it = prevIt;

      // try again
      mFontContext->textItNext (&it, &quad);
      }
    prevIt = it;

    switch (it.codepoint) {
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
        if ((it.codepoint >= 0x4E00 && it.codepoint <= 0x9FFF) ||
            (it.codepoint >= 0x3000 && it.codepoint <= 0x30FF) ||
            (it.codepoint >= 0xFF00 && it.codepoint <= 0xFFEF) ||
            (it.codepoint >= 0x1100 && it.codepoint <= 0x11FF) ||
            (it.codepoint >= 0x3130 && it.codepoint <= 0x318F) ||
            (it.codepoint >= 0xAC00 && it.codepoint <= 0xD7AF))
          type = NVG_CJK_CHAR;
        else
          type = NVG_CHAR;
        break;
      }

    if (type == NVG_NEWLINE) {
     //{{{  Always handle new lines.
     rows[nrows].start = rowStart != NULL ? rowStart : it.str;
     rows[nrows].end = rowEnd != NULL ? rowEnd : it.str;
     rows[nrows].width = rowWidth * inverseScale;
     rows[nrows].minx = rowMinX * inverseScale;
     rows[nrows].maxx = rowMaxX * inverseScale;
     rows[nrows].next = it.next;
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
         rowStartX = it.x;
         rowStart = it.str;
         rowEnd = it.next;
         rowWidth = it.nextx - rowStartX; // q.x1 - rowStartX;
         rowMinX = quad.x0 - rowStartX;
         rowMaxX = quad.x1 - rowStartX;
         wordStart = it.str;
         wordStartX = it.x;
         wordMinX = quad.x0 - rowStartX;
         // Set null break point
         breakEnd = rowStart;
         breakWidth = 0.0;
         breakMaxX = 0.0;
         }
       }
       //}}}
     else {
       float nextWidth = it.nextx - rowStartX;
       //{{{  track last non-white space character
       if (type == NVG_CHAR || type == NVG_CJK_CHAR) {
         rowEnd = it.next;
         rowWidth = it.nextx - rowStartX;
         rowMaxX = quad.x1 - rowStartX;
         }
       //}}}
       //{{{  track last end of a word
       if (((ptype == NVG_CHAR || ptype == NVG_CJK_CHAR) && type == NVG_SPACE) || type == NVG_CJK_CHAR) {
         breakEnd = it.str;
         breakWidth = rowWidth;
         breakMaxX = rowMaxX;
         }
       //}}}
       //{{{  track last beginning of a word
       if ((ptype == NVG_SPACE && (type == NVG_CHAR || type == NVG_CJK_CHAR)) || type == NVG_CJK_CHAR) {
         wordStart = it.str;
         wordStartX = it.x;
         wordMinX = quad.x0 - rowStartX;
         }
       //}}}

       // Break to new line when a character is beyond break width.
       if ((type == NVG_CHAR || type == NVG_CJK_CHAR) && nextWidth > breakRowWidth) {
         // The run length is too long, need to break to new line.
         if (breakEnd == rowStart) {
           //{{{  current word longer than row length, just break it from here
           rows[nrows].start = rowStart;
           rows[nrows].end = it.str;
           rows[nrows].width = rowWidth * inverseScale;
           rows[nrows].minx = rowMinX * inverseScale;
           rows[nrows].maxx = rowMaxX * inverseScale;
           rows[nrows].next = it.str;
           nrows++;
           if (nrows >= maxRows)
             return nrows;
           rowStartX = it.x;
           rowStart = it.str;
           rowEnd = it.next;
           rowWidth = it.nextx - rowStartX;
           rowMinX = quad.x0 - rowStartX;
           rowMaxX = quad.x1 - rowStartX;
           wordStart = it.str;
           wordStartX = it.x;
           wordMinX = quad.x0 - rowStartX;
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
           rowEnd = it.next;
           rowWidth = it.nextx - rowStartX;
           rowMinX = wordMinX;
           rowMaxX = quad.x1 - rowStartX;
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

    pcodepoint = it.codepoint;
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
GLuint cVg::imageHandle (int image) {
  return findTextureById(image)->tex;
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
  return createTexture (TEXTURE_RGBA, w, h, imageFlags, data);
  }
//}}}
//{{{
int cVg::createImageFromHandle (GLuint textureId, int w, int h, int imageFlags) {

  auto texture = allocTexture();
  if (texture == nullptr)
    return 0;

  texture->type = TEXTURE_RGBA;
  texture->tex = textureId;
  texture->flags = imageFlags;
  texture->width = w;
  texture->height = h;

  return texture->id;
  }
//}}}

//{{{
void cVg::updateImage (int image, const unsigned char* data) {

  int w = 0;
  int h = 0;
  getTextureSize(image, w, h);
  updateTexture (image, 0,0, w,h, data);
  }
//}}}

//{{{
bool cVg::deleteImage (int image) {

  for (int i = 0; i < mNumTextures; i++) {
    if (mTextures[i].id == image) {
      if (mTextures[i].tex != 0 && (mTextures[i].flags & IMAGE_NODELETE) == 0)
        glDeleteTextures (1, &mTextures[i].tex);
      mTextures[i].reset();
      return true;
      }
    }

  return false;
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
  state->scissor.mTransform.setIdentity();
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
string cVg::getFrameStats() {
  return "vertices:" + dec (mVertices.getNumVertices()) + " drawArrays:" + dec (mDrawArrays);
  }
//}}}

//{{{
void cVg::beginFrame (int windowWidth, int windowHeight, float devicePixelRatio) {

  mNumStates = 0;
  saveState();
  resetState();

  setDevicePixelRatio (devicePixelRatio);

  mViewport[0] = (float)windowWidth;
  mViewport[1] = (float)windowHeight;
  }
//}}}
//{{{
void cVg::endFrame() {

  auto state = &mStates[mNumStates-1];
  renderFrame (mVertices, state->compositeOp);

  if (fontImageIdx) {
    int fontImage = fontImages[fontImageIdx];
    int iw = 0;
    int ih = 0;

    // delete images that smaller than current one
    if (fontImage == 0)
      return;

    getTextureSize (fontImage, iw, ih);

    int i, j;
    for (i = j = 0; i < fontImageIdx; i++) {
      if (fontImages[i] != 0) {
        int nw = 0;
        int nh = 0;
        getTextureSize(fontImages[i], nw, nh);
        if (nw < iw || nh < ih)
          deleteImage (fontImages[i]);
        else
          fontImages[j++] = fontImages[i];
        }
      }

    // make current font image to first
    fontImages[j++] = fontImages[0];
    fontImages[0] = fontImage;
    fontImageIdx = 0;

    // clear all images after j
    for (int i = j; i < kMaxFontImages; i++)
      fontImages[i] = 0;
    }

  mVertices.reset();
  }
//}}}
//}}}

// private
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
//{{{  cVg::c2dVertices
//{{{
cVg::c2dVertices::c2dVertices() {
  mVertices = (c2dVertex*)malloc (kInitNumVertices * sizeof(c2dVertex));
  mNumAllocatedVertices = kInitNumVertices;
  }
//}}}
//{{{
cVg::c2dVertices::~c2dVertices() {
  free (mVertices);
  }
//}}}

//{{{
void cVg::c2dVertices::reset() {
  mNumVertices = 0;
  }
//}}}

//{{{
int cVg::c2dVertices::alloc (int numVertices) {
// allocate n vertices and return index of first

  if (mNumVertices + numVertices > mNumAllocatedVertices) {
    mNumAllocatedVertices = maxi (mNumVertices + numVertices, 4096) + mNumAllocatedVertices/2; // 1.5x Overallocate
    cLog::log (LOGINFO2, "realloc vertices " + dec(mNumAllocatedVertices));
    mVertices = (c2dVertex*)realloc (mVertices, mNumAllocatedVertices * sizeof(c2dVertex));
    }

  int firstVertexIndex = mNumVertices;
  mNumVertices += numVertices;
  return firstVertexIndex;
  }
//}}}
//{{{
void cVg::c2dVertices::trim (int numVertices) {
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
  mPaths = (cPath*)malloc (sizeof(cPath) * kInitPathSize);
  mNumAllocatedPaths = kInitPathSize;

  mPoints = (cPoint*)malloc (sizeof(cPoint) * kInitPointsSize);
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
          if (i+1 < numValues) // ensure enough values supplied for command type
            transform.point (values[i], values[i+1]);
          else
            cLog::log (LOGERROR, "not enough values in addCommand");
          i += 2;
          break;

        case eBEZIERTO:
          if (i+5 < numValues) { // ensure enough values supplied for command type
            transform.point (values[i], values[i+1]);
            transform.point (values[i+2], values[i+3]);
            transform.point (values[i+4], values[i+5]);
            }
          else
            cLog::log (LOGERROR, "not enough values in addCommand");

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
void cVg::cShape::expandFill (c2dVertices& vertices, float w, eLineCap lineJoin, float miterLimit, float fringeWidth) {

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
void cVg::cShape::triangleFill (c2dVertices& vertices, int& vertexIndex, int& numVertices) {

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
void cVg::cShape::expandStroke (c2dVertices& vertices, float w, eLineCap lineCap, eLineCap lineJoin, float miterLimit, float fringeWidth) {

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
  return maxi (2, (int)ceilf(arc / da));
  }
//}}}
//{{{
int cVg::cShape::pointEquals (float x1, float y1, float x2, float y2, float tol) {
  float dx = x2 - x1;
  float dy = y2 - y1;
  return dx*dx + dy*dy < tol*tol;
  }
//}}}
//{{{
float cVg::cShape::polyArea (cPoint* points, int mNumPoints) {

  float area = 0;
  for (int i = 2; i < mNumPoints; i++)
    area += (points[i].x - points[0].x) * (points[i-1].y - points[0].y) -
            (points[i-1].x - points[0].x) *  (points[i].y - points[0].y);
  return area * 0.5f;
  }
//}}}
//{{{
void cVg::cShape::polyReverse (cPoint* points, int numPoints) {

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
cVg::cPath* cVg::cShape::lastPath() {
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
    mPaths = (cPath*)realloc (mPaths, sizeof(cPath)* mNumAllocatedPaths);
    }

  auto path = &mPaths[mNumPaths];
  //memset (path, 0, sizeof(*path));
  path->mNumPoints = 0;
  path->mFirstPointIndex = mNumPoints;
  path->mWinding = eSOLID;
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
cVg::cShape::cPoint* cVg::cShape::lastPoint() {
  return mNumPoints ? &mPoints[mNumPoints-1] : NULL;
  }
//}}}
//{{{
void cVg::cShape::addPoint (float x, float y, cPoint::eFlags flags) {

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
    cLog::log (LOGINFO2, "realloc points " + dec(mNumAllocatedPoints));
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
void cVg::cShape::commandsToPaths() {
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
void cVg::cShape::chooseBevel (int bevel, cPoint* p0, cPoint* p1, float w, float* x0, float* y0, float* x1, float* y1) {

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
cVg::c2dVertex* cVg::cShape::roundJoin (c2dVertex* vertexPtr, cPoint* point0, cPoint* point1,
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
cVg::c2dVertex* cVg::cShape::bevelJoin (c2dVertex* vertexPtr, cPoint* point0, cPoint* point1,
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
cVg::c2dVertex* cVg::cShape::buttCapStart (c2dVertex* vertexPtr, cPoint* point, float dx, float dy, float w, float d, float aa) {

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
cVg::c2dVertex* cVg::cShape::buttCapEnd (c2dVertex* vertexPtr, cPoint* point, float dx, float dy, float w, float d, float aa) {

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
cVg::c2dVertex* cVg::cShape::roundCapStart (c2dVertex* vertexPtr, cPoint* point, float dx, float dy, float w, int ncap, float aa) {

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
cVg::c2dVertex* cVg::cShape::roundCapEnd (c2dVertex* vertexPtr, cPoint* point, float dx, float dy, float w, int ncap, float aa) {

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
//}}}

//{{{
cVg::cDraw* cVg::allocDraw() {
// allocate a draw, return pointer to it

  if (mNumDraws + 1 > mNumAllocatedDraws) {
    mNumAllocatedDraws = maxi (mNumDraws + 1, 128) + mNumAllocatedDraws / 2; // 1.5x Overallocate
    mDraws = (cDraw*)realloc (mDraws, sizeof(cDraw) * mNumAllocatedDraws);
    }

  return &mDraws[mNumDraws++];
  }
//}}}
//{{{
int cVg::allocFrags (int numFrags) {
// allocate numFrags, return index of first

  if (mNumFrags + numFrags > mNumAllocatedFrags) {
    mNumAllocatedFrags = maxi (mNumFrags + numFrags, 128) + mNumAllocatedFrags / 2; // 1.5x Overallocate
    mFrags = (cFrag*)realloc (mFrags, mNumAllocatedFrags * sizeof(cFrag));
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
    mNumAllocatedPathVertices = maxi (mNumPathVertices + numPaths, 128) + mNumAllocatedPathVertices / 2; // 1.5x Overallocate
    mPathVertices = (cPathVertices*)realloc (mPathVertices, mNumAllocatedPathVertices * sizeof(cPathVertices));
    }

  int firstPathVerticeIndex = mNumPathVertices;
  mNumPathVertices += numPaths;

  return firstPathVerticeIndex;
  }
//}}}

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
void cVg::setUniforms (int firstFragIndex, int imageId) {

  mShader.setFrags ((float*)(&mFrags[firstFragIndex]));

  if (imageId) {
    auto tex = findTextureById (imageId);
    setBindTexture (tex ? tex->tex : 0);
    }
  else
    setBindTexture (0);
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

// texture
//{{{
cVg::cTexture* cVg::allocTexture() {

  cLog::log (LOGINFO, "allocTexture");

  cTexture* texture = nullptr;
  for (int i = 0; i < mNumTextures; i++) {
    if (mTextures[i].id == 0) {
      texture = &mTextures[i];
      break;
      }
    }

  if (texture == nullptr) {
    if (mNumTextures + 1 > mNumAllocatedTextures) {
      mNumAllocatedTextures = maxi (mNumTextures + 1, 4) +  mNumAllocatedTextures / 2; // 1.5x Overallocate
      mTextures = (cTexture*)realloc (mTextures, mNumAllocatedTextures * sizeof(cTexture));
      if (mTextures == nullptr)
        return nullptr;
      }
    texture = &mTextures[mNumTextures++];
    }

  texture->reset();
  texture->id = ++mTextureId;

  cLog::log (LOGINFO, "allocTexture " + dec(texture->id));

  return texture;
  }
//}}}
//{{{
int cVg::createTexture (int type, int w, int h, int imageFlags, const unsigned char* data) {

  cLog::log (LOGINFO, "createTexture");

  auto texture = allocTexture();
  if (texture == nullptr)
    return 0;

  // Check for non-power of 2.
  if (nearestPow2 (w) != (unsigned int)w || nearestPow2(h) != (unsigned int)h) {
    if ((imageFlags & IMAGE_REPEATX) != 0 || (imageFlags & IMAGE_REPEATY) != 0) {
      cLog::log (LOGINFO, "Repeat X/Y is not supported for non power-of-two textures %dx%d", w, h);
      imageFlags &= ~(IMAGE_REPEATX | IMAGE_REPEATY);
      }

    if (imageFlags & IMAGE_GENERATE_MIPMAPS) {
      cLog::log (LOGINFO, "Mip-maps is not support for non power-of-two textures %dx )", w, h);
      imageFlags &= ~IMAGE_GENERATE_MIPMAPS;
      }
    }

  glGenTextures (1, &texture->tex);
  texture->width = w;
  texture->height = h;
  texture->type = type;
  texture->flags = imageFlags;
  setBindTexture (texture->tex);

  glPixelStorei (GL_UNPACK_ALIGNMENT,1);

  if (type == TEXTURE_RGBA)
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  else // alpha
    glTexImage2D (GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);

  if (imageFlags & cVg::IMAGE_GENERATE_MIPMAPS)
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
     imageFlags & cVg::IMAGE_NEAREST ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR);

  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, imageFlags &  cVg::IMAGE_NEAREST ? GL_NEAREST : GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, imageFlags &  cVg::IMAGE_NEAREST ? GL_NEAREST : GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, imageFlags &  cVg::IMAGE_REPEATX ? GL_REPEAT : GL_CLAMP_TO_EDGE);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, imageFlags &  cVg::IMAGE_REPEATY ? GL_REPEAT : GL_CLAMP_TO_EDGE);

  glPixelStorei (GL_UNPACK_ALIGNMENT, 4);

  if (imageFlags & cVg::IMAGE_GENERATE_MIPMAPS)
    glGenerateMipmap (GL_TEXTURE_2D);

  setBindTexture (0);

  return texture->id;
  }
//}}}

//{{{
cVg::cTexture* cVg::findTextureById (int id) {

  for (int i = 0; i < mNumTextures; i++)
    if (mTextures[i].id == id)
      return &mTextures[i];

  return nullptr;
  }
//}}}
//{{{
bool cVg::updateTexture (int id, int x, int y, int w, int h, const unsigned char* data) {

  cLog::log (LOGINFO, "updateTexture");

  auto texture = findTextureById (id);
  if (texture == nullptr)
    return false;
  setBindTexture (texture->tex);

  glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

  // No support for all of skip, need to update a whole row at a time.
  if (texture->type == TEXTURE_RGBA)
    data += y * texture->width * 4;
  else // alpha
    data += y * texture->width;

  x = 0;
  w = texture->width;

  if (texture->type == TEXTURE_RGBA)
    glTexSubImage2D (GL_TEXTURE_2D, 0, x,y, w,h, GL_RGBA, GL_UNSIGNED_BYTE, data);
  else // alpha
    glTexSubImage2D (GL_TEXTURE_2D, 0, x,y, w,h, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);

  glPixelStorei (GL_UNPACK_ALIGNMENT, 4);

  setBindTexture (0);
  return true;
  }
//}}}
//{{{
bool cVg::getTextureSize (int id, int& w, int& h) {

  auto texture = findTextureById (id);
  if (texture == nullptr)
    return false;

  w = texture->width;
  h = texture->height;

  return true;
  }
//}}}

// render
//{{{
void cVg::renderText (int firstVertexIndex, int numVertices, cPaint& paint, cScissor& scissor) {

  //cLog::log (LOGINFO, "renderText " + dec(firstVertexIndex) + " " + dec(numVertices) + " " + dec(paint.imageId));

  auto draw = allocDraw();
  draw->set (cDraw::TEXT, paint.imageId, 0, 0, allocFrags (1), firstVertexIndex, numVertices);
  mFrags[draw->mFirstFragIndex].setImage (&paint, &scissor, findTextureById (paint.imageId));
  }
//}}}
//{{{
void cVg::renderTriangles (int firstVertexIndex, int numVertices, cPaint& paint, cScissor& scissor) {

  auto draw = allocDraw();
  draw->set (cDraw::TRIANGLE, paint.imageId, 0, 0, allocFrags (1), firstVertexIndex, numVertices);
  mFrags[draw->mFirstFragIndex].setFill (&paint, &scissor, 1.0f, 1.0f, -1.0f, findTextureById(paint.imageId));
  }
//}}}
//{{{
void cVg::renderFill (cShape& shape, cPaint& paint, cScissor& scissor, float fringe) {

  auto draw = allocDraw();
  if ((shape.mNumPaths == 1) && shape.mPaths[0].mConvex) {
    // convex
    draw->set (cDraw::CONVEX_FILL, paint.imageId, allocPathVertices (shape.mNumPaths), shape.mNumPaths,
               allocFrags (1), 0,0);
    mFrags[draw->mFirstFragIndex].setFill (&paint, &scissor, fringe, fringe, -1.0f, findTextureById (paint.imageId));
    }
  else {
    // stencil
    draw->set (cDraw::STENCIL_FILL, paint.imageId, allocPathVertices (shape.mNumPaths), shape.mNumPaths,
               allocFrags (2), shape.mBoundsVertexIndex, 4);
    mFrags[draw->mFirstFragIndex].setSimple();
    mFrags[draw->mFirstFragIndex+1].setFill (&paint, &scissor, fringe, fringe, -1.0f, findTextureById(paint.imageId));
    }

  auto fromPath = shape.mPaths;
  auto toPathVertices = &mPathVertices[draw->mFirstPathVerticesIndex];
  while (fromPath < shape.mPaths + shape.mNumPaths)
    *toPathVertices++ = fromPath++->mPathVertices;
  }
//}}}
//{{{
void cVg::renderStroke (cShape& shape, cPaint& paint, cScissor& scissor, float fringe, float strokeWidth) {
// only uses toPathVertices firstStrokeVertexIndex, strokeVertexCount, no fill

  auto draw = allocDraw();
  draw->set (cDraw::STROKE, paint.imageId, allocPathVertices (shape.mNumPaths), shape.mNumPaths, allocFrags (2), 0,0);
  mFrags[draw->mFirstFragIndex].setFill (&paint, &scissor, strokeWidth, fringe, -1.0f, findTextureById(paint.imageId));
  mFrags[draw->mFirstFragIndex+1].setFill (&paint, &scissor, strokeWidth, fringe, 1.0f - 0.5f/255.0f, findTextureById(paint.imageId));

  auto fromPath = shape.mPaths;
  auto toPathVertices = &mPathVertices[draw->mFirstPathVerticesIndex];
  while (fromPath < shape.mPaths + shape.mNumPaths)
    *toPathVertices++ = fromPath++->mPathVertices;
  }
//}}}
//{{{
void cVg::renderFrame (c2dVertices& vertices, cCompositeOpState compositeOp) {

  mDrawArrays = 0;
  //{{{  init gl blendFunc
  GLenum srcRGB = convertBlendFuncFactor (compositeOp.srcRGB);
  GLenum dstRGB = convertBlendFuncFactor (compositeOp.dstRGB);
  GLenum srcAlpha = convertBlendFuncFactor (compositeOp.srcAlpha);
  GLenum dstAlpha = convertBlendFuncFactor (compositeOp.dstAlpha);

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
  glBufferData (GL_ARRAY_BUFFER, vertices.getNumVertices() * sizeof(c2dVertex), vertices.getVertexPtr(0), GL_STREAM_DRAW);

  glEnableVertexAttribArray (0);
  glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, sizeof(c2dVertex), (const GLvoid*)(size_t)0);

  glEnableVertexAttribArray (1);
  glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, sizeof(c2dVertex), (const GLvoid*)(0 + 2*sizeof(float)));
  //}}}

  for (auto draw = mDraws; draw < mDraws + mNumDraws; draw++)
    switch (draw->mType) {
      case cDraw::TEXT:
        //{{{  text triangles
        if (mDrawTriangles) {
          setUniforms (draw->mFirstFragIndex, draw->mImageId);
          glDrawArrays (GL_TRIANGLES, draw->mTriangleFirstVertexIndex, draw->mNumTriangleVertices);
          mDrawArrays++;
          }
        break;
        //}}}
      case cDraw::TRIANGLE:
        //{{{  fill triangles
        if (mDrawSolid) {
          setUniforms (draw->mFirstFragIndex, draw->mImageId);
          glDrawArrays (GL_TRIANGLES, draw->mTriangleFirstVertexIndex, draw->mNumTriangleVertices);
          mDrawArrays++;
          }
        break;
        //}}}
      case cDraw::CONVEX_FILL: {
        //{{{  convexFill
        setUniforms (draw->mFirstFragIndex, draw->mImageId);

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
      case cDraw::STENCIL_FILL: {
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

        setUniforms (draw->mFirstFragIndex + 1, draw->mImageId);
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
      case cDraw::STROKE: {
        //{{{  stroke
        glEnable (GL_STENCIL_TEST);

        auto pathVertices = &mPathVertices[draw->mFirstPathVerticesIndex];

        // fill stroke base without overlap
        if (mDrawSolid) {
          glStencilOp (GL_KEEP, GL_KEEP, GL_INCR);
          setStencilFunc (GL_EQUAL, 0x00, 0xFF);
          setUniforms (draw->mFirstFragIndex + 1, draw->mImageId);
          for (int i = 0; i < draw->mNumPaths; i++) {
            glDrawArrays (GL_TRIANGLE_STRIP, pathVertices[i].mFirstStrokeVertexIndex, pathVertices[i].mNumStrokeVertices);
            mDrawArrays++;
            }
          }

        setUniforms (draw->mFirstFragIndex, draw->mImageId);
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
      }

  // reset counts
  mNumDraws = 0;
  mNumPathVertices = 0;
  mNumFrags = 0;
  }
//}}}

// text
//{{{
float cVg::getFontScale (cState* state) {
  return minf (quantize (state->mTransform.getAverageScale(), 0.01f), 4.f);
  }
//}}}
//{{{
int cVg::allocTextAtlas() {

  cLog::log (LOGINFO, "allocTextAtlas");

  flushTextTexture();
  if (fontImageIdx >= kMaxFontImages-1)
    return 0;

  // if next fontImage already have a texture
  int iw = 0;
  int ih = 0;
  if (fontImages[fontImageIdx+1] != 0)
    getTextureSize (fontImages[fontImageIdx+1], iw, ih);

  else {
    // calculate the new font image size and create it
    getTextureSize (fontImages[fontImageIdx], iw, ih);
    if (iw > ih)
      ih *= 2;
    else
      iw *= 2;
    if ((iw > cFontContext::kMaxFontImageSize) || (ih > cFontContext::kMaxFontImageSize)) {
      iw = cFontContext::kMaxFontImageSize;
      ih = cFontContext::kMaxFontImageSize;
      }

    fontImages[fontImageIdx+1] = createTexture (TEXTURE_ALPHA, iw, ih, 0, NULL);
    }
  ++fontImageIdx;

  mFontContext->resetAtlas (iw, ih);

  return 1;
  }
//}}}
//{{{
void cVg::flushTextTexture() {

  int dirty[4];
  if (mFontContext->validateTexture (dirty)) {

    // Update texture
    int fontImage = fontImages[fontImageIdx];
    if (fontImage != 0) {
      int iw, ih;
      const unsigned char* data = mFontContext->getTextureData (&iw, &ih);

      int x = dirty[0];
      int y = dirty[1];
      int w = dirty[2] - dirty[0];
      int h = dirty[3] - dirty[1];

      cLog::log (LOGINFO, "flushTextTexture - dirty - idx:%d - iw:%d ih:%d x:%d y:%d w:%d h:%d",
                          fontImageIdx, iw, ih, x,y,w,h);
      updateTexture (fontImage, x,y, w,h, data);
      }
    else
      cLog::log (LOGINFO, "flushTextTexture - dirty");
    }
  }
//}}}
