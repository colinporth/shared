// cDumpTransportStream.h
//{{{  includes
#pragma once
#include "cTransportStream.h"
//}}}

//{{{  defines
#define DVBSUB_PAGE_SEGMENT     0x10
#define DVBSUB_REGION_SEGMENT   0x11
#define DVBSUB_CLUT_SEGMENT     0x12
#define DVBSUB_OBJECT_SEGMENT   0x13
#define DVBSUB_DISPLAYDEFINITION_SEGMENT 0x14
#define DVBSUB_DISPLAY_SEGMENT  0x80

#define AV_RB16(p) (*(p)<<8)|*(p+1)

#define RGBA(r,g,b,a) (((unsigned)(a) << 24) | ((r) << 16) | ((g) << 8) | (b))
//}}}
//{{{  structs
//{{{
struct DVBSubCLUT {
  int id;
  int version;

  uint32_t clut4[4];
  uint32_t clut16[16];
  uint32_t clut256[256];

  DVBSubCLUT* next;
  };
//}}}
//{{{
struct DVBSubObjectDisplay {
  int object_id;
  int region_id;

  int x_pos;
  int y_pos;

  int fgcolor;
  int bgcolor;

  DVBSubObjectDisplay* region_list_next;
  DVBSubObjectDisplay* object_list_next;
  };
//}}}
//{{{
struct DVBSubObject {
  int id;
  int version;

  int type;

  DVBSubObjectDisplay* display_list;

  DVBSubObject* next;
  };
//}}}
//{{{
struct DVBSubRegionDisplay {
  int region_id;

  int x_pos;
  int y_pos;

  DVBSubRegionDisplay* next;
  };
//}}}
//{{{
struct DVBSubRegion {
  int id;
  int version;

  int width;
  int height;
  int depth;

  int clut;
  int bgcolor;

  uint8_t computed_clut[4*256];
  int has_computed_clut;

  uint8_t* pbuf;
  int buf_size;
  int dirty;

  DVBSubObjectDisplay* display_list;

  DVBSubRegion* next;
  };
//}}}
//{{{
struct DVBSubDisplayDefinition {
  int version;

  int x;
  int y;
  int width;
  int height;
  };
//}}}
//{{{
struct DVBSubContext {
  int composition_id;
  int ancillary_id;

  int version;
  int time_out;
  int compute_edt; /**< if 1 end display time calculated using pts
                          if 0 (Default) calculated using time out */
  int compute_clut;
  int clut_count2[257][256];
  int substream;
  int64_t prev_start;
  DVBSubRegion* region_list;
  DVBSubCLUT* clut_list;
  DVBSubObject* object_list;

  DVBSubRegionDisplay* display_list;
  DVBSubDisplayDefinition* display_definition;
  };

//}}}
//}}}
static DVBSubCLUT default_clut;
//{{{
enum AVSubtitleType {
  SUBTITLE_NONE,
  SUBTITLE_BITMAP,                // A bitmap, pict will be set
  // Plain text, the text field must be set by the decoder and is
  // authoritative. ass and pict fields may contain approximations.
  SUBTITLE_TEXT,
  };
//}}}
//{{{
struct AVSubtitleRect {
  int x;         // top left corner  of pict, undefined when pict is not set
  int y;         // top left corner  of pict, undefined when pict is not set
  int w;         // width            of pict, undefined when pict is not set
  int h;         // height           of pict, undefined when pict is not set
  int nb_colors; // number of colors in pict, undefined when pict is not set

  // data+linesize for the bitmap of this subtitle.
  // Can be set for text/ass as well once they are rendered.
  uint8_t* data[4];
  int linesize[4];

  enum AVSubtitleType type;
  char* text;                     // 0 terminated plain UTF-8 text
  };
//}}}
//{{{
struct AVSubtitle {
  uint16_t format; /* 0 = graphics */
  uint32_t start_display_time; /* relative to packet pts, in ms */
  uint32_t end_display_time; /* relative to packet pts, in ms */
  unsigned num_rects;
  AVSubtitleRect **rects;
  int64_t pts;    ///< Same as packet pts, in AV_TIME_BASE
  };
//}}}

//{{{
class cBitStream {
public:
   cBitStream (const uint8_t* buffer, int32_t bytes) : mBuffer(buffer), mPosition(0), mLimit(bytes * 8) {}

  // gets
  //{{{
  inline uint32_t getBits (int32_t n) {

    uint32_t s = mPosition & 7;
    int32_t shl = n + s;

    const uint8_t* p = mBuffer + (mPosition >> 3);
    if ((mPosition += n) > mLimit)
      return 0;

    uint32_t cache = 0;
    uint32_t next = *p++ & (255 >> s);
    while ((shl -= 8) > 0) {
      cache |= next << shl;
      next = *p++;
      }

    return cache | (next >> -shl);
    }
  //}}}
  inline const uint8_t* getBuffer() { return mBuffer; }
  inline int32_t getPosition() { return mPosition; }
  inline int32_t getLimit() { return mLimit; }

  inline uint32_t getCache() { return mCache; }
  inline uint32_t peekBits (int32_t n) { return mCache >> (32 - n); }
  inline int32_t getBitPosition() { return int32_t(mPtr - mBuffer) * 8 - 24 + mShift; }

  // actions
  inline void setBitStreamPos (int32_t position) { mPosition = position; }

  inline void fillCache() {
    mPtr = mBuffer + mPosition / 8;
    mCache = ((((((mPtr[0] << 8) + mPtr[1]) << 8) + mPtr[2]) << 8) + mPtr[3]) << (mPosition & 7);
    mPtr += 4;
    mShift = (mPosition & 7) - 8;
    }

  inline void flushBits (int32_t n) {
    mCache <<= n;
    mShift += n;
    }

  inline void checkBits() {
    while (mShift >= 0) {
      mCache |= (uint32_t)*mPtr++ << mShift;
      mShift -= 8;
      }
    }

private:
  const uint8_t* mBuffer = nullptr;
  int32_t mPosition = 0;
  int32_t mLimit = 0;

  const uint8_t* mPtr = nullptr;
  uint32_t mCache = 0;
  int32_t mShift = 0;
  };
//}}}

//{{{
static DVBSubObject* get_object (DVBSubContext* ctx, int object_id) {

  DVBSubObject* ptr = ctx->object_list;

  while (ptr && ptr->id != object_id)
    ptr = ptr->next;

  return ptr;
  }
//}}}
//{{{
static DVBSubCLUT* get_clut (DVBSubContext* ctx, int clut_id) {

  DVBSubCLUT* ptr = ctx->clut_list;
  while (ptr && ptr->id != clut_id)
    ptr = ptr->next;

  return ptr;
  }
