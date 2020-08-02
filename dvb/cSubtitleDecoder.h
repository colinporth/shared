// cSubtitleDecoder.h
#pragma once

class cSubtitleDecoder {
public:
  //{{{  defines
  #define AVRB16(p) ((*(p) << 8) | *(p+1))

  #define SCALEBITS 10
  #define ONE_HALF  (1 << (SCALEBITS - 1))
  #define FIX(x)    ((int) ((x) * (1<<SCALEBITS) + 0.5))
  #define RGBA(r,g,b,a) (((unsigned)(a) << 24) | ((r) << 16) | ((g) << 8) | (b))
  //}}}
  //{{{
  class cSubtitle {
  public:
    //{{{
    class cRect {
    public:
      //{{{
      ~cRect() {
        free (mPixelData);
        free (mClutData);
        }

      //}}}

      int mX = 0;
      int mY = 0;
      int mWidth = 0;
      int mHeight = 0;

      int mStride = 0;
      uint8_t* mPixelData = nullptr;

      int mNumColours = 0;
      uint8_t* mClutData = nullptr;
      };
    //}}}

    cSubtitle() {}
    //{{{
    ~cSubtitle() {
      for (auto rect : mRects)
        delete rect;
      }
    //}}}

    //{{{
    void debug() {

      if (mRects.empty())
        cLog::log (LOGINFO, "subtitle empty");
      else
        for (unsigned int i = 0; i < mRects.size(); i++) {
          cLog::log (LOGINFO, "subtitle rect:" + dec(i) +
                              " x:"  + dec(mRects[i]->mX) +
                              " y:"  + dec(mRects[i]->mY) +
                              " w:"  + dec(mRects[i]->mWidth) +
                              " h:"  + dec(mRects[i]->mHeight) +
                              " c:"  + dec(mRects[i]->mNumColours) +
                              " l:"  + dec(mRects[i]->mStride));

          int xStep = mRects[i]->mWidth / 80;
          int yStep = mRects[i]->mHeight / 16;
          for (int y = 0; y < mRects[i]->mHeight; y += yStep) {
            uint8_t* p = mRects[i]->mPixelData + mRects[i]->mWidth;
            std::string str = "  ";
            for (int x = 0; x < mRects[i]->mWidth; x += xStep) {
              int value = p[x];
              str += value ? hex(value,1) : ".";
              }
            cLog::log (LOGINFO, str);
            }
          }
      }
    //}}}

    // vars
    std::vector <cRect*> mRects;
    };
  //}}}

  cSubtitleDecoder() {}
  ~cSubtitleDecoder() {
    //closeDecoder();
    }

  //{{{
  cSubtitle* decode (const uint8_t* buf, int bufSize) {
  // return malloc'ed sSubtitle if succesful

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

    while ((bufEnd - bufPtr >= 6) && (*bufPtr++ == 0x0f)) {
      int segmentType = *bufPtr++;
      int pageId = AVRB16(bufPtr); bufPtr += 2;
      int segmentLength = AVRB16(bufPtr); bufPtr += 2;
      if (bufEnd - bufPtr < segmentLength) {
        //{{{  error return
        cLog::log (LOGERROR, "incomplete or broken packet");
        return nullptr;
        }
        //}}}

      switch (segmentType) {
        case 0x10:
          if (!parsePage (bufPtr, segmentLength)) return nullptr; break;

        case 0x11:
          if (!parseRegion (bufPtr, segmentLength)) return nullptr;  break;

        case 0x12:
          if (!parseClut (bufPtr, segmentLength)) return nullptr;  break;

        case 0x13:
          if (!parseObject (bufPtr, segmentLength)) return nullptr; break;

        case 0x14:
          if (!parseDisplayDefinition (bufPtr, segmentLength)) return nullptr; break;

        case 0x80:
          mSubtitle = createSubtitle(); break;

        default:
          cLog::log (LOGERROR, "unknown seg:%x, pageId:%d, size:%d", segmentType, pageId, segmentLength);
          break;
        }

      bufPtr += segmentLength;
      }

    return mSubtitle;
    }
  //}}}

private:
  //{{{  structs
  //{{{
  struct sObjectDisplay {
    int mObjectId;
    int mRegionId;

    int xPos;
    int yPos;

    int mForegroundColor;
    int mBackgroundColor;

    sObjectDisplay* mRegionListNext;
    sObjectDisplay* mObjectListNext;
    };
  //}}}
  //{{{
  struct sObject {
    int mId;
    int mVersion;
    int mType;

