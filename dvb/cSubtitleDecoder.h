// cSubtitleDecoder.h
#pragma once

//{{{
class cSubtitle {
public:
  //{{{
  class cRectData {
  public:
    //{{{
    ~cRectData() {
      free (mPixelData);
      }

    //}}}

    int mX = 0;
    int mY = 0;
    int mWidth = 0;
    int mHeight = 0;

    int mNumColours = 0;
    uint8_t* mPixelData = nullptr;
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
  void debug (std::string prefix) {

    if (mRects.empty())
      cLog::log (LOGINFO, "subtitle empty");
    else
      for (unsigned int i = 0; i < mRects.size(); i++) {
        cLog::log (LOGINFO, prefix +
                            "rect:" + dec(i) +
                            " x:"  + dec(mRects[i]->mX) +
                            " y:"  + dec(mRects[i]->mY) +
                            " w:"  + dec(mRects[i]->mWidth) +
                            " h:"  + dec(mRects[i]->mHeight) +
                            " c:"  + dec(mRects[i]->mNumColours));
        }
    }
  //}}}
  //{{{
  void moreDebug() {

    if (!mRects.empty())
      for (unsigned int i = 0; i < mRects.size(); i++) {
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
  std::vector <cRectData*> mRects;
  bool mChanged = false;
  };
//}}}

class cSubtitleDecoder {
public:
  //{{{  defines
  #define AVRB16(p) ((*(p) << 8) | *(p+1))

  #define SCALEBITS 10
  #define ONE_HALF  (1 << (SCALEBITS - 1))
  #define FIX(x)    ((int) ((x) * (1<<SCALEBITS) + 0.5))
  //#define RGBA(r,g,b,a) (((unsigned)(a) << 24) | ((r) << 16) | ((g) << 8) | (b))
  #define RGBA(r,g,b,a) (((unsigned)(a) << 24) | ((b) << 16) | ((g) << 8) | (r))
  //}}}
  //{{{
  cSubtitleDecoder() {
    mVersion = -1;
    mDefaultClut.mId = -1;
    //{{{  init 2bit clut
    mDefaultClut.mClut4[0] = RGBA (  0,   0,   0,   0);
    mDefaultClut.mClut4[1] = RGBA (255, 255, 255, 255);
    mDefaultClut.mClut4[2] = RGBA (  0,   0,   0, 255);
    mDefaultClut.mClut4[3] = RGBA (127, 127, 127, 255);
    //}}}
    //{{{  init 4bit clut
    mDefaultClut.mClut16[0] = RGBA (0, 0, 0, 0);

    for (int i = 1; i <= 0x0F; i++) {
      int r;
      int g;
      int b;
      int a = 0xFF;

      if (i < 8) {
        r = (i & 1) ? 0xFF : 0;
        g = (i & 2) ? 0xFF : 0;
        b = (i & 4) ? 0xFF : 0;
        }
      else {
        r = (i & 1) ? 0x7F : 0;
        g = (i & 2) ? 0x7F : 0;
        b = (i & 4) ? 0x7F : 0;
        }

      mDefaultClut.mClut16[i] = RGBA (r, g, b, a);
      }
    //}}}
    //{{{  init 8bit clut
    mDefaultClut.mClut256[0] = RGBA (0, 0, 0, 0);

    for (int i = 1; i <= 0xFF; i++) {
      int r;
      int g;
      int b;
      int a;

      if (i < 8) {
        r = (i & 1) ? 0xFF : 0;
        g = (i & 2) ? 0xFF : 0;
        b = (i & 4) ? 0xFF : 0;
        a = 0x3F;
        }

      else
        switch (i & 0x88) {
          case 0x00:
            r = ((i & 1) ? 85 : 0) + ((i & 0x10) ? 170 : 0);
            g = ((i & 2) ? 85 : 0) + ((i & 0x20) ? 170 : 0);
            b = ((i & 4) ? 85 : 0) + ((i & 0x40) ? 170 : 0);
            a = 0xFF;
            break;

          case 0x08:
            r = ((i & 1) ? 85 : 0) + ((i & 0x10) ? 170 : 0);
            g = ((i & 2) ? 85 : 0) + ((i & 0x20) ? 170 : 0);
            b = ((i & 4) ? 85 : 0) + ((i & 0x40) ? 170 : 0);
            a = 0x7F;
            break;

          case 0x80:
            r = 127 + ((i & 1) ? 43 : 0) + ((i & 0x10) ? 85 : 0);
            g = 127 + ((i & 2) ? 43 : 0) + ((i & 0x20) ? 85 : 0);
            b = 127 + ((i & 4) ? 43 : 0) + ((i & 0x40) ? 85 : 0);
            a = 0xFF;
            break;

          case 0x88:
            r = ((i & 1) ? 43 : 0) + ((i & 0x10) ? 85 : 0);
            g = ((i & 2) ? 43 : 0) + ((i & 0x20) ? 85 : 0);
            b = ((i & 4) ? 43 : 0) + ((i & 0x40) ? 85 : 0);
            a = 0xFF;
            break;
          }

      mDefaultClut.mClut256[i] = RGBA(r, g, b, a);
      }
    //}}}
    mDefaultClut.mNext = NULL;
    }
  //}}}
  //{{{
  ~cSubtitleDecoder() {

    deleteRegions();
    deleteObjects();
    deleteCluts();

    free (mDisplayDefinition);

    while (mDisplayList) {
      sRegionDisplay* display = mDisplayList;
      mDisplayList = display->mNext;
      free (display);
      }
    }
  //}}}

  //{{{
  bool decode (const uint8_t* buf, int bufSize, cSubtitle* subtitle) {
  // return new cSubtitle if succesful

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
        return false;
        }
        //}}}

      switch (segmentType) {
        case 0x10:
          if (!parsePage (bufPtr, segmentLength))
            return false;
          break;

        case 0x11:
          if (!parseRegion (bufPtr, segmentLength))
            return false;
          break;

        case 0x12:
          if (!parseClut (bufPtr, segmentLength))
            return false;
          break;

        case 0x13:
          if (!parseObject (bufPtr, segmentLength))
            return false;
          break;

        case 0x14:
          if (!parseDisplayDefinition (bufPtr, segmentLength))
            return false;
          break;

        case 0x80:
          updateSubtitle (subtitle);
          return true;

        default:
          cLog::log (LOGERROR, "unknown seg:%x, pageId:%d, size:%d", segmentType, pageId, segmentLength);
          break;
        }

      bufPtr += segmentLength;
      }

    return false;
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

    void init (int objectId, int regionId, int xpos, int ypos) {
      mObjectId = objectId;
      mRegionId = regionId;
      xPos = xpos;
      yPos = ypos;
      mForegroundColor = 0;;
      mBackgroundColor = 0;
      mRegionListNext = nullptr;
      mObjectListNext = nullptr;
      }

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
    int mVersion;

    int mX;
    int mY;
    int mWidth;
    int mHeight;
    };
  //}}}