//}}}
//{{{
static DVBSubRegion* get_region (DVBSubContext* ctx, int region_id) {

  DVBSubRegion* ptr = ctx->region_list;

  while (ptr && ptr->id != region_id)
    ptr = ptr->next;

  return ptr;
  }
//}}}

//{{{
static void delete_region_display_list (DVBSubContext* ctx, DVBSubRegion* region) {

  DVBSubObject* object, *obj2, **obj2_ptr;
  DVBSubObjectDisplay* display, *obj_disp, **obj_disp_ptr;

  while (region->display_list) {
    display = region->display_list;

    object = get_object (ctx, display->object_id);
    if (object) {
      obj_disp_ptr = &object->display_list;
      obj_disp = *obj_disp_ptr;

      while (obj_disp && obj_disp != display) {
         obj_disp_ptr = &obj_disp->object_list_next;
         obj_disp = *obj_disp_ptr;
          }

      if (obj_disp) {
        *obj_disp_ptr = obj_disp->object_list_next;

        if (!object->display_list) {
          obj2_ptr = &ctx->object_list;
          obj2 = *obj2_ptr;

          while (obj2 != object) {
            //av_assert0(obj2);
            obj2_ptr = &obj2->next;
            obj2 = *obj2_ptr;
            }

          *obj2_ptr = obj2->next;
          free (&obj2);
          }
        }
      }

    region->display_list = display->region_list_next;

    free (&display);
    }

  }
//}}}
//{{{
static void delete_cluts (DVBSubContext* ctx) {

  while (ctx->clut_list) {
    DVBSubCLUT* clut = ctx->clut_list;
    ctx->clut_list = clut->next;
    free (&clut);
    }
  }
//}}}
//{{{
static void delete_objects (DVBSubContext* ctx) {

  while (ctx->object_list) {
    DVBSubObject* object = ctx->object_list;
    ctx->object_list = object->next;
    free (&object);
    }
  }
//}}}
//{{{
static void delete_regions (DVBSubContext* ctx) {

  while (ctx->region_list) {
    DVBSubRegion* region = ctx->region_list;
    ctx->region_list = region->next;
    delete_region_display_list (ctx, region);
    free (&region->pbuf);
    free (&region);
    }
  }
//}}}

//{{{
static int dvbsub_init_decoder (DVBSubContext* priv_data, uint8_t* extradata, int extradata_size) {

  int i, r, g, b, a = 0;
  DVBSubContext* ctx = priv_data;

  if (ctx->substream < 0) {
    ctx->composition_id = -1;
    ctx->ancillary_id   = -1;
    }
  else if (!extradata || (extradata_size < 4) || ((extradata_size % 5 != 0) && (extradata_size != 4))) {
    //av_log(avctx, AV_LOG_WARNING, "Invalid DVB subtitles stream extradata!\n");
    ctx->composition_id = -1;
    ctx->ancillary_id   = -1;
    }
  else {
    if (extradata_size > 5*ctx->substream + 2) {
      ctx->composition_id = AV_RB16(extradata + 5*ctx->substream);
      ctx->ancillary_id   = AV_RB16(extradata + 5*ctx->substream + 2);
      }
    else {
      //av_log(AV_LOG_WARNING, "Selected DVB subtitles sub-stream %d is not available\n", ctx->substream);
      ctx->composition_id = AV_RB16(extradata);
      ctx->ancillary_id   = AV_RB16(extradata + 2);
      }
    }

  ctx->version = -1;
  ctx->prev_start = 0; //AV_NOPTS_VALUE;

  default_clut.id = -1;
  default_clut.next = NULL;

  default_clut.clut4[0] = RGBA(  0,   0,   0,   0);
  default_clut.clut4[1] = RGBA(255, 255, 255, 255);
  default_clut.clut4[2] = RGBA(  0,   0,   0, 255);
  default_clut.clut4[3] = RGBA(127, 127, 127, 255);

  default_clut.clut16[0] = RGBA(  0,   0,   0,   0);
  for (i = 1; i < 16; i++) {
    if (i < 8) {
      r = (i & 1) ? 255 : 0;
      g = (i & 2) ? 255 : 0;
      b = (i & 4) ? 255 : 0;
      }
    else {
      r = (i & 1) ? 127 : 0;
      g = (i & 2) ? 127 : 0;
      b = (i & 4) ? 127 : 0;
      }
    default_clut.clut16[i] = RGBA(r, g, b, 255);
    }

  default_clut.clut256[0] = RGBA(  0,   0,   0,   0);
  for (i = 1; i < 256; i++) {
    if (i < 8) {
      r = (i & 1) ? 255 : 0;
      g = (i & 2) ? 255 : 0;
      b = (i & 4) ? 255 : 0;
      a = 63;
      }
    else {
      switch (i & 0x88) {
        case 0x00:
          r = ((i & 1) ? 85 : 0) + ((i & 0x10) ? 170 : 0);
          g = ((i & 2) ? 85 : 0) + ((i & 0x20) ? 170 : 0);
          b = ((i & 4) ? 85 : 0) + ((i & 0x40) ? 170 : 0);
          a = 255;
          break;
        case 0x08:
          r = ((i & 1) ? 85 : 0) + ((i & 0x10) ? 170 : 0);
          g = ((i & 2) ? 85 : 0) + ((i & 0x20) ? 170 : 0);
          b = ((i & 4) ? 85 : 0) + ((i & 0x40) ? 170 : 0);
          a = 127;
          break;
        case 0x80:
          r = 127 + ((i & 1) ? 43 : 0) + ((i & 0x10) ? 85 : 0);
          g = 127 + ((i & 2) ? 43 : 0) + ((i & 0x20) ? 85 : 0);
          b = 127 + ((i & 4) ? 43 : 0) + ((i & 0x40) ? 85 : 0);
          a = 255;
          break;
        case 0x88:
          r = ((i & 1) ? 43 : 0) + ((i & 0x10) ? 85 : 0);
          g = ((i & 2) ? 43 : 0) + ((i & 0x20) ? 85 : 0);
          b = ((i & 4) ? 43 : 0) + ((i & 0x40) ? 85 : 0);
          a = 255;
          break;
        }
      }
    default_clut.clut256[i] = RGBA(r, g, b, a);
    }

  return 0;
  }
//}}}
//{{{
static int dvbsub_close_decoder (DVBSubContext* priv_data) {

  DVBSubContext* ctx = priv_data;
  DVBSubRegionDisplay* display;

  delete_regions (ctx);
  delete_objects (ctx);
  delete_cluts (ctx);

  free (&ctx->display_definition);

  while (ctx->display_list) {
    display = ctx->display_list;
    ctx->display_list = display->next;
    free (&display);
    }

  return 0;
  }
