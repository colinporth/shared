// cDumpTransportStream.h
#pragma once

class cSubtitleDecoder {
public:
  //{{{  defines
  #define AVRB16(p) ((*(p) << 8) | *(p+1))

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

    //{{{
    void debug() {

      for (unsigned int i = 0; i < numRects; i++) {
        cLog::log (LOGINFO, "x:"   + dec(rects[i]->x) +
                            " y:"  + dec(rects[i]->y) +
                            " w:"  + dec(rects[i]->w) +
                            " h:"  + dec(rects[i]->h) +
                            " c:"  + dec(rects[i]->numColours) +
                            " l:"  + dec(rects[i]->stride));

        int xStep = rects[i]->w / 64;
        int yStep = rects[i]->h / 16;
        for (int y = 0; y < rects[i]->h; y += yStep) {
          uint8_t* p = rects[i]->data[0] + rects[i]->w;
          std::string str;
          for (int x = 0; x < rects[i]->w; x += xStep)
            str += p[x] ? 'X' : '.';
          cLog::log (LOGINFO, str);
          }
        }
      }
    //}}}
    };
  //}}}

  cSubtitleDecoder() {}
  ~cSubtitleDecoder() {}
  //{{{
  sSubtitle* decode (const uint8_t* buf, int bufSize) {

    // skip past first 2 bytes
    const uint8_t* bufEnd = buf + bufSize;
    const uint8_t* bufPtr = buf + 2;
    if (mBufferDebug) {
      //{{{  debug print start of buffer
      cLog::log (LOGINFO, "subtitle decode size:" + dec(bufSize));

      const uint8_t* debugBufPtr = buf;

      for (int j = 0; (j < 4) && (debugBufPtr < bufEnd); j++) {
        std::string str = "sub " + hex (j*32,2) + " ";
        for (int i = 0; (i < 32) && (debugBufPtr < bufEnd); i++) {
          int value = *debugBufPtr++;
          str += hex(value,2) + " ";
          }

        cLog::log (LOGINFO, str);
        }
      }
      //}}}

    int gotSegments = 0;
    while (((bufEnd - bufPtr) >= 6) && (*bufPtr == 0x0f)) {
      bufPtr += 1;
      int segment_type = *bufPtr++;
      int pageId = AVRB16(bufPtr); bufPtr += 2;
      int segmentLength = AVRB16(bufPtr); bufPtr += 2;
      if (bufEnd - bufPtr < segmentLength) {
        //{{{  error return
        cLog::log (LOGERROR, "incomplete or broken packet");
        return nullptr;
        }
        //}}}

      switch (segment_type) {
        case 0x10: // page
          if (!parsePage (bufPtr, segmentLength)) return nullptr; gotSegments |= 0x01; break;

        case 0x11: // region
          if (!parseRegion (bufPtr, segmentLength)) return nullptr; gotSegments |= 0x02; break;

        case 0x12: // clut
          if (!parseClut (bufPtr, segmentLength)) return nullptr; gotSegments |= 0x04; break;

        case 0x13: // object
          if (!parseObject (bufPtr, segmentLength)) return nullptr; gotSegments |= 0x08; break;

        case 0x14: // display definition
          if (!parseDisplayDefinition (bufPtr, segmentLength)) return nullptr; gotSegments |= 0x20; break;

        case 0x80: // display
          saveToSubtitle(); gotSegments |= 0x10; break;

        default:
          cLog::log (LOGINFO, "segment:%x, pageId:%d, length:%d", segment_type, pageId, segmentLength);
          break;
        }

      bufPtr += segmentLength;
      }

    if ((gotSegments & 0x0f) == 0x0F) {
      //{{{  Some streams do not send a display segment
      cLog::log (LOGINFO, "Missing display_endSegment, emulating");
      saveToSubtitle();
      }
      //}}}

    return mSubtitle;
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
    int mId;
    int mVersion;
    int mType;

    sObjectDisplay* mDisplayList;

    sObject* next;
    };
  //}}}
  //{{{
  struct sRegionDisplay {
    int mRegionId;

    int xPos;
    int yPos;

    sRegionDisplay* next;
    };
  //}}}
  //{{{
  struct sRegion {
    int mId;
    int mVersion;

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

    sObjectDisplay* mDisplayList;

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
    int (*counttab2)[256] = mClutCount2;

    int count, i, x, y;
    ptrdiff_t stride = rect->stride;

    memset (mClutCount2, 0, sizeof(mClutCount2));

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
    mVersion = -1;
    mPrevStart = 0;

    mDefaultClut.id = -1;
    mDefaultClut.next = NULL;
    mDefaultClut.clut4[0] = RGBA (  0,   0,   0,   0);
    mDefaultClut.clut4[1] = RGBA (255, 255, 255, 255);
    mDefaultClut.clut4[2] = RGBA (  0,   0,   0, 255);
    mDefaultClut.clut4[3] = RGBA (127, 127, 127, 255);
    mDefaultClut.clut16[0] = RGBA (0, 0, 0, 0);

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
      mDefaultClut.clut16[i] = RGBA (r, g, b, 255);
      }

    mDefaultClut.clut256[0] = RGBA (0, 0, 0, 0);
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
      mDefaultClut.clut256[i] = RGBA(r, g, b, a);
      }

    return 0;
    }
  //}}}

  //{{{
  void deleteRegionDisplayList (sRegion* region) {

    while (region->mDisplayList) {
      sObjectDisplay* display = region->mDisplayList;

      sObject* object = getObject (display->objectId);
      if (object) {
        sObjectDisplay** objDispPtr = &object->mDisplayList;
        sObjectDisplay* objDisp = *objDispPtr;

        while (objDisp && objDisp != display) {
          objDispPtr = &objDisp->objectListNext;
          objDisp = *objDispPtr;
          }

        if (objDisp) {
          *objDispPtr = objDisp->objectListNext;

          if (!object->mDisplayList) {
            sObject** obj2Ptr = &mObjectList;
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

      region->mDisplayList = display->regionListNext;

      free (&display);
      }

    }
  //}}}
  //{{{
  void deleteCluts() {

    while (mClutList) {
      sClut* clut = mClutList;
      mClutList = clut->next;
      free (&clut);
      }
    }
  //}}}
  //{{{
  void deleteObjects() {

    while (mObjectList) {
      sObject* object = mObjectList;
      mObjectList = object->next;
      free (&object);
      }
    }
  //}}}
  //{{{
  void deleteRegions() {

    while (mRegionList) {
      sRegion* region = mRegionList;

      mRegionList = region->next;
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

    free (&mDisplayDefinition);
    mDisplayDefinition = NULL;

    while (mDisplayList) {
      display = mDisplayList;
      mDisplayList = display->next;
      free (&display);
      }

    return 0;
    }
  //}}}

  //{{{
  sObject* getObject (int objectId) {

    sObject* object = mObjectList;

    while (object && object->mId != objectId)
      object = object->next;

    return object;
    }
  //}}}
  //{{{
  sClut* getClut (int clutId) {

    sClut* clut = mClutList;

    while (clut && clut->id != clutId)
      clut = clut->next;

    return clut;
    }
  //}}}
  //{{{
  sRegion* getRegion (int regionId) {

    sRegion* ptr = mRegionList;

    while (ptr && ptr->mId != regionId)
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

    dstBuf += xPos;
    int dstPixels = xPos;
    std::string str;

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
          int runLength = bitStream.getBits (3);
          str += ",3b:" + hex(runLength);
          if (runLength == 0) {
            if (mRunDebug)
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
              //{{{  0
              bits = mapTable ? mapTable[0] : 0;
              *dstBuf++ = bits;
              dstPixels ++;
              }
              //}}}
            else if (bits == 1) {
              //{{{  1
              bits = mapTable ? mapTable[0] : 0;
              int runLength = 2;
              str += ":run:" + dec(runLength);
              while ((runLength-- > 0) && (dstPixels < dstBufSize)) {
                *dstBuf++ = bits;
                dstPixels++;
                }
              }
              //}}}
            else if (bits == 2) {
              //{{{  2
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
              //}}}
            else if (bits == 3) {
              //{{{  3
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
              //}}}
            }
          }
        }
      }

    int bits = bitStream.getBits (8);
    if (bits)
      cLog::log (LOGERROR, "line overflow");

    str += "] [4b:" + hex(bits) + "] ok";;
    if (mRunDebug)
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
  void parseBlock (sObjectDisplay* display, const uint8_t* buf, int bufSize, int topBottom, int nonModifyingColor) {

    const uint8_t* bufEnd = buf + bufSize;
    if (mBlockDebug) {
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
  void saveToSubtitle() {

    int offsetX = 0;
    int offsetY = 0;

    if (mDisplayDefinition) {
      offsetX = mDisplayDefinition->x;
      offsetY = mDisplayDefinition->y;
      }

    mSubtitle = (sSubtitle*) calloc (1, sizeof(sSubtitle));
    mSubtitle->endDisplayTime = mTimeOut * 1000;

    for (sRegionDisplay* display = mDisplayList; display; display = display->next) {
      sRegion* region = getRegion (display->mRegionId);
      if (region && region->dirty)
        mSubtitle->numRects++;
      }

    if (mSubtitle->numRects > 0) {
      mSubtitle->rects = (sRect**)malloc (mSubtitle->numRects * sizeof(*mSubtitle->rects));
      for (unsigned int i = 0; i < mSubtitle->numRects; i++)
        mSubtitle->rects[i] = (sRect*)calloc (1, sizeof(*mSubtitle->rects[i]));

      int i = 0;
      for (sRegionDisplay* display = mDisplayList; display; display = display->next) {
        sRegion* region = getRegion (display->mRegionId);
        if (!region)
          continue;
        if (!region->dirty)
          continue;

        mSubtitle->rects[i]->x = display->xPos + offsetX;
        mSubtitle->rects[i]->y = display->yPos + offsetY;
        mSubtitle->rects[i]->w = region->width;
        mSubtitle->rects[i]->h = region->height;
        mSubtitle->rects[i]->numColours = 1 << region->depth;
        mSubtitle->rects[i]->stride = region->width;

        sClut* clut = getClut (region->clut);
        if (!clut)
          clut = &mDefaultClut;
        uint32_t* clutTable;
        switch (region->depth) {
          case 2:  clutTable = clut->clut4; break;
          case 8:  clutTable = clut->clut256; break;
          case 4:
          default: clutTable = clut->clut16; break;
          }

        mSubtitle->rects[i]->data[1] = (uint8_t*)malloc (1024);
        memcpy (mSubtitle->rects[i]->data[1], clutTable, (1 << region->depth) * sizeof(*clutTable));

        mSubtitle->rects[i]->data[0] = (uint8_t*)malloc (region->pixelBufSize);
        memcpy (mSubtitle->rects[i]->data[0], region->pixelBuf, region->pixelBufSize);

        if (((clut == &mDefaultClut) && (mComputeClut == -1)) || (mComputeClut == 1)) {
          if (!region->hasComputedClut) {
            initDefaultClut (region->computedClut, mSubtitle->rects[i], mSubtitle->rects[i]->w, mSubtitle->rects[i]->h);
            region->hasComputedClut = 1;
            }
          memcpy (mSubtitle->rects[i]->data[1], region->computedClut, sizeof(region->computedClut));
          }
        i++;
        }
      }
    }
  //}}}

  //{{{
  bool parsePage (const uint8_t* buf, int bufSize) {

    if (mSegmentDebug)
      cLog::log (LOGINFO, "page");

    if (bufSize < 1)
      return false;
    const uint8_t* bufEnd = buf + bufSize;

    mTimeOut = *buf++;
    int pageVersion = ((*buf) >> 4) & 15;
    if (mVersion == pageVersion)
      return true;

    mVersion = pageVersion;
    int pageState = ((*buf++) >> 2) & 3;

    if (mSegmentDebug)
      cLog::log (LOGINFO, "- time out %ds, state %d", mTimeOut, pageState);

    if ((pageState == 1) || (pageState == 2)) {
      deleteRegions();
      deleteObjects();
      deleteCluts();
      }

    sRegionDisplay* tmpDisplayList = mDisplayList;
    mDisplayList = NULL;
    while (buf + 5 < bufEnd) {
      int regionId = *buf++;
      buf += 1;

      sRegionDisplay* display = mDisplayList;
      while (display && (display->mRegionId != regionId))
        display = display->next;

      if (display) {
        cLog::log (LOGERROR, "duplicate region");
        break;
        }

      display = tmpDisplayList;
      sRegionDisplay** tmpPtr = &tmpDisplayList;
      while (display && (display->mRegionId != regionId)) {
        tmpPtr = &display->next;
        display = display->next;
        }

      if (!display)
        display = (sRegionDisplay*)calloc (1, sizeof(*display));
      display->mRegionId = regionId;
      display->xPos = AVRB16(buf); buf += 2;
      display->yPos = AVRB16(buf); buf += 2;
      *tmpPtr = display->next;
      display->next = mDisplayList;
      mDisplayList = display;

      cLog::log (LOGINFO, "Region:%d, x:%d y:%d", regionId, display->xPos, display->yPos);
      }

    while (tmpDisplayList) {
      sRegionDisplay* display = tmpDisplayList;
      tmpDisplayList = display->next;
      free (&display);
      }

    return true;
    }
  //}}}
  //{{{
  bool parseRegion (const uint8_t* buf, int bufSize) {

    if (mSegmentDebug)
      cLog::log (LOGINFO, "region");

    if (bufSize < 10)
      return false;
    const uint8_t* bufEnd = buf + bufSize;

    int regionId = *buf++;
    sRegion* region = getRegion (regionId);
    if (!region) {
      region = (sRegion*)calloc (1, sizeof(*region));
      region->mId = regionId;
      region->mVersion = -1;
      region->next = mRegionList;
      mRegionList = region;
      }

    region->mVersion = ((*buf)>>4) & 15;
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
        object->mId = objectId;
        object->next = mObjectList;
        mObjectList = object;
        }

      object->mType = (*buf) >> 6;

      sObjectDisplay* display = (sObjectDisplay*)calloc (1, sizeof(*display));
      display->objectId = objectId;
      display->regionId = regionId;
      display->xPos = AVRB16(buf) & 0xFFF; buf += 2;
      display->yPos = AVRB16(buf) & 0xFFF; buf += 2;

      if (display->xPos >= region->width ||
        display->yPos >= region->height) {
        cLog::log (LOGERROR, "Object outside region");
        free (display);
        return false;
        }

      if (((object->mType == 1) || (object->mType == 2)) && (buf+1 < bufEnd)) {
        display->fgcolor = *buf++;
        display->bgcolor = *buf++;
        }

      display->regionListNext = region->mDisplayList;
      region->mDisplayList = display;

      display->objectListNext = object->mDisplayList;
      object->mDisplayList = display;
      }

    return true;
    }
  //}}}
  //{{{
  bool parseClut (const uint8_t* buf, int bufSize) {

    if (mSegmentDebug)
      cLog::log (LOGINFO, "clut");

    const uint8_t* bufEnd = buf + bufSize;

    int clutId = *buf++;
    int version = ((*buf)>>4)&15;
    buf += 1;

    sClut* clut = getClut (clutId);
    if (!clut) {
      clut = (sClut*)malloc (sizeof(*clut));
      memcpy (clut, &mDefaultClut, sizeof(*clut));
      clut->id = clutId;
      clut->version = -1;
      clut->next = mClutList;
      mClutList = clut;
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

    return true;
    }
  //}}}
  //{{{
  bool parseObject (const uint8_t* buf, int bufSize) {

    if (mSegmentDebug)
      cLog::log (LOGINFO, "object");

    const uint8_t* bufEnd = buf + bufSize;

    int objectId = AVRB16(buf); buf += 2;
    sObject* object = getObject (objectId);
    if (!object)
      return false;

    int codingMethod = ((*buf) >> 2) & 3;
    int nonModifyingColor = ((*buf++) >> 1) & 1;

    if (codingMethod == 0) {
      int topFieldLen = AVRB16(buf); buf += 2;
      int bottomFieldLen = AVRB16(buf); buf += 2;

      if ((buf + topFieldLen + bottomFieldLen) > bufEnd) {
        cLog::log (LOGERROR, "Field data size %d+%d too large", topFieldLen, bottomFieldLen);
        return false;
        }

      for (sObjectDisplay* display = object->mDisplayList; display; display = display->objectListNext) {
        const uint8_t* block = buf;
        parseBlock (display, block, topFieldLen, 0, nonModifyingColor);

        int bfl = bottomFieldLen;
        if (bottomFieldLen > 0)
          block = buf + topFieldLen;
        else
          bfl = topFieldLen;
        parseBlock (display, block, bfl, 1, nonModifyingColor);
        }
      }
    else
      cLog::log (LOGERROR, "Unknown object coding %d", codingMethod);

    return true;
    }
  //}}}
  //{{{
  bool parseDisplayDefinition (const uint8_t *buf, int bufSize) {

    if (mSegmentDebug)
      cLog::log (LOGINFO, "displayDefinition");

    if (bufSize < 5)
      return false;

    int info_byte = *buf++;
    int dds_version = info_byte >> 4;
    if (mDisplayDefinition && (mDisplayDefinition->version == dds_version))
      return true;

    if (!mDisplayDefinition)
      mDisplayDefinition = (sDisplayDefinition*)calloc (1, sizeof(*mDisplayDefinition));

    mDisplayDefinition->version = dds_version;
    mDisplayDefinition->x       = 0;
    mDisplayDefinition->y       = 0;
    mDisplayDefinition->width   = AVRB16(buf) + 1; buf += 2;
    mDisplayDefinition->height  = AVRB16(buf) + 1; buf += 2;

    if (info_byte & (1 << 3)) {
      // display_window_flag
      if (bufSize < 13)
        return false;

      mDisplayDefinition->x = AVRB16(buf); buf += 2;
      mDisplayDefinition->width  = AVRB16(buf) - mDisplayDefinition->x + 1; buf += 2;
      mDisplayDefinition->y = AVRB16(buf); buf += 2;
      mDisplayDefinition->height = AVRB16(buf) - mDisplayDefinition->y + 1; buf += 2;
      }

    return true;
    }
  //}}}

  //  vars
  bool mBufferDebug = false;
  bool mSegmentDebug = false;
  bool mBlockDebug = false;
  bool mRunDebug = false;

  int mVersion = 0;
  int mTimeOut = 0;

  uint64_t mPrevStart = 0;

  int mComputeClut = 0;
  sClut mDefaultClut = { 0 };
  int mClutCount2[257][256] = { 0 };

  sRegion* mRegionList = nullptr;
  sClut* mClutList = nullptr;
  sObject* mObjectList = nullptr;
  sRegionDisplay* mDisplayList = nullptr;
  sDisplayDefinition* mDisplayDefinition = nullptr;

  sSubtitle* mSubtitle;
  };
