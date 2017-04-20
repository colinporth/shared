// fontstash.h
#pragma once
#include <stdlib.h>

//{{{
enum FONSflags {
  FONS_ZERO_TOPLEFT = 1,
  FONS_ZERO_BOTTOMLEFT = 2,
  };
//}}}
//{{{
enum FONSalign {
  // Horizontal align
  FONS_ALIGN_LEFT     = 1<<0, // Default
  FONS_ALIGN_CENTER   = 1<<1,
  FONS_ALIGN_RIGHT    = 1<<2,

  // Vertical align
  FONS_ALIGN_TOP      = 1<<3,
  FONS_ALIGN_MIDDLE   = 1<<4,
  FONS_ALIGN_BOTTOM   = 1<<5,
  FONS_ALIGN_BASELINE = 1<<6, // Default
  };
//}}}
//{{{
enum FONSerrorCode {
  // Font atlas is full.
  FONS_ATLAS_FULL = 1,
  // Scratch memory used to render glyphs is full, requested size reported in 'val', you may need to bump up FONS_SCRATCH_BUF_SIZE.
  FONS_SCRATCH_FULL = 2,
  // Calls to fonsPushState has created too large stack, if you need deep state stack bump up FONS_MAX_STATES.
  FONS_STATES_OVERFLOW = 3,
  // Trying to pop too many states fonsPopState().
  FONS_STATES_UNDERFLOW = 4,
  };
//}}}
//{{{
struct FONSparams {
  int width, height;
  unsigned char flags;

  void* userPtr;
  int (*renderCreate)(void* uptr, int width, int height);
  int (*renderResize)(void* uptr, int width, int height);
  void (*renderUpdate)(void* uptr, int* rect, const unsigned char* data);
  void (*renderDraw)(void* uptr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts);
  void (*renderDelete)(void* uptr);
  };

typedef struct FONSparams FONSparams;
//}}}
//{{{
struct FONSquad {
  float x0,y0,s0,t0;
  float x1,y1,s1,t1;
  };

typedef struct FONSquad FONSquad;
//}}}
//{{{
struct FONStextIter {
  float x, y, nextx, nexty, scale, spacing;
  unsigned int codepoint;
  short isize;
  struct FONSfont* font;
  int prevGlyphIndex;
  const char* str;
  const char* next;
  const char* end;
  unsigned int utf8state;
  };

typedef struct FONStextIter FONStextIter;
//}}}
typedef struct FONScontext FONScontext;
//{{{  defines
#define FONS_INVALID -1

#define FONS_NOTUSED(v)  (void)sizeof(v)

void* fons_tmpalloc (size_t size, void* up);
void fons_tmpfree (void* ptr, void* up);

#define STBTT_malloc(x,u) fons_tmpalloc(x,u)
#define STBTT_free(x,u) fons_tmpfree(x,u)

#define FONS_SCRATCH_BUF_SIZE 64000
#define FONS_HASH_LUT_SIZE 256

#define FONS_INIT_FONTS 4
#define FONS_INIT_GLYPHS 256
#define FONS_INIT_ATLAS_NODES 256
#define FONS_VERTEX_COUNT 1024

#define FONS_MAX_STATES 20
#define FONS_MAX_FALLBACKS 20
//}}}

#include "stb_truetype.h"
//{{{
struct FONSttFontImpl {
  stbtt_fontinfo font;
  };

typedef struct FONSttFontImpl FONSttFontImpl;
//}}}
//{{{
int fonsTTinit (FONScontext* context) {
  FONS_NOTUSED(context);
  return 1;
  }
//}}}
//{{{
int fonsTTloadFont (FONScontext* context, FONSttFontImpl* font, unsigned char* data, int dataSize) {

  FONS_NOTUSED(dataSize);
  font->font.userdata = context;
  return stbtt_InitFont (&font->font, data, 0);
  }
//}}}
//{{{
void fonsTTgetFontVMetrics (FONSttFontImpl* font, int* ascent, int* descent, int* lineGap) {
  stbtt_GetFontVMetrics (&font->font, ascent, descent, lineGap);
  }
//}}}
//{{{
float fonsTTgetPixelHeightScale (FONSttFontImpl* font, float size) {
  return stbtt_ScaleForPixelHeight (&font->font, size);
  }
//}}}
//{{{
int fonsTTgetGlyphIndex (FONSttFontImpl* font, int codepoint) {
  return stbtt_FindGlyphIndex (&font->font, codepoint);
  }
//}}}
//{{{
int fonsTTbuildGlyphBitmap (FONSttFontImpl* font, int glyph, float size, float scale,
                            int *advance, int *lsb, int *x0, int *y0, int *x1, int *y1) {
  FONS_NOTUSED(size);
  stbtt_GetGlyphHMetrics(&font->font, glyph, advance, lsb);
  stbtt_GetGlyphBitmapBox(&font->font, glyph, scale, scale, x0, y0, x1, y1);
  return 1;
  }
//}}}
//{{{
void fonsTTrenderGlyphBitmap (FONSttFontImpl* font, unsigned char* output, int outWidth, int outHeight, int outStride,
                              float scaleX, float scaleY, int glyph) {
  stbtt_MakeGlyphBitmap (&font->font, output, outWidth, outHeight, outStride, scaleX, scaleY, glyph);
  }
//}}}
//{{{
int fonsTTgetGlyphKernAdvance (FONSttFontImpl* font, int glyph1, int glyph2) {
  return stbtt_GetGlyphKernAdvance (&font->font, glyph1, glyph2);
  }
//}}}

//{{{
struct FONSglyph {
  unsigned int codepoint;
  int index;
  int next;
  short size;
  short x0,y0,x1,y1;
  short xadv,xoff,yoff;
  };

typedef struct FONSglyph FONSglyph;
//}}}
//{{{
struct FONSfont {
  FONSttFontImpl font;
  char name[64];
  unsigned char* data;
  int dataSize;
  unsigned char freeData;
  float ascender;
  float descender;
  float lineh;
  FONSglyph* glyphs;
  int cglyphs;
  int nglyphs;
  int lut[FONS_HASH_LUT_SIZE];
  int fallbacks[FONS_MAX_FALLBACKS];
  int nfallbacks;
  };