//}}}

//{{{
static int dvbsub_read_2bit_string (uint8_t* destbuf, int dbuf_len,
                                    const uint8_t** srcbuf, int buf_size,
                                    int non_mod, uint8_t* map_table, int x_pos) {

  int bits;
  int run_length;
  int pixels_read = x_pos;
  destbuf += x_pos;

  cBitStream bitStream (*srcbuf, buf_size << 3);
  while ((bitStream.getPosition() < (buf_size << 3)) && pixels_read < dbuf_len) {
    bits = bitStream.getBits (2);
    if (bits) {
      if (non_mod != 1 || bits != 1) {
        if (map_table)
          *destbuf++ = map_table[bits];
        else
          *destbuf++ = bits;
        }
      pixels_read++;
      }

    else {
      bits = bitStream.getBits (1);
      if (bits == 1) {
        //{{{  bits == 1
        run_length = bitStream.getBits (3) + 3;
        bits = bitStream.getBits (2);

        if (non_mod == 1 && bits == 1)
          pixels_read += run_length;
        else {
          if (map_table)
            bits = map_table[bits];
          while ((run_length-- > 0) && (pixels_read < dbuf_len)) {
            *destbuf++ = bits;
            pixels_read++;
            }
          }
        }
        //}}}
      else {
        bits = bitStream.getBits (1);
        if (bits == 0) {
          bits = bitStream.getBits (2);
          if (bits == 2) {
            //{{{  bits == 2
            run_length = bitStream.getBits (4) + 12;
            bits = bitStream.getBits (2);

            if (non_mod == 1 && bits == 1)
              pixels_read += run_length;
            else {
              if (map_table)
                bits = map_table[bits];
              while ((run_length-- > 0) && (pixels_read < dbuf_len)) {
                *destbuf++ = bits;
                pixels_read++;
                }
              }
            }
            //}}}
          else if (bits == 3) {
            //{{{  bits == 3
            run_length = bitStream.getBits (8) + 29;
            bits = bitStream.getBits (2);

            if (non_mod == 1 && bits == 1)
               pixels_read += run_length;
            else {
              if (map_table)
                bits = map_table[bits];
              while ((run_length-- > 0) && (pixels_read < dbuf_len)) {
                *destbuf++ = bits;
                pixels_read++;
                }
              }
            }
            //}}}
          else if (bits == 1) {
            //{{{  bits == 1
            if (map_table)
              bits = map_table[0];
            else
              bits = 0;
            run_length = 2;
            while ((run_length-- > 0) && (pixels_read < dbuf_len)) {
              *destbuf++ = bits;
              pixels_read++;
              }
            }
            //}}}
          else {
            (*srcbuf) += (bitStream.getPosition() + 7) >> 3;
            return pixels_read;
            }
          }

        else {
          if (map_table)
            bits = map_table[0];
          else
            bits = 0;
          *destbuf++ = bits;
          pixels_read++;
          }
        }
      }
    }

  if (bitStream.getBits (6))
    cLog::log (LOGERROR, "line overflow");

  (*srcbuf) += (bitStream.getPosition() + 7) >> 3;
  return pixels_read;
  }
//}}}
//{{{
static int dvbsub_read_4bit_string (uint8_t* destbuf, int dbuf_len,
                                    const uint8_t **srcbuf, int buf_size,
                                    int non_mod, uint8_t *map_table, int x_pos) {

  int bits;
  int run_length;
  int pixels_read = x_pos;
  destbuf += x_pos;

  cBitStream bitStream (*srcbuf, buf_size << 3);
  while ((bitStream.getPosition() < (buf_size << 3)) && (pixels_read < dbuf_len)) {
    bits = bitStream.getBits (4);
    if (bits) {
      if (non_mod != 1 || bits != 1) {
        if (map_table)
          *destbuf++ = map_table[bits];
        else
          *destbuf++ = bits;
        }
      pixels_read++;
      }

    else {
      bits = bitStream.getBits (1);
      if (bits == 0) {
        //{{{  bits == 0
        run_length = bitStream.getBits (3);
        if (run_length == 0) {
          (*srcbuf) += (bitStream.getPosition() + 7) >> 3;
          return pixels_read;
          }

        run_length += 2;

        if (map_table)
          bits = map_table[0];
        else
          bits = 0;

        while ((run_length-- > 0) && (pixels_read < dbuf_len)) {
          *destbuf++ = bits;
          pixels_read++;
          }
        }
        //}}}
      else {
        bits = bitStream.getBits (1);
        if (bits == 0) {
          //{{{  bits == 0
          run_length = bitStream.getBits (2) + 4;
          bits = bitStream.getBits (4);

          if (non_mod == 1 && bits == 1)
            pixels_read += run_length;
          else {
            if (map_table)
              bits = map_table[bits];
            while ((run_length-- > 0) && (pixels_read < dbuf_len)) {
              *destbuf++ = bits;
              pixels_read++;
              }
            }
          }
          //}}}
        else {
          bits = bitStream.getBits (2);
          if (bits == 2) {
            //{{{  bits == 2
            run_length = bitStream.getBits (4) + 9;
            bits = bitStream.getBits (4);

            if (non_mod == 1 && bits == 1)
              pixels_read += run_length;
            else {
              if (map_table)
                bits = map_table[bits];
              while ((run_length-- > 0) && (pixels_read < dbuf_len)) {
                *destbuf++ = bits;
                pixels_read++;
                }
              }
            }
            //}}}
          else if (bits == 3) {
            //{{{  bits == 3
            run_length = bitStream.getBits (8) + 25;
            bits = bitStream.getBits (4);

            if (non_mod == 1 && bits == 1)
              pixels_read += run_length;
            else {
              if (map_table)
                bits = map_table[bits];
              while ((run_length-- > 0) && (pixels_read < dbuf_len)) {
                *destbuf++ = bits;
                pixels_read++;
                }
              }
            //}}}
            }
          else if (bits == 1) {
            //{{{  bits == 1
            if (map_table)
              bits = map_table[0];
            else
              bits = 0;
            run_length = 2;
            while ((run_length-- > 0) && (pixels_read < dbuf_len)) {
              *destbuf++ = bits;
              pixels_read++;
              }
            }
            //}}}
          else {
            if (map_table)
              bits = map_table[0];
           else
             bits = 0;
            *destbuf++ = bits;
            pixels_read ++;
            }
          }
        }
      }
    }

  if (bitStream.getBits (8))
    cLog::log (LOGERROR, "line overflow");

  (*srcbuf) += (bitStream.getPosition() + 7) >> 3;

  return pixels_read;
  }
