// cDumpTransportStream.h
//{{{  includes
#pragma once
#include "cTransportStream.h"
//}}}
const bool kDebug = false;
//{{{
struct sRect {
  int x;
  int y;
  int w;
  int h;
  int numColours;

  uint8_t* data[2];
  int stride;
  };
//}}}
//{{{
struct sSubtitle {
  unsigned numRects;
  sRect** rects;

  uint32_t startDisplayTime; // relative to packet pts, in ms
  uint32_t endDisplayTime;   // relative to packet pts, in ms
  int64_t pts;                 // Same as packet pts, in AV_TIME_BASE
  };
//}}}
//{{{
class cSubtitleDecoder {
  //{{{  defines
  #define AVRB16(p) ((*(p) << 8) | *(p+1))

  #define AV_NOPTS_VALUE 0x8000000000000000

  #define SCALEBITS 10
  #define ONE_HALF  (1 << (SCALEBITS - 1))
  #define FIX(x)    ((int) ((x) * (1<<SCALEBITS) + 0.5))
  //{{{
  #define YUV_TO_RGB1_CCIR(cb1, cr1) {\
    cb = (cb1) - 128;\
    cr = (cr1) - 128;\
    r_add = FIX(1.40200*255.0/224.0) * cr + ONE_HALF;\
    g_add = - FIX(0.34414*255.0/224.0) * cb - FIX(0.71414*255.0/224.0) * cr + ONE_HALF;\
    b_add = FIX(1.77200*255.0/224.0) * cb + ONE_HALF;\
    }
  //}}}
  //{{{
  #define YUV_TO_RGB2_CCIR(r, g, b, y1) {\
    y = ((y1) - 16) * FIX(255.0/219.0);\
    r = (y + r_add) >> SCALEBITS;\
    g = (y + g_add) >> SCALEBITS;\
    b = (y + b_add) >> SCALEBITS;\
    }
  //}}}

  #define RGBA(r,g,b,a) (((unsigned)(a) << 24) | ((r) << 16) | ((g) << 8) | (b))
  //}}}
public:
  cSubtitleDecoder() {}
  //{{{
  bool decode (const uint8_t* buf, int bufSize, sSubtitle* sub) {

    const uint8_t* bufEnd = buf + bufSize;

    // skip past first 2 bytes
    const uint8_t* bufPtr = buf + 2;
    if (mDebug) {
      //{{{  debug log
      const uint8_t* debugPtr = buf;

      for (int j = 0; (j < 4) && (debugPtr < bufEnd); j++) {
        std::string str = "sub " + hex (j*32,2) + " ";
        for (int i = 0; (i < 32) && (debugPtr < bufEnd); i++) {
          int value = *debugPtr++;
          str += hex(value,2) + " ";
          }

        cLog::log (LOGINFO, str);
        }
      }
      //}}}

    int gotDds = 0;
    int gotSegment = 0;
    while (((bufEnd - bufPtr) >= 6) && (*bufPtr == 0x0f)) {
      bufPtr += 1;
      int segment_type = *bufPtr++;

      int pageId = AVRB16(bufPtr); bufPtr += 2;
      int segmentLength = AVRB16(bufPtr); bufPtr += 2;

      if (bufEnd - bufPtr < segmentLength) {
        cLog::log (LOGERROR, "incomplete or broken packet");
        hasSubtitle = false;
        return false;
        }

      if ((pageId == compositionId) ||
          (pageId == ancillaryId) ||
          (compositionId == -1) ||
          (ancillaryId == -1)) {
        int ret = 0;
        switch (segment_type) {
          //{{{
          case 0x10: // page
            ret = parsePage (bufPtr, segmentLength, sub);
            gotSegment |= 0x01;

            break;
          //}}}
          //{{{
          case 0x11: // region
            ret = parseRegion (bufPtr, segmentLength);
            gotSegment |= 0x02;

            break;
          //}}}
          //{{{
          case 0x12: // clut
            ret = parseClut (bufPtr, segmentLength);
            gotSegment |= 0x04;

            break;
          //}}}
          //{{{
          case 0x13: // object
            ret = parseObject (bufPtr, segmentLength);
            gotSegment |= 0x08;

            break;
          //}}}
          //{{{
          case 0x14: // display definition
            ret = parseDisplayDefinition (bufPtr, segmentLength);
            gotDds = 1;

            break;
          //}}}
          //{{{
          case 0x80: // display
            ret = displayEnd (sub);
            gotSegment |= 0x10;

            break;
          //}}}
          default:
            cLog::log (LOGINFO, "segment:%x, pageId:%d, length:%d", segment_type, pageId, segmentLength);
            break;
            }
        if (ret < 0) {
          hasSubtitle = false;
          return ret;
          }
        }
      bufPtr += segmentLength;
      }

    // Some streams do not send a display segment
    if (gotSegment == 0x0F) {
      cLog::log (LOGINFO, "Missing display_endSegment, emulating");
      displayEnd (sub);
      }
    if (compute_edt == 1) {
      // FFSWAP (int64_t, ctx->prevStart, sub->pts);
      }

    return hasSubtitle;
    }
  //}}}