typedef struct FONSfont FONSfont;
//}}}
//{{{
struct FONSstate {
  int font;
  int align;
  float size;
  unsigned int color;
  float spacing;
  };

typedef struct FONSstate FONSstate;
//}}}
//{{{
struct FONSatlasNode {
  short x, y, width;
  };

typedef struct FONSatlasNode FONSatlasNode;
//}}}
//{{{
struct FONSatlas {
  int width, height;
  FONSatlasNode* nodes;
  int nnodes;
  int cnodes;
  };

typedef struct FONSatlas FONSatlas;
//}}}
//{{{
struct FONScontext {
  FONSparams params;
  float itw,ith;
  unsigned char* texData;
  int dirtyRect[4];

  FONSfont** fonts;
  FONSatlas* atlas;
  int cfonts;
  int nfonts;

  float verts[FONS_VERTEX_COUNT*2];
  float tcoords[FONS_VERTEX_COUNT*2];
  unsigned int colors[FONS_VERTEX_COUNT];
  int nverts;

  unsigned char* scratch;
  int nscratch;

  FONSstate states[FONS_MAX_STATES];
  int nstates;

  void (*handleError)(void* uptr, int error, int val);
  void* errorUptr;
  };
//}}}

#define FONS_UTF8_ACCEPT 0
#define FONS_UTF8_REJECT 12
//{{{
void* fons_tmpalloc (size_t size, void* up) {

  auto stash = (FONScontext*)up;

  // 16-byte align the returned pointer
  size = (size + 0xf) & ~0xf;

  if (stash->nscratch+(int)size > FONS_SCRATCH_BUF_SIZE) {
    if (stash->handleError)
      stash->handleError(stash->errorUptr, FONS_SCRATCH_FULL, stash->nscratch+(int)size);
    return NULL;
    }

  unsigned char* ptr = stash->scratch + stash->nscratch;
  stash->nscratch += (int)size;
  return ptr;
  }
//}}}
//{{{
void fons_tmpfree (void* ptr, void* up) {
  (void)ptr;
  (void)up;
  // empty
  }
//}}}
//{{{
void fons_freeFont (FONSfont* font) {

  if (font == NULL)
    return;

  free (font->glyphs);
  if (font->freeData && font->data)
    free (font->data);

  free (font);
  }
//}}}
//{{{
int fons_allocFont (FONScontext* stash) {

  if (stash->nfonts+1 > stash->cfonts) {
    stash->cfonts = stash->cfonts == 0 ? 8 : stash->cfonts * 2;
    stash->fonts = (FONSfont**)realloc(stash->fonts, sizeof(FONSfont*) * stash->cfonts);
    if (stash->fonts == NULL)
      return -1;
    }

  auto font = (FONSfont*)malloc(sizeof(FONSfont));
  if (font == NULL)
    goto error;
  memset (font, 0, sizeof(FONSfont));

  font->glyphs = (FONSglyph*)malloc(sizeof(FONSglyph) * FONS_INIT_GLYPHS);
  if (font->glyphs == NULL)
    goto error;
  font->cglyphs = FONS_INIT_GLYPHS;
  font->nglyphs = 0;

  stash->fonts[stash->nfonts++] = font;
  return stash->nfonts-1;

error:
  fons_freeFont (font);
  return FONS_INVALID;
  }
//}}}
//{{{
FONSstate* fons_getState (FONScontext* stash) {
  return &stash->states[stash->nstates-1];
  }
//}}}

//{{{
// Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
unsigned int fons_decutf8 (unsigned int* state, unsigned int* codep, unsigned int byte) {

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
  *codep = (*state != FONS_UTF8_ACCEPT) ? (byte & 0x3fu) | (*codep << 6) : (0xff >> type) & (byte);
  *state = utf8d[256 + *state + type];
  return *state;
  }