//}}}
//{{{
static int dvbsub_read_8bit_string (uint8_t* destbuf, int dbuf_len,
                                    const uint8_t** srcbuf, int buf_size,
                                    int non_mod, uint8_t* map_table, int x_pos) {

  const uint8_t *sbuf_end = (*srcbuf) + buf_size;

  int bits;
  int run_length;
  int pixels_read = x_pos;

  destbuf += x_pos;

  while (*srcbuf < sbuf_end && pixels_read < dbuf_len) {
    bits = *(*srcbuf)++;

    if (bits) {
      if (non_mod != 1 || bits != 1) {
        if (map_table)
          *destbuf++ = map_table[bits];
        else
          *destbuf++ = bits;
        }
      pixels_read++;
      }
    else {
      bits = *(*srcbuf)++;
      run_length = bits & 0x7f;
      if ((bits & 0x80) == 0) {
        if (run_length == 0) {
          return pixels_read;
          }

        bits = 0;
        }
      else
        bits = *(*srcbuf)++;

      if (non_mod == 1 && bits == 1)
        pixels_read += run_length;
      else {
        if (map_table)
          bits = map_table[bits];
        while (run_length-- > 0 && pixels_read < dbuf_len) {
          *destbuf++ = bits;
          pixels_read++;
          }
        }
      }
    }

  if (*(*srcbuf)++)
    cLog::log (LOGERROR, "line overflow");

  return pixels_read;
  }
//}}}
//{{{
static void compute_default_clut (DVBSubContext* ctx, uint8_t* clut, AVSubtitleRect* rect, int w, int h) {

  uint8_t list[256] = {0};
  uint8_t list_inv[256];
  int counttab[256] = {0};
  int (*counttab2)[256] = ctx->clut_count2;
  int count, i, x, y;
  ptrdiff_t stride = rect->linesize[0];

  memset(ctx->clut_count2, 0 , sizeof(ctx->clut_count2));

  #define V(x,y) rect->data[0][(x) + (y)*stride]
    for (y = 0; y<h; y++) {
        for (x = 0; x<w; x++) {
            int v = V(x,y) + 1;
            int vl = x     ? V(x-1,y) + 1 : 0;
            int vr = x+1<w ? V(x+1,y) + 1 : 0;
            int vt = y     ? V(x,y-1) + 1 : 0;
            int vb = y+1<h ? V(x,y+1) + 1 : 0;
            counttab[v-1] += !!((v!=vl) + (v!=vr) + (v!=vt) + (v!=vb));
            counttab2[vl][v-1] ++;
            counttab2[vr][v-1] ++;
            counttab2[vt][v-1] ++;
            counttab2[vb][v-1] ++;
        }
    }

  #define L(x,y) list[d[(x) + (y)*stride]]

  for (i = 0; i<256; i++) {
    counttab2[i+1][i] = 0;
    }
  for (i = 0; i<256; i++) {
    int bestscore = 0;
    int bestv = 0;

    for (x = 0; x < 256; x++) {
      int scorev = 0;
      if (list[x])
        continue;
      scorev += counttab2[0][x];
      for (y = 0; y < 256; y++) {
        scorev += list[y] * counttab2[y+1][x];
        }

      if (scorev) {
        int score = 1024LL*scorev / counttab[x];
        if (score > bestscore) {
          bestscore = score;
          bestv = x;
          }
        }
      }

    if (!bestscore)
      break;
    list    [ bestv ] = 1;
    list_inv[     i ] = bestv;
    }

  count = std::max(i - 1, 1);
  for (i--; i >= 0; i--) {
    int v = i * 255 / count;
    //AV_WN32 (clut + 4*list_inv[i], RGBA(v/2,v,v/2,v));
    *(clut + 4*list_inv[i]) = RGBA(v/2,v,v/2,v);
    }
  }
//}}}

//{{{
//static int save_subtitle_set (DVBSubContext* priv_data, AVSubtitle* sub, int* got_output) {

  //DVBSubContext* ctx = priv_data;
  //DVBSubRegionDisplay* display;
  //DVBSubDisplayDefinition* display_def = ctx->display_definition;
  //DVBSubRegion* region;
  //AVSubtitleRect* rect;
  //DVBSubCLUT* clut;

  //uint32_t* clut_table;
  //unsigned int i;
  //int offset_x=0, offset_y=0;
  //int ret = 0;

  //if (display_def) {
    //offset_x = display_def->x;
    //offset_y = display_def->y;
    //}

  ///* Not touching AVSubtitles again*/
  ////if (sub->num_rects) {
  ////  avpriv_request_sample (ctx, "Different Version of Segment asked Twice");
  ////  return AVERROR_PATCHWELCOME;
  ////  }

  //for (display = ctx->display_list; display; display = display->next) {
    //region = get_region (ctx, display->region_id);
    ////if (region && region->dirty)
    ////  sub->num_rects++;
    //}

  //if (ctx->compute_edt == 0) {
    ////sub->end_display_time = ctx->time_out * 1000;
    //*got_output = 1;
    //}
  ////else if (ctx->prev_start != AV_NOPTS_VALUE) {
  ////  sub->end_display_time = av_rescale_q ((sub->pts - ctx->prev_start ), AV_TIME_BASE_Q, (AVRational){ 1, 1000 }) - 1;
  ////  *got_output = 1;
  ////  }

  //if (sub->num_rects > 0) {
    //// array calloc
    ////sub->rects = calloc (sizeof(*sub->rects), sub->num_rects);
    //if (!sub->rects) {
      //ret = 1;
      //goto fail;
      //}

    //for (i = 0; i < sub->num_rects; i++) {
      //sub->rects[i] = (AVSubtitleRect*)calloc (1,sizeof(*sub->rects[i]));
      //if (!sub->rects[i]) {
        //ret = 1;
        //goto fail;
        //}
      //}

    //i = 0;

    //for (display = ctx->display_list; display; display = display->next) {
      //region = get_region (ctx, display->region_id);
      //if (!region)
        //continue;
      //if (!region->dirty)
        //continue;

      //rect = sub->rects[i];
      //rect->x = display->x_pos + offset_x;
      //rect->y = display->y_pos + offset_y;
      //rect->w = region->width;
      //rect->h = region->height;
      //rect->nb_colors = (1 << region->depth);
      //rect->type = SUBTITLE_BITMAP;
      //rect->linesize[0] = region->width;

      //clut = get_clut (ctx, region->clut);
      //if (!clut)
        //clut = &default_clut;

      //switch (region->depth) {
        //case 2:
          //clut_table = clut->clut4;
          //break;

        //case 8:
          //clut_table = clut->clut256;
          //break;

        //case 4:
        //default:
          //clut_table = clut->clut16;
          //break;
        //}

      //rect->data[1] = (uint8_t*)calloc (1, 1024); //av_palette size
      //if (!rect->data[1]) {
        //ret = 1;
        //goto fail;
        //}
      //memcpy (rect->data[1], clut_table, (1 << region->depth) * sizeof(*clut_table));

      //rect->data[0] = (uint8_t*)malloc (region->buf_size);
      //if (!rect->data[0]) {
        //ret = 1;
        //goto fail;
        //}

      //memcpy (rect->data[0], region->pbuf, region->buf_size);

      //if ((clut == &default_clut && ctx->compute_clut == -1) || ctx->compute_clut == 1) {
        //if (!region->has_computed_clut) {
          //compute_default_clut (ctx, region->computed_clut, rect, rect->w, rect->h);
          //region->has_computed_clut = 1;
          //}

        //memcpy (rect->data[1], region->computed_clut, sizeof(region->computed_clut));
        //}
      //i++;
      //}
    //}

  //return 0;