private:
  //{{{
  struct sClut {
    int id = 0;
    int version = 0;

    uint32_t clut4[4] = { 0 };
    uint32_t clut16[16] = { 0 };
    uint32_t clut256[256] = { 0 };

    sClut* next;
    };
  //}}}
  //{{{
  struct sObjectDisplay {
    int objectId;
    int regionId;

    int xPos;
    int yPos;

    int fgcolor;
    int bgcolor;

    sObjectDisplay* regionListNext;
    sObjectDisplay* objectListNext;
    };
  //}}}
  //{{{
  struct sObject {
    int id;
    int version;
    int type;

    sObjectDisplay* displayList;

    sObject* next;
    };
  //}}}
  //{{{
  struct sRegionDisplay {
    int regionId;
    int xPos;
    int yPos;

    sRegionDisplay* next;
    };
  //}}}
  //{{{
  struct sRegion {
    int id;
    int version;

    int width;
    int height;
    int depth;

    int clut;
    int bgcolor;

    uint8_t computedClut[4*256];
    int hasComputedClut;

    uint8_t* pixelBuf;
    int pixelBufSize;
    int dirty;

    sObjectDisplay* displayList;

    sRegion* next;
    };
  //}}}
  //{{{
  struct sDisplayDefinition {
    int version;

    int x;
    int y;
    int width;
    int height;
    };
  //}}}

  //{{{
  class cBitStream {
  public:
    cBitStream (const uint8_t* bitStream, int size) : mBitStream(bitStream), mBitPointer(0), mSize(size) {}

    //{{{
    uint32_t getBit() {

      uint32_t result = (uint32_t)((mBitStream [mBitPointer >> 3] >> (7-(mBitPointer & 0x7))) & 0x01);
      mBitPointer += 1;
      return result;
      }
    //}}}
    //{{{
    uint32_t getBits (int numBits) {

      uint32_t result = 0;

      mLastByte = mBitStream [mBitPointer >> 3];

      for (int i = 0; i < numBits; i++)
        result = (result << 1) | getBit();

      return result;
      }
    //}}}

    //{{{
    int getBitsUsed() {
      return mBitPointer;
      }
    //}}}
    //{{{
    int getBytesUsed() {
      return (mBitPointer + 7) / 8;
      }
    //}}}

    //{{{
    void debug() {
      uint64_t address = (uint64_t)mBitStream;
      cLog::log (LOGINFO, "bitstream " + hex(address,16) + " " + dec(mSize,4));

      int j = 0;
      std::string str;
      auto bufPtr = mBitStream;
      auto bufEnd = mBitStream + mSize;
      while (bufPtr != bufEnd) {
        int value = *bufPtr++;
        str += hex (value,2) + " ";
        if (!(++j % 32)) {
          cLog::log (LOGINFO, str);
          str.clear();
          }
        }
      cLog::log (LOGINFO, str);
      }
    //}}}

  private:
    const uint8_t* mBitStream;
    int mBitPointer;
    int mSize;
    uint8_t mLastByte;
    };
  //}}}

  //{{{
  void initDefaultClut (uint8_t* clut, sRect* rect, int w, int h) {

    uint8_t list[256] = { 0 };
    uint8_t listInv[256];
    int counttab[256] = { 0 };
    int (*counttab2)[256] = clutCount2;

    int count, i, x, y;
    ptrdiff_t stride = rect->stride;

    memset (clutCount2, 0, sizeof(clutCount2));

    #define V(x,y) rect->data[0][(x) + (y)*stride]

    for (y = 0; y < h; y++) {
      for (x = 0; x < w; x++) {
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

    for (i = 0; i < 256; i++)
      counttab2[i+1][i] = 0;

    for (i = 0; i < 256; i++) {
      int bestscore = 0;
      int bestv = 0;

      for (x = 0; x < 256; x++) {
        int scorev = 0;
        if (list[x])
          continue;
        scorev += counttab2[0][x];
        for (y = 0; y < 256; y++)
          scorev += list[y] * counttab2[y+1][x];

        if (scorev) {
          int score = 1024LL * scorev / counttab[x];
          if (score > bestscore) {
            bestscore = score;
            bestv = x;
            }
          }
        }

      if (!bestscore)
        break;
      list[bestv] = 1;
      listInv[i] = bestv;
      }

    count = std::max(i - 1, 1);
    for (i--; i >= 0; i--) {
      int v = i * 255 / count;
      //AV_WN32 (clut + 4*listInv[i], RGBA(v/2,v,v/2,v));
      *(clut + 4*listInv[i]) = RGBA(v/2,v,v/2,v);
      }
    }
  //}}}
  //{{{
  int initDecoder() {

    //if (substream < 0) {
    compositionId = -1;
    ancillaryId = -1;
    version = -1;
    prevStart = AV_NOPTS_VALUE;

    defaultClut.id = -1;
    defaultClut.next = NULL;
    defaultClut.clut4[0] = RGBA (  0,   0,   0,   0);
    defaultClut.clut4[1] = RGBA (255, 255, 255, 255);
    defaultClut.clut4[2] = RGBA (  0,   0,   0, 255);
    defaultClut.clut4[3] = RGBA (127, 127, 127, 255);
    defaultClut.clut16[0] = RGBA (0, 0, 0, 0);

    int r = 0;
    int g = 0;
    int b = 0;
    int a = 0;
    for (int i = 1; i < 16; i++) {
      if (i < 8) {
        //{{{  set rgb
        r = (i & 1) ? 255 : 0;
        g = (i & 2) ? 255 : 0;
        b = (i & 4) ? 255 : 0;
        }
        //}}}
      else {
        //{{{  set rgb
        r = (i & 1) ? 127 : 0;
        g = (i & 2) ? 127 : 0;
        b = (i & 4) ? 127 : 0;
        }
        //}}}
      defaultClut.clut16[i] = RGBA (r, g, b, 255);
      }

    defaultClut.clut256[0] = RGBA (0, 0, 0, 0);
    for (int i = 1; i < 256; i++) {
      if (i < 8) {
        //{{{  set rgb
        r = (i & 1) ? 255 : 0;
        g = (i & 2) ? 255 : 0;
        b = (i & 4) ? 255 : 0;
        a = 63;
        }
        //}}}
      else {
        switch (i & 0x88) {
          case 0x00:
            //{{{  set rgba
            r = ((i & 1) ? 85 : 0) + ((i & 0x10) ? 170 : 0);
            g = ((i & 2) ? 85 : 0) + ((i & 0x20) ? 170 : 0);
            b = ((i & 4) ? 85 : 0) + ((i & 0x40) ? 170 : 0);
            a = 255;
            break;
            //}}}
          case 0x08:
            //{{{  set rgba
            r = ((i & 1) ? 85 : 0) + ((i & 0x10) ? 170 : 0);
            g = ((i & 2) ? 85 : 0) + ((i & 0x20) ? 170 : 0);
            b = ((i & 4) ? 85 : 0) + ((i & 0x40) ? 170 : 0);
            a = 127;
            break;
            //}}}
          case 0x80:
            //{{{  set rgba
            r = 127 + ((i & 1) ? 43 : 0) + ((i & 0x10) ? 85 : 0);
            g = 127 + ((i & 2) ? 43 : 0) + ((i & 0x20) ? 85 : 0);
            b = 127 + ((i & 4) ? 43 : 0) + ((i & 0x40) ? 85 : 0);
            a = 255;
            break;
            //}}}
          case 0x88:
            //{{{  set rgba
            r = ((i & 1) ? 43 : 0) + ((i & 0x10) ? 85 : 0);
            g = ((i & 2) ? 43 : 0) + ((i & 0x20) ? 85 : 0);
            b = ((i & 4) ? 43 : 0) + ((i & 0x40) ? 85 : 0);
            a = 255;
            break;
            //}}}
          }
        }
      defaultClut.clut256[i] = RGBA(r, g, b, a);
      }

    return 0;
    }
  //}}}

  //{{{
  void deleteRegionDisplayList (sRegion* region) {

    while (region->displayList) {
      sObjectDisplay* display = region->displayList;

      sObject* object = getObject (display->objectId);
      if (object) {
        sObjectDisplay** objDispPtr = &object->displayList;
        sObjectDisplay* objDisp = *objDispPtr;

        while (objDisp && objDisp != display) {
          objDispPtr = &objDisp->objectListNext;
          objDisp = *objDispPtr;
          }

        if (objDisp) {
          *objDispPtr = objDisp->objectListNext;

          if (!object->displayList) {
            sObject** obj2Ptr = &objectList;
            sObject* obj2 = *obj2Ptr;

            while (obj2 != object) {
              //av_assert0(obj2);
              obj2Ptr = &obj2->next;
              obj2 = *obj2Ptr;
              }

            *obj2Ptr = obj2->next;
            free (&obj2);
            obj2 = NULL;
            }
          }
        }

      region->displayList = display->regionListNext;

      free (&display);
      }

    }
  //}}}
  //{{{
  void deleteCluts() {

    while (clutList) {
      sClut* clut = clutList;
      clutList = clut->next;
      free (&clut);
      }
    }
  //}}}
  //{{{
  void deleteObjects() {

    while (objectList) {
      sObject* object = objectList;
      objectList = object->next;
      free (&object);
      }
    }
  //}}}
  //{{{
  void deleteRegions() {

    while (regionList) {
      sRegion* region = regionList;

      regionList = region->next;
      deleteRegionDisplayList (region);

      free (&region->pixelBuf);
      free (&region);
      }
    }
  //}}}
  //{{{
  int closeDecoder() {

    sRegionDisplay* display;

    deleteRegions();
    deleteObjects();
    deleteCluts();

    free (&displayDefinition);
    displayDefinition = NULL;

    while (displayList) {
      display = displayList;
      displayList = display->next;
      free (&display);
      }

    return 0;
    }
  //}}}

  //{{{
  sObject* getObject (int objectId) {

    sObject* ptr = objectList;

    while (ptr && ptr->id != objectId)
      ptr = ptr->next;

    return ptr;
    }
  //}}}
  //{{{
  sClut* getClut (int clutId) {

    sClut* ptr = clutList;

    while (ptr && ptr->id != clutId)
      ptr = ptr->next;

    return ptr;
    }
  //}}}
  //{{{
  sRegion* getRegion (int regionId) {

    sRegion* ptr = regionList;

    while (ptr && ptr->id != regionId)
      ptr = ptr->next;

    return ptr;
    }
  //}}}

  //{{{
  int read2bitString (uint8_t* dstBuf, int dstBufSize,
                             const uint8_t** srcBuf, int srcBufSize,
                             int nonModifyingColor, uint8_t* mapTable, int xPos) {

    int dstPixels = xPos;
    dstBuf += xPos;

    cBitStream bitStream (*srcBuf, srcBufSize);
    while ((bitStream.getBitsUsed() < (srcBufSize * 8)) && (dstPixels < dstBufSize)) {
      int bits = bitStream.getBits (2);
      if (bits) {
        if (nonModifyingColor != 1 || bits != 1) {
          if (mapTable)
            *dstBuf++ = mapTable[bits];
          else
            *dstBuf++ = bits;
          }
        dstPixels++;
        }
      else {
        bits = bitStream.getBit();
        if (bits == 1) {
          //{{{  bits == 1
          int runLength = bitStream.getBits (3) + 3;
          bits = bitStream.getBits (2);

          if (nonModifyingColor == 1 && bits == 1)
            dstPixels += runLength;
          else {
            if (mapTable)
              bits = mapTable[bits];
            while ((runLength-- > 0) && (dstPixels < dstBufSize)) {
              *dstBuf++ = bits;
              dstPixels++;
              }
            }
          }
          //}}}
        else {
          bits = bitStream.getBit();
          if (bits == 0) {
            bits = bitStream.getBits (2);
            if (bits == 0) {
              //{{{  bits == 0
              (*srcBuf) += bitStream.getBytesUsed();
              return dstPixels;
              }
              //}}}
            else if (bits == 1) {
              //{{{  bits == 1
              if (mapTable)
                bits = mapTable[0];
              else
                bits = 0;
              int runLength = 2;
              while ((runLength-- > 0) && (dstPixels < dstBufSize)) {
                *dstBuf++ = bits;
                dstPixels++;
                }
              }
              //}}}
            else if (bits == 2) {
              //{{{  bits == 2
              int runLength = bitStream.getBits (4) + 12;
              bits = bitStream.getBits (2);

              if ((nonModifyingColor == 1) && (bits == 1))
                dstPixels += runLength;
              else {
                if (mapTable)
                  bits = mapTable[bits];
                while ((runLength-- > 0) && (dstPixels < dstBufSize)) {
                  *dstBuf++ = bits;
                  dstPixels++;
                  }
                }
              }
              //}}}
            else { // if (bits == 3)
              //{{{  bits == 3
              int runLength = bitStream.getBits (8) + 29;
              bits = bitStream.getBits (2);

              if (nonModifyingColor == 1 && bits == 1)
                 dstPixels += runLength;
              else {
                if (mapTable)
                  bits = mapTable[bits];
                while ((runLength-- > 0) && (dstPixels < dstBufSize)) {
                  *dstBuf++ = bits;
                  dstPixels++;
                  }
                }
              }
              //}}}
            }

          else {
            if (mapTable)
              bits = mapTable[0];
            else
              bits = 0;
            *dstBuf++ = bits;
            dstPixels++;
            }
          }
        }
      }

    if (bitStream.getBits (6))
      cLog::log (LOGERROR, "line overflow");

    (*srcBuf) += bitStream.getBytesUsed();
    return dstPixels;
    }
  //}}}
  //{{{
  int read4bitString (uint8_t* dstBuf, int dstBufSize,
                                   const uint8_t** srcBuf, int srcBufSize,
                                   int nonModifyingColor, uint8_t* mapTable, int xPos) {

    std::string str;

    dstBuf += xPos;
    int dstPixels = xPos;

    cBitStream bitStream (*srcBuf, srcBufSize);

    while ((bitStream.getBitsUsed() < (srcBufSize * 8)) && (dstPixels < dstBufSize)) {
      int bits = bitStream.getBits (4);
      str += "[4b:" + hex(bits,1);
      if (bits) {
        if (nonModifyingColor != 1 || bits != 1)
          *dstBuf++ = mapTable ? mapTable[bits] : bits;
        dstPixels++;
        }
      else {
        bits = bitStream.getBit();
        str += ",1b:" + hex(bits,1);
        if (bits == 0) {
          //{{{  bits == 0
          int runLength = bitStream.getBits (3);
          str += ",3b:" + hex(runLength);
          if (runLength == 0) {
            if (mDebug)
              cLog::log (LOGINFO, str + "]");
            (*srcBuf) += bitStream.getBytesUsed();
            return dstPixels;
            }

          runLength += 2;
          bits = mapTable ? mapTable[0] : 0;
          while ((runLength-- > 0) && (dstPixels < dstBufSize)) {
            *dstBuf++ = bits;
            dstPixels++;
            }
          }
          //}}}
        else {
          bits = bitStream.getBit();
          str += ",1b:" + hex(bits,1);
          if (bits == 0) {
            //{{{  bits = 0
            int runBits = bitStream.getBits (2);
            str += ",2b:" + hex(runBits,1);
            int runLength = runBits + 4;

            bits = bitStream.getBits (4);
            str += ",4b:" + hex(bits,1);

            if (nonModifyingColor == 1 && bits == 1)
              dstPixels += runLength;
            else {
              if (mapTable)
                bits = mapTable[bits];

              while ((runLength-- > 0) && (dstPixels < dstBufSize)) {
                *dstBuf++ = bits;
                dstPixels++;
                }
              }
            }
            //}}}
          else {
            bits = bitStream.getBits (2);
            str += ",2b:" + hex(bits,1);
            if (bits == 0) {
              bits = mapTable ? mapTable[0] : 0;
              *dstBuf++ = bits;
              dstPixels ++;
              }
            else if (bits == 1) {
              bits = mapTable ? mapTable[0] : 0;
              int runLength = 2;
              str += ":run:" + dec(runLength);
              while ((runLength-- > 0) && (dstPixels < dstBufSize)) {
                *dstBuf++ = bits;
                dstPixels++;
                }
              }
            else if (bits == 2) {
              int runBits = bitStream.getBits (4);
              int runLength = runBits + 9;
              str += ",4b:" + hex(runBits,1) + ":run:" + dec(runLength);
              bits = bitStream.getBits (4);
              str += ",4b:" + hex(bits,1);

              if (nonModifyingColor == 1 && bits == 1)
                dstPixels += runLength;
              else {
                if (mapTable)
                  bits = mapTable[bits];
                while ((runLength-- > 0) && (dstPixels < dstBufSize)) {
                  *dstBuf++ = bits;
                  dstPixels++;
                  }
                }
              }
            else if (bits == 3) {
              int runBits = bitStream.getBits (8);
              int runLength = runBits + 25;
              str += ",8b:" + hex(runBits,2) + ":run:" + dec(runLength);
              bits = bitStream.getBits (4);
              str += ",4b:" + hex(bits);

              if (nonModifyingColor == 1 && bits == 1)
                dstPixels += runLength;
              else {
                if (mapTable)
                  bits = mapTable[bits];
                while ((runLength-- > 0) && (dstPixels < dstBufSize)) {
                  *dstBuf++ = bits;
                  dstPixels++;
                  }
                }
              }
            }
          }
        }
      str += "] ";
      }

    int bits = bitStream.getBits (8);
    if (bits)
      cLog::log (LOGERROR, "line overflow");

    str += " [4b:" + hex(bits) + "] ok";;
    if (mDebug)
      cLog::log (LOGINFO, str);

    (*srcBuf) += bitStream.getBytesUsed();
    return dstPixels;
    }
  //}}}
  //{{{
  int read8bitString (uint8_t* dstBuf, int dstBufSize,
                                   const uint8_t** srcBuf, int srcBufSize,
                                   int nonModifyingColor, uint8_t* mapTable, int xPos) {

    const uint8_t* srcBufEnd = (*srcBuf) + srcBufSize;
    int dstPixels = xPos;
    dstBuf += xPos;

    while ((*srcBuf < srcBufEnd) && (dstPixels < dstBufSize)) {
      int bits = *(*srcBuf)++;
      if (bits) {
        if (nonModifyingColor != 1 || bits != 1) {
          if (mapTable)
            *dstBuf++ = mapTable[bits];
          else
            *dstBuf++ = bits;
          }
        dstPixels++;
        }
      else {
        bits = *(*srcBuf)++;
        int runLength = bits & 0x7f;
        if ((bits & 0x80) == 0) {
          if (runLength == 0)
            return dstPixels;
          bits = 0;
          }
        else
          bits = *(*srcBuf)++;

        if ((nonModifyingColor == 1) && (bits == 1))
          dstPixels += runLength;
        else {
          if (mapTable)
            bits = mapTable[bits];
          while (runLength-- > 0 && dstPixels < dstBufSize) {
            *dstBuf++ = bits;
            dstPixels++;
            }
          }
        }
      }

    if (*(*srcBuf)++)
      cLog::log (LOGERROR, "line overflow");

    return dstPixels;
    }
  //}}}
  //{{{
  int saveSet (sSubtitle* sub) {

    int offsetX = 0;
    int offsetY = 0;

    sDisplayDefinition* displayDef = displayDefinition;
    if (displayDef) {
      offsetX = displayDef->x;
      offsetY = displayDef->y;
      }

    for (sRegionDisplay* display = displayList; display; display = display->next) {
      sRegion* region = getRegion (display->regionId);
      if (region && region->dirty)
        sub->numRects++;
      }

    if (compute_edt == 0) {
      sub->endDisplayTime = time_out * 1000;
      hasSubtitle = true;
      }
    else if (prevStart != AV_NOPTS_VALUE) {
      //sub->endDisplay_time = avRescale_q ((sub->pts - prevStart ), AV_TIME_BASE_Q, (AVRational){ 1, 1000 }) - 1;
      hasSubtitle = true;
      }

    if (sub->numRects > 0) {
      sub->rects = (sRect**)malloc (sub->numRects * sizeof(*sub->rects));
      for (unsigned int i = 0; i < sub->numRects; i++)
        sub->rects[i] = (sRect*)calloc (1, sizeof(*sub->rects[i]));

      int i = 0;
      for (sRegionDisplay* display = displayList; display; display = display->next) {
        sRegion* region = getRegion (display->regionId);
        if (!region)
          continue;
        if (!region->dirty)
          continue;

        sub->rects[i]->x = display->xPos + offsetX;
        sub->rects[i]->y = display->yPos + offsetY;
        sub->rects[i]->w = region->width;
        sub->rects[i]->h = region->height;
        sub->rects[i]->numColours = 1 << region->depth;
        sub->rects[i]->stride = region->width;

        sClut* clut = getClut (region->clut);
        if (!clut)
          clut = &defaultClut;
        uint32_t* clutTable;
        switch (region->depth) {
          case 2:  clutTable = clut->clut4; break;
          case 8:  clutTable = clut->clut256; break;
          case 4:
          default: clutTable = clut->clut16; break;
          }

        sub->rects[i]->data[1] = (uint8_t*)malloc (1024);
        memcpy (sub->rects[i]->data[1], clutTable, (1 << region->depth) * sizeof(*clutTable));

        sub->rects[i]->data[0] = (uint8_t*)malloc (region->pixelBufSize);
        memcpy (sub->rects[i]->data[0], region->pixelBuf, region->pixelBufSize);

        if (((clut == &defaultClut) && (computeClut == -1)) || (computeClut == 1)) {
          if (!region->hasComputedClut) {
            initDefaultClut (region->computedClut, sub->rects[i], sub->rects[i]->w, sub->rects[i]->h);
            region->hasComputedClut = 1;
            }
          memcpy (sub->rects[i]->data[1], region->computedClut, sizeof(region->computedClut));
          }
        i++;
        }
      }

    return 0;
    }
  //}}}
  //{{{
  void parsePixelDataBlock (sObjectDisplay* display, const uint8_t* buf, int bufSize, int topBottom, int nonModifyingColor) {

    const uint8_t* bufEnd = buf + bufSize;
    if (mDebug) {
      //{{{  debug buf
      uint64_t address = (uint64_t)buf;
      cLog::log (LOGINFO, "parsePixelDataBlock add:" + hex(address,16) + " size:" + dec(bufSize,4));

      int j = 0;
      std::string str;
      auto bufPtr = buf;
      while (bufPtr != bufEnd) {
        int value = *bufPtr++;
        str += hex (value,2) + " ";
        if (!(++j % 32)) {
          cLog::log (LOGINFO, str);
          str.clear();
          }
        }

      cLog::log (LOGINFO, str);
      }
      //}}}

    sRegion* region = getRegion (display->regionId);
    if (!region)
      return;

    uint8_t map2to4[] = {  0x0,  0x7,  0x8,  0xf};
    uint8_t map2to8[] = { 0x00, 0x77, 0x88, 0xff};
    uint8_t map4to8[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                          0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    uint8_t* mapTable;

    uint8_t* pixelBuf = region->pixelBuf;
    region->dirty = 1;

    int xPos = display->xPos;
    int yPos = display->yPos + topBottom;

    while (buf < bufEnd) {
      if (((*buf != 0xF0) && (xPos >= region->width)) || (yPos >= region->height)) {
        cLog::log (LOGERROR, "Invalid object location %d-%d %d-%d %02x",
                              xPos, region->width, yPos, region->height, *buf);
        return;
        }

      switch (*buf++) {
        //{{{
        case 0x10:
          if (region->depth == 8)
            mapTable = map2to8;
          else if (region->depth == 4)
            mapTable = map2to4;
          else
            mapTable = NULL;

          xPos = read2bitString (pixelBuf + (yPos * region->width), region->width,
                                 &buf, int(bufEnd - buf), nonModifyingColor, mapTable, xPos);
          break;
        //}}}
        //{{{
        case 0x11:
          if (region->depth < 4) {
            cLog::log (LOGERROR, "4-bit pixel string in %d-bit region!", region->depth);
            return;
            }

          if (region->depth == 8)
            mapTable = map4to8;
          else
            mapTable = NULL;

          xPos = read4bitString (pixelBuf + (yPos * region->width), region->width,
                                 &buf, int(bufEnd - buf), nonModifyingColor, mapTable, xPos);
          break;
        //}}}
        //{{{

        case 0x12:
          if (region->depth < 8) {
            cLog::log (LOGERROR, "8-bit pixel string in %d-bit region!", region->depth);
            return;
            }

          xPos = read8bitString (pixelBuf + (yPos * region->width), region->width,
                                 &buf, int(bufEnd - buf), nonModifyingColor, NULL, xPos);
            break;
        //}}}
        //{{{
        case 0x20:
          map2to4[0] = (*buf) >> 4;
          map2to4[1] = (*buf++) & 0xf;
          map2to4[2] = (*buf) >> 4;
          map2to4[3] = (*buf++) & 0xf;

          break;
        //}}}
        //{{{
        case 0x21:
          for (int i = 0; i < 4; i++)
            map2to8[i] = *buf++;

          break;
        //}}}
        //{{{
        case 0x22:
          for (int i = 0; i < 16; i++)
            map4to8[i] = *buf++;

          break;
        //}}}
        //{{{
        case 0xF0:
          xPos = display->xPos;
          yPos += 2;

          break;
        //}}}
        default:
          cLog::log (LOGINFO, "Unknown/unsupported pixel block 0x%x\n", *(buf-1));
          break;
        }
      }

    region->hasComputedClut = 0;
    }
  //}}}

  //{{{
  int parsePage (const uint8_t* buf, int bufSize, sSubtitle* sub) {

    cLog::log (LOGINFO, "page");

    if (bufSize < 1)
      return -1;
    const uint8_t* bufEnd = buf + bufSize;

    int timeout = *buf++;
    int pageVersion = ((*buf) >> 4) & 15;
    int pageState = ((*buf++) >> 2) & 3;

    if (version == pageVersion)
      return 0;

    time_out = timeout;
    version = pageVersion;

    cLog::log (LOGINFO, "- time out %ds, state %d", time_out, pageState);

    if (compute_edt == 1)
      saveSet (sub);

    if ((pageState == 1) || (pageState == 2)) {
      deleteRegions();
      deleteObjects();
      deleteCluts();
      }

    sRegionDisplay* tmpDisplayList = displayList;
    displayList = NULL;
    while (buf + 5 < bufEnd) {
      int regionId = *buf++;
      buf += 1;

      sRegionDisplay* display = displayList;
      while (display && (display->regionId != regionId))
        display = display->next;

      if (display) {
        cLog::log (LOGERROR, "duplicate region");
        break;
        }

      display = tmpDisplayList;
      sRegionDisplay** tmpPtr = &tmpDisplayList;
      while (display && (display->regionId != regionId)) {
        tmpPtr = &display->next;
        display = display->next;
        }

      if (!display)
        display = (sRegionDisplay*)calloc (1, sizeof(*display));
      display->regionId = regionId;
      display->xPos = AVRB16(buf); buf += 2;
      display->yPos = AVRB16(buf); buf += 2;
      *tmpPtr = display->next;
      display->next = displayList;
      displayList = display;

      cLog::log (LOGINFO, "Region %d, %d,%d", regionId, display->xPos, display->yPos);
      }

    while (tmpDisplayList) {
      sRegionDisplay* display = tmpDisplayList;
      tmpDisplayList = display->next;
      free (&display);
      }

    return 0;
    }
  //}}}
  //{{{
  int parseRegion (const uint8_t* buf, int bufSize) {

    cLog::log (LOGINFO, "region");

    if (bufSize < 10)
      return -1;
    const uint8_t* bufEnd = buf + bufSize;

    int regionId = *buf++;
    sRegion* region = getRegion (regionId);
    if (!region) {
      region = (sRegion*)calloc (1, sizeof(*region));
      region->id = regionId;
      region->version = -1;
      region->next = regionList;
      regionList = region;
      }

    int version = ((*buf)>>4) & 15;
    int fill = ((*buf++) >> 3) & 1;

    region->width = AVRB16(buf); buf += 2;
    region->height = AVRB16(buf); buf += 2;

    if ((region->width * region->height) != region->pixelBufSize) {
      free (region->pixelBuf);
      region->pixelBufSize = region->width * region->height;
      region->pixelBuf = (uint8_t*)malloc (region->pixelBufSize);
      fill = 1;
      region->dirty = 0;
      }

    region->depth = 1 << (((*buf++) >> 2) & 7);
    if (region->depth < 2 || region->depth > 8) {
      cLog::log (LOGERROR, "region depth %d is invalid", region->depth);
      region->depth= 4;
      }
    region->clut = *buf++;

    if (region->depth == 8) {
      region->bgcolor = *buf++;
      buf += 1;
      }
    else {
      buf += 1;
      if (region->depth == 4)
        region->bgcolor = ((*buf++) >> 4) & 15;
      else
        region->bgcolor = ((*buf++) >> 2) & 3;
      }

    if (fill)
      memset (region->pixelBuf, region->bgcolor, region->pixelBufSize);

    deleteRegionDisplayList (region);

    while (buf + 5 < bufEnd) {
      int objectId = AVRB16(buf); buf += 2;

      sObject* object = getObject (objectId);
      if (!object) {
        object = (sObject*)calloc (1, sizeof(*object));
        object->id = objectId;
        object->next = objectList;
        objectList = object;
        }

      object->type = (*buf) >> 6;

      sObjectDisplay* display = (sObjectDisplay*)calloc (1, sizeof(*display));
      display->objectId = objectId;
      display->regionId = regionId;
      display->xPos = AVRB16(buf) & 0xfff; buf += 2;
      display->yPos = AVRB16(buf) & 0xfff; buf += 2;

      if (display->xPos >= region->width ||
        display->yPos >= region->height) {
        cLog::log (LOGERROR, "Object outside region");
        free (display);
        return -1;
        }

      if (((object->type == 1) || (object->type == 2)) && (buf+1 < bufEnd)) {
        display->fgcolor = *buf++;
        display->bgcolor = *buf++;
        }

      display->regionListNext = region->displayList;
      region->displayList = display;

      display->objectListNext = object->displayList;
      object->displayList = display;
      }

    return 0;
    }
  //}}}
  //{{{
  int parseClut (const uint8_t* buf, int bufSize) {

    cLog::log (LOGINFO, "clut");

    const uint8_t* bufEnd = buf + bufSize;

    int clutId = *buf++;
    int version = ((*buf)>>4)&15;
    buf += 1;

    sClut* clut = getClut (clutId);
    if (!clut) {
      clut = (sClut*)malloc (sizeof(*clut));
      memcpy (clut, &defaultClut, sizeof(*clut));
      clut->id = clutId;
      clut->version = -1;
      clut->next = clutList;
      clutList = clut;
      }

    if (clut->version != version) {
      clut->version = version;
      while (buf + 4 < bufEnd) {
        int entryId = *buf++;
        int depth = (*buf) & 0xe0;
        if (depth == 0)
          cLog::log (LOGERROR, "Invalid clut depth 0x%x!n", *buf);

        int y, cr, cb, alpha;
        int fullRange = (*buf++) & 1;
        if (fullRange) {
          //{{{  full range
          y     = *buf++;
          cr    = *buf++;
          cb    = *buf++;
          alpha = *buf++;
          }
          //}}}
        else {
          //{{{  not full range
          y     = buf[0] & 0xfc;
          cr    = (((buf[0] & 3) << 2) | ((buf[1] >> 6) & 3)) << 4;
          cb    = (buf[1] << 2) & 0xf0;
          alpha = (buf[1] << 6) & 0xc0;
          buf += 2;
          }
          //}}}

        if (y == 0)
          alpha = 0xff;

        int r = 0;
        int g = 0;
        int b = 0;
        int r_add;
        int g_add;
        int b_add;
        YUV_TO_RGB1_CCIR(cb, cr);
        YUV_TO_RGB2_CCIR(r, g, b, y);

        if (depth & 0x80 && entryId < 4)
          clut->clut4[entryId] = RGBA(r, g, b, 255 - alpha);
        else if (depth & 0x40 && entryId < 16)
          clut->clut16[entryId] = RGBA(r, g, b, 255 - alpha);
        else if (depth & 0x20)
          clut->clut256[entryId] = RGBA(r, g, b, 255 - alpha);
        }
      }

    return 0;
    }
  //}}}
  //{{{
  int parseObject (const uint8_t* buf, int bufSize) {

    cLog::log (LOGINFO, "object");

    const uint8_t* bufEnd = buf + bufSize;

    int objectId = AVRB16(buf); buf += 2;
    sObject* object = getObject (objectId);
    if (!object)
      return -1;

    int codingMethod = ((*buf) >> 2) & 3;
    int nonModifyingColor = ((*buf++) >> 1) & 1;

    if (codingMethod == 0) {
      int topFieldLen = AVRB16(buf); buf += 2;
      int bottomFieldLen = AVRB16(buf); buf += 2;

      if ((buf + topFieldLen + bottomFieldLen) > bufEnd) {
        cLog::log (LOGERROR, "Field data size %d+%d too large", topFieldLen, bottomFieldLen);
        return -1;
        }

      for (sObjectDisplay* display = object->displayList; display; display = display->objectListNext) {
        const uint8_t* block = buf;
        parsePixelDataBlock (display, block, topFieldLen, 0, nonModifyingColor);

        int bfl = bottomFieldLen;
        if (bottomFieldLen > 0)
          block = buf + topFieldLen;
        else
          bfl = topFieldLen;
        parsePixelDataBlock (display, block, bfl, 1, nonModifyingColor);
        }
      }
    else
      cLog::log (LOGERROR, "Unknown object coding %d", codingMethod);

    return 0;
    }
  //}}}
  //{{{
  int parseDisplayDefinition (const uint8_t *buf, int bufSize) {

    cLog::log (LOGINFO, "displayDefinition");

    sDisplayDefinition* displayDef = displayDefinition;

    if (bufSize < 5)
      return -1;

    int info_byte = *buf++;
    int dds_version = info_byte >> 4;
    if (displayDef && displayDef->version == dds_version)
      return 0; // already have this display definition version

    if (!displayDef) {
      displayDef = (sDisplayDefinition*)calloc (1, sizeof(*displayDef));
      displayDefinition = displayDef;
      }
    displayDef->version = dds_version;
    displayDef->x       = 0;
    displayDef->y       = 0;
    displayDef->width   = AVRB16(buf) + 1; buf += 2;
    displayDef->height  = AVRB16(buf) + 1; buf += 2;

    if (info_byte & (1 << 3)) {
      // display_window_flag
      if (bufSize < 13)
        return -1;

      displayDef->x = AVRB16(buf); buf += 2;
      displayDef->width  = AVRB16(buf) - displayDef->x + 1; buf += 2;
      displayDef->y = AVRB16(buf); buf += 2;
      displayDef->height = AVRB16(buf) - displayDef->y + 1; buf += 2;
      }

    return 0;
    }
  //}}}
  //{{{
  int displayEnd (sSubtitle* sub) {

    cLog::log (LOGINFO, "displayEnd");

    if (compute_edt == 0)
      saveSet (sub);

    return 0;
    }
  //}}}

  //{{{  vars
  bool mDebug = kDebug;
  bool hasSubtitle = false;

  int compositionId = -1;
  int ancillaryId = -1;

  int version = 0;
  int time_out = 0;
  int compute_edt = 0; // if 1 end display time calculated using pts
                       // if 0 (Default) calculated using time out */

  int computeClut = 0 ;
  int clutCount2[257][256] = { 0 };
  sClut defaultClut;

  int substream = 0;
  int64_t prevStart = 0;

  sRegion* regionList = nullptr;
  sClut* clutList = nullptr;
  sObject* objectList = nullptr;

  sRegionDisplay* displayList = nullptr;
  sDisplayDefinition* displayDefinition = nullptr;
  //}}}
  };
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
    bool subDecodePes (cPidInfo* pidInfo) {

      cLog::log (LOGINFO, getPtsString (pidInfo->mPts) +
                          " size:" + dec(int (pidInfo->mBufPtr - pidInfo->mBuffer),4) +
                          " " + pidInfo->getInfoString());

      if (kDebug) {
        sSubtitle subtitles;
        memset (&subtitles, 0, sizeof (subtitles));

        cSubtitleDecoder decoder;
        decoder.decode (pidInfo->mBuffer, int (pidInfo->mBufPtr - pidInfo->mBuffer), &subtitles);

        for (unsigned int i = 0; i < subtitles.numRects; i++) {
          cLog::log (LOGINFO, "x:"   + dec(subtitles.rects[i]->x) +
                              " y:"  + dec(subtitles.rects[i]->y) +
                              " w:"  + dec(subtitles.rects[i]->w) +
                              " h:"  + dec(subtitles.rects[i]->h) +
                              " c:"  + dec(subtitles.rects[i]->numColours) +
                              " l:"  + dec(subtitles.rects[i]->stride));

          for (int j = 0; j < subtitles.rects[i]->h; j += 4) {
            uint8_t* p = subtitles.rects[i]->data[0] + subtitles.rects[i]->w;
            std::string str;
            for (int i = 0; i < 64; i++)
              str += *p++ ? 'X' : '.';
            cLog::log (LOGINFO, str);
            }
          }
        //subCloseDecoder (&context);
        }

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
