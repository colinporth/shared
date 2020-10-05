// cAtlasText.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include "cAtlasText.h"

#include <algorithm>

#include "../utils/utils.h"
#include "../utils/cLog.h"

#define STB_TRUETYPE_IMPLEMENTATION
//{{{
static void* STBTT_malloc (size_t size, cAtlasText* fontContext) {
// allocate and return size from mScratchBuf, no free

  // 16-byte align allocation
  size = (size + 0xf) & ~0xf;

  // do we have enough in mScratchBuf
  if (fontContext->mScratchBufSize + (int)size > cAtlasText::kScratchBufSize)
    return nullptr;

  // crude allcoate from mScratchBuf
  unsigned char* scratchPtr = fontContext->mScratchBuf + fontContext->mScratchBufSize;
  fontContext->mScratchBufSize += (int)size;

  return scratchPtr;
  }
//}}}
//{{{
static void STBTT_free (void* ptr, cAtlasText* fontContext) {
  // no free, just allocate
  }
//}}}
#include "stb_truetype.h"

using namespace std;
//}}}

//{{{
static unsigned int hashInt (unsigned int a) {

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
static unsigned int decUtf8 (unsigned int* state, unsigned int* codep, unsigned int byte) {
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
  *codep = (*state != 0) ? (byte & 0x3fu) | (*codep << 6) : (0xff >> type) & (byte);
  *state = utf8d[256 + *state + type];
  return *state;
  }
//}}}
//{{{  cAtlasText::cAtlas members
// public
//{{{
cAtlasText::cAtlas::cAtlas (int width, int height, int maxNumNodes) {

  mWidth = width;
  mHeight = height;

  // Allocate space for skyline nodes
  mNodes = (sNode*)malloc(sizeof(sNode) * maxNumNodes);
  mNumNodes = 0;
  mMaxNumNodes = maxNumNodes;

  // Init root node.
  mNodes[0].mX = 0;
  mNodes[0].mY = 0;
  mNodes[0].mWidth = (short)width;

  mNumNodes++;
  }
//}}}
//{{{
// Atlas based on Skyline Bin Packer by Jukka Jylänki
cAtlasText::cAtlas::~cAtlas() {

  if (mNodes != NULL)
    free (mNodes);
  }
//}}}

//{{{
void cAtlasText::cAtlas::reset (int width, int height) {

  mWidth = width;
  mHeight = height;
  mNumNodes = 0;

  // Init root node.
  mNodes[0].mX = 0;
  mNodes[0].mY = 0;
  mNodes[0].mWidth = (short)width;
  mNumNodes++;
  }
//}}}
//{{{
void cAtlasText::cAtlas::expand (int width, int height) {

  // Insert node for empty space
  if (width > mWidth)
    insertNode (mNumNodes, mWidth, 0, width - mWidth);

  mWidth = width;
  mHeight = height;
  }
//}}}

// private
//{{{
int cAtlasText::cAtlas::insertNode (int index, int x, int y, int width) {

  // Insert node
  if (mNumNodes+1 > mMaxNumNodes) {
    mMaxNumNodes = mMaxNumNodes == 0 ? 8 : mMaxNumNodes * 2;
    mNodes = (sNode*)realloc (mNodes, sizeof(sNode) * mMaxNumNodes);
    }

  for (int i = mNumNodes; i > index; i--)
    mNodes[i] = mNodes[i-1];
  mNodes[index].mX = (short)x;
  mNodes[index].mY = (short)y;
  mNodes[index].mWidth = (short)width;
  mNumNodes++;

  return 1;
  }
//}}}
//{{{
void cAtlasText::cAtlas::removeNode (int index) {

  if (mNumNodes == 0)
    return;

  for (int i = index; i < mNumNodes-1; i++)
    mNodes[i] = mNodes[i+1];

  mNumNodes--;
  }
//}}}

//{{{
int cAtlasText::cAtlas::rectFits (int i, int width, int height) {
// Checks if there is enough space at the location of skyline span 'i',
// and return the max mHeight of all skyline spans under that at that location,
// (think tetris block being dropped at that position). Or -1 if no space found.

  int x = mNodes[i].mX;
  int y = mNodes[i].mY;

  if (x + width > mWidth)
    return -1;

  int spaceLeft = width;
  while (spaceLeft > 0) {
    if (i == mNumNodes)
     return -1;
    y = max (y, int(mNodes[i].mY));
    if (y + height > mHeight)
      return -1;
    spaceLeft -= mNodes[i].mWidth;
    ++i;
    }
  return y;
  }
//}}}
//{{{
int cAtlasText::cAtlas::addSkylineLevel (int index, int x, int y, int width, int height) {

  // Insert new node
  if (insertNode (index, x, y + height, width) == 0)
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
//{{{
int cAtlasText::cAtlas::addRect (int rw, int rh, int* rx, int* ry) {

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

//}}}

// public
//{{{
cAtlasText::cAtlasText (int width, int height) {

  mScratchBuf = (unsigned char*)malloc (kScratchBufSize);
  mAtlas = new cAtlas (width, height, kInitAtlasNodes);

  // create texture for cache
  mWidth = width;
  mHeight = height;
  itw = 1.0f / width;
  ith = 1.0f / height;

  texData = (unsigned char*)malloc (width * height);
  memset (texData, 0, width * height);

  dirtyRect[0] = width;
  dirtyRect[1] = height;
  dirtyRect[2] = 0;
  dirtyRect[3] = 0;

  pushState();
  clearState();
  }
//}}}
//{{{
cAtlasText::~cAtlasText() {

  for (auto font : mFonts) {
    free (font->mFontInfo);
    free (font);
    }
  mFonts.clear();

  delete mAtlas;

  free (texData);
  free (mScratchBuf);
  }
//}}}

//{{{
int cAtlasText::addFont (const string& name, unsigned char* data, int dataSize) {

  int fontIndex = allocFont();
  auto font = mFonts[fontIndex];
  font->name = name;

  // init hash lookup
  for (int i = 0; i < kHashLutSize; ++i)
    font->mHashLut[i] = -1;

  // inti font, read font data, point to us for alloc context
  mScratchBufSize = 0;
  font->mData = data;
  font->mDataSize = dataSize;
  font->mFontInfo = (stbtt_fontinfo*)malloc (sizeof(stbtt_fontinfo));
  font->mFontInfo->userdata = this;
  if (!stbtt_InitFont (font->mFontInfo, data, 0)) {
    cLog::log (LOGERROR, "addFont failed to load " + name);
    return kInvalid;
    }

  // store normalized line height, real line height is lineh * font size.
  int ascent;
  int descent;
  int lineGap;
  ttGetFontVMetrics (font->mFontInfo, &ascent, &descent, &lineGap);

  float fh = float(ascent - descent);
  font->mAscender = ascent / fh;
  font->mDescender = descent / fh;
  font->mLineh = (fh + lineGap) / fh;

  return fontIndex;
  }
//}}}
//{{{
int cAtlasText::resetAtlas (int width, int height) {

  flushPendingGlyphs();

  // reset atlas
  mAtlas->reset (width, height);

  // clear texture data
  texData = (unsigned char*)realloc (texData, width * height);
  memset (texData, 0, width * height);

  // reset dirty rect
  dirtyRect[0] = width;
  dirtyRect[1] = height;
  dirtyRect[2] = 0;
  dirtyRect[3] = 0;

  // reset cached glyphs
  for (auto font : mFonts) {
    font->mNumGlyphs = 0;
    for (int j = 0; j < kHashLutSize; j++)
      font->mHashLut[j] = -1;
    }

  mWidth = width;
  mHeight = height;
  itw = 1.0f / mWidth;
  ith = 1.0f / mHeight;

  return 1;
  }
//}}}

//{{{
int cAtlasText::getFontByName (const string& name) {

  int index = 0;
  for (auto font : mFonts) {
    if (font->name == name)
      return index;
    index++;
    }

  return kInvalid;
  }
//}}}
//{{{
cAtlasText::sGlyph* cAtlasText::getGlyph (cFont* font, unsigned int codepoint, short isize) {

  int i, g, advance, lsb, x0, y0, x1, y1, gw, gh, gx, gy, x, y;
  float scale;
  sGlyph* glyph = NULL;
  unsigned int h;
  float size = isize/10.0f;
  int pad, added;
  unsigned char* dst;
  cFont* renderFont = font;

  if (isize < 2)
    return NULL;
  pad = 2;

  // Reset allocator.
  mScratchBufSize = 0;

  // Find code point and size.
  h = hashInt (codepoint) & (kHashLutSize -1);
  i = font->mHashLut[h];
  while (i != -1) {
    if ((font->mGlyphs[i].codepoint == codepoint) && (font->mGlyphs[i].size == isize))
      return &font->mGlyphs[i];
    i = font->mGlyphs[i].next;
    }

  // Could not find glyph, create it.
  g = ttGetGlyphIndex (font->mFontInfo, codepoint);

  scale = ttGetPixelHeightScale (renderFont->mFontInfo, size);
  ttBuildGlyphBitmap (renderFont->mFontInfo, g, size, scale, &advance, &lsb, &x0, &y0, &x1, &y1);
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
  glyph->next = font->mHashLut[h];
  font->mHashLut[h] = font->mNumGlyphs-1;

  // Rasterize
  dst = &texData[(glyph->x0+pad) + (glyph->y0+pad) * mWidth];
  ttRenderGlyphBitmap (renderFont->mFontInfo, dst, gw-pad*2,gh-pad*2, mWidth, scale,scale, g);

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

  dirtyRect[0] = min (dirtyRect[0], int(glyph->x0));
  dirtyRect[1] = min (dirtyRect[1], int(glyph->y0));
  dirtyRect[2] = max (dirtyRect[2], int(glyph->x1));
  dirtyRect[3] = max (dirtyRect[3], int(glyph->y1));

  return glyph;
  }
//}}}
//{{{
float cAtlasText::getVertAlign (cFont* font, int align, short isize) {

  if (align & ALIGN_TOP)
    return font->mAscender * (float)isize/10.0f;
  else if (align & ALIGN_MIDDLE)
    return (font->mAscender + font->mDescender) / 2.0f * (float)isize/10.0f;
  else if (align & ALIGN_BASELINE)
    return 0.0f;
  else if (align & ALIGN_BOTTOM)
    return font->mDescender * (float)isize/10.0f;

  return 0.0;
  }
//}}}
//{{{
float cAtlasText::getVertMetrics (float& ascender, float& descender) {

  auto state = getState();

  if (state->font < 0) {
    ascender = 0.f;
    descender = 0.f;
    return 0.f;
    }

  auto font = mFonts[state->font];
  short isize = (short)(state->size*10.0f);
  ascender = font->mAscender * isize / 10.f;
  descender = font->mDescender * isize / 10.f;
  return font->mLineh * isize / 10.f;
  }
//}}}
//{{{
void cAtlasText::getLineBounds (float y, float& miny, float& maxy) {

  auto state = getState();
  short isize;

  if (state->font < 0)
    return;

  auto font = mFonts[state->font];
  isize = (short)(state->size * 10.f);
  y += getVertAlign (font, state->align, isize);

  miny = y - font->mAscender * (float)isize / 10.f;
  maxy = miny + font->mLineh * isize / 10.f;
  }
//}}}
//{{{
float cAtlasText::getTextBounds (float x, float y, const char* str, const char* end, float* bounds) {

  auto state = getState();

  int prevGlyphIndex = -1;
  short isize = (short)(state->size * 10.0f);

  float startx, advance;
  float minx, miny, maxx, maxy;

  if (state->font < 0)
    return 0;

  auto font = mFonts[state->font];
  float scale = ttGetPixelHeightScale (font->mFontInfo, (float)isize / 10.0f);

  // Align vertically.
  y += getVertAlign (font, state->align, isize);

  minx = maxx = x;
  miny = maxy = y;
  startx = x;

  if (end == NULL)
    end = str + strlen(str);

  unsigned int codepoint;
  unsigned int utf8state = 0;
  for (; str != end; ++str) {
    if (decUtf8 (&utf8state, &codepoint, *(const unsigned char*)str))
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
  if (state->align & ALIGN_RIGHT) {
    minx -= advance;
    maxx -= advance;
    }
  else if (state->align & ALIGN_CENTER) {
    minx -= advance * 0.5f;
    maxx -= advance * 0.5f;
    }

  bounds[0] = minx;
  bounds[1] = miny;
  bounds[2] = maxx;
  bounds[3] = maxy;

  return advance;
  }
//}}}

//{{{
int cAtlasText::getAtlasDirty (int* dirty) {

  if ((dirtyRect[0] < dirtyRect[2]) && (dirtyRect[1] < dirtyRect[3])) {
    dirty[0] = dirtyRect[0];
    dirty[1] = dirtyRect[1];
    dirty[2] = dirtyRect[2];
    dirty[3] = dirtyRect[3];

    // reset dirty rect
    dirtyRect[0] = mWidth;
    dirtyRect[1] = mHeight;
    dirtyRect[2] = 0;
    dirtyRect[3] = 0;
    return 1;
    }

  return 0;
  }
//}}}
//{{{
const unsigned char* cAtlasText::getAtlasTextureData (int& width, int& height) {

  width = mWidth;
  height = mHeight;
  return texData;
  }
//}}}

void cAtlasText::setColor (unsigned int color) { getState()->color = color; }
void cAtlasText::setFont (int font) { getState()->font = font; }
void cAtlasText::setSize (float size) { getState()->size = size; }
void cAtlasText::setSpacing (float spacing) { getState()->spacing = spacing; }
void cAtlasText::setAlign (int align) { getState()->align = align; }
//{{{
void cAtlasText::setFontSizeSpacingAlign (int font, float size, float spacing, int align) {

  getState()->font = font;
  getState()->size = size;
  getState()->spacing = spacing;
  getState()->align = align;
  }
//}}}

//{{{
int cAtlasText::textIt (sTextIt* it, float x, float y, const char* str, const char* end) {

  auto state = getState();
  if (state->font < 0)
    return 0;

  it->font = mFonts[state->font];
  it->isize = (short)(state->size * 10.f);
  it->scale = ttGetPixelHeightScale (it->font->mFontInfo, (float)it->isize / 10.f);

  // align horizontally
  if (state->align & ALIGN_RIGHT) {
    float textBounds[4];
    float width = getTextBounds (x,y, str,end, textBounds);
    x -= width;
    }
  else if (state->align & ALIGN_CENTER) {
    float textBounds[4];
    float width = getTextBounds (x,y, str,end, textBounds);
    x -= width * 0.5f;
    }

  // align vertically
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
int cAtlasText::textItNext (sTextIt* it, sQuad* quad) {

  const char* str = it->next;

  it->str = it->next;
  if (str == it->end)
    return 0;

  for (; str != it->end; str++) {
    if (decUtf8 (&it->utf8state, &it->codepoint, *(const unsigned char*)str))
      continue;

    str++;

    // get glyph,quad
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

// private
//{{{
cAtlasText::sFontState* cAtlasText::getState() {
  return &states[mNumStates-1];
  }
//}}}
//{{{
void cAtlasText::clearState() {

  auto state = getState();

  state->font = 0;
  state->size = 12.0f;
  state->spacing = 0;
  state->align = ALIGN_LEFT | ALIGN_BASELINE;
  state->color = 0xffffffff;
  }
//}}}
//{{{
void cAtlasText::pushState() {

  if (mNumStates >= kMaxFontStates)
    return;

  if (mNumStates > 0)
    memcpy (&states[mNumStates], &states[mNumStates-1], sizeof(sFontState));

  mNumStates++;
  }
//}}}
//{{{
void cAtlasText::popState() {

  if (mNumStates <= 1)
    return;

  mNumStates--;
  }
//}}}

//{{{
void cAtlasText::ttGetFontVMetrics (stbtt_fontinfo* fontInfo, int* ascent, int* descent, int* lineGap) {
  stbtt_GetFontVMetrics (fontInfo, ascent, descent, lineGap);
  }
//}}}
//{{{
float cAtlasText::ttGetPixelHeightScale (stbtt_fontinfo* fontInfo, float size) {
  return stbtt_ScaleForPixelHeight (fontInfo, size);
  }
//}}}
//{{{
int cAtlasText::ttGetGlyphIndex (stbtt_fontinfo* fontInfo, int codepoint) {
  return stbtt_FindGlyphIndex (fontInfo, codepoint);
  }
//}}}
//{{{
int cAtlasText::ttGetGlyphKernAdvance (stbtt_fontinfo* fontInfo, int glyph1, int glyph2) {
  return stbtt_GetGlyphKernAdvance (fontInfo, glyph1, glyph2);
  }
//}}}
//{{{
int cAtlasText::ttBuildGlyphBitmap (stbtt_fontinfo* fontInfo, int glyph, float size, float scale,
                        int *advance, int *lsb, int *x0, int *y0, int *x1, int *y1) {

  stbtt_GetGlyphHMetrics (fontInfo, glyph, advance, lsb);
  stbtt_GetGlyphBitmapBox (fontInfo, glyph, scale, scale, x0, y0, x1, y1);
  return 1;
  }
//}}}
//{{{
void cAtlasText::ttRenderGlyphBitmap (stbtt_fontinfo* fontInfo, unsigned char* output,
                          int outWidth, int outHeight, int outStride,
                          float scaleX, float scaleY, int glyph) {
  stbtt_MakeGlyphBitmap (fontInfo, output, outWidth, outHeight, outStride, scaleX, scaleY, glyph);
  }
//}}}

//{{{
int cAtlasText::allocFont() {

  auto font = new cFont;
  mFonts.push_back (font);

  font->mGlyphs = (sGlyph*)malloc(sizeof(sGlyph) * kInitGlyphs);
  font->mNumAllocatedGlyphs = kInitGlyphs;
  font->mNumGlyphs = 0;
  return (int)mFonts.size() - 1;
  }
//}}}
//{{{
void cAtlasText::freeFont (cFont* font) {

  if (font == NULL)
    free (font->mGlyphs);
  free (font);
  }
//}}}

//{{{
void cAtlasText::getQuad (cFont* font, int prevGlyphIndex, sGlyph* glyph,
              float scale, float spacing, float* x, float* y, sQuad* q) {

  if (prevGlyphIndex != -1) {
    float adv = ttGetGlyphKernAdvance (font->mFontInfo, prevGlyphIndex, glyph->index) * scale;
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
cAtlasText::sGlyph* cAtlasText::allocGlyph (cFont* font) {

  if (font->mNumGlyphs+1 > font->mNumAllocatedGlyphs) {
    font->mNumAllocatedGlyphs = font->mNumAllocatedGlyphs == 0 ? 8 : font->mNumAllocatedGlyphs * 2;
    font->mGlyphs = (sGlyph*)realloc (font->mGlyphs, sizeof(sGlyph) * font->mNumAllocatedGlyphs);
    }

  font->mNumGlyphs++;
  return &font->mGlyphs[font->mNumGlyphs-1];
  }
//}}}
//{{{
int cAtlasText::expandAtlas (int width, int height) {

  width = max (width, mWidth);
  height = max (height, mHeight);
  if ((width == mWidth) && (height == mHeight))
    return 1;

  flushPendingGlyphs();

  // copy old texture data
  unsigned char* data = (unsigned char*)malloc(width * height);
  for (int i = 0; i < mHeight; i++) {
    unsigned char* dst = &data[i*width];
    unsigned char* src = &texData[i*mWidth];
    memcpy (dst, src, mWidth);
    }
  free (texData);

  texData = data;

  // increase atlas size
  mAtlas->expand (width, height);

  // add existing data as dirty
  int maxy = 0;
  for (int i = 0; i < mAtlas->mNumNodes; i++)
    maxy = max (maxy, int(mAtlas->mNodes[i].mY));

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
void cAtlasText::flushPendingGlyphs() {
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