//fail:
  //if (sub->rects) {
    //for (i = 0; i < sub->num_rects; i++) {
      //rect = sub->rects[i];
      //if (rect) {
        //free (&rect->data[0]);
        //free (&rect->data[1]);
        //}
      //free (&sub->rects[i]);
      //}
    //free (&sub->rects);
    //}

  //sub->num_rects = 0;
  //return ret;
  //}
//}}}
//{{{
//static void dvbsub_parse_pixel_data_block (DVBSubContext* priv_data, DVBSubObjectDisplay *display,
                                          //const uint8_t *buf, int buf_size, int top_bottom, int non_mod) {

  //DVBSubContext* ctx = priv_data;
  //DVBSubRegion* region = get_region (ctx, display->region_id);
  //const uint8_t* buf_end = buf + buf_size;
  //uint8_t *pbuf;
  //int x_pos, y_pos;
  //int i;

  //uint8_t map2to4[] = {  0x0,  0x7,  0x8,  0xf};
  //uint8_t map2to8[] = { 0x00, 0x77, 0x88, 0xff};
  //uint8_t map4to8[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                        //0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
  //uint8_t* map_table;

  //{{{
  ////ff_dlog(avctx, "DVB pixel block size %d, %s field:\n", buf_size, top_bottom ? "bottom" : "top");
  ////for (i = 0; i < buf_size; i++) {
  ////  if (i % 16 == 0)
  ////    ff_dlog(avctx, "0x%8p: ", buf+i);
  ////  ff_dlog(avctx, "%02x ", buf[i]);
  ////  if (i % 16 == 15)
  ////    ff_dlog(avctx, "\n");
  ////}
  ////if (i % 16)
  ////  ff_dlog(avctx, "\n");
  //}}}

  //if (!region)
    //return;

  //pbuf = region->pbuf;
  //region->dirty = 1;

  //x_pos = display->x_pos;
  //y_pos = display->y_pos;

  //y_pos += top_bottom;

  //while (buf < buf_end) {
    //if ((*buf != 0xf0 && x_pos >= region->width) || y_pos >= region->height) {
      //cLog::log (LOGERROR, "Invalid object location! %d-%d %d-%d %02x",
                            //x_pos, region->width, y_pos, region->height, *buf);
      //return;
      //}

    //switch (*buf++) {
      //{{{
      //case 0x10:
        //if (region->depth == 8)
          //map_table = map2to8;
        //else if (region->depth == 4)
          //map_table = map2to4;
        //else
          //map_table = NULL;

        //x_pos = dvbsub_read_2bit_string (pbuf + (y_pos * region->width),
                                         //region->width, &buf, buf_end - buf,
                                         //non_mod, map_table, x_pos);
        //break;
      //}}}
      //{{{
      //case 0x11:
        //if (region->depth < 4) {
          //cLog::log (LOGERROR, "4-bit pixel string in %d-bit region!", region->depth);
          //return;
          //}

        //if (region->depth == 8)
          //map_table = map4to8;
        //else
          //map_table = NULL;
        //x_pos = dvbsub_read_4bit_string (pbuf + (y_pos * region->width),
                                         //region->width, &buf, buf_end - buf,
                                         //non_mod, map_table, x_pos);
        //break;
      //}}}
      //{{{

      //case 0x12:
        //if (region->depth < 8) {
          //cLog::log (LOGERROR, "8-bit pixel string in %d-bit region!", region->depth);
          //return;
          //}

        //x_pos = dvbsub_read_8bit_string (ctx, buf + (y_pos * region->width),
                                         //region->width, &buf, buf_end - buf,
                                         //non_mod, NULL, x_pos);
          //break;
      //}}}
      //{{{
      //case 0x20:
        //map2to4[0] = (*buf) >> 4;
        //map2to4[1] = (*buf++) & 0xf;
        //map2to4[2] = (*buf) >> 4;
        //map2to4[3] = (*buf++) & 0xf;
        //break;
      //}}}
      //{{{
      //case 0x21:
        //for (i = 0; i < 4; i++)
          //map2to8[i] = *buf++;
        //break;
      //}}}
      //{{{
      //case 0x22:
        //for (i = 0; i < 16; i++)
          //map4to8[i] = *buf++;
        //break;
      //}}}
      //{{{
      //case 0xf0:
        //x_pos = display->x_pos;
        //y_pos += 2;
        //break;
      //}}}
      //default:
        //cLog::log (LOGINFO, "Unknown/unsupported pixel block 0x%x\n", *(buf-1));
      //}
    //}

  //region->has_computed_clut = 0;
  //}
//}}}
//{{{
//static int dvbsub_parse_object_segment (DVBSubContext* priv_data, const uint8_t* buf, int buf_size) {

  //DVBSubContext* ctx = priv_data;

  //const uint8_t* buf_end = buf + buf_size;
  //int object_id;
  //DVBSubObject* object;
  //DVBSubObjectDisplay* display;
  //int top_field_len, bottom_field_len;

  //int coding_method, non_modifying_color;

  //object_id = AV_RB16 (buf);
  //buf += 2;

  //object = get_object (ctx, object_id);

  //if (!object)
    //return 1;

  //coding_method = ((*buf) >> 2) & 3;
  //non_modifying_color = ((*buf++) >> 1) & 1;

  //if (coding_method == 0) {
    //top_field_len = AV_RB16(buf);
    //buf += 2;
    //bottom_field_len = AV_RB16(buf);
    //buf += 2;

    //if (buf + top_field_len + bottom_field_len > buf_end) {
      //cLog::log (LOGERROR, "Field data size %d+%d too large", top_field_len, bottom_field_len);
      //return 1;
      //}

    //for (display = object->display_list; display; display = display->object_list_next) {
      //const uint8_t *block = buf;
      //int bfl = bottom_field_len;

      //dvbsub_parse_pixel_data_block (display, block, top_field_len, 0, non_modifying_color);

      //if (bottom_field_len > 0)
        //block = buf + top_field_len;
      //else
        //bfl = top_field_len;

      //dvbsub_parse_pixel_data_block (display, block, bfl, 1, non_modifying_color);
      //}
    //}
  //else
    //cLog::log (LOGERROR, "Unknown object coding %d", coding_method);

  //return 0;
  //}