//}}}
//{{{
unsigned int fons_hashint (unsigned int a) {

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
int fons_mini (int a, int b) {
  return a < b ? a : b;
  }
//}}}
//{{{
int fons_maxi (int a, int b) {
  return a > b ? a : b;
  }
//}}}

//{{{
void fonsPushState (FONScontext* stash) {

  if (stash->nstates >= FONS_MAX_STATES) {
    if (stash->handleError)
      stash->handleError(stash->errorUptr, FONS_STATES_OVERFLOW, 0);
    return;
    }

  if (stash->nstates > 0)
    memcpy(&stash->states[stash->nstates], &stash->states[stash->nstates-1], sizeof(FONSstate));
  stash->nstates++;
  }
//}}}
//{{{
void fonsPopState (FONScontext* stash) {

  if (stash->nstates <= 1) {
    if (stash->handleError)
      stash->handleError(stash->errorUptr, FONS_STATES_UNDERFLOW, 0);
    return;
    }

  stash->nstates--;
  }
//}}}
//{{{
void fonsClearState (FONScontext* stash) {

  auto state = fons_getState (stash);
  state->size = 12.0f;
  state->color = 0xffffffff;
  state->font = 0;
  state->spacing = 0;
  state->align = FONS_ALIGN_LEFT | FONS_ALIGN_BASELINE;
  }
//}}}
//{{{
void fons_flush (FONScontext* stash) {
// Flush texture

  if (stash->dirtyRect[0] < stash->dirtyRect[2] && stash->dirtyRect[1] < stash->dirtyRect[3]) {
    if (stash->params.renderUpdate != NULL)
      stash->params.renderUpdate (stash->params.userPtr, stash->dirtyRect, stash->texData);
    // Reset dirty rect
    stash->dirtyRect[0] = stash->params.width;
    stash->dirtyRect[1] = stash->params.height;
    stash->dirtyRect[2] = 0;
    stash->dirtyRect[3] = 0;
    }

  // Flush triangles
  if (stash->nverts > 0) {
    if (stash->params.renderDraw != NULL)
      stash->params.renderDraw (stash->params.userPtr, stash->verts, stash->tcoords, stash->colors, stash->nverts);
    stash->nverts = 0;
    }
  }
//}}}

//{{{  font atlas
//{{{
// Atlas based on Skyline Bin Packer by Jukka Jylänki
void fons_deleteAtlas (FONSatlas* atlas) {

  if (atlas == NULL)
    return;

  if (atlas->nodes != NULL)
    free (atlas->nodes);

  free (atlas);
  }
//}}}
//{{{
FONSatlas* fons_allocAtlas (int w, int h, int nnodes) {

  // Allocate memory for the font stash.
  auto atlas = (FONSatlas*)malloc(sizeof(FONSatlas));
  if (atlas == NULL)
    goto error;
  memset (atlas, 0, sizeof (FONSatlas));

  atlas->width = w;
  atlas->height = h;

  // Allocate space for skyline nodes
  atlas->nodes = (FONSatlasNode*)malloc(sizeof(FONSatlasNode) * nnodes);
  if (atlas->nodes == NULL)
    goto error;

  memset (atlas->nodes, 0, sizeof(FONSatlasNode) * nnodes);
  atlas->nnodes = 0;
  atlas->cnodes = nnodes;

  // Init root node.
  atlas->nodes[0].x = 0;
  atlas->nodes[0].y = 0;
  atlas->nodes[0].width = (short)w;
  atlas->nnodes++;

  return atlas;

error:
  if (atlas)
    fons_deleteAtlas (atlas);
  return NULL;
  }
//}}}

//{{{
int fons_atlasInsertNode (FONSatlas* atlas, int idx, int x, int y, int w) {

  // Insert node
  if (atlas->nnodes+1 > atlas->cnodes) {
    atlas->cnodes = atlas->cnodes == 0 ? 8 : atlas->cnodes * 2;
    atlas->nodes = (FONSatlasNode*)realloc(atlas->nodes, sizeof(FONSatlasNode) * atlas->cnodes);
    if (atlas->nodes == NULL)
      return 0;
    }

  for (int i = atlas->nnodes; i > idx; i--)
    atlas->nodes[i] = atlas->nodes[i-1];
  atlas->nodes[idx].x = (short)x;
  atlas->nodes[idx].y = (short)y;
  atlas->nodes[idx].width = (short)w;
  atlas->nnodes++;

  return 1;
  }
//}}}
//{{{
void fons_atlasRemoveNode (FONSatlas* atlas, int idx) {

  if (atlas->nnodes == 0)
    return;

  for (int i = idx; i < atlas->nnodes-1; i++)
    atlas->nodes[i] = atlas->nodes[i+1];

  atlas->nnodes--;
  }
//}}}

//{{{
void fons_atlasExpand (FONSatlas* atlas, int w, int h) {

  // Insert node for empty space
  if (w > atlas->width)
    fons_atlasInsertNode (atlas, atlas->nnodes, atlas->width, 0, w - atlas->width);

  atlas->width = w;
  atlas->height = h;
  }
//}}}
//{{{
void fons_atlasReset (FONSatlas* atlas, int w, int h) {

  atlas->width = w;
  atlas->height = h;
  atlas->nnodes = 0;

  // Init root node.
  atlas->nodes[0].x = 0;
  atlas->nodes[0].y = 0;
  atlas->nodes[0].width = (short)w;
  atlas->nnodes++;
  }
//}}}
//{{{
int fons_atlasAddSkylineLevel (FONSatlas* atlas, int idx, int x, int y, int w, int h) {

  // Insert new node
  if (fons_atlasInsertNode (atlas, idx, x, y+h, w) == 0)
    return 0;

  // Delete skyline segments that fall under the shadow of the new segment.
  for (int i = idx+1; i < atlas->nnodes; i++) {
    if (atlas->nodes[i].x < atlas->nodes[i-1].x + atlas->nodes[i-1].width) {
      int shrink = atlas->nodes[i-1].x + atlas->nodes[i-1].width - atlas->nodes[i].x;
      atlas->nodes[i].x += (short)shrink;
      atlas->nodes[i].width -= (short)shrink;
      if (atlas->nodes[i].width <= 0) {
        fons_atlasRemoveNode (atlas, i);
        i--;
        }
      else
        break;
      }
    else
      break;
    }

  // Merge same height skyline segments that are next to each other.
  for (int i = 0; i < atlas->nnodes-1; i++) {
    if (atlas->nodes[i].y == atlas->nodes[i+1].y) {
      atlas->nodes[i].width += atlas->nodes[i+1].width;
      fons_atlasRemoveNode (atlas, i+1);
      i--;
      }
    }

  return 1;
  }
//}}}
//{{{
int fons_atlasRectFits (FONSatlas* atlas, int i, int w, int h) {
// Checks if there is enough space at the location of skyline span 'i',
// and return the max height of all skyline spans under that at that location,
// (think tetris block being dropped at that position). Or -1 if no space found.

  int x = atlas->nodes[i].x;
  int y = atlas->nodes[i].y;

  if (x + w > atlas->width)
    return -1;

  int spaceLeft = w;
  while (spaceLeft > 0) {
    if (i == atlas->nnodes) return -1;
    y = fons_maxi (y, atlas->nodes[i].y);
    if (y + h > atlas->height) return -1;
    spaceLeft -= atlas->nodes[i].width;
    ++i;
    }
  return y;
  }
//}}}
//{{{
int fons_atlasAddRect (FONSatlas* atlas, int rw, int rh, int* rx, int* ry) {

  int besth = atlas->height, bestw = atlas->width, besti = -1;
  int bestx = -1, besty = -1, i;

  // Bottom left fit heuristic.
  for (i = 0; i < atlas->nnodes; i++) {
    int y = fons_atlasRectFits (atlas, i, rw, rh);
    if (y != -1) {
      if (y + rh < besth || (y + rh == besth && atlas->nodes[i].width < bestw)) {
        besti = i;
        bestw = atlas->nodes[i].width;
        besth = y + rh;
        bestx = atlas->nodes[i].x;
        besty = y;
        }
      }
    }

  if (besti == -1)
    return 0;

  // Perform the actual packing.
  if (fons_atlasAddSkylineLevel (atlas, besti, bestx, besty, rw, rh) == 0)
    return 0;

  *rx = bestx;
  *ry = besty;

  return 1;
  }
//}}}
//{{{
void fons_addWhiteRect (FONScontext* stash, int w, int h) {

  int x, y, gx, gy;
  unsigned char* dst;
  if (fons_atlasAddRect (stash->atlas, w, h, &gx, &gy) == 0)
    return;

  // Rasterize
  dst = &stash->texData[gx + gy * stash->params.width];
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++)
      dst[x] = 0xff;
    dst += stash->params.width;
    }

  stash->dirtyRect[0] = fons_mini(stash->dirtyRect[0], gx);
  stash->dirtyRect[1] = fons_mini(stash->dirtyRect[1], gy);
  stash->dirtyRect[2] = fons_maxi(stash->dirtyRect[2], gx+w);
  stash->dirtyRect[3] = fons_maxi(stash->dirtyRect[3], gy+h);
  }