    sObjectDisplay* mDisplayList;

    sObject* mNext;
    };
  //}}}

  //{{{
  struct sRegionDisplay {
    int mRegionId;

    int xPos;
    int yPos;

    sRegionDisplay* mNext;
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
  struct sClut {
    int id = 0;
    int version = 0;

    uint32_t clut4[4] = { 0 };
    uint32_t clut16[16] = { 0 };
    uint32_t clut256[256] = { 0 };

    sClut* next;
    };
  //}}}
  //}}}
  //{{{
  class cBitStream {
  public:
    cBitStream (const uint8_t* bitStream, int size) : mBitStream(bitStream), mSize(size) {}

    //{{{
    uint32_t getBit() {

      uint32_t result = (uint32_t)((mBitStream [mBitsRead >> 3] >> (7-(mBitsRead & 0x7))) & 0x01);
      mBitsRead += 1;
      return result;
      }
    //}}}
    //{{{
    uint32_t getBits (int numBits) {

      uint32_t result = 0;
      for (int i = 0; i < numBits; i++)
        result = (result << 1) | getBit();

      return result;
      }
    //}}}

    //{{{
    int getBitsRead() {
      return mBitsRead;
      }
    //}}}
    //{{{
    int getBytesRead() {
      return (mBitsRead + 7) / 8;
      }
    //}}}

  private:
    const uint8_t* mBitStream = nullptr;
    int mBitsRead = 0;
    int mSize = 0;
    };
  //}}}