//}}}
//{{{
//static int dvbsub_parse_clut_segment (DVBSubContext* priv_data, const uint8_t *buf, int buf_size) {

  //DVBSubContext* ctx = priv_data;

  //const uint8_t*buf_end = buf + buf_size;
  //int i, clut_id;
  //int version;
  //DVBSubCLUT* clut;
  //int entry_id, depth , full_range;
  //int y, cr, cb, alpha;
  //int r, g, b, r_add, g_add, b_add;

  ////ff_dlog(avctx, "DVB clut packet:\n");

  //for (i=0; i < buf_size; i++) {
    ////ff_dlog(avctx, "%02x ", buf[i]);
    ////if (i % 16 == 15)
    ////  ff_dlog(avctx, "\n");
    //}

  ////if (i % 16)
  ////  ff_dlog(avctx, "\n");

  //clut_id = *buf++;
  //version = ((*buf)>>4)&15;
  //buf += 1;

  //clut = get_clut (ctx, clut_id);
  //if (!clut) {
    //clut = malloc (sizeof(*clut));
    //if (!clut)
      //return 1;

    //memcpy (clut, &default_clut, sizeof(*clut));

    //clut->id = clut_id;
    //clut->version = -1;

    //clut->next = ctx->clut_list;
    //ctx->clut_list = clut;
    //}

  //if (clut->version != version) {
    //clut->version = version;
    //while (buf + 4 < buf_end) {
      //entry_id = *buf++;
      //depth = (*buf) & 0xe0;
      //if (depth == 0)
        //cLog::log (LOGERROR, "Invalid clut depth 0x%x!n", *buf);

      //full_range = (*buf++) & 1;
      //if (full_range) {
        //y     = *buf++;
        //cr    = *buf++;
        //cb    = *buf++;
        //alpha = *buf++;
        //}
      //else {
        //y     = buf[0] & 0xfc;
        //cr    = (((buf[0] & 3) << 2) | ((buf[1] >> 6) & 3)) << 4;
        //cb    = (buf[1] << 2) & 0xf0;
        //alpha = (buf[1] << 6) & 0xc0;
        //buf += 2;
        //}

      //if (y == 0)
        //alpha = 0xff;

      ////YUV_TO_RGB1_CCIR(cb, cr);
      ////YUV_TO_RGB2_CCIR(r, g, b, y);

      ////ff_dlog(avctx, "clut %d := (%d,%d,%d,%d)\n", entry_id, r, g, b, alpha);
      //if (!!(depth & 0x80) + !!(depth & 0x40) + !!(depth & 0x20) > 1) {
        ////ff_dlog(avctx, "More than one bit level marked: %x\n", depth);
        ////if (avctx->strict_std_compliance > FF_COMPLIANCE_NORMAL)
        ////  return 1;
        //}

      //if (depth & 0x80 && entry_id < 4)
        //clut->clut4[entry_id] = RGBA(r,g,b,255 - alpha);
      //else if (depth & 0x40 && entry_id < 16)
        //clut->clut16[entry_id] = RGBA(r,g,b,255 - alpha);
      //else if (depth & 0x20)
        //clut->clut256[entry_id] = RGBA(r,g,b,255 - alpha);
      //}
    //}

  //return 0;
  //}
//}}}
//{{{
//static int dvbsub_parse_region_segment (DVBSubContext* priv_data, int max_pixels, const uint8_t *buf, int buf_size) {

  //DVBSubContext* ctx = priv_data;

  //const uint8_t* buf_end = buf + buf_size;
  //int region_id, object_id;
  //int av_unused version;
  //DVBSubRegion* region;
  //DVBSubObject* object;
  //DVBSubObjectDisplay* display;
  //int fill;
  //int ret;

  //if (buf_size < 10)
      //return AVERROR_INVALIDDATA;

  //region_id = *buf++;

  //region = get_region(ctx, region_id);

  //if (!region) {
      //region = av_mallocz(sizeof(*region));
      //if (!region)
          //return AVERROR(ENOMEM);

      //region->id = region_id;
      //region->version = -1;

      //region->next = ctx->region_list;
      //ctx->region_list = region;
  //}

  //version = ((*buf)>>4) & 15;
  //fill = ((*buf++) >> 3) & 1;
  //region->width = AV_RB16(buf);
  //buf += 2;
  //region->height = AV_RB16(buf);
  //buf += 2;

  ////ret = av_image_check_size2(region->width, region->height, max_pixels, AV_PIX_FMT_PAL8, 0, avctx);
  ////if (ret >= 0 && region->width * region->height * 2 > 320 * 1024 * 8) {
  ////  ret = 1;
  ////  cLog::loig (LOGERROR, "Pixel buffer memory constraint violated");
  ////  }
  ////if (ret < 0) {
  ////  region->width= region->height= 0;
  ////  return ret;
  ////  }

  //if (region->width * region->height != region->buf_size) {
    //free (region->pbuf);
    //region->buf_size = region->width * region->height;
    //region->pbuf = av_malloc(region->buf_size);
    //if (!region->pbuf) {
      //region->buf_size = 0;
      //region->width = 0;
      //region->height = 0;
      //return 1;
      //}

    //fill = 1;
    //region->dirty = 0;
    //}

  //region->depth = 1 << (((*buf++) >> 2) & 7);
  //if (region->depth < 2 || region->depth > 8) {
    //cLog::log (LOGERROR, "region depth %d is invalid", region->depth);
    //region->depth= 4;
    //}
  //region->clut = *buf++;

  //if (region->depth == 8) {
    //region->bgcolor = *buf++;
    //buf += 1;
    //}
  //else {
    //buf += 1;
    //if (region->depth == 4)
      //region->bgcolor = (((*buf++) >> 4) & 15);
    //else
      //region->bgcolor = (((*buf++) >> 2) & 3);
    //}

  ////ff_dlog(avctx, "Region %d, (%dx%d)\n", region_id, region->width, region->height);

  //if (fill) {
    //memset (region->pbuf, region->bgcolor, region->buf_size);
    ////ff_dlog(avctx, "Fill region (%d)\n", region->bgcolor);
    //}

  //delete_region_display_list (ctx, region);

  //while (buf + 5 < buf_end) {
    //object_id = AV_RB16(buf);
    //buf += 2;

    //object = get_object (ctx, object_id);
    //if (!object) {
      //object = calloc (sizeof(*object));
      //if (!object)
        //return 1;

      //object->id = object_id;
      //object->next = ctx->object_list;
      //ctx->object_list = object;
      //}

    //object->type = (*buf) >> 6;

    //display = calloc (sizeof(*display));
    //if (!display)
       //return A1;

    //display->object_id = object_id;
    //display->region_id = region_id;

    //display->x_pos = AV_RB16(buf) & 0xfff;
    //buf += 2;
   //display->y_pos = AV_RB16(buf) & 0xfff;
   //buf += 2;

    //if (display->x_pos >= region->width ||
      //display->y_pos >= region->height) {
      //cLog::log (LOGERROR, "Object outside region");
      //free (display);
      //return 1;
      //}

    //if ((object->type == 1 || object->type == 2) && buf+1 < buf_end) {
      //display->fgcolor = *buf++;
      //display->bgcolor = *buf++;
      //}

    //display->region_list_next = region->display_list;
    //region->display_list = display;

    //display->object_list_next = object->display_list;
    //object->display_list = display;
    //}

  //return 0;
  //}