//}}}

//{{{
FONSglyph* fons_allocGlyph (FONSfont* font) {

  if (font->nglyphs+1 > font->cglyphs) {
    font->cglyphs = font->cglyphs == 0 ? 8 : font->cglyphs * 2;
    font->glyphs = (FONSglyph*)realloc (font->glyphs, sizeof(FONSglyph) * font->cglyphs);
    if (font->glyphs == NULL) return NULL;
    }

  font->nglyphs++;
  return &font->glyphs[font->nglyphs-1];
  }
//}}}

//{{{
void fonsGetAtlasSize (FONScontext* stash, int* width, int* height) {

  if (stash == NULL)
    return;
  *width = stash->params.width;
  *height = stash->params.height;
  }
//}}}
//{{{
int fonsExpandAtlas (FONScontext* stash, int width, int height) {

  int i, maxy = 0;
  unsigned char* data = NULL;
  if (stash == NULL) return 0;

  width = fons_maxi(width, stash->params.width);
  height = fons_maxi(height, stash->params.height);

  if (width == stash->params.width && height == stash->params.height)
    return 1;

  // Flush pending glyphs.
  fons_flush(stash);

  // Create new texture
  if (stash->params.renderResize != NULL) {
    if (stash->params.renderResize(stash->params.userPtr, width, height) == 0)
      return 0;
  }
  // Copy old texture data over.
  data = (unsigned char*)malloc(width * height);
  if (data == NULL)
    return 0;
  for (i = 0; i < stash->params.height; i++) {
    unsigned char* dst = &data[i*width];
    unsigned char* src = &stash->texData[i*stash->params.width];
    memcpy(dst, src, stash->params.width);
    if (width > stash->params.width)
      memset(dst+stash->params.width, 0, width - stash->params.width);
    }
  if (height > stash->params.height)
    memset(&data[stash->params.height * width], 0, (height - stash->params.height) * width);

  free(stash->texData);
  stash->texData = data;

  // Increase atlas size
  fons_atlasExpand(stash->atlas, width, height);

  // Add existing data as dirty.
  for (i = 0; i < stash->atlas->nnodes; i++)
    maxy = fons_maxi(maxy, stash->atlas->nodes[i].y);
  stash->dirtyRect[0] = 0;
  stash->dirtyRect[1] = 0;
  stash->dirtyRect[2] = stash->params.width;
  stash->dirtyRect[3] = maxy;

  stash->params.width = width;
  stash->params.height = height;
  stash->itw = 1.0f/stash->params.width;
  stash->ith = 1.0f/stash->params.height;
  return 1;
  }
//}}}
//{{{
int fonsResetAtlas (FONScontext* stash, int width, int height) {

  int i, j;
  if (stash == NULL)
    return 0;

  // Flush pending glyphs.
  fons_flush(stash);

  // Create new texture
  if (stash->params.renderResize != NULL) {
    if (stash->params.renderResize (stash->params.userPtr, width, height) == 0)
      return 0;
    }

  // Reset atlas
  fons_atlasReset(stash->atlas, width, height);

  // Clear texture data.
  stash->texData = (unsigned char*)realloc(stash->texData, width * height);
  if (stash->texData == NULL) return 0;
  memset(stash->texData, 0, width * height);

  // Reset dirty rect
  stash->dirtyRect[0] = width;
  stash->dirtyRect[1] = height;
  stash->dirtyRect[2] = 0;
  stash->dirtyRect[3] = 0;

  // Reset cached glyphs
  for (i = 0; i < stash->nfonts; i++) {
    FONSfont* font = stash->fonts[i];
    font->nglyphs = 0;
    for (j = 0; j < FONS_HASH_LUT_SIZE; j++)
      font->lut[j] = -1;
    }

  stash->params.width = width;
  stash->params.height = height;
  stash->itw = 1.0f/stash->params.width;
  stash->ith = 1.0f/stash->params.height;

  // Add white rect at 0,0 for debug drawing.
  fons_addWhiteRect(stash, 2,2);

  return 1;
  }
//}}}
//}}}

//{{{
void fonsDeleteInternal (FONScontext* stash) {

  int i;
  if (stash == NULL)
    return;

  if (stash->params.renderDelete)
    stash->params.renderDelete (stash->params.userPtr);

  for (i = 0; i < stash->nfonts; ++i)
    fons_freeFont (stash->fonts[i]);

  if (stash->atlas)
    fons_deleteAtlas(stash->atlas);

  free (stash->fonts);
  free (stash->texData);
  free (stash->scratch);
  free (stash);
  }
