// ts.cpp
//{{{  includes
#include "pch.h"

#include "dvbSubtitle.h"
//}}}
//{{{  defines
#define DVBSUB_PAGE_SEGMENT     0x10
#define DVBSUB_REGION_SEGMENT   0x11
#define DVBSUB_CLUT_SEGMENT     0x12
#define DVBSUB_OBJECT_SEGMENT   0x13
#define DVBSUB_DISPLAYDEFINITION_SEGMENT 0x14
#define DVBSUB_DISPLAY_SEGMENT  0x80
//}}}

#define RGBA(r,g,b,a) (((unsigned)(a) << 24) | ((r) << 16) | ((g) << 8) | (b))

//{{{
typedef struct DVBSubCLUT {
  int id;
  int version;

  uint32_t clut4[4];
  uint32_t clut16[16];
  uint32_t clut256[256];
  struct DVBSubCLUT *next;
  } DVBSubCLUT;
//}}}
//{{{
typedef struct DVBSubObjectDisplay {
  int object_id;
  int region_id;

  int x_pos;
  int y_pos;

  int fgcolor;
  int bgcolor;

  struct DVBSubObjectDisplay* region_list_next;
  struct DVBSubObjectDisplay* object_list_next;
  } DVBSubObjectDisplay;
//}}}
//{{{
typedef struct DVBSubObject {
   int id;
  int version;
  int type;
  DVBSubObjectDisplay* display_list;
  struct DVBSubObject* next;
  } DVBSubObject;
//}}}
//{{{
typedef struct DVBSubRegionDisplay {
  int region_id;
  int x_pos;
  int y_pos;
  struct DVBSubRegionDisplay *next;
  } DVBSubRegionDisplay;
//}}}
//{{{
typedef struct DVBSubRegion {
  int id;
  int version;

  int width;
  int height;
  int depth;

  int clut;
  int bgcolor;

  uint8_t* pbuf;
  int buf_size;
  int dirty;

  DVBSubObjectDisplay* display_list;

  struct DVBSubRegion* next;
  } DVBSubRegion;
//}}}
//{{{
typedef struct DVBSubDisplayDefinition {
  int version;
  int x;
  int y;
  int width;
  int height;
  } DVBSubDisplayDefinition;
//}}}
//{{{
typedef struct DVBSubContext {

  int composition_id;
  int ancillary_id;

  int version;
  int time_out;
  int compute_edt; /**< if 1 end display time calculated using pts
                        if 0 (Default) calculated using time out */
  int compute_clut;
  int64_t prev_start;

  DVBSubRegion* region_list;
  DVBSubCLUT* clut_list;
  DVBSubObject* object_list;

  DVBSubRegionDisplay* display_list;
  DVBSubDisplayDefinition* display_definition;

  } DVBSubContext;
//}}}

static bool first = true;
static DVBSubCLUT default_clut;
static DVBSubContext subContext;

static uint32_t bitWindow;
static uint32_t bitsInWindow;
static const uint8_t* bufPtr;
//{{{
static void getBitsInit (const uint8_t* buf, int bufSize) {

  bufPtr = buf;
  bitWindow = (*bufPtr++) << 16;
  bitsInWindow = 8;
  }
//}}}
//{{{
static int getBits (uint32_t bitCount) {

  int result = bitWindow >> (24 - (bitCount));

  bitWindow = (bitWindow << bitCount) & 0xFFFFFF;
  bitsInWindow -= bitCount;

  while (bitsInWindow < 16) {
    bitWindow |= (*bufPtr++) << (16 - bitsInWindow);
    bitsInWindow += 8;
    }

  return result;
  }
//}}}

//{{{
static DVBSubObject* getObject (DVBSubContext* ctx, int object_id) {

  DVBSubObject *ptr = ctx->object_list;

  while (ptr && ptr->id != object_id) {
    ptr = ptr->next;
    }

  return ptr;
  }