//}}}
//{{{
//static int dvbsub_parse_page_segment (DVBSubContext* priv_data,
                                      //const uint8_t *buf, int buf_size, AVSubtitle *sub, int *got_output) {

  //DVBSubContext* ctx = priv_data;
  //DVBSubRegionDisplay* display;
  //DVBSubRegionDisplay* tmp_display_list, **tmp_ptr;

  //const uint8_t buf_end = buf + buf_size;
  //int region_id;
  //int page_state;
  //int timeout;
  //int version;

  //if (buf_size < 1)
    //return 1;

  //timeout = *buf++;
  //version = ((*buf)>>4) & 15;
  //page_state = ((*buf++) >> 2) & 3;

  //if (ctx->version == version) {
    //return 0;
    //}

  //ctx->time_out = timeout;
  //ctx->version = version;

  ////ff_dlog(avctx, "Page time out %ds, state %d\n", ctx->time_out, page_state);

  //if (ctx->compute_edt == 1)
    //save_subtitle_set (sub, got_output);

  //if (page_state == 1 || page_state == 2) {
    //delete_regions(ctx);
    //delete_objects(ctx);
    //delete_cluts(ctx);
    //}

  //tmp_display_list = ctx->display_list;
  //ctx->display_list = NULL;
  //while (buf + 5 < buf_end) {
    //region_id = *buf++;
    //buf += 1;

    //display = ctx->display_list;
    //while (display && display->region_id != region_id) {
      //display = display->next;
      //}
    //if (display) {
      //cLog::log (LOGERROR, "duplicate region");
      //break;
      //}

    //display = tmp_display_list;
    //tmp_ptr = &tmp_display_list;
    //while (display && display->region_id != region_id) {
      //tmp_ptr = &display->next;
      //display = display->next;
      //}

    //if (!display) {
      //display = calloc (sizeof(*display));
      //if (!display)
        //return 1;
      //}

    //display->region_id = region_id;

    //display->x_pos = AV_RB16(buf);
    //buf += 2;
    //display->y_pos = AV_RB16(buf);
    //buf += 2;

    //*tmp_ptr = display->next;

    //display->next = ctx->display_list;
    //ctx->display_list = display;

    ////ff_dlog(avctx, "Region %d, (%d,%d)\n", region_id, display->x_pos, display->y_pos);
    //}

  //while (tmp_display_list) {
    //display = tmp_display_list;
    //tmp_display_list = display->next;
    //free (&display);
    //}

  //return 0;
  //}
//}}}

//{{{
//static int dvbsub_parse_display_definition_segment (DVBSubContext* priv_data, const uint8_t *buf, int buf_size) {

  //DVBSubContext* ctx = priv_data;
  //DVBSubDisplayDefinition* display_def = ctx->display_definition;
  //int dds_version, info_byte;

  //if (buf_size < 5)
    //return 1;

  //info_byte = bytestream_get_byte (&buf);
  //dds_version = info_byte >> 4;
  //if (display_def && display_def->version == dds_version)
    //return 0; // already have this display definition version

  //if (!display_def) {
    //display_def = calloc(sizeof(*display_def));
    //if (!display_def)
      //return 1;
    //ctx->display_definition = display_def;
    //}

  //display_def->version = dds_version;
  //display_def->x       = 0;
  //display_def->y       = 0;
  //display_def->width   = bytestream_get_be16(&buf) + 1;
  //display_def->height  = bytestream_get_be16(&buf) + 1;
  ////if (!avctx->width || !avctx->height) {
  ////  int ret = ff_set_dimensions (avctx, display_def->width, display_def->height);
  ////  if (ret < 0)
  ////      return ret;
  ////  }

  //if (info_byte & 1<<3) { // display_window_flag
    //if (buf_size < 13)
      //return 1;

    //display_def->x = bytestream_get_be16(&buf);
    //display_def->width  = bytestream_get_be16(&buf) - display_def->x + 1;
    //display_def->y = bytestream_get_be16(&buf);
    //display_def->height = bytestream_get_be16(&buf) - display_def->y + 1;
    //}

  //return 0;
  //}
//}}}
//{{{
//static int dvbsub_display_end_segment (DVBSubContext* priv_data, const uint8_t* buf,
                                      //int buf_size, AVSubtitle* sub, int* got_output) {

  //DVBSubContext* ctx = priv_data;
  //if (ctx->compute_edt == 0)
    //save_subtitle_set (sub, got_output);

  ////#ifdef DEBUG
  ////  save_display_set(ctx);
  ////#endif

  //return 0;
  //}