  //{{{
  struct sClut {
    int mId = 0;
    int mVersion = 0;

    uint32_t mClut4[4] = { 0 };
    uint32_t mClut16[16] = { 0 };
    uint32_t mClut256[256] = { 0 };

    sClut* mNext;
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
            sObject** object2Ptr = &mObjectList;
            sObject* object2 = *object2Ptr;

            while (object2 != object) {
              object2Ptr = &object2->mNext;
              object2 = *object2Ptr;
              }

            *object2Ptr = object2->mNext;
            free (object2);
            }
          }
        }

      region->mDisplayList = display->mRegionListNext;

      free (display);
      }

    }
  //}}}
  //{{{
  void deleteRegions() {

    while (mRegionList) {
      sRegion* region = mRegionList;

      mRegionList = region->next;
      deleteRegionDisplayList (region);

      free (region->pixelBuf);
      free (region);
      }
    }
  //}}}
  //{{{
  void deleteObjects() {

    while (mObjectList) {
      sObject* object = mObjectList;
      mObjectList = object->mNext;
      free (object);
      }
    }
  //}}}
  //{{{
  void deleteCluts() {

    while (mClutList) {
      sClut* clut = mClutList;
      mClutList = clut->mNext;
      free (clut);
      }
    }
  //}}}

  //{{{
  sClut* getClut (int clutId) {

    sClut* clut = mClutList;
    while (clut && (clut->mId != clutId))
      clut = clut->mNext;

    return clut;
    }
  //}}}
  //{{{
  sObject* getObject (int objectId) {

    sObject* object = mObjectList;
    while (object && (object->mId != objectId))
      object = object->mNext;

    return object;
    }
  //}}}
  //{{{
  sRegion* getRegion (int regionId) {

    sRegion* ptr = mRegionList;
    while (ptr && (ptr->mId != regionId))
      ptr = ptr->next;

    return ptr;
    }
  //}}}

  //{{{
  int parse2bit (const uint8_t** buf, int bufSize, uint8_t* dstBuf, int dstBufSize,
                 int nonModifyColor, int xPos, uint8_t* mapTable) {

    dstBuf += xPos;
    int dstPixels = xPos;

    cBitStream bitStream (*buf, bufSize);
    while ((bitStream.getBitsRead() < (bufSize * 8)) && (dstPixels < dstBufSize)) {
      int bits = bitStream.getBits (2);
      if (bits) {
        if (nonModifyColor != 1 || bits != 1) {
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

          if (nonModifyColor == 1 && bits == 1)
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
              *buf += bitStream.getBytesRead();
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

              if ((nonModifyColor == 1) && (bits == 1))
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

              if (nonModifyColor == 1 && bits == 1)
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

    *buf += bitStream.getBytesRead();
    return dstPixels;
    }
  //}}}
  //{{{
  int parse4bit (const uint8_t** buf, int bufSize, uint8_t* dstBuf, int dstBufSize,
                 int nonModifyColor, int xPos, uint8_t* mapTable) {

    dstBuf += xPos;
    int dstPixels = xPos;

    std::string str;

    cBitStream bitStream (*buf, bufSize);
    while ((bitStream.getBitsRead() < (bufSize * 8)) && (dstPixels < dstBufSize)) {
      int bits = bitStream.getBits (4);
      if (mRunDebug)
        str += "[4b:" + hex(bits,1);

      if (bits) {
        if (nonModifyColor != 1 || bits != 1)
          *dstBuf++ = mapTable ? mapTable[bits] : bits;
        dstPixels++;
        }

      else {
        bits = bitStream.getBit();
        if (mRunDebug)
          str += ",1b:" + hex(bits,1);
        if (bits == 0) {
          //{{{  simple runlength
          int runLength = bitStream.getBits (3);
          if (mRunDebug)
            str += ",3b:" + hex(runLength);
          if (runLength == 0) {
            if (mRunDebug)
              cLog::log (LOGINFO, str + "]");
            *buf += bitStream.getBytesRead();
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
          if (mRunDebug) str += ",1b:" + hex(bits,1);
          if (bits == 0) {
            //{{{  bits = 0
            int runBits = bitStream.getBits (2);
            if (mRunDebug)
              str += ",2b:" + hex(runBits,1);
            int runLength = runBits + 4;

            bits = bitStream.getBits (4);
            if (mRunDebug)
              str += ",4b:" + hex(bits,1);

            if (nonModifyColor == 1 && bits == 1)
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
            if (mRunDebug) str += ",2b:" + hex(bits,1);
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
              if (mRunDebug)
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
              if (mRunDebug)
                str += ",4b:" + hex(runBits,1) + ":run:" + dec(runLength);
              bits = bitStream.getBits (4);
              if (mRunDebug)
                str += ",4b:" + hex(bits,1);

              if (nonModifyColor == 1 && bits == 1)
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
              if (mRunDebug)
                str += ",8b:" + hex(runBits,2) + ":run:" + dec(runLength);
              bits = bitStream.getBits (4);
              if (mRunDebug)
                str += ",4b:" + hex(bits);

              if (nonModifyColor == 1 && bits == 1)
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
    if (mRunDebug) {
      str += "] [4b:" + hex(bits) + "]";;
      cLog::log (LOGINFO, str);
      }

    if (bits)
      cLog::log (LOGERROR, "line overflow");

    *buf += bitStream.getBytesRead();
    return dstPixels;
    }
  //}}}
  //{{{
  int parse8bit (const uint8_t** buf, int bufSize, uint8_t* dstBuf, int dstBufSize,
                 int nonModifyColor, int xPos) {

    dstBuf += xPos;
    int dstPixels = xPos;

    const uint8_t* bufEnd = *buf + bufSize;
    while ((*buf < bufEnd) && (dstPixels < dstBufSize)) {
      int bits = *(*buf)++;
      if (bits) {
        if (nonModifyColor != 1 || bits != 1)
          *dstBuf++ = bits;
        dstPixels++;
        }
      else {
        bits = *(*buf)++;
        int runLength = bits & 0x7f;
        if ((bits & 0x80) == 0) {
          if (runLength == 0)
            return dstPixels;
          bits = 0;
          }
        else
          bits = *(*buf)++;

        if ((nonModifyColor == 1) && (bits == 1))
          dstPixels += runLength;
        else {
          while (runLength-- > 0 && dstPixels < dstBufSize) {
            *dstBuf++ = bits;
            dstPixels++;
            }
          }
        }
      }

    if (*(*buf)++)
      cLog::log (LOGERROR, "line overflow");

    return dstPixels;
    }
  //}}}
  //{{{
  void parseObjectBlock (sObjectDisplay* display, const uint8_t* buf, int bufSize,
                         bool bottom, int nonModifyColor) {

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
      if (((*buf != 0xF0) && (xPos >= region->width)) ||
          (yPos >= region->height)) {
        //{{{  error return
        cLog::log (LOGERROR, "invalid object location %d %d %d %d %02x",
                              xPos, region->width, yPos, region->height, *buf);
        return;
        }
        //}}}

      int type = *buf++;
      int bufLeft = int(bufEnd - buf);
      uint8_t* pixelPtr = pixelBuf + (yPos * region->width);
      switch (type) {
        //{{{
        case 0x10: // 2 bit
          if (region->depth == 8)
            mapTable = map2to8;
          else if (region->depth == 4)
            mapTable = map2to4;
          else
            mapTable = NULL;

          xPos = parse2bit (&buf, bufLeft, pixelPtr, region->width, nonModifyColor, xPos, mapTable);
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

          xPos = parse4bit (&buf, bufLeft, pixelPtr, region->width, nonModifyColor, xPos, mapTable);
          break;
        //}}}
        //{{{

        case 0x12: // 8 bit
          if (region->depth < 8) {
            cLog::log (LOGERROR, "8-bit pixel string in %d-bit region!", region->depth);
            return;
            }

          xPos = parse8bit (&buf, bufLeft, pixelPtr, region->width, nonModifyColor, xPos);
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
          cLog::log (LOGINFO, "unknown objectBlock %x", type);
          break;
        }
      }

    region->hasComputedClut = 0;
    }
  //}}}

  //{{{
  bool parsePage (const uint8_t* buf, int bufSize) {

    if (mSegmentDebug || mPageDebug)
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

    if (mSegmentDebug || mPageDebug)
      cLog::log (LOGINFO, "- timeOut:" + dec(mTimeOut) + " state:" + dec(pageState));

    if ((pageState == 1) || (pageState == 2)) {
      //{{{  delete regions, objects, cluts
      deleteRegions();
      deleteObjects();
      deleteCluts();
      }
      //}}}

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

      if (!display) {
        display = (sRegionDisplay*)malloc (sizeof(*display));
        display->mNext = nullptr;
        }
      display->mRegionId = regionId;
      display->xPos = AVRB16(buf); buf += 2;
      display->yPos = AVRB16(buf); buf += 2;
      *tmpPtr = display->mNext;
      display->mNext = mDisplayList;

      mDisplayList = display;

      if (mPageDebug)
        cLog::log (LOGINFO, "- regionId:" + dec(regionId) +
                            " " + dec(display->xPos) + "," + dec(display->yPos));
      }

    while (tmpDisplayList) {
      //{{{  free tmpDisplayList
      sRegionDisplay* display = tmpDisplayList;
      tmpDisplayList = display->mNext;
      free (display);
      }
      //}}}

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

      int xpos = AVRB16(buf) & 0xFFF; buf += 2;
      int ypos = AVRB16(buf) & 0xFFF; buf += 2;
      auto display = (sObjectDisplay*)malloc (sizeof(sObjectDisplay));
      display->init (objectId, regionId, xpos, ypos);

      if (display->xPos >= region->width ||
        //{{{  error return
        display->yPos >= region->height) {
        cLog::log (LOGERROR, "Object outside region");
        free (display);
        return false;
        }
        //}}}

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

    if (mSegmentDebug || mClutDebug)
      cLog::log (LOGINFO, "clut segment");

    const uint8_t* bufEnd = buf + bufSize;

    int clutId = *buf++;
    int version = ((*buf++) >> 4) & 15;

    sClut* clut = getClut (clutId);
    if (!clut) {
      clut = (sClut*)malloc (sizeof(*clut));
      memcpy (clut, &mDefaultClut, sizeof(*clut));
      clut->mId = clutId;
      clut->mVersion = -1;
      clut->mNext = mClutList;
      mClutList = clut;
      }

    if (clut->mVersion != version) {
      clut->mVersion = version;
      while (buf + 4 < bufEnd) {
        int entryId = *buf++;
        int depth = (*buf) & 0xe0;
        if (depth == 0) {
          //{{{  error return
          cLog::log (LOGERROR, "Invalid clut depth 0x%x!n", *buf);
          return false;
          }
          //}}}

        int y, cr, cb, alpha;
        int fullRange = (*buf++) & 1;
        if (fullRange) {
          //{{{  full range
          y = *buf++;
          cr = *buf++;
          cb = *buf++;
          alpha = *buf++;
          }
          //}}}
        else {
          //{{{  not full range, ??? should lsb's be extended into mask ???
          y = buf[0] & 0xFC;
          cr = (((buf[0] & 3) << 2) | (((buf[1] >> 6) & 3)) << 4);
          cb = (buf[1] << 2) & 0xF0;
          alpha = (buf[1] << 6) & 0xC0;
          buf += 2;
          }
          //}}}
        if (y == 0)
          alpha = 0xff;

        int rAdd = FIX(1.40200 * 255.0 / 224.0) * (cr - 128) + ONE_HALF;
        int gAdd = -FIX(0.34414 * 255.0 / 224.0) * (cb - 128) - FIX(0.71414*255.0/224.0) * (cr - 128) + ONE_HALF;
        int bAdd = FIX(1.77200 * 255.0 / 224.0) * (cb - 128) + ONE_HALF;
        y = (y - 16) * FIX(255.0 / 219.0);
        int r = (y + rAdd) >> SCALEBITS;
        int g = (y + gAdd) >> SCALEBITS;
        int b = (y + bAdd) >> SCALEBITS;

        if ((depth & 0x80) && (entryId < 4))
          clut->mClut4[entryId] = RGBA(r, g, b, 0xFF - alpha);
        else if ((depth & 0x40) && (entryId < 16))
          clut->mClut16[entryId] = RGBA(r, g, b, 0xFF - alpha);
        else if (depth & 0x20)
          clut->mClut256[entryId] = RGBA(r, g, b, 0xFF - alpha);
        else
          cLog::log (LOGERROR, "clut error depth:" + hex(depth) + " entryId:" + hex(entryId));
        if (mClutDebug)
          cLog::log (LOGINFO, "- depth:" + hex(depth) +
                              " id:" + hex(entryId) +
                              (fullRange == 1 ? " fullRange" : "") +
                              " y:" + hex(y & 0xFF, 2) +
                              " cr:" + hex(cr & 0xFF, 2) +
                              " cb:" + hex(cb & 0xFF, 2) +
                              " r:" + hex(r & 0xFF, 2) +
                              " g:" + hex(g & 0xFF, 2) +
                              " b:" + hex(b & 0xFF, 2) +
                              " a:" + hex(0xFF - alpha, 2));
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
    int nonModifyColor = ((*buf++) >> 1) & 1;

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
        parseObjectBlock (display, block, topFieldLen, false, nonModifyColor);

        int bfl = bottomFieldLen;
        if (bottomFieldLen > 0)
          block = buf + topFieldLen;
        else
          bfl = topFieldLen;
        parseObjectBlock (display, block, bfl, true, nonModifyColor);
        }
      }
    else
      cLog::log (LOGERROR, "unknown object coding %d", codingMethod);

    return true;
    }
  //}}}
  //{{{
  bool parseDisplayDefinition (const uint8_t* buf, int bufSize) {

    if (mSegmentDebug || mDisplayDefinitionDebug)
      cLog::log (LOGINFO, "displayDefinition segment");

    if (bufSize < 5)
      return false;

    int infoByte = *buf++;
    int ddsVersion = infoByte >> 4;

    if (mDisplayDefinition &&
        (mDisplayDefinition->mVersion == ddsVersion))
      return true;

    if (!mDisplayDefinition)
      mDisplayDefinition = (sDisplayDefinition*)malloc (sizeof(*mDisplayDefinition));
    mDisplayDefinition->mVersion = ddsVersion;
    mDisplayDefinition->mX = 0;
    mDisplayDefinition->mY = 0;
    mDisplayDefinition->mWidth = AVRB16(buf) + 1; buf += 2;
    mDisplayDefinition->mHeight = AVRB16(buf) + 1; buf += 2;

    int displayWindow = infoByte & (1 << 3);
    if (displayWindow) {
      if (bufSize < 13)
        return false;

      mDisplayDefinition->mX = AVRB16(buf); buf += 2;
      mDisplayDefinition->mWidth  = AVRB16(buf) - mDisplayDefinition->mX + 1; buf += 2;
      mDisplayDefinition->mY = AVRB16(buf); buf += 2;
      mDisplayDefinition->mHeight = AVRB16(buf) - mDisplayDefinition->mY + 1; buf += 2;
      }

    if (mDisplayDefinitionDebug)
      cLog::log (LOGINFO, std::string(displayWindow != 0 ? "window" : "") +
                          " x:" + dec(mDisplayDefinition->mX) +
                          " y:" + dec(mDisplayDefinition->mY) +
                          " w:" + dec(mDisplayDefinition->mWidth) +
                          " h:" + dec(mDisplayDefinition->mHeight));
    return true;
    }
  //}}}

  //{{{
  bool updateSubtitle (cSubtitle* subtitle) {

    int offsetX = 0;
    int offsetY = 0;
    if (mDisplayDefinition) {
      offsetX = mDisplayDefinition->mX;
      offsetY = mDisplayDefinition->mY;
      }

    int i = 0;
    for (sRegionDisplay* display = mDisplayList; display; display = display->mNext) {
      sRegion* region = getRegion (display->mRegionId);
      if (!region || !region->dirty)
        continue;

      if (i >= subtitle->mRects.size())
        subtitle->mRects.push_back (new cSubtitle::cRectData());

      subtitle->mRects[i]->mX = display->xPos + offsetX;
      subtitle->mRects[i]->mY = display->yPos + offsetY;
      subtitle->mRects[i]->mWidth = region->width;
      subtitle->mRects[i]->mHeight = region->height;
      subtitle->mRects[i]->mNumColours = 1 << region->depth;

      auto clut = getClut (region->clut);
      if (!clut)
        clut = &mDefaultClut;

      subtitle->mRects[i]->mPixelData = (uint8_t*)realloc (subtitle->mRects[i]->mPixelData, region->pixelBufSize * 4);
      uint32_t* ptr = (uint32_t*) (subtitle->mRects[i]->mPixelData);
      for (int pix = 0; pix < region->pixelBufSize; pix++)
        switch (region->depth) {
          case 2:
            *ptr++ = clut->mClut4[region->pixelBuf[pix]];
            break;
          case 4:
            *ptr++ = clut->mClut16[region->pixelBuf[pix]];
            break;
          case 8:
            *ptr++ = clut->mClut256[region->pixelBuf[pix]];
            break;
          default:
            cLog::log (LOGERROR, "unknown depth:" + dec(region->depth));
          }

      subtitle->mChanged = true;
      i++;
      }

    // !!! should remove or disable unused rects !!!
    return true;
    }
  //}}}

  //  vars
  //{{{  debug flags
  const bool mBufferDebug = false;
  const bool mSegmentDebug = false;
  const bool mDisplayDefinitionDebug = false;
  const bool mPageDebug = false;
  const bool mBlockDebug = false;
  const bool mRunDebug = false;
  const bool mClutDebug = false;
  //}}}
  int mVersion = 0;
  int mTimeOut = 0;

  sRegion* mRegionList = nullptr;
  sObject* mObjectList = nullptr;
  sRegionDisplay* mDisplayList = nullptr;
  sDisplayDefinition* mDisplayDefinition = nullptr;

  sClut* mClutList = nullptr;
  sClut mDefaultClut = { 0 };
  int mClutCount2[257][256] = { 0 };
  };