//}}}
//{{{
static DVBSubCLUT* getClut (DVBSubContext* ctx, int clut_id) {

  DVBSubCLUT *ptr = ctx->clut_list;

  while (ptr && ptr->id != clut_id) {
    ptr = ptr->next;
    }

  return ptr;
  }
//}}}
//{{{
static DVBSubRegion* getRegion (DVBSubContext* ctx, int region_id) {

  DVBSubRegion *ptr = ctx->region_list;

  while (ptr && ptr->id != region_id) {
    ptr = ptr->next;
    }

  return ptr;
  }
//}}}

//{{{
static void deleteRegionDisplayList (DVBSubContext* ctx, DVBSubRegion* region) {

  DVBSubObject* object = NULL;
  DVBSubObject* obj2 = NULL;
  DVBSubObject** obj2_ptr = NULL;
  DVBSubObjectDisplay* display = NULL;
  DVBSubObjectDisplay* obj_disp = NULL;
  DVBSubObjectDisplay** obj_disp_ptr = NULL;

  while (region->display_list) {
    display = region->display_list;

    object = getObject (ctx, display->object_id);

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
static void deleteCluts (DVBSubContext* ctx) {

  while (ctx->clut_list) {
    DVBSubCLUT *clut = ctx->clut_list;
    ctx->clut_list = clut->next;
    free(&clut);
    }
  }
//}}}
//{{{
static void deleteObjects (DVBSubContext* ctx) {

  while (ctx->object_list) {
    DVBSubObject *object = ctx->object_list;
    ctx->object_list = object->next;
    free(&object);
    }
  }
//}}}
//{{{
static void deleteRegions (DVBSubContext* ctx) {

  while (ctx->region_list) {
    DVBSubRegion* region = ctx->region_list;
    ctx->region_list = region->next;

    deleteRegionDisplayList (ctx, region);

    free (&region->pbuf);
    free (&region);
    }
  }
//}}}

//{{{
static int dvbsubRead2bitString (uint8_t* destbuf, int dbuf_len,
                                    const uint8_t** srcbuf, int buf_size,
                                    int non_mod, uint8_t* map_table, int x_pos) {
  int bits;
  int run_length;
  int pixelsRead = x_pos;

  getBitsInit (*srcbuf, buf_size);

  destbuf += x_pos;

  while (/*getBits_count (&gb) < buf_size << 3 &&*/ pixelsRead < dbuf_len) {
    bits = getBits (2);

    if (bits) {
      if (non_mod != 1 || bits != 1) {
        if (map_table)
          *destbuf++ = map_table[bits];
        else
          *destbuf++ = bits;
        }
      pixelsRead++;
      }

    else {
      bits = getBits (1);
      if (bits == 1) {
        //{{{
        run_length = getBits (3) + 3;
        bits = getBits (2);

        if (non_mod == 1 && bits == 1)
          pixelsRead += run_length;
        else {
          if (map_table)
            bits = map_table[bits];
          while (run_length-- > 0 && pixelsRead < dbuf_len) {
            *destbuf++ = bits;
            pixelsRead++;
            }
          }
        }
        //}}}
      else {
        bits = getBits (1);
        if (bits == 0) {
          bits = getBits (2);
          if (bits == 2) {
            //{{{
            run_length = getBits (4) + 12;
            bits = getBits (2);

            if (non_mod == 1 && bits == 1)
              pixelsRead += run_length;
            else {
              if (map_table)
                bits = map_table[bits];
              while (run_length-- > 0 && pixelsRead < dbuf_len) {
                *destbuf++ = bits;
                pixelsRead++;
                }
              }
            }
            //}}}
          else if (bits == 3) {
            //{{{
            run_length = getBits (8) + 29;
            bits = getBits (2);

            if (non_mod == 1 && bits == 1)
              pixelsRead += run_length;
            else {
              if (map_table)
                bits = map_table[bits];
              while (run_length-- > 0 && pixelsRead < dbuf_len) {
                *destbuf++ = bits;
                pixelsRead++;
                }
              }
            }
            //}}}
          else if (bits == 1) {
            //{{{
            if (map_table)
              bits = map_table[0];
            else
              bits = 0;
             run_length = 2;
             while (run_length-- > 0 && pixelsRead < dbuf_len) {
               *destbuf++ = bits;
               pixelsRead++;
              }
            }
            //}}}
          else {
            //(*srcbuf) += (getBits_count (&gb) + 7) >> 3;
            return pixelsRead;
            }
          }
        else {
          //{{{
          if (map_table)
            bits = map_table[0];
          else
            bits = 0;
          *destbuf++ = bits;
          pixelsRead++;
          }
          //}}}
        }
      }
    }

  //(*srcbuf) += (getBits_count(&gb) + 7) >> 3;
  return pixelsRead;
  }
//}}}
//{{{
static int dvbsubRead4bitString (uint8_t* destbuf, int dbuf_len,
                                    const uint8_t** srcbuf, int buf_size,
                                    int non_mod, uint8_t* map_table, int x_pos) {

  int bits;
  int run_length;
  int pixelsRead = x_pos;

  getBitsInit (*srcbuf, buf_size);

  destbuf += x_pos;

  while (/*getBits_count(&gb) < buf_size << 3 && */pixelsRead < dbuf_len) {
    bits = getBits (4);

    if (bits) {
      //{{{
      if (non_mod != 1 || bits != 1) {
        if (map_table)
          *destbuf++ = map_table[bits];
        else
          *destbuf++ = bits;
        }

      pixelsRead++;
      }
      //}}}
    else {
      bits = getBits (1);
      if (bits == 0) {
        run_length = getBits (3);
        if (run_length == 0) {
          //(*srcbuf) += (getBits_count(&gb) + 7) >> 3;
          return pixelsRead;
          }

        run_length += 2;
        if (map_table)
          bits = map_table[0];
        else
          bits = 0;
        while (run_length-- > 0 && pixelsRead < dbuf_len) {
          *destbuf++ = bits;
          pixelsRead++;
          }
        }
      else {
        bits = getBits (1);
        if (bits == 0) {
          //{{{
          run_length = getBits (2) + 4;
          bits = getBits (4);

          if (non_mod == 1 && bits == 1)
            pixelsRead += run_length;
          else {
            //{{{
            if (map_table)
              bits = map_table[bits];
            while (run_length-- > 0 && pixelsRead < dbuf_len) {
              *destbuf++ = bits;
              pixelsRead++;
              }
            }
            //}}}
          }
          //}}}
        else {
          bits = getBits (2);
          if (bits == 2) {
            //{{{
            run_length = getBits (4) + 9;
            bits = getBits (4);

            if (non_mod == 1 && bits == 1)
              pixelsRead += run_length;
            else {
              if (map_table)
                bits = map_table[bits];
              while (run_length-- > 0 && pixelsRead < dbuf_len) {
                *destbuf++ = bits;
                pixelsRead++;
                }
              }
            }
            //}}}
          else if (bits == 3) {
            //{{{
            run_length = getBits (8) + 25;
            bits = getBits (4);

            if (non_mod == 1 && bits == 1)
              pixelsRead += run_length;
            else {
              if (map_table)
                bits = map_table[bits];
              while (run_length-- > 0 && pixelsRead < dbuf_len) {
                *destbuf++ = bits;
                pixelsRead++;
                }
              }
            }
            //}}}
          else if (bits == 1) {
            //{{{
            if (map_table)
              bits = map_table[0];
            else
              bits = 0;
            run_length = 2;
            while (run_length-- > 0 && pixelsRead < dbuf_len) {
              *destbuf++ = bits;
              pixelsRead++;
              }
            }
            //}}}
          else {
            //{{{
            if (map_table)
              bits = map_table[0];
            else
              bits = 0;
            *destbuf++ = bits;
            pixelsRead ++;
            }
            //}}}
          }
        }
      }
    }

  //(*srcbuf) += (getBits_count(&gb) + 7) >> 3;
  return pixelsRead;
  }
//}}}
//{{{
static int dvbsubRead8bitString (uint8_t* destbuf, int dbuf_len,
                                    const uint8_t** srcbuf, int buf_size,
                                    int non_mod, uint8_t* map_table, int x_pos) {

  const uint8_t *sbuf_end = (*srcbuf) + buf_size;
  int bits;
  int run_length;
  int pixelsRead = x_pos;

  destbuf += x_pos;

  while (*srcbuf < sbuf_end && pixelsRead < dbuf_len) {
    bits = *(*srcbuf)++;

    if (bits) {
      if (non_mod != 1 || bits != 1) {
        if (map_table)
          *destbuf++ = map_table[bits];
         else
          *destbuf++ = bits;
        }
      pixelsRead++;
      }

    else {
      bits = *(*srcbuf)++;
      run_length = bits & 0x7f;

      if ((bits & 0x80) == 0) {
        if (run_length == 0) {
          return pixelsRead;
          }

        bits = 0;
        }
      else {
        bits = *(*srcbuf)++;
        }

      if (non_mod == 1 && bits == 1)
        pixelsRead += run_length;
      else {
        if (map_table)
          bits = map_table[bits];
        while (run_length-- > 0 && pixelsRead < dbuf_len) {
          *destbuf++ = bits;
          pixelsRead++;
          }
        }
      }
    }

  return pixelsRead;
  }
//}}}
//{{{
static void dvbsubParsePixelDataBlock (DVBSubObjectDisplay *display,
                                           const uint8_t* buf, int buf_size,
                                           int top_bottom, int non_mod) {
  DVBSubContext* ctx = &subContext;
  DVBSubRegion* region = getRegion (ctx, display->region_id);
  const uint8_t* buf_end = buf + buf_size;

  uint8_t* pbuf;
  int x_pos, y_pos;
  int i;

  uint8_t map2to4[] = { 0x0,  0x7,  0x8,  0xf};
  uint8_t map2to8[] = {0x00, 0x77, 0x88, 0xff};
  uint8_t map4to8[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                       0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
  uint8_t* map_table = NULL;

  if (!region)
    return;

  pbuf = region->pbuf;
  region->dirty = 1;

  x_pos = display->x_pos;
  y_pos = display->y_pos;
  y_pos += top_bottom;

  while (buf < buf_end) {
    if ((*buf != 0xf0 && x_pos >= region->width) || y_pos >= region->height)
      return;

    switch (*buf++) {
      case 0x10:
        //{{{  2bit
        if (region->depth == 8)
          map_table = map2to8;
        else if (region->depth == 4)
          map_table = map2to4;
        else
          map_table = NULL;

        printf ("dvbsubRead2bitString\n");
        x_pos = dvbsubRead2bitString (pbuf + (y_pos * region->width), region->width,
                                      &buf, buf_end - buf, non_mod, map_table, x_pos);
        break;
        //}}}
      case 0x11:
        //{{{  4bit
        if (region->depth < 4)
          return;

        if (region->depth == 8)
          map_table = map4to8;
        else
          map_table = NULL;

        printf ("dvbsubRead4bitString\n");
        x_pos = dvbsubRead4bitString (pbuf + (y_pos * region->width), region->width,
                                      &buf, buf_end - buf, non_mod, map_table, x_pos);
        break;
        //}}}
      case 0x12:
        //{{{  8bit
        if (region->depth < 8)
          return;

        printf ("dvbsubRead8bitString\n");
        x_pos = dvbsubRead8bitString (pbuf + (y_pos * region->width), region->width,
                                      &buf, buf_end - buf, non_mod, NULL, x_pos);
        break;

        //}}}
      case 0x20:
        map2to4[0] = (*buf) >> 4;
        map2to4[1] = (*buf++) & 0xf;
        map2to4[2] = (*buf) >> 4;
        map2to4[3] = (*buf++) & 0xf;
        break;
      case 0x21:
        for (i = 0; i < 4; i++)
          map2to8[i] = *buf++;
        break;
      case 0x22:
        for (i = 0; i < 16; i++)
          map4to8[i] = *buf++;
        break;
      case 0xf0:
        x_pos = display->x_pos;
        y_pos += 2;
        break;
      default:;
      }
    }
  }
//}}}

//{{{
static int dvbsubParsePageSegment (const uint8_t *buf, int buf_size) {

  DVBSubContext* ctx = &subContext;

  const uint8_t *buf_end = buf + buf_size;
  if (buf_size < 1)
    return -1;

  int timeout = *buf++;
  int version = ((*buf)>>4) & 15;
  int page_state = ((*buf++) >> 2) & 3;
  if (ctx->version == version)
    return 0;
  ctx->time_out = timeout;
  ctx->version = version;
  if(ctx->compute_edt == 1)
    printf ("save_subtitle_set(avctx, sub, got_output)\n");

  if (page_state == 1 || page_state == 2) {
    deleteRegions (ctx);
    deleteObjects (ctx);
    deleteCluts( ctx);
    }

  DVBSubRegionDisplay* display = NULL;
  DVBSubRegionDisplay* tmp_display_list = ctx->display_list;
  ctx->display_list = NULL;

  while (buf + 5 < buf_end) {
    int region_id = *buf++;
    buf += 1;

    display = tmp_display_list;
    DVBSubRegionDisplay** tmp_ptr = &tmp_display_list;

    while (display && (display->region_id != region_id)) {
      tmp_ptr = &display->next;
      display = display->next;
      }

    if (!display) {
      display = (DVBSubRegionDisplay*)malloc(sizeof(DVBSubRegionDisplay));
      display->next = NULL;
      }

    display->region_id = region_id;
    display->x_pos = ((*buf) << 8) + *(buf+1);
    buf += 2;
    display->y_pos = ((*buf) << 8) + *(buf+1);
    buf += 2;

    *tmp_ptr = display->next;
    display->next = ctx->display_list;
    ctx->display_list = display;
    }

  while (tmp_display_list) {
    display = tmp_display_list;
    tmp_display_list = display->next;
    free (&display);
    }

  return 0;
  }
//}}}
//{{{
static int dvbsubParseRegionSegment (const uint8_t *buf, int buf_size) {

  DVBSubContext *ctx = &subContext;
  const uint8_t *buf_end = buf + buf_size;
  int region_id, object_id;
  int version;
  DVBSubRegion* region = NULL;
  DVBSubObject* object = NULL;
  DVBSubObjectDisplay* display = NULL;
  int fill;

  if (buf_size < 10)
    return -1;
  region_id = *buf++;
  region = getRegion (ctx, region_id);

  if (!region) {
    region = (DVBSubRegion*)malloc (sizeof (DVBSubRegion));
    region->id = region_id;
    region->version = -1;
    region->next = ctx->region_list;
    region->pbuf = NULL;
    region->display_list = NULL;
    ctx->region_list = region;
    }

  version = ((*buf)>>4) & 15;
  fill = ((*buf++) >> 3) & 1;
  region->width = ((*buf) << 8) + *(buf+1);
  buf += 2;
  region->height = ((*buf) << 8) + *(buf+1);
  buf += 2;

  if (region->width * region->height != region->buf_size) {
    free (region->pbuf);
    region->buf_size = region->width * region->height;
    region->pbuf = (uint8_t*)malloc(region->buf_size);
    if (!region->pbuf) {
      region->buf_size = region->width = region->height = 0;
      return -1;
      }

    fill = 1;
    region->dirty = 0;
    }

  region->depth = 1 << (((*buf++) >> 2) & 7);
  if (region->depth<2 || region->depth>8)
    region->depth = 4;
  region->clut = *buf++;

  if (region->depth == 8) {
    region->bgcolor = *buf++;
    buf += 1;
    }
  else {
    buf += 1;

    if (region->depth == 4)
      region->bgcolor = (((*buf++) >> 4) & 15);
    else
     region->bgcolor = (((*buf++) >> 2) & 3);
    }

  if (fill)
    memset(region->pbuf, region->bgcolor, region->buf_size);
  deleteRegionDisplayList (ctx, region);

  while (buf + 5 < buf_end) {
    object_id = ((*buf) << 8) + *(buf + 1);
    buf += 2;
    object = getObject(ctx, object_id);
    if (!object) {
      object = (DVBSubObject*)malloc (sizeof (DVBSubObject));
      object->id = object_id;
      object->next = ctx->object_list;
      object->display_list = NULL;
      ctx->object_list = object;
      }
    object->type = (*buf) >> 6;

    display = (DVBSubObjectDisplay*)malloc (sizeof (DVBSubObjectDisplay));
    display->object_id = object_id;
    display->region_id = region_id;
    display->x_pos = (((*buf) << 8) + *(buf + 1)) & 0xfff;
    buf += 2;
    display->y_pos = (((*buf) << 8) + *(buf + 1)) & 0xfff;
    buf += 2;
    display->region_list_next = NULL;
    display->object_list_next = NULL;

    if ((object->type == 1 || object->type == 2) && buf+1 < buf_end) {
      display->fgcolor = *buf++;
      display->bgcolor = *buf++;
      }

    display->region_list_next = region->display_list;
    region->display_list = display;

    display->object_list_next = object->display_list;
    object->display_list = display;
    }

  return 0;
  }
//}}}
//{{{
static int dvbsubParseClutSegment (const uint8_t *buf, int buf_size) {

  DVBSubContext *ctx = &subContext;

  const uint8_t* buf_end = buf + buf_size;
  int clut_id = *buf++;
  int version = ((*buf) >> 4) & 15;
  buf += 1;

  DVBSubCLUT* clut = getClut (ctx, clut_id);
  if (!clut) {
    clut = (DVBSubCLUT*)malloc (sizeof (DVBSubCLUT));
    memcpy (clut, &default_clut, sizeof (DVBSubCLUT));
    clut->id = clut_id;
    clut->version = -1;
    clut->next = ctx->clut_list;
    ctx->clut_list = clut;
    }

  if (clut->version != version) {
    clut->version = version;
    while (buf + 4 < buf_end) {
      int entry_id = *buf++;
      int depth = (*buf) & 0xe0;
      if (depth == 0) {
        return -1;
        }
      int full_range = (*buf++) & 1;

      int y, cr, cb, alpha;
      if (full_range) {
        y = *buf++;
        cr = *buf++;
        cb = *buf++;
        alpha = *buf++;
        }
      else {
        y = buf[0] & 0xfc;
        cr = (((buf[0] & 3) << 2) | ((buf[1] >> 6) & 3)) << 4;
        cb = (buf[1] << 2) & 0xf0;
        alpha = (buf[1] << 6) & 0xc0;
        buf += 2;
        }

      if (y == 0)
        alpha = 0xff;

      int r = 0;
      int g = 0;
      int b = 0;
      int r_add, g_add, b_add;
      //YUV_TO_RGB1_CCIR(cb, cr);
      //YUV_TO_RGB2_CCIR(r, g, b, y);
      if (!!(depth & 0x80) + !!(depth & 0x40) + !!(depth & 0x20) > 1) {
        //if (avctx->strict_std_compliance > FF_COMPLIANCE_NORMAL)
        //    return AVERROR_INVALIDDATA;
        }

      if (depth & 0x80)
        clut->clut4[entry_id] = RGBA(r,g,b,255 - alpha);
      else if (depth & 0x40)
        clut->clut16[entry_id] = RGBA(r,g,b,255 - alpha);
      else if (depth & 0x20)
        clut->clut256[entry_id] = RGBA(r,g,b,255 - alpha);
      }
    }
  return 0;
  }
//}}}
//{{{
static int dvbsubParseObjectSegment (const uint8_t *buf, int buf_size) {

  DVBSubContext* ctx = &subContext;

  const uint8_t* buf_end = buf + buf_size;

  int object_id = ((*buf) << 8) + *(buf + 1);
  buf += 2;

  DVBSubObject* object = getObject (ctx, object_id);
  if (!object)
    return -1;

  int coding_method = ((*buf) >> 2) & 3;
  int non_modifying_color = ((*buf++) >> 1) & 1;

  if (coding_method == 0) {
    int top_field_len = ((*buf) << 8) + *(buf + 1);
    buf += 2;
    int bottom_field_len = ((*buf) << 8) + *(buf + 1);
    buf += 2;

    if (buf + top_field_len + bottom_field_len > buf_end)
      return -1;

    for (DVBSubObjectDisplay* display = object->display_list; display; display = display->object_list_next) {
      const uint8_t *block = buf;
      int bfl = bottom_field_len;

      dvbsubParsePixelDataBlock (display, block, top_field_len, 0,non_modifying_color);

      if (bottom_field_len > 0)
        block = buf + top_field_len;
      else
        bfl = top_field_len;

      dvbsubParsePixelDataBlock(display, block, bfl, 1, non_modifying_color);
      }

    /*  } else if (coding_method == 1) {*/
    //} else {
    //    av_log(avctx, AV_LOG_ERROR, "Unknown object coding %d\n", coding_method);
    }

  return 0;
  }
//}}}
//{{{
static int dvbsubParseDisplayDefinitionSegment (const uint8_t *buf, int buf_size) {

  DVBSubContext* ctx = &subContext;
  DVBSubDisplayDefinition* display_def = ctx->display_definition;
  int dds_version, info_byte;

  if (buf_size < 5)
    return -1;

  info_byte = *buf++;

  dds_version = info_byte >> 4;
  if (display_def && display_def->version == dds_version)
    return 0; // already have this display definition version

  if (!display_def) {
    display_def = (DVBSubDisplayDefinition*)malloc(sizeof(*display_def));
    ctx->display_definition = display_def;
    }

  display_def->version = dds_version;
  display_def->x = 0;
  display_def->y = 0;
  display_def->width = ((*buf) << 8) + *(buf+1) + 1; buf +=2;
  display_def->height = ((*buf) << 8) + *(buf+1) + 1; buf +=2;
  //if (!avctx->width || !avctx->height) {
  //  avctx->width  = display_def->width;
  //  avctx->height = display_def->height;
  //}

  if (info_byte & 1<<3) { // display_window_flag
    if (buf_size < 13)
      -1;
    display_def->x = ((*buf) << 8) + *(buf+1);  buf +=2;
    display_def->width  = ((*buf) << 8) + *(buf+1) - display_def->x + 1; buf +=2;
    display_def->y = ((*buf) << 8) + *(buf+1);  buf +=2;
    display_def->height = ((*buf) << 8) + *(buf+1) - display_def->y + 1; buf +=2;
    }

  return 0;
  }
//}}}
//{{{
static int dvbsubDisplayEndSegment (const uint8_t *buf, int buf_size) {

  printf ("DVBSUB_DISPLAY_SEGMENT %d\n", buf_size);
  return 0;
  }
//}}}

//{{{
static void dvbsubInitDecoder() {


  DVBSubContext* ctx = &subContext;

  ctx->composition_id = -1;
  ctx->ancillary_id   = -1;

  ctx->version = -1;
  //ctx->prev_start = AV_NOPTS_VALUE;
  ctx->prev_start = -1;
  ctx->region_list = NULL;
  ctx->clut_list = NULL;
  ctx->object_list = NULL;

  ctx->display_list = NULL;
  ctx->display_definition = NULL;

  default_clut.id = -1;
  default_clut.next = NULL;

  int i, r, g, b, a = 0;
  //{{{  clut4
  default_clut.clut4[0] = RGBA(  0,   0,   0,   0);
  default_clut.clut4[1] = RGBA(255, 255, 255, 255);
  default_clut.clut4[2] = RGBA(  0,   0,   0, 255);
  default_clut.clut4[3] = RGBA(127, 127, 127, 255);
  //}}}
  //{{{  clut16
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
  //}}}
  //{{{  clut256
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
  //}}}
  }
//}}}
//{{{
int dvbsubDecode (uint8_t* buf, int buf_size) {

  const uint8_t* p = buf;
  const uint8_t* p_end = buf + buf_size;

  if (first) {
    dvbsubInitDecoder();
    first = false;
    }

  printf ("dvbsub_decode %d\n", buf_size);
  #ifdef buf_debug1
  for (int j = 0; j < buf_size; j++) {
    printf ("%02x ", p[j]);
    if ((j % 32) == 31)
      printf ("\n");
   }
  printf ("\n");
  #endif

  // skip subtitle id and stream id
  p += 2;

  int ret = 0;
  int got_dds = 0;
  int gotSegment = 0;
  while (p_end - p >= 6 && *p == 0x0f) {
    p += 1;
    int segment_type = *p++;
    int page_id = ((*p) << 8) + *(p+1);
    p += 2;
    int segment_length = ((*p) << 8) + *(p+1);
    p += 2;

    if (p_end - p < segment_length) {
      ret = -1;
      goto end;
      }

    switch (segment_type) {
      case DVBSUB_PAGE_SEGMENT:
        printf ("DVBSUB_PAGE_SEGMENT %d\n", segment_length);
        ret = dvbsubParsePageSegment (p, segment_length);
        gotSegment |= 1;
        break;
      case DVBSUB_REGION_SEGMENT:
        printf ("DVBSUB_REGION_SEGMENT %d\n", segment_length);
        ret = dvbsubParseRegionSegment(p, segment_length);
        gotSegment |= 2;
        break;
      case DVBSUB_CLUT_SEGMENT:
        printf ("DVBSUB_CLUT_SEGMENT %d\n", segment_length);
        ret = dvbsubParseClutSegment(p, segment_length);
        if (ret < 0) goto end;
        gotSegment |= 4;
        break;
      case DVBSUB_OBJECT_SEGMENT:
        printf ("DVBSUB_OBJECT_SEGMENT %d\n", segment_length);
        ret = dvbsubParseObjectSegment( p, segment_length);
        gotSegment |= 8;
        break;
      case DVBSUB_DISPLAYDEFINITION_SEGMENT:
        printf ("DVBSUB_DISPLAYDEFINITION_SEGMENT %d\n", segment_length);
        ret = dvbsubParseDisplayDefinitionSegment(p, segment_length);
        got_dds = 1;
        break;
      case DVBSUB_DISPLAY_SEGMENT:
        ret = dvbsubDisplayEndSegment (p, segment_length);
        if (gotSegment == 15 && !got_dds) {
          // Default from ETSI EN 300 743 V1.3.1 (7.2.1)
          //avctx->width  = 720;
          //avctx->height = 576;
          }
        gotSegment |= 16;
        break;

      default:
        break;
      }

    if (ret < 0) {
      printf ("dvbsub_decode error\n");
      goto end;
      }

    p += segment_length;
    }

  // Some streams do not send a display segment
  if (gotSegment == 15)
    dvbsubDisplayEndSegment (p, 0);

end:
  return p - buf;
  }
//}}}