//}}}
//{{{
FONScontext* fonsCreateInternal (FONSparams* params) {

  // Allocate memory for the font stash.
  auto stash = (FONScontext*)malloc (sizeof (FONScontext));
  if (stash == NULL)
    goto error;
  memset (stash, 0, sizeof (FONScontext));

  stash->params = *params;

  // Allocate scratch buffer.
  stash->scratch = (unsigned char*)malloc (FONS_SCRATCH_BUF_SIZE);
  if (stash->scratch == NULL)
    goto error;

  // Initialize implementation library
  if (!fonsTTinit (stash))
    goto error;

  if (stash->params.renderCreate != NULL)
    if (stash->params.renderCreate (stash->params.userPtr, stash->params.width, stash->params.height) == 0)
      goto error;

  stash->atlas = fons_allocAtlas (stash->params.width, stash->params.height, FONS_INIT_ATLAS_NODES);
  if (stash->atlas == NULL)
    goto error;

  // Allocate space for fonts.
  stash->fonts = (FONSfont**)malloc (sizeof(FONSfont*) * FONS_INIT_FONTS);
  if (stash->fonts == NULL)
    goto error;
  memset (stash->fonts, 0, sizeof(FONSfont*) * FONS_INIT_FONTS);
  stash->cfonts = FONS_INIT_FONTS;
  stash->nfonts = 0;

  // Create texture for the cache.
  stash->itw = 1.0f/stash->params.width;
  stash->ith = 1.0f/stash->params.height;
  stash->texData = (unsigned char*)malloc (stash->params.width * stash->params.height);
  if (stash->texData == NULL)
    goto error;
  memset (stash->texData, 0, stash->params.width * stash->params.height);

  stash->dirtyRect[0] = stash->params.width;
  stash->dirtyRect[1] = stash->params.height;
  stash->dirtyRect[2] = 0;
  stash->dirtyRect[3] = 0;

  // Add white rect at 0,0 for debug drawing.
  fons_addWhiteRect (stash, 2,2);
  fonsPushState (stash);
  fonsClearState (stash);
  return stash;

error:
  fonsDeleteInternal (stash);
  return NULL;
  }
//}}}
//{{{
int fonsAddFallbackFont (FONScontext* stash, int base, int fallback) {

  auto baseFont = stash->fonts[base];
  if (baseFont->nfallbacks < FONS_MAX_FALLBACKS) {
    baseFont->fallbacks[baseFont->nfallbacks++] = fallback;
    return 1;
    }

  return 0;
  }
//}}}

// external interface
//{{{
int fonsAddFontMem (FONScontext* stash, const char* name, unsigned char* data, int dataSize, int freeData) {

  int idx = fons_allocFont (stash);
  if (idx == FONS_INVALID)
    return FONS_INVALID;

  auto font = stash->fonts[idx];
  strncpy (font->name, name, sizeof(font->name));
  font->name[sizeof(font->name)-1] = '\0';

  // Init hash lookup.
  for (int i = 0; i < FONS_HASH_LUT_SIZE; ++i)
    font->lut[i] = -1;

  // Read in the font data.
  font->dataSize = dataSize;
  font->data = data;
  font->freeData = (unsigned char)freeData;

  // Init font
  stash->nscratch = 0;
  if (!fonsTTloadFont(stash, &font->font, data, dataSize)) {
    fons_freeFont (font);
    stash->nfonts--;
    return FONS_INVALID;
    }

  // Store normalized line height. The real line height is got
  // by multiplying the lineh by font size.
  int ascent, descent, lineGap;
  fonsTTgetFontVMetrics (&font->font, &ascent, &descent, &lineGap);
  int fh = ascent - descent;

  font->ascender = (float)ascent / (float)fh;
  font->descender = (float)descent / (float)fh;
  font->lineh = (float)(fh + lineGap) / (float)fh;
  return idx;
  }
//}}}
//{{{
int fonsAddFont (FONScontext* stash, const char* name, const char* path) {

  // Read in the font data.
  auto fp = fopen (path, "rb");
  if (fp == NULL)
    return FONS_INVALID;

  fseek (fp, 0, SEEK_END);
  auto dataSize = ftell (fp);

  fseek (fp, 0, SEEK_SET);
  auto data = (unsigned char*)malloc (dataSize);
  if (data == NULL) {
    fclose (fp);
    return FONS_INVALID;
    }

  fread (data, 1, dataSize, fp);
  fclose (fp);

  return fonsAddFontMem (stash, name, data, dataSize, 1);
  }
//}}}
//{{{
int fonsGetFontByName (FONScontext* s, const char* name) {

  for (int i = 0; i < s->nfonts; i++)
    if (strcmp(s->fonts[i]->name, name) == 0)
      return i;

  return FONS_INVALID;
  }
//}}}

void fonsSetSize (FONScontext* stash, float size) { fons_getState(stash)->size = size; }
void fonsSetColor (FONScontext* stash, unsigned int color) { fons_getState(stash)->color = color; }
void fonsSetSpacing (FONScontext* stash, float spacing) { fons_getState(stash)->spacing = spacing; }
void fonsSetAlign (FONScontext* stash, int align) { fons_getState(stash)->align = align; }
void fonsSetFont (FONScontext* stash, int font) { fons_getState(stash)->font = font; }