  //{{{
  void initDefaultClut (uint8_t* clut, cSubtitle::cRect* rect, int w, int h) {

    uint8_t list[256] = { 0 };
    uint8_t listInv[256];
    int counttab[256] = { 0 };
    int (*counttab2)[256] = mClutCount2;

    int count, i, x, y;
    ptrdiff_t stride = rect->mStride;

    memset (mClutCount2, 0, sizeof(mClutCount2));

    #define V(x,y) rect->mPixelData[(x) + (y)*stride]

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

      sObject* object = getObject (display->mObjectId);
      if (object) {
        sObjectDisplay** objDispPtr = &object->mDisplayList;
        sObjectDisplay* objDisp = *objDispPtr;

        while (objDisp && (objDisp != display)) {
          objDispPtr = &objDisp->mObjectListNext;
          objDisp = *objDispPtr;
          }

        if (objDisp) {
          *objDispPtr = objDisp->mObjectListNext;

          if (!object->mDisplayList) {
            sObject** obj2Ptr = &mObjectList;
            sObject* obj2 = *obj2Ptr;

            while (obj2 != object) {
              obj2Ptr = &obj2->mNext;
              obj2 = *obj2Ptr;
              }

            *obj2Ptr = obj2->mNext;
            free (&obj2);
            }
          }
        }

      region->mDisplayList = display->mRegionListNext;

      free (&display);
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
  void deleteObjects() {

    while (mObjectList) {
      sObject* object = mObjectList;
      mObjectList = object->mNext;
      free (&object);
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
  void closeDecoder() {

    deleteRegions();
    deleteObjects();
    deleteCluts();

    free (&mDisplayDefinition);

    while (mDisplayList) {
      sRegionDisplay* display = mDisplayList;
      mDisplayList = display->mNext;
      free (&display);
      }
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
  sObject* getObject (int objectId) {

    sObject* object = mObjectList;
    while (object && object->mId != objectId)
      object = object->mNext;

    return object;
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
  int parse2bitString (uint8_t* dstBuf, int dstBufSize,
                       const uint8_t** srcBuf, int srcBufSize,
                       int nonModifyingColor, uint8_t* mapTable, int xPos) {

    int dstPixels = xPos;
    dstBuf += xPos;

    cBitStream bitStream (*srcBuf, srcBufSize);
    while ((bitStream.getBitsRead() < (srcBufSize * 8)) && (dstPixels < dstBufSize)) {
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
              (*srcBuf) += bitStream.getBytesRead();
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

    (*srcBuf) += bitStream.getBytesRead();
    return dstPixels;
    }
  //}}}
  //{{{
  int parse4bitString (uint8_t* dstBuf, int dstBufSize,
                       const uint8_t** srcBuf, int srcBufSize,
                       int nonModifyingColor, uint8_t* mapTable, int xPos) {

    dstBuf += xPos;
    int dstPixels = xPos;
    std::string str;

    cBitStream bitStream (*srcBuf, srcBufSize);
    while ((bitStream.getBitsRead() < (srcBufSize * 8)) && (dstPixels < dstBufSize)) {
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
            (*srcBuf) += bitStream.getBytesRead();
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

    (*srcBuf) += bitStream.getBytesRead();
    return dstPixels;
    }
  //}}}
  //{{{
  int parse8bitString (uint8_t* dstBuf, int dstBufSize,
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
  void parseBlock (sObjectDisplay* display, const uint8_t* buf, int bufSize, bool bottom, int nonModifyingColor) {

    const uint8_t* bufEnd = buf + bufSize;
    if (mBlockDebug) {
      //{{{  debug buf
      uint64_t address = (uint64_t)buf;
      cLog::log (LOGINFO, "parseBlock @:" + hex(address,16) + " size:" + dec(bufSize,4));

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

    sRegion* region = getRegion (display->mRegionId);
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
    int yPos = display->yPos + (bottom ? 1 : 0);

    while (buf < bufEnd) {
      if (((*buf != 0xF0) && (xPos >= region->width)) || (yPos >= region->height)) {
        cLog::log (LOGERROR, "invalid object location %d-%d %d-%d %02x",
                              xPos, region->width, yPos, region->height, *buf);
        return;
        }

      switch (*buf++) {
        //{{{
        case 0x10: // 2 bit
          if (region->depth == 8)
            mapTable = map2to8;
          else if (region->depth == 4)
            mapTable = map2to4;
          else
            mapTable = NULL;

          xPos = parse2bitString (pixelBuf + (yPos * region->width), region->width,
                                  &buf, int(bufEnd - buf), nonModifyingColor, mapTable, xPos);
          break;
        //}}}
        //{{{
        case 0x11: // 4 bit
          if (region->depth < 4) {
            cLog::log (LOGERROR, "4-bit pixel string in %d-bit region!", region->depth);
            return;
            }

          if (region->depth == 8)
            mapTable = map4to8;
          else
            mapTable = NULL;

          xPos = parse4bitString (pixelBuf + (yPos * region->width), region->width,
                                  &buf, int(bufEnd - buf), nonModifyingColor, mapTable, xPos);
          break;
        //}}}
        //{{{

        case 0x12: // 8 bit
          if (region->depth < 8) {
            cLog::log (LOGERROR, "8-bit pixel string in %d-bit region!", region->depth);
            return;
            }

          xPos = parse8bitString (pixelBuf + (yPos * region->width), region->width,
                                  &buf, int(bufEnd - buf), nonModifyingColor, NULL, xPos);
            break;
        //}}}
        //{{{
        case 0x20: // map 2 to 4
          map2to4[0] = (*buf) >> 4;
          map2to4[1] = (*buf++) & 0xf;
          map2to4[2] = (*buf) >> 4;
          map2to4[3] = (*buf++) & 0xf;

          break;
        //}}}
        //{{{
        case 0x21: // map 2 to 8
          for (int i = 0; i < 4; i++)
            map2to8[i] = *buf++;

          break;
        //}}}
        //{{{
        case 0x22: // map 4 top 8
          for (int i = 0; i < 16; i++)
            map4to8[i] = *buf++;

          break;
        //}}}
        //{{{
        case 0xF0: // end of line
          xPos = display->xPos;
          yPos += 2;

          break;
        //}}}
        default:
          cLog::log (LOGINFO, "unknownblock %x", *(buf-1));
          break;
        }
      }

    region->hasComputedClut = 0;
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
      cLog::log (LOGINFO, "- timeOut:%d state:%d", mTimeOut, pageState);

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
        display = display->mNext;

      if (display) {
        cLog::log (LOGERROR, "duplicate region");
        break;
        }

      display = tmpDisplayList;
      sRegionDisplay** tmpPtr = &tmpDisplayList;
      while (display && (display->mRegionId != regionId)) {
        tmpPtr = &display->mNext;
        display = display->mNext;
        }

      if (!display)
        display = (sRegionDisplay*)calloc (1, sizeof(*display));
      display->mRegionId = regionId;
      display->xPos = AVRB16(buf); buf += 2;
      display->yPos = AVRB16(buf); buf += 2;
      *tmpPtr = display->mNext;
      display->mNext = mDisplayList;
      mDisplayList = display;

      if (mRegionDebug)
        cLog::log (LOGINFO, "- region:%d x:%d y:%d", regionId, display->xPos, display->yPos);
      }

    while (tmpDisplayList) {
      sRegionDisplay* display = tmpDisplayList;
      tmpDisplayList = display->mNext;
      free (&display);
      }

    return true;
    }
  //}}}
  //{{{
  bool parseRegion (const uint8_t* buf, int bufSize) {

    if (mSegmentDebug)
      cLog::log (LOGINFO, "region segment");

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
        object->mNext = mObjectList;
        mObjectList = object;
        }

      object->mType = (*buf) >> 6;

      sObjectDisplay* display = (sObjectDisplay*)calloc (1, sizeof(*display));
      display->mObjectId = objectId;
      display->mRegionId = regionId;
      display->xPos = AVRB16(buf) & 0xFFF; buf += 2;
      display->yPos = AVRB16(buf) & 0xFFF; buf += 2;

      if (display->xPos >= region->width ||
        display->yPos >= region->height) {
        cLog::log (LOGERROR, "Object outside region");
        free (display);
        return false;
        }

      if (((object->mType == 1) || (object->mType == 2)) && (buf+1 < bufEnd)) {
        display->mForegroundColor = *buf++;
        display->mBackgroundColor = *buf++;
        }

      display->mRegionListNext = region->mDisplayList;
      region->mDisplayList = display;

      display->mObjectListNext = object->mDisplayList;
      object->mDisplayList = display;
      }

    return true;
    }
  //}}}
  //{{{
  bool parseClut (const uint8_t* buf, int bufSize) {

    if (mSegmentDebug)
      cLog::log (LOGINFO, "clut segment");

    const uint8_t* bufEnd = buf + bufSize;

    int clutId = *buf++;
    int version = ((*buf++) >> 4) & 15;

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
          y = buf[0] & 0xFC;
          cr = (((buf[0] & 3) << 2) | ((buf[1] >> 6) & 3)) << 4;
          cb = (buf[1] << 2) & 0xF0;
          alpha = (buf[1] << 6) & 0xC0;

          buf += 2;
          }
          //}}}
        if (y == 0)
          alpha = 0xff;

        int rAdd = FIX(1.40200*255.0/224.0) * (cr - 128) + ONE_HALF;
        int gAdd = - FIX(0.34414*255.0/224.0) * (cb - 128) - FIX(0.71414*255.0/224.0) * (cr - 128) + ONE_HALF;
        int bAdd = FIX(1.77200*255.0/224.0) * (cb - 128) + ONE_HALF;
        y = (y - 16) * FIX(255.0 / 219.0);
        int r = (y + rAdd) >> SCALEBITS;
        int g = (y + gAdd) >> SCALEBITS;
        int b = (y + bAdd) >> SCALEBITS;

        if (depth & 0x80 && entryId < 4)
          clut->clut4[entryId] = RGBA(r, g, b, 0xFF - alpha);
        else if (depth & 0x40 && entryId < 16)
          clut->clut16[entryId] = RGBA(r, g, b, 0xFF - alpha);
        else if (depth & 0x20)
          clut->clut256[entryId] = RGBA(r, g, b, 0xFF - alpha);
        if (mClutDebug)
          cLog::log (LOGINFO, "- clut depth:" + hex(depth) + " id:" + hex(entryId) +
                              " " + hex(r & 0xFF, 2) + ":" + hex(g & 0xFF, 2) + ":" + hex(b & 0xFF, 2) + ":" + hex(0xFF - alpha, 2));
        }
      }

    return true;
    }
  //}}}
  //{{{
  bool parseObject (const uint8_t* buf, int bufSize) {

    if (mSegmentDebug)
      cLog::log (LOGINFO, "object segment");

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
        //{{{  error return
        cLog::log (LOGERROR, "Field data size %d+%d too large", topFieldLen, bottomFieldLen);
        return false;
        }
        //}}}

      for (auto display = object->mDisplayList; display; display = display->mObjectListNext) {
        const uint8_t* block = buf;
        parseBlock (display, block, topFieldLen, false, nonModifyingColor);

        int bfl = bottomFieldLen;
        if (bottomFieldLen > 0)
          block = buf + topFieldLen;
        else
          bfl = topFieldLen;
        parseBlock (display, block, bfl, true, nonModifyingColor);
        }
      }
    else
      cLog::log (LOGERROR, "unknown object coding %d", codingMethod);

    return true;
    }
  //}}}
  //{{{
  bool parseDisplayDefinition (const uint8_t *buf, int bufSize) {

    if (mSegmentDebug)
      cLog::log (LOGINFO, "displayDefinition segment");

    if (bufSize < 5)
      return false;

    int infoByte = *buf++;
    int ddsVersion = infoByte >> 4;

    if (mDisplayDefinition &&
        (mDisplayDefinition->version == ddsVersion))
      return true;

    if (!mDisplayDefinition)
      mDisplayDefinition = (sDisplayDefinition*)malloc (sizeof(*mDisplayDefinition));
    mDisplayDefinition->version = ddsVersion;
    mDisplayDefinition->x = 0;
    mDisplayDefinition->y = 0;
    mDisplayDefinition->width = AVRB16(buf) + 1; buf += 2;
    mDisplayDefinition->height = AVRB16(buf) + 1; buf += 2;

    if (infoByte & (1 << 3)) {
      // display_window_flag
      if (bufSize < 13)
        return false;

      mDisplayDefinition->x = AVRB16(buf); buf += 2;
      mDisplayDefinition->width  = AVRB16(buf) - mDisplayDefinition->x + 1; buf += 2;
      mDisplayDefinition->y = AVRB16(buf); buf += 2;
      mDisplayDefinition->height = AVRB16(buf) - mDisplayDefinition->y + 1; buf += 2;
      }

    if (mDisplayDefnitionDebug)
      cLog::log (LOGINFO, "- displayDefinition x:" + dec(mDisplayDefinition->x) +
                          " y:" + dec(mDisplayDefinition->y) +
                          " w:" + dec(mDisplayDefinition->width) +
                          " h:" + dec(mDisplayDefinition->height));
    return true;
    }
  //}}}

  //{{{
  cSubtitle* createSubtitle() {

    auto subtitle = new cSubtitle();

    int offsetX = 0;
    int offsetY = 0;
    if (mDisplayDefinition) {
      offsetX = mDisplayDefinition->x;
      offsetY = mDisplayDefinition->y;
      }

    for (sRegionDisplay* display = mDisplayList; display; display = display->mNext) {
      sRegion* region = getRegion (display->mRegionId);
      if (!region || !region->dirty)
        continue;

      auto subtitleRect = new cSubtitle::cRect();
      subtitleRect->mX = display->xPos + offsetX;
      subtitleRect->mY = display->yPos + offsetY;
      subtitleRect->mWidth = region->width;
      subtitleRect->mHeight = region->height;
      subtitleRect->mNumColours = 1 << region->depth;
      subtitleRect->mStride = region->width;

      auto clut = getClut (region->clut);
      if (!clut)
        clut = &mDefaultClut;

      uint32_t* clutTable;
      switch (region->depth) {
        case 2:  clutTable = clut->clut4; break;
        case 4:  clutTable = clut->clut16; break;
        case 8:  clutTable = clut->clut256; break;
        default: clutTable = clut->clut16; break;
        }

      subtitleRect->mClutData = (uint8_t*)malloc (1024);
      memcpy (subtitleRect->mClutData, clutTable, (1 << region->depth) * sizeof(*clutTable));

      subtitleRect->mPixelData = (uint8_t*)malloc (region->pixelBufSize);
      memcpy (subtitleRect->mPixelData, region->pixelBuf, region->pixelBufSize);

      if (((clut == &mDefaultClut) && (mComputeClut == -1)) || (mComputeClut == 1)) {
        if (!region->hasComputedClut) {
          initDefaultClut (region->computedClut, subtitleRect, subtitleRect->mWidth, subtitleRect->mHeight);
          region->hasComputedClut = 1;
          }
        memcpy (subtitleRect->mClutData, region->computedClut, sizeof(region->computedClut));
        }

      subtitle->mRects.push_back (subtitleRect);
      }

    return subtitle;
    }
  //}}}

  //  vars
  bool mBufferDebug = false;
  bool mSegmentDebug = false;
  bool mDisplayDefnitionDebug = true;
  bool mRegionDebug = true;
  bool mBlockDebug = false;
  bool mClutDebug = true;
  bool mRunDebug = false;

  int mVersion = 0;
  int mTimeOut = 0;

  int mComputeClut = 0;
  sClut mDefaultClut = { 0 };
  int mClutCount2[257][256] = { 0 };

  sRegion* mRegionList = nullptr;
  sClut* mClutList = nullptr;
  sObject* mObjectList = nullptr;
  sRegionDisplay* mDisplayList = nullptr;
  sDisplayDefinition* mDisplayDefinition = nullptr;

  cSubtitle* mSubtitle;
  };