//}}}
//{{{
//static int dvbsub_decode (DVBSubContext* priv_data, void* data, int* got_sub_ptr, const uint8_t* buf, int buf_size) {

  //DVBSubContext* ctx = priv_data;
  //AVSubtitle* sub = data;
  //const uint8_t* p, *p_end;
  //int segment_type;
  //int page_id;
  //int segment_length;
  //int i;
  //int ret = 0;
  //int got_segment = 0;
  //int got_dds = 0;

  ////ff_dlog(avctx, "DVB sub packet:\n");
  ////for (i=0; i < buf_size; i++) {
    ////ff_dlog(avctx, "%02x ", buf[i]);
    ////if (i % 16 == 15)
    ////  ff_dlog(avctx, "\n");
    ////}
  ////if (i % 16)
  ////  ff_dlog(avctx, "\n");
  ////if (buf_size <= 6 || *buf != 0x0f) {
  ////  ff_dlog(avctx, "incomplete or broken packet");
  ////  return 0;
  ////  }

  //p = buf;
  //p_end = buf + buf_size;
  //while (p_end - p >= 6 && *p == 0x0f) {
    //p += 1;
    //segment_type = *p++;
    //page_id = AV_RB16(p);
    //p += 2;
    //segment_length = AV_RB16(p);
    //p += 2;

    //if (avctx->debug & FF_DEBUG_STARTCODE)
      //cLog::log (LOGDEBUG, "segment_type:%d page_id:%d segment_length:%d", segment_type, page_id, segment_length);

    //if (p_end - p < segment_length) {
      ////ff_dlog(avctx, "incomplete or broken packet");
      //ret = -1;
      //goto end;
      //}

    //if (page_id == ctx->composition_id || page_id == ctx->ancillary_id ||
        //ctx->composition_id == -1 || ctx->ancillary_id == -1) {
      //int ret = 0;
      //switch (segment_type) {
        //{{{
        //case DVBSUB_PAGE_SEGMENT:
          //ret = dvbsub_parse_page_segment (p, segment_length, sub, got_sub_ptr);
          //got_segment |= 1;
          //break;
        //}}}
        //{{{
        //case DVBSUB_REGION_SEGMENT:
          //ret = dvbsub_parse_region_segment (p, segment_length);
          //got_segment |= 2;
          //break;
        //}}}
        //{{{
        //case DVBSUB_CLUT_SEGMENT:
         //ret = dvbsub_parse_clut_segment (p, segment_length);
         //if (ret < 0) goto end;
          //got_segment |= 4;
          //break;
        //}}}
        //{{{
        //case DVBSUB_OBJECT_SEGMENT:
          //ret = dvbsub_parse_object_segment (p, segment_length);
          //got_segment |= 8;
          //break;
        //}}}
        //{{{
        //case DVBSUB_DISPLAYDEFINITION_SEGMENT:
         //ret = dvbsub_parse_display_definition_segment (p, segment_length);
          //got_dds = 1;
          //break;
        //}}}
        //{{{
        //case DVBSUB_DISPLAY_SEGMENT:
          //ret = dvbsub_display_end_segment (p, segment_length, sub, got_sub_ptr);
          //if (got_segment == 15 && !got_dds && !avctx->width && !avctx->height) {
              //// Default from ETSI EN 300 743 V1.3.1 (7.2.1)
              ////avctx->width  = 720;
              ////avctx->height = 576;
              //}
            //got_segment |= 16;
            //break;
        //}}}
        //default:
          ////ff_dlog (avctx, "Subtitling segment type 0x%x, page id %d, length %d\n", segment_type, page_id, segment_length);
          //break;
          //}
      //if (ret < 0)
        //goto end;
      //}

    //p += segment_length;
    //}

  //// Some streams do not send a display segment but if we have all the other
  //// segments then we need no further data.
  //if (got_segment == 15) {
    //cLog::log (LOGDEBUG, "Missing display_end_segment, emulating");
    //dvbsub_display_end_segment (p, 0, sub, got_sub_ptr);
    //}

//end:
  //if (ret < 0) {
    //*got_sub_ptr = 0;
    //avsubtitle_free (sub);
    //return ret;
    //}
  //else {
    ////if (ctx->compute_edt == 1)
    ////  FFSWAP (int64_t, ctx->prev_start, sub->pts);
    //}

  //return p - buf;
  //}
//}}}

class cDumpTransportStream : public cTransportStream {
  public:
    cDumpTransportStream (const std::string& rootName,
                          const std::vector <std::string>& channelStrings,
                          const std::vector <std::string>& saveStrings)
      : mRootName(rootName),
        mChannelStrings(channelStrings), mSaveStrings(saveStrings),
        mRecordAll ((channelStrings.size() == 1) && (channelStrings[0] == "all")) {}

    virtual ~cDumpTransportStream() {}

  protected:
    //{{{
    void start (cService* service, const std::string& name,
                std::chrono::system_clock::time_point time,
                std::chrono::system_clock::time_point starttime,
                bool selected) {

      std::lock_guard<std::mutex> lockGuard (mFileMutex);

      service->closeFile();

      bool record = selected || mRecordAll;
      std::string saveName;

      if (!mRecordAll) {
        // filter and rename channel prefix
        size_t i = 0;
        for (auto& channelString : mChannelStrings) {
          if (channelString == service->getChannelString()) {
            record = true;
            if (i < mSaveStrings.size())
              saveName = mSaveStrings[i] +  " ";
            break;
            }
          i++;
          }
        }

      saveName += date::format ("%d %b %y %a %H.%M", date::floor<std::chrono::seconds>(time));

      if (record) {
        if ((service->getVidPid() > 0) && (service->getAudPid() > 0)) {
          auto validName = validFileString (name, "<>:/|?*\"\'\\");
          auto fileNameStr = mRootName + "/" + saveName + " - " + validName + ".ts";
          service->openFile (fileNameStr, 0x1234);
          cLog::log (LOGINFO, fileNameStr);
          }
        }
      }
    //}}}
    //{{{
    void pesPacket (int sid, int pid, uint8_t* ts) {
    // look up service and write it

      std::lock_guard<std::mutex> lockGuard (mFileMutex);

      auto serviceIt = mServiceMap.find (sid);
      if (serviceIt != mServiceMap.end())
        serviceIt->second.writePacket (ts, pid);
      }
    //}}}
    //{{{
    void stop (cService* service) {

      std::lock_guard<std::mutex> lockGuard (mFileMutex);

      service->closeFile();
      }
    //}}}

    //{{{
    bool audDecodePes (cPidInfo* pidInfo, bool skip) {

      //cLog::log (LOGINFO, getPtsString (pidInfo->mPts) + " a - " + dec(int (pidInfo->mBufPtr - pidInfo->mBuffer)));
      return false;
      }
    //}}}
    //{{{
    bool vidDecodePes (cPidInfo* pidInfo, bool skip) {

      //cLog::log (LOGINFO, getPtsString (pidInfo->mPts) + " v - " + dec(int (pidInfo->mBufPtr - pidInfo->mBuffer)));

      return false;
      }
    //}}}
    //{{{
    bool subDecodePes (cPidInfo* pidInfo, bool skip) {

      cLog::log (LOGINFO, getPtsString (pidInfo->mPts) + " s - " + dec(int (pidInfo->mBufPtr - pidInfo->mBuffer)));
      return false;
      }
    //}}}

  private:
    std::string mRootName;

    std::vector<std::string> mChannelStrings;
    std::vector<std::string> mSaveStrings;
    bool mRecordAll;

    std::mutex mFileMutex;
    };