//{{{
FONSglyph* fons_getGlyph (FONScontext* stash, FONSfont* font, unsigned int codepoint, short isize) {

  int i, g, advance, lsb, x0, y0, x1, y1, gw, gh, gx, gy, x, y;
  float scale;
  FONSglyph* glyph = NULL;
  unsigned int h;
  float size = isize/10.0f;
  int pad, added;
  unsigned char* dst;
  FONSfont* renderFont = font;

  if (isize < 2)
    return NULL;
  pad = 2;

  // Reset allocator.
  stash->nscratch = 0;

  // Find code point and size.
  h = fons_hashint (codepoint) & (FONS_HASH_LUT_SIZE-1);
  i = font->lut[h];
  while (i != -1) {
    if (font->glyphs[i].codepoint == codepoint && font->glyphs[i].size == isize)
      return &font->glyphs[i];
    i = font->glyphs[i].next;
    }

  // Could not find glyph, create it.
  g = fonsTTgetGlyphIndex (&font->font, codepoint);
  // Try to find the glyph in fallback fonts.
  if (g == 0) {
    for (i = 0; i < font->nfallbacks; ++i) {
      FONSfont* fallbackFont = stash->fonts[font->fallbacks[i]];
      int fallbackIndex = fonsTTgetGlyphIndex(&fallbackFont->font, codepoint);
      if (fallbackIndex != 0) {
        g = fallbackIndex;
        renderFont = fallbackFont;
        break;
        }
      }
    // It is possible that we did not find a fallback glyph.
    // In that case the glyph index 'g' is 0, and we'll proceed below and cache empty glyph.
    }

  scale = fonsTTgetPixelHeightScale (&renderFont->font, size);
  fonsTTbuildGlyphBitmap (&renderFont->font, g, size, scale, &advance, &lsb, &x0, &y0, &x1, &y1);
  gw = x1-x0 + pad*2;
  gh = y1-y0 + pad*2;

  // Find free spot for the rect in the atlas
  added = fons_atlasAddRect (stash->atlas, gw, gh, &gx, &gy);
  if (added == 0 && stash->handleError != NULL) {
    // Atlas is full, let the user to resize the atlas (or not), and try again.
    stash->handleError (stash->errorUptr, FONS_ATLAS_FULL, 0);
    added = fons_atlasAddRect (stash->atlas, gw, gh, &gx, &gy);
    }
  if (added == 0)
    return NULL;

  // Init glyph.
  glyph = fons_allocGlyph (font);
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
  dst = &stash->texData[(glyph->x0+pad) + (glyph->y0+pad) * stash->params.width];
  fonsTTrenderGlyphBitmap (&renderFont->font, dst, gw-pad*2,gh-pad*2, stash->params.width, scale,scale, g);

  // Make sure there is one pixel empty border.
  dst = &stash->texData[glyph->x0 + glyph->y0 * stash->params.width];
  for (y = 0; y < gh; y++) {
    dst[y*stash->params.width] = 0;
    dst[gw-1 + y*stash->params.width] = 0;
    }
  for (x = 0; x < gw; x++) {
    dst[x] = 0;
    dst[x + (gh-1)*stash->params.width] = 0;
    }

  stash->dirtyRect[0] = fons_mini(stash->dirtyRect[0], glyph->x0);
  stash->dirtyRect[1] = fons_mini(stash->dirtyRect[1], glyph->y0);
  stash->dirtyRect[2] = fons_maxi(stash->dirtyRect[2], glyph->x1);
  stash->dirtyRect[3] = fons_maxi(stash->dirtyRect[3], glyph->y1);

  return glyph;
  }
//}}}
//{{{
void fons_getQuad (FONScontext* stash, FONSfont* font,
                   int prevGlyphIndex, FONSglyph* glyph,
                   float scale, float spacing, float* x, float* y, FONSquad* q) {

  float rx,ry,xoff,yoff,x0,y0,x1,y1;

  if (prevGlyphIndex != -1) {
    float adv = fonsTTgetGlyphKernAdvance(&font->font, prevGlyphIndex, glyph->index) * scale;
    *x += (int)(adv + spacing + 0.5f);
    }

  // Each glyph has 2px border to allow good interpolation,
  // one pixel to prevent leaking, and one to allow good interpolation for rendering.
  // Inset the texture region by one pixel for correct interpolation.
  xoff = (short)(glyph->xoff+1);
  yoff = (short)(glyph->yoff+1);
  x0 = (float)(glyph->x0+1);
  y0 = (float)(glyph->y0+1);
  x1 = (float)(glyph->x1-1);
  y1 = (float)(glyph->y1-1);

  if (stash->params.flags & FONS_ZERO_TOPLEFT) {
    rx = (float)(int)(*x + xoff);
    ry = (float)(int)(*y + yoff);

    q->x0 = rx;
    q->y0 = ry;
    q->x1 = rx + x1 - x0;
    q->y1 = ry + y1 - y0;

    q->s0 = x0 * stash->itw;
    q->t0 = y0 * stash->ith;
    q->s1 = x1 * stash->itw;
    q->t1 = y1 * stash->ith;
    }
  else {
    rx = (float)(int)(*x + xoff);
    ry = (float)(int)(*y - yoff);

    q->x0 = rx;
    q->y0 = ry;
    q->x1 = rx + x1 - x0;
    q->y1 = ry - y1 + y0;

    q->s0 = x0 * stash->itw;
    q->t0 = y0 * stash->ith;
    q->s1 = x1 * stash->itw;
    q->t1 = y1 * stash->ith;
    }

  *x += (int)(glyph->xadv / 10.0f + 0.5f);
  }
//}}}
//{{{
void fons_vertex (FONScontext* stash, float x, float y, float s, float t, unsigned int c) {

  stash->verts[stash->nverts*2+0] = x;
  stash->verts[stash->nverts*2+1] = y;
  stash->tcoords[stash->nverts*2+0] = s;
  stash->tcoords[stash->nverts*2+1] = t;
  stash->colors[stash->nverts] = c;
  stash->nverts++;
  }
//}}}
//{{{
float fons_getVertAlign (FONScontext* stash, FONSfont* font, int align, short isize) {

  if (stash->params.flags & FONS_ZERO_TOPLEFT) {
    if (align & FONS_ALIGN_TOP)
      return font->ascender * (float)isize/10.0f;
    else if (align & FONS_ALIGN_MIDDLE)
      return (font->ascender + font->descender) / 2.0f * (float)isize/10.0f;
    else if (align & FONS_ALIGN_BASELINE)
      return 0.0f;
    else if (align & FONS_ALIGN_BOTTOM)
      return font->descender * (float)isize/10.0f;
    }
  else if (align & FONS_ALIGN_TOP)
    return -font->ascender * (float)isize/10.0f;
  else if (align & FONS_ALIGN_MIDDLE)
    return -(font->ascender + font->descender) / 2.0f * (float)isize/10.0f;
  else if (align & FONS_ALIGN_BASELINE)
    return 0.0f;
  else if (align & FONS_ALIGN_BOTTOM)
    return -font->descender * (float)isize/10.0f;

  return 0.0;
  }
//}}}

//{{{
float fonsTextBounds (FONScontext* stash, float x, float y, const char* str, const char* end, float* bounds) {

  auto state = fons_getState (stash);

  unsigned int codepoint;
  unsigned int utf8state = 0;

  int prevGlyphIndex = -1;
  short isize = (short)(state->size * 10.0f);

  float startx, advance;
  float minx, miny, maxx, maxy;

  if (stash == NULL)
    return 0;
  if (state->font < 0 || state->font >= stash->nfonts)
    return 0;

  auto font = stash->fonts[state->font];
  if (font->data == NULL)
    return 0;

  float scale = fonsTTgetPixelHeightScale (&font->font, (float)isize / 10.0f);

  // Align vertically.
  y += fons_getVertAlign (stash, font, state->align, isize);

  minx = maxx = x;
  miny = maxy = y;
  startx = x;

  if (end == NULL)
    end = str + strlen(str);

  for (; str != end; ++str) {
    if (fons_decutf8(&utf8state, &codepoint, *(const unsigned char*)str))
      continue;
    auto glyph = fons_getGlyph (stash, font, codepoint, isize);
    if (glyph != NULL) {
      FONSquad q;
      fons_getQuad (stash, font, prevGlyphIndex, glyph, scale, state->spacing, &x, &y, &q);
      if (q.x0 < minx)
        minx = q.x0;
      if (q.x1 > maxx)
        maxx = q.x1;
      if (stash->params.flags & FONS_ZERO_TOPLEFT) {
        if (q.y0 < miny)
          miny = q.y0;
        if (q.y1 > maxy)
          maxy = q.y1;
        }
      else {
        if (q.y1 < miny)
          miny = q.y1;
        if (q.y0 > maxy)
          maxy = q.y0;
        }
      }
    prevGlyphIndex = glyph != NULL ? glyph->index : -1;
    }

  advance = x - startx;

  // Align horizontally
  if (state->align & FONS_ALIGN_LEFT) {
    // empty
    }
  else if (state->align & FONS_ALIGN_RIGHT) {
    minx -= advance;
    maxx -= advance;
    }
 else if (state->align & FONS_ALIGN_CENTER) {
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
float fonsDrawText (FONScontext* stash, float x, float y, const char* str, const char* end) {

  auto state = fons_getState(stash);
  unsigned int codepoint;
  unsigned int utf8state = 0;

  FONSglyph* glyph = NULL;
  FONSquad q;
  int prevGlyphIndex = -1;
  short isize = (short)(state->size*10.0f);
  float scale;
  FONSfont* font;
  float width;

  if (stash == NULL)
    return x;
  if (state->font < 0 || state->font >= stash->nfonts)
    return x;
  font = stash->fonts[state->font];
  if (font->data == NULL)
    return x;

  scale = fonsTTgetPixelHeightScale(&font->font, (float)isize/10.0f);

  if (end == NULL)
    end = str + strlen(str);

  // Align horizontally
  if (state->align & FONS_ALIGN_LEFT) {
    // empty
    }
  else if (state->align & FONS_ALIGN_RIGHT) {
    width = fonsTextBounds(stash, x,y, str, end, NULL);
    x -= width;
    }
  else if (state->align & FONS_ALIGN_CENTER) {
    width = fonsTextBounds(stash, x,y, str, end, NULL);
    x -= width * 0.5f;
    }

  // Align vertically.
  y += fons_getVertAlign(stash, font, state->align, isize);

  for (; str != end; ++str) {
    if (fons_decutf8(&utf8state, &codepoint, *(const unsigned char*)str))
      continue;
    glyph = fons_getGlyph(stash, font, codepoint, isize);
    if (glyph != NULL) {
      fons_getQuad(stash, font, prevGlyphIndex, glyph, scale, state->spacing, &x, &y, &q);

      if (stash->nverts+6 > FONS_VERTEX_COUNT)
        fons_flush(stash);

      fons_vertex(stash, q.x0, q.y0, q.s0, q.t0, state->color);
      fons_vertex(stash, q.x1, q.y1, q.s1, q.t1, state->color);
      fons_vertex(stash, q.x1, q.y0, q.s1, q.t0, state->color);

      fons_vertex(stash, q.x0, q.y0, q.s0, q.t0, state->color);
      fons_vertex(stash, q.x0, q.y1, q.s0, q.t1, state->color);
      fons_vertex(stash, q.x1, q.y1, q.s1, q.t1, state->color);
      }
    prevGlyphIndex = glyph != NULL ? glyph->index : -1;
    }

  fons_flush(stash);
  return x;
  }
//}}}
//{{{
int fonsTextIterInit (FONScontext* stash, FONStextIter* iter,
                      float x, float y, const char* str, const char* end) {

  auto state = fons_getState (stash);
  float width;

  memset (iter, 0, sizeof (*iter));

  if (stash == NULL)
    return 0;
  if (state->font < 0 || state->font >= stash->nfonts)
    return 0;
  iter->font = stash->fonts[state->font];
  if (iter->font->data == NULL)
    return 0;

  iter->isize = (short)(state->size*10.0f);
  iter->scale = fonsTTgetPixelHeightScale (&iter->font->font, (float)iter->isize/10.0f);

  // Align horizontally
  if (state->align & FONS_ALIGN_LEFT) {
    // empty
    }
  else if (state->align & FONS_ALIGN_RIGHT) {
    width = fonsTextBounds (stash, x,y, str, end, NULL);
    x -= width;
    }
  else if (state->align & FONS_ALIGN_CENTER) {
    width = fonsTextBounds (stash, x,y, str, end, NULL);
    x -= width * 0.5f;
    }

  // Align vertically.
  y += fons_getVertAlign (stash, iter->font, state->align, iter->isize);

  if (end == NULL)
    end = str + strlen(str);

  iter->x = iter->nextx = x;
  iter->y = iter->nexty = y;
  iter->spacing = state->spacing;
  iter->str = str;
  iter->next = str;
  iter->end = end;
  iter->codepoint = 0;
  iter->prevGlyphIndex = -1;

  return 1;
  }
//}}}
//{{{
int fonsTextIterNext (FONScontext* stash, FONStextIter* iter, FONSquad* quad) {

  FONSglyph* glyph = NULL;
  const char* str = iter->next;
  iter->str = iter->next;

  if (str == iter->end)
    return 0;

  for (; str != iter->end; str++) {
    if (fons_decutf8 (&iter->utf8state, &iter->codepoint, *(const unsigned char*)str))
      continue;
    str++;
    // Get glyph and quad
    iter->x = iter->nextx;
    iter->y = iter->nexty;
    glyph = fons_getGlyph (stash, iter->font, iter->codepoint, iter->isize);
    if (glyph != NULL)
      fons_getQuad (stash, iter->font, iter->prevGlyphIndex, glyph, iter->scale, iter->spacing, &iter->nextx, &iter->nexty, quad);
    iter->prevGlyphIndex = glyph != NULL ? glyph->index : -1;
    break;
    }

  iter->next = str;
  return 1;
  }
//}}}
//{{{
void fonsDrawDebug (FONScontext* stash, float x, float y) {

  int i;
  int w = stash->params.width;
  int h = stash->params.height;
  float u = w == 0 ? 0 : (1.0f / w);
  float v = h == 0 ? 0 : (1.0f / h);

  if (stash->nverts+6+6 > FONS_VERTEX_COUNT)
    fons_flush (stash);

  // Draw background
  fons_vertex (stash, x+0, y+0, u, v, 0x0fffffff);
  fons_vertex (stash, x+w, y+h, u, v, 0x0fffffff);
  fons_vertex (stash, x+w, y+0, u, v, 0x0fffffff);

  fons_vertex (stash, x+0, y+0, u, v, 0x0fffffff);
  fons_vertex (stash, x+0, y+h, u, v, 0x0fffffff);
  fons_vertex (stash, x+w, y+h, u, v, 0x0fffffff);

  // Draw texture
  fons_vertex (stash, x+0, y+0, 0, 0, 0xffffffff);
  fons_vertex (stash, x+w, y+h, 1, 1, 0xffffffff);
  fons_vertex (stash, x+w, y+0, 1, 0, 0xffffffff);

  fons_vertex (stash, x+0, y+0, 0, 0, 0xffffffff);
  fons_vertex (stash, x+0, y+h, 0, 1, 0xffffffff);
  fons_vertex (stash, x+w, y+h, 1, 1, 0xffffffff);

  // Drawbug draw atlas
  for (i = 0; i < stash->atlas->nnodes; i++) {
    FONSatlasNode* n = &stash->atlas->nodes[i];

    if (stash->nverts+6 > FONS_VERTEX_COUNT)
      fons_flush (stash);

    fons_vertex (stash, x+n->x+0, y+n->y+0, u, v, 0xc00000ff);
    fons_vertex (stash, x+n->x+n->width, y+n->y+1, u, v, 0xc00000ff);
    fons_vertex (stash, x+n->x+n->width, y+n->y+0, u, v, 0xc00000ff);

    fons_vertex (stash, x+n->x+0, y+n->y+0, u, v, 0xc00000ff);
    fons_vertex (stash, x+n->x+0, y+n->y+1, u, v, 0xc00000ff);
    fons_vertex (stash, x+n->x+n->width, y+n->y+1, u, v, 0xc00000ff);
    }

  fons_flush (stash);
  }
//}}}
//{{{
void fonsVertMetrics (FONScontext* stash, float* ascender, float* descender, float* lineh) {

  auto state = fons_getState(stash);

  if (stash == NULL)
    return;
  if (state->font < 0 || state->font >= stash->nfonts)
    return;

  auto font = stash->fonts[state->font];
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
void fonsLineBounds (FONScontext* stash, float y, float* miny, float* maxy) {

  auto state = fons_getState(stash);
  short isize;

  if (stash == NULL) 
    return;
  if (state->font < 0 || state->font >= stash->nfonts)
    return;

  auto font = stash->fonts[state->font];
  isize = (short)(state->size*10.0f);
  if (font->data == NULL)
    return;

  y += fons_getVertAlign (stash, font, state->align, isize);

  if (stash->params.flags & FONS_ZERO_TOPLEFT) {
    *miny = y - font->ascender * (float)isize/10.0f;
    *maxy = *miny + font->lineh*isize/10.0f;
    }
  else {
    *maxy = y + font->descender * (float)isize/10.0f;
    *miny = *maxy - font->lineh*isize/10.0f;
    }
  }
//}}}
//{{{
const unsigned char* fonsGetTextureData (FONScontext* stash, int* width, int* height) {

  if (width != NULL)
    *width = stash->params.width;
  if (height != NULL)
    *height = stash->params.height;

  return stash->texData;
  }
//}}}
//{{{
int fonsValidateTexture (FONScontext* stash, int* dirty) {

  if (stash->dirtyRect[0] < stash->dirtyRect[2] && stash->dirtyRect[1] < stash->dirtyRect[3]) {
    dirty[0] = stash->dirtyRect[0];
    dirty[1] = stash->dirtyRect[1];
    dirty[2] = stash->dirtyRect[2];
    dirty[3] = stash->dirtyRect[3];

    // Reset dirty rect
    stash->dirtyRect[0] = stash->params.width;
    stash->dirtyRect[1] = stash->params.height;
    stash->dirtyRect[2] = 0;
    stash->dirtyRect[3] = 0;
    return 1;
    }

  return 0;
  }
//}}}

//{{{
void fonsSetErrorCallback (FONScontext* stash, void (*callback)(void* uptr, int error, int val), void* uptr) {

  if (stash == NULL)
    return;

  stash->handleError = callback;
  stash->errorUptr = uptr;
  }
//}}}
