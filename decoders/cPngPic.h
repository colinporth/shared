// cPng.h
#pragma once
#include "iPic.h"
//{{{  defines
#define MAKE_BYTE(b) ((b) & 0xFF)
#define MAKE_DWORD(a,b,c,d) ((MAKE_BYTE(a) << 24) | (MAKE_BYTE(b) << 16) | (MAKE_BYTE(c) << 8) | MAKE_BYTE(d))
#define MAKE_DWORD_PTR(p) MAKE_DWORD((p)[0], (p)[1], (p)[2], (p)[3])

#define CHUNK_IHDR MAKE_DWORD('I','H','D','R')
#define CHUNK_IDAT MAKE_DWORD('I','D','A','T')
#define CHUNK_IEND MAKE_DWORD('I','E','N','D')

#define FIRST_LENGTH_CODE_INDEX 257
#define LAST_LENGTH_CODE_INDEX 285

#define NUM_DEFLATE_CODE_SYMBOLS 288  /*256 literals, the end code, some length codes, and 2 unused codes */
#define NUM_DISTANCE_SYMBOLS 32 /*the distance codes have their own symbols, 30 used, 2 unused */
#define NUM_CODE_LENGTH_CODES 19  /*the code length codes. 0-15: code lengths, 16: copy previous 3-6 times, 17: 3-10 zeros, 18: 11-138 zeros */
#define MAX_SYMBOLS 288 /* largest number of symbols used by any tree type */

#define DEFLATE_CODE_BITLEN 15
#define DISTANCE_BITLEN 15
#define CODE_LENGTH_BITLEN 7
#define MAX_BIT_LENGTH 15 /* largest bitlen used by any tree type */

#define DEFLATE_CODE_BUFFER_SIZE (NUM_DEFLATE_CODE_SYMBOLS * 2)
#define DISTANCE_BUFFER_SIZE (NUM_DISTANCE_SYMBOLS * 2)
#define CODE_LENGTH_BUFFER_SIZE (NUM_DISTANCE_SYMBOLS * 2)

#define upng_chunk_length(chunk) MAKE_DWORD_PTR(chunk)
#define upng_chunk_type(chunk) MAKE_DWORD_PTR((chunk) + 4)
#define upng_chunk_critical(chunk) (((chunk)[4] & 32) == 0)
//}}}

class cPngPic : public iPic {
public:
  //{{{
  enum ePngError {
    PNG_EOK           = 0, // success (no error)
    PNG_ENOMEM        = 1, // memory allocation failed
    PNG_ENOTFOUND     = 2, // resource not found (file missing)
    PNG_ENOTPNG       = 3, // image data does not have a PNG header
    PNG_EMALFORMED    = 4, // image data is not a valid PNG image
    PNG_EUNSUPPORTED  = 5, // critical PNG chunk type is not supported
    PNG_EUNINTERLACED = 6, // image interlacing is not supported
    PNG_EUNFORMAT     = 7, // image color format is not supported
    PNG_EPARAM        = 8  // invalid parameter to method call
    } ;
  //}}}
  //{{{
  enum ePngFormat {
    PNG_BADFORMAT,
    PNG_RGB8,
    PNG_RGB16,
    PNG_RGBA8,
    PNG_RGBA16,
    PNG_LUMINANCE1,
    PNG_LUMINANCE2,
    PNG_LUMINANCE4,
    PNG_LUMINANCE8,
    PNG_LUMINANCE_ALPHA1,
    PNG_LUMINANCE_ALPHA2,
    PNG_LUMINANCE_ALPHA4,
    PNG_LUMINANCE_ALPHA8
    };
  //}}}
  //{{{
  enum ePngColor {
    PNG_LUM    = 0,
    PNG_RGB    = 2,
    PNG_LUMA   = 4,
    PNG_RGBA   = 6
    };
  //}}}
  cPngPic() {}
  cPngPic (const uint8_t* buffer) : mSrcBuffer(buffer) {}
  virtual ~cPngPic() {}
  void* operator new (std::size_t size) { return malloc (size); }
  void operator delete (void *ptr) { free (ptr); }

  // iPic
  virtual uint16_t getWidth() { return mWidth; }
  virtual uint16_t getHeight() { return mHeight; }
  //{{{
  virtual uint16_t getComponents() {
    switch (mColourType) {
      case PNG_LUM:
        return 1;
      case PNG_RGB:
        return 3;
      case PNG_LUMA:
        return 2;
      case PNG_RGBA:
        return 4;
      default:
        return 0;
      }
    }
  //}}}
  virtual uint8_t* getPic() { return (uint8_t*)mPicBuf; }
  virtual void setPic (const uint8_t* buffer, int size, uint16_t components) {}

  uint16_t getBitDepth() { return mColourDepth; }
  ePngFormat getFormat() { return mFormat; }
  //{{{
  uint16_t getPixelSize() {
    uint32_t bits = getBitDepth() * getComponents();
    bits += bits % 8;
    return bits;
    }
  //}}}
  uint16_t getBpp() { return getBitDepth() * getComponents(); }
  ePngError getError() { return mError; }

  //{{{
  bool readHeader() {
  //read the information from the header and store it in the upng_Info. return value is error

    /* check that PNG header matches expected value */
    if (mSrcBuffer[0] != 137 || mSrcBuffer[1] != 80 || mSrcBuffer[2] != 78 || mSrcBuffer[3] != 71 ||
        mSrcBuffer[4] != 13  || mSrcBuffer[5] != 10 || mSrcBuffer[6] != 26 || mSrcBuffer[7] != 10) {
      mError = PNG_ENOTPNG;
      return false;
      }

    // check that the first chunk is the IHDR chunk
    if (MAKE_DWORD_PTR(mSrcBuffer + 12) != CHUNK_IHDR) {
      mError = PNG_EMALFORMED;
      return false;
      }

    // read the values given in the header
    mWidth = MAKE_DWORD_PTR(mSrcBuffer + 16);
    mHeight = MAKE_DWORD_PTR(mSrcBuffer + 20);
    mColourDepth = mSrcBuffer[24];
    mColourType = (ePngColor)mSrcBuffer[25];

    // determine our color format
    mFormat = determineFormat();
    if (mFormat == PNG_BADFORMAT) {
      mError = PNG_EUNFORMAT;
      return false;
      }

    return mError == PNG_EOK;
    }
  //}}}
  //{{{
  uint8_t* decodeBody() {
  /*read a PNG, the result will be in the same color type as the PNG (hence "generic")*/

    // point to first byte of first chunk after header
    auto chunk = mSrcBuffer + 33;
    //{{{  scan chunks, find size of all IDAT chunks, verify well-formed-ness
    uint32_t compressed_size = 0;
    while (true) {
      uint32_t length;

      /* get length; sanity check it */
      length = upng_chunk_length (chunk);
      if (length > INT_MAX) {
        mError = PNG_EMALFORMED;
        return nullptr;
        }

      /* parse chunks */
      if (upng_chunk_type (chunk) == CHUNK_IDAT)
        compressed_size += length;
      else if (upng_chunk_type (chunk) == CHUNK_IEND)
        break;
      else if (upng_chunk_critical (chunk)) {
        mError = PNG_EUNSUPPORTED;
        return nullptr;
        }

      chunk += upng_chunk_length(chunk) + 12;
      }
    //}}}

    auto compressed = (uint8_t*)malloc (compressed_size);
    if (compressed == NULL) {
      //{{{  error
      mError = PNG_ENOMEM;
      return nullptr;
      }
      //}}}
    //{{{  scan again, copy chunks into compressed
    // scan through the chunks again, copying into our compressed buffer
    chunk = mSrcBuffer + 33;
    uint32_t compressed_index = 0;
    while (true) {
      uint32_t length = upng_chunk_length (chunk);
      auto data = chunk + 8;

      // parse chunks
      if (upng_chunk_type(chunk) == CHUNK_IDAT) {
        memcpy (compressed + compressed_index, data, length);
        compressed_index += length;
        }
      else if (upng_chunk_type (chunk) == CHUNK_IEND)
        break;

      chunk += upng_chunk_length (chunk) + 12;
      }
    //}}}

    // allocate inflated,filtered image
    uint32_t inflated_size = ((mWidth * (mHeight * getBpp() + 7)) / 8) + mHeight;
    auto inflated = (uint8_t*)malloc (inflated_size);
    if (inflated == NULL) {
      //{{{  error
      free (compressed);
      mError = PNG_ENOMEM;
      return nullptr;
      }
      //}}}

    // decompress image data
    mError = uzInflate (inflated, inflated_size, compressed, compressed_size);
    if (mError != PNG_EOK) {
      free (compressed);
      free (inflated);
      return nullptr;
      }
    // free compressed image
    free (compressed);

    // allocate final image
    mPicSize = (mHeight * mWidth * getBpp() + 7) / 8;
    mPicBuf = (uint8_t*)malloc (mPicSize);
    if (mPicBuf == NULL) {
      //{{{  error
      free (inflated);
      mPicSize = 0;
      mError = PNG_ENOMEM;
      return nullptr;
      }
      //}}}

    // unfilter scanlines
    postProcessScanlines (mPicBuf, inflated);
    free (inflated);

    if (mError != PNG_EOK) {
      free (mPicBuf);
      mPicBuf = NULL;
      mPicSize = 0;
      }

    auto rgbaBuf = (uint32_t*)mPicBuf;
    for (int i = 0; i < mWidth * mHeight; i++)
      rgbaBuf[i] = (rgbaBuf[i] & 0xFF00FF00) | ((rgbaBuf[i] & 0x000000FF) << 16) | ((rgbaBuf[i] & 0x00FF0000) >> 16);

    return mPicBuf;
    }
  //}}}

protected:
  const uint8_t* mSrcBuffer = nullptr;

private:
  //{{{  const
  //{{{
  const uint32_t LENGTH_BASE[29] = {
  /*the base lengths represented by codes 257-285 */
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59,
    67, 83, 99, 115, 131, 163, 195, 227, 258
  };
  //}}}
  //{{{
  const uint32_t LENGTH_EXTRA[29] = {
  /*the extra bits used by codes 257-285 (added to base length) */
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5,
    5, 5, 5, 0
  };
  //}}}
  //{{{
  const uint32_t DISTANCE_BASE[30] = {
  /*the base backwards distances (the bits of distance codes appear after length codes and use their own huffman tree) */
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513,
    769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
  };
  //}}}
  //{{{
  const uint32_t DISTANCE_EXTRA[30] = {
  /*the extra bits of backwards distances (added to base) */
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10,
    11, 11, 12, 12, 13, 13
  };
  //}}}
  //{{{
  const uint32_t CLCL[NUM_CODE_LENGTH_CODES]
  /*the order in which "code length alphabet code lengths" are stored, out of this the huffman tree of the dynamic huffman tree lengths is generated */
  = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
  //}}}
  //{{{
  const uint32_t FIXED_DEFLATE_CODE_TREE[NUM_DEFLATE_CODE_SYMBOLS * 2] = {
    289, 370, 290, 307, 546, 291, 561, 292, 293, 300, 294, 297, 295, 296, 0, 1,
    2, 3, 298, 299, 4, 5, 6, 7, 301, 304, 302, 303, 8, 9, 10, 11, 305, 306, 12,
    13, 14, 15, 308, 339, 309, 324, 310, 317, 311, 314, 312, 313, 16, 17, 18,
    19, 315, 316, 20, 21, 22, 23, 318, 321, 319, 320, 24, 25, 26, 27, 322, 323,
    28, 29, 30, 31, 325, 332, 326, 329, 327, 328, 32, 33, 34, 35, 330, 331, 36,
    37, 38, 39, 333, 336, 334, 335, 40, 41, 42, 43, 337, 338, 44, 45, 46, 47,
    340, 355, 341, 348, 342, 345, 343, 344, 48, 49, 50, 51, 346, 347, 52, 53,
    54, 55, 349, 352, 350, 351, 56, 57, 58, 59, 353, 354, 60, 61, 62, 63, 356,
    363, 357, 360, 358, 359, 64, 65, 66, 67, 361, 362, 68, 69, 70, 71, 364,
    367, 365, 366, 72, 73, 74, 75, 368, 369, 76, 77, 78, 79, 371, 434, 372,
    403, 373, 388, 374, 381, 375, 378, 376, 377, 80, 81, 82, 83, 379, 380, 84,
    85, 86, 87, 382, 385, 383, 384, 88, 89, 90, 91, 386, 387, 92, 93, 94, 95,
    389, 396, 390, 393, 391, 392, 96, 97, 98, 99, 394, 395, 100, 101, 102, 103,
    397, 400, 398, 399, 104, 105, 106, 107, 401, 402, 108, 109, 110, 111, 404,
    419, 405, 412, 406, 409, 407, 408, 112, 113, 114, 115, 410, 411, 116, 117,
    118, 119, 413, 416, 414, 415, 120, 121, 122, 123, 417, 418, 124, 125, 126,
    127, 420, 427, 421, 424, 422, 423, 128, 129, 130, 131, 425, 426, 132, 133,
    134, 135, 428, 431, 429, 430, 136, 137, 138, 139, 432, 433, 140, 141, 142,
    143, 435, 483, 436, 452, 568, 437, 438, 445, 439, 442, 440, 441, 144, 145,
    146, 147, 443, 444, 148, 149, 150, 151, 446, 449, 447, 448, 152, 153, 154,
    155, 450, 451, 156, 157, 158, 159, 453, 468, 454, 461, 455, 458, 456, 457,
    160, 161, 162, 163, 459, 460, 164, 165, 166, 167, 462, 465, 463, 464, 168,
    169, 170, 171, 466, 467, 172, 173, 174, 175, 469, 476, 470, 473, 471, 472,
    176, 177, 178, 179, 474, 475, 180, 181, 182, 183, 477, 480, 478, 479, 184,
    185, 186, 187, 481, 482, 188, 189, 190, 191, 484, 515, 485, 500, 486, 493,
    487, 490, 488, 489, 192, 193, 194, 195, 491, 492, 196, 197, 198, 199, 494,
    497, 495, 496, 200, 201, 202, 203, 498, 499, 204, 205, 206, 207, 501, 508,
    502, 505, 503, 504, 208, 209, 210, 211, 506, 507, 212, 213, 214, 215, 509,
    512, 510, 511, 216, 217, 218, 219, 513, 514, 220, 221, 222, 223, 516, 531,
    517, 524, 518, 521, 519, 520, 224, 225, 226, 227, 522, 523, 228, 229, 230,
    231, 525, 528, 526, 527, 232, 233, 234, 235, 529, 530, 236, 237, 238, 239,
    532, 539, 533, 536, 534, 535, 240, 241, 242, 243, 537, 538, 244, 245, 246,
    247, 540, 543, 541, 542, 248, 249, 250, 251, 544, 545, 252, 253, 254, 255,
    547, 554, 548, 551, 549, 550, 256, 257, 258, 259, 552, 553, 260, 261, 262,
    263, 555, 558, 556, 557, 264, 265, 266, 267, 559, 560, 268, 269, 270, 271,
    562, 565, 563, 564, 272, 273, 274, 275, 566, 567, 276, 277, 278, 279, 569,
    572, 570, 571, 280, 281, 282, 283, 573, 574, 284, 285, 286, 287, 0, 0
  };
  //}}}
  //{{{
  const uint32_t FIXED_DISTANCE_TREE[NUM_DISTANCE_SYMBOLS * 2] = {
    33, 48, 34, 41, 35, 38, 36, 37, 0, 1, 2, 3, 39, 40, 4, 5, 6, 7, 42, 45, 43,
    44, 8, 9, 10, 11, 46, 47, 12, 13, 14, 15, 49, 56, 50, 53, 51, 52, 16, 17,
    18, 19, 54, 55, 20, 21, 22, 23, 57, 60, 58, 59, 24, 25, 26, 27, 61, 62, 28,
    29, 30, 31, 0, 0
  };
  //}}}
  //}}}
  //{{{
  typedef struct tHuffTree {
    uint32_t* tree2d;
    uint32_t maxbitlen; /*maximum number of bits a single code can get */
    uint32_t numcodes;  /*number of symbols in the alphabet = number of codes */
    } tHuffTree;
  //}}}

  //{{{
  uint8_t readBit (uint32_t* bitPointer, const uint8_t* bitstream) {

    uint8_t result = (uint8_t)((bitstream [(*bitPointer) >> 3] >> ((*bitPointer) & 0x7)) & 1);
    (*bitPointer)++;

    return result;
    }
  //}}}
  //{{{
  uint32_t readBits (uint32_t* bitPointer, const uint8_t* bitstream, uint32_t nbits) {

    uint32_t result = 0;
    for (uint32_t i = 0; i < nbits; i++)
      result |= ((uint32_t)readBit (bitPointer, bitstream)) << i;

    return result;
    }
  //}}}
  //{{{
  ePngFormat determineFormat() {

    switch (mColourType) {
      //{{{
      case PNG_LUM:
        switch (mColourDepth) {
          case 1:
            return PNG_LUMINANCE1;
          case 2:
            return PNG_LUMINANCE2;
          case 4:
            return PNG_LUMINANCE4;
          case 8:
            return PNG_LUMINANCE8;
          default:
            return PNG_BADFORMAT;
          }
      //}}}
      //{{{
      case PNG_RGB:
        switch (mColourDepth) {
          case 8:
            return PNG_RGB8;
          case 16:
            return PNG_RGB16;
          default:
            return PNG_BADFORMAT;
          }
      //}}}
      //{{{
      case PNG_LUMA:
        switch (mColourDepth) {
          case 1:
            return PNG_LUMINANCE_ALPHA1;
          case 2:
            return PNG_LUMINANCE_ALPHA2;
          case 4:
            return PNG_LUMINANCE_ALPHA4;
          case 8:
            return PNG_LUMINANCE_ALPHA8;
          default:
            return PNG_BADFORMAT;
          }
      //}}}
      //{{{
      case PNG_RGBA:
        switch (mColourDepth) {
          case 8:
            return PNG_RGBA8;
          case 16:
            return PNG_RGBA16;
          default:
            return PNG_BADFORMAT;
          }
      //}}}
      default:
        return PNG_BADFORMAT;
      }
    }
  //}}}

  //{{{
  void huffTreeInit (tHuffTree* tree, uint32_t* buffer, uint32_t numcodes, uint32_t maxbitlen) {
  /* the buffer must be numcodes*2 in size! */

    tree->tree2d = buffer;
    tree->numcodes = numcodes;
    tree->maxbitlen = maxbitlen;
    }
  //}}}
  //{{{
  void huffTreeCreateLengths (tHuffTree* tree, const uint32_t* bitlen) {
  /*given the code lengths (as stored in the PNG file), generate the tree as defined by Deflate. maxbitlen is the maximum bits that a code in the tree can have. return value is error.*/

    uint32_t tree1d[MAX_SYMBOLS];
    uint32_t blcount[MAX_BIT_LENGTH];
    uint32_t nextcode[MAX_BIT_LENGTH+1];
    uint32_t bits, n, i;
    uint32_t nodefilled = 0;  /*up to which node it is filled */
    uint32_t treepos = 0; /*position in the tree (1 of the numcodes columns) */

    // initialize local vectors
    memset (blcount, 0, sizeof(blcount));
    memset (nextcode, 0, sizeof(nextcode));

    // step 1: count number of instances of each code length
    for (bits = 0; bits < tree->numcodes; bits++)
      blcount[bitlen[bits]]++;

    // step 2: generate the nextcode values
    for (bits = 1; bits <= tree->maxbitlen; bits++)
      nextcode[bits] = (nextcode[bits - 1] + blcount[bits - 1]) << 1;

    // step 3: generate all the codes
    for (n = 0; n < tree->numcodes; n++)
      if (bitlen[n] != 0)
        tree1d[n] = nextcode[bitlen[n]]++;

    // convert tree1d[] to tree2d[][]. In the 2D array, a value of 32767 means uninited,
    // a value >= numcodes is an address to another bit, a value < numcodes is a code.
    // The 2 rows are the 2 possible bit values (0 or 1), there are as many columns as codes - 1
    // a good huffmann tree has N * 2 - 1 nodes, of which N - 1 are internal nodes.
    // Here, the internal nodes are stored (what their 0 and 1 option point to).
    // There is only memory for such good tree currently,
    // if there are more nodes (due to too long length codes), error 55 will happen
    for (n = 0; n < tree->numcodes * 2; n++)
      tree->tree2d[n] = 32767;  /*32767 here means the tree2d isn't filled there yet */

    for (n = 0; n < tree->numcodes; n++) {  /*the codes */
      for (i = 0; i < bitlen[n]; i++) { /*the bits for this code */
        uint8_t bit = (uint8_t)((tree1d[n] >> (bitlen[n] - i - 1)) & 1);
        /* check if oversubscribed */
        if (treepos > tree->numcodes - 2) {
          mError = PNG_EMALFORMED;
          return;
          }

        if (tree->tree2d[2 * treepos + bit] == 32767) { /*not yet filled in */
          if (i + 1 == bitlen[n]) { /*last bit */
            tree->tree2d[2 * treepos + bit] = n;  /*put the current code in it */
            treepos = 0;
            }
          else {  /*put address of the next step in here, first that address has to be found of course (it's just nodefilled + 1)... */
            nodefilled++;
            tree->tree2d[2 * treepos + bit] = nodefilled + tree->numcodes;  /*addresses encoded with numcodes added to it */
            treepos = nodefilled;
            }
          }
        else
          treepos = tree->tree2d[2 * treepos + bit] - tree->numcodes;
        }
      }

    for (n = 0; n < tree->numcodes * 2; n++)
      if (tree->tree2d[n] == 32767)
        tree->tree2d[n] = 0;  /*remove possible remaining 32767's */
    }
  //}}}
  //{{{
  uint32_t huffDecodeSymbol (const uint8_t* in, uint32_t* bp, const tHuffTree* codetree, uint32_t inlength) {

    uint32_t treepos = 0;
    while (true) {
      /* error: end of input memory reached without endcode */
      if (((*bp) & 0x07) == 0 && ((*bp) >> 3) > inlength) {
        mError = PNG_EMALFORMED;
        return 0;
        }

      uint8_t bit = readBit(bp, in);
      uint32_t ct = codetree->tree2d[(treepos << 1) | bit];
      if (ct < codetree->numcodes)
        return ct;

      treepos = ct - codetree->numcodes;
      if (treepos >= codetree->numcodes) {
        mError = PNG_EMALFORMED;
        return 0;
        }
      }
    }
  //}}}
  //{{{
  void getTreeInflateDynamic (tHuffTree* codetree, tHuffTree* codetreeD, tHuffTree* codelengthcodetree,
                              const uint8_t* in, uint32_t* bp, uint32_t inlength) {
  /* get the tree of a deflated block with dynamic tree, the tree itself is also Huffman compressed with a known tree*/

    uint32_t codelengthcode[NUM_CODE_LENGTH_CODES];
    uint32_t bitlen[NUM_DEFLATE_CODE_SYMBOLS];
    uint32_t bitlenD[NUM_DISTANCE_SYMBOLS];
    uint32_t n, hlit, hdist, hclen, i;

    /*make sure that length values that aren't filled in will be 0, or a wrong tree will be generated */
    /*C-code note: use no "return" between ctor and dtor of an uivector! */
    if ((*bp) >> 3 >= inlength - 2) {
      mError = PNG_EMALFORMED;
      return;
      }

    /* clear bitlen arrays */
    memset (bitlen, 0, sizeof(bitlen));
    memset (bitlenD, 0, sizeof(bitlenD));

    /*the bit pointer is or will go past the memory */
    hlit = readBits (bp, in, 5) + 257;  /*number of literal/length codes + 257. Unlike the spec, the value 257 is added to it here already */
    hdist = readBits (bp, in, 5) + 1; /*number of distance codes. Unlike the spec, the value 1 is added to it here already */
    hclen = readBits (bp, in, 4) + 4; /*number of code length codes. Unlike the spec, the value 4 is added to it here already */

    for (i = 0; i < NUM_CODE_LENGTH_CODES; i++) {
      if (i < hclen)
        codelengthcode[CLCL[i]] = readBits(bp, in, 3);
      else
        codelengthcode[CLCL[i]] = 0;  /*if not, it must stay 0 */
      }

    huffTreeCreateLengths (codelengthcodetree, codelengthcode);

    /* bail now if we encountered an error earlier */
    if (mError != PNG_EOK)
      return;

    /*now we can use this tree to read the lengths for the tree that this function will return */
    i = 0;
    while (i < hlit + hdist) {  /*i is the current symbol we're reading in the part that contains the code lengths of lit/len codes and dist codes */
      uint32_t code = huffDecodeSymbol (in, bp, codelengthcodetree, inlength);
      if (mError != PNG_EOK) {
        break;
        }

      if (code <= 15) { /*a length code */
        if (i < hlit)
          bitlen[i] = code;
        else
          bitlenD[i - hlit] = code;
        i++;
        }
      else if (code == 16) {  /*repeat previous */
        uint32_t replength = 3; /*read in the 2 bits that indicate repeat length (3-6) */
        uint32_t value; /*set value to the previous code */

        if ((*bp) >> 3 >= inlength) {
          mError = PNG_EMALFORMED;
          break;
          }
        /*error, bit pointer jumps past memory */
        replength += readBits (bp, in, 2);

        if ((i - 1) < hlit)
          value = bitlen[i - 1];
        else
          value = bitlenD[i - hlit - 1];

        /*repeat this value in the next lengths */
        for (n = 0; n < replength; n++) {
          /* i is larger than the amount of codes */
          if (i >= hlit + hdist) {
            mError = PNG_EMALFORMED;
            break;
            }

          if (i < hlit)
            bitlen[i] = value;
          else
            bitlenD[i - hlit] = value;
          i++;
          }
        }
      else if (code == 17) {  /*repeat "0" 3-10 times */
        uint32_t replength = 3; /*read in the bits that indicate repeat length */
        if ((*bp) >> 3 >= inlength) {
          mError = PNG_EMALFORMED;
          break;
          }

        /*error, bit pointer jumps past memory */
        replength += readBits (bp, in, 3);

        /*repeat this value in the next lengths */
        for (n = 0; n < replength; n++) {
          /* error: i is larger than the amount of codes */
          if (i >= hlit + hdist) {
            mError = PNG_EMALFORMED;
            break;
            }

          if (i < hlit)
            bitlen[i] = 0;
          else
            bitlenD[i - hlit] = 0;
          i++;
          }
        }
      else if (code == 18) {  /*repeat "0" 11-138 times */
        uint32_t replength = 11;  /*read in the bits that indicate repeat length */
        /* error, bit pointer jumps past memory */
        if ((*bp) >> 3 >= inlength) {
          mError = PNG_EMALFORMED;
          break;
          }

        replength += readBits(bp, in, 7);

        /*repeat this value in the next lengths */
        for (n = 0; n < replength; n++) {
          /* i is larger than the amount of codes */
          if (i >= hlit + hdist) {
            mError = PNG_EMALFORMED;
            break;
            }
          if (i < hlit)
            bitlen[i] = 0;
          else
            bitlenD[i - hlit] = 0;
          i++;
          }
        }
      else {
        /* somehow an unexisting code appeared. This can never happen. */
        mError = PNG_EMALFORMED;
        break;
        }
      }

    if (mError == PNG_EOK && bitlen[256] == 0)
      mError = PNG_EMALFORMED;

    /*the length of the end code 256 must be larger than 0 */
    /*now we've finally got hlit and hdist, so generate the code trees, and the function is done */
    if (mError == PNG_EOK)
      huffTreeCreateLengths (codetree, bitlen);

    if (mError == PNG_EOK)
      huffTreeCreateLengths (codetreeD, bitlenD);
    }
  //}}}

  //{{{
  void inflateHuff (uint8_t* out, uint32_t outsize, const uint8_t *in, uint32_t *bp, uint32_t *pos, uint32_t inlength, uint32_t btype) {
  // inflate a block with dynamic of fixed Huffman tree

    uint32_t codetree_buffer[DEFLATE_CODE_BUFFER_SIZE];
    uint32_t codetreeD_buffer[DISTANCE_BUFFER_SIZE];
    uint32_t done = 0;

    tHuffTree codetree;
    tHuffTree codetreeD;

    if (btype == 1) {
      // fixed trees
      huffTreeInit (&codetree, (uint32_t*)FIXED_DEFLATE_CODE_TREE, NUM_DEFLATE_CODE_SYMBOLS, DEFLATE_CODE_BITLEN);
      huffTreeInit (&codetreeD, (uint32_t*)FIXED_DISTANCE_TREE, NUM_DISTANCE_SYMBOLS, DISTANCE_BITLEN);
      }
    else if (btype == 2) {
      // dynamic trees
      uint32_t codelengthcodetree_buffer[CODE_LENGTH_BUFFER_SIZE];
      tHuffTree codelengthcodetree;

      huffTreeInit (&codetree, codetree_buffer, NUM_DEFLATE_CODE_SYMBOLS, DEFLATE_CODE_BITLEN);
      huffTreeInit (&codetreeD, codetreeD_buffer, NUM_DISTANCE_SYMBOLS, DISTANCE_BITLEN);
      huffTreeInit (&codelengthcodetree, codelengthcodetree_buffer, NUM_CODE_LENGTH_CODES, CODE_LENGTH_BITLEN);
      getTreeInflateDynamic (&codetree, &codetreeD, &codelengthcodetree, in, bp, inlength);
      }

    while (done == 0) {
      uint32_t code = huffDecodeSymbol (in, bp, &codetree, inlength);
      if (mError != PNG_EOK)
        return;

      if (code == 256) // end code
        done = 1;
      else if (code <= 255) { // literal symbol
        if ((*pos) >= outsize) {
          mError = PNG_EMALFORMED;
          return;
          }

        // store output
        out[(*pos)++] = (uint8_t)(code);
        }
      else if (code >= FIRST_LENGTH_CODE_INDEX && code <= LAST_LENGTH_CODE_INDEX) { // length code
        // part 1: get length base */
        uint32_t length = LENGTH_BASE[code - FIRST_LENGTH_CODE_INDEX];
        uint32_t codeD, distance, numextrabitsD;
        uint32_t start, forward, backward, numextrabits;

        // part 2: get extra bits and add the value of that to length
        numextrabits = LENGTH_EXTRA[code - FIRST_LENGTH_CODE_INDEX];

        // error, bit pointer will jump past memory
        if (((*bp) >> 3) >= inlength) {
          mError = PNG_EMALFORMED;
          return;
          }
        length += readBits(bp, in, numextrabits);

        // part 3: get distance code
        codeD = huffDecodeSymbol(in, bp, &codetreeD, inlength);
        if (mError != PNG_EOK)
          return;

        // invalid distance code (30-31 are never used)
        if (codeD > 29) {
          mError = PNG_EMALFORMED;
          return;
          }

        distance = DISTANCE_BASE[codeD];

        // part 4: get extra bits from distance
        numextrabitsD = DISTANCE_EXTRA[codeD];

        // error, bit pointer will jump past memory
        if (((*bp) >> 3) >= inlength) {
          mError = PNG_EMALFORMED;
          return;
          }

        distance += readBits(bp, in, numextrabitsD);

        // part 5: fill in all the out[n] values based on the length and dist
        start = (*pos);
        backward = start - distance;

        if ((*pos) + length >= outsize) {
          mError = PNG_EMALFORMED;
          return;
          }

        for (forward = 0; forward < length; forward++) {
          out[(*pos)++] = out[backward];
          backward++;

          if (backward >= start)
            backward = start - distance;
          }
        }
      }
    }
  //}}}
  //{{{
  void inflateUncompressed (uint8_t* out, uint32_t outsize, const uint8_t* in, uint32_t* bp, uint32_t* pos, uint32_t inlength) {

    uint32_t p;
    uint32_t len, nlen, n;

    // go to first boundary of byte
    while (((*bp) & 0x7) != 0)
      (*bp)++;
    p = (*bp) / 8;    /*byte position */

    // read len (2 bytes) and nlen (2 bytes)
    if (p >= inlength - 4) {
      mError = PNG_EMALFORMED;
      return;
      }

    len = in[p] + 256 * in[p + 1];
    p += 2;
    nlen = in[p] + 256 * in[p + 1];
    p += 2;

    // check if 16-bit nlen is really the one's complement of len
    if (len + nlen != 65535) {
      mError = PNG_EMALFORMED;
      return;
      }

    if ((*pos) + len >= outsize) {
      mError = PNG_EMALFORMED;
      return;
      }

    /* read the literal data: len bytes are now stored in the out buffer */
    if (p + len > inlength) {
      mError = PNG_EMALFORMED;
      return;
      }

    for (n = 0; n < len; n++)
      out[(*pos)++] = in[p++];

    (*bp) = p * 8;
    }
  //}}}
  //{{{
  ePngError uzInflateData (uint8_t* out, uint32_t outsize, const uint8_t* in, uint32_t insize, uint32_t inpos) {
  // inflate the deflated data (cfr. deflate spec); return value is the error

    // bit pointer in the "in" data, current byte is bp >> 3, current bit is bp & 0x7 (from lsb to msb of the byte)
    uint32_t bp = 0;

    // byte position in the out buffer
    uint32_t pos = 0;

    uint32_t done = 0;
    while (done == 0) {
      // ensure next bit doesn't point past the end of the buffer
      if ((bp >> 3) >= insize) {
        mError = PNG_EMALFORMED;
        return mError;
        }

      // read block control bits */
      done = readBit (&bp, &in[inpos]);
      uint32_t btype = readBit (&bp, &in[inpos]) | (readBit(&bp, &in[inpos]) << 1);

      // process control type appropriateyly
      if (btype == 3) {
        mError = PNG_EMALFORMED;
        return mError;
        }
      else if (btype == 0)
        inflateUncompressed (out, outsize, &in[inpos], &bp, &pos, insize);  // no compression
      else
        inflateHuff (out, outsize, &in[inpos], &bp, &pos, insize, btype);  //compression, btype 01 or 10

      // stop if an error has occured */
      if (mError != PNG_EOK)
        return mError;
      }

    return mError;
    }
  //}}}
  //{{{
  ePngError uzInflate (uint8_t* out, uint32_t outsize, const uint8_t* in, uint32_t insize) {

    // we require two bytes for the zlib data header
    if (insize < 2) {
      mError = PNG_EMALFORMED;
      return mError;
      }

    // 256 * in[0] + in[1] must be a multiple of 31, the FCHECK value is supposed to be made that way
    if ((in[0] * 256 + in[1]) % 31 != 0) {
      mError = PNG_EMALFORMED;
      return mError;
      }

    // error: only compression method 8: inflate with sliding window of 32k is supported by the PNG spec
    if ((in[0] & 15) != 8 || ((in[0] >> 4) & 15) > 7) {
      mError = PNG_EMALFORMED;
      return mError;
      }

    // the specification of PNG says about the zlib stream: "The additional flags shall not specify a preset dictionary."
    if (((in[1] >> 5) & 1) != 0) {
      mError = PNG_EMALFORMED;
      return mError;
      }

    // create output buffer
    uzInflateData (out, outsize, in, insize, 2);

    return mError;
    }
  //}}}

  //{{{
  int paeth_predictor (int a, int b, int c) {
  /*Paeth predicter, used by PNG filter type 4*/

    int p = a + b - c;
    int pa = p > a ? p - a : a - p;
    int pb = p > b ? p - b : b - p;
    int pc = p > c ? p - c : c - p;

    if (pa <= pb && pa <= pc)
      return a;
    else if (pb <= pc)
      return b;
    else
      return c;
    }
  //}}}
  //{{{
  void unfilterScanline (uint8_t* recon, const uint8_t* scanline, const uint8_t* precon, uint32_t bytewidth, uint8_t filterType, uint32_t length) {
  // For PNG filter method 0
  //   unfilter a PNG image scanline by scanline. when the pixels are smaller than 1 byte, the filter works byte per byte (bytewidth = 1)
  //   precon is the previous unfiltered scanline, recon the result, scanline the current one
  //   the incoming scanlines do NOT include the filtertype byte, that one is given in the parameter filterType instead
  //   recon and scanline MAY be the same memory address! precon must be disjoint.

    uint32_t i;
    switch (filterType) {
      //{{{
      case 0:
        for (i = 0; i < length; i++)
          recon[i] = scanline[i];
        break;
      //}}}
      //{{{
      case 1:
        for (i = 0; i < bytewidth; i++)
          recon[i] = scanline[i];
        for (i = bytewidth; i < length; i++)
          recon[i] = scanline[i] + recon[i - bytewidth];
        break;
      //}}}
      //{{{
      case 2:
        if (precon)
          for (i = 0; i < length; i++)
            recon[i] = scanline[i] + precon[i];
        else
          for (i = 0; i < length; i++)
            recon[i] = scanline[i];
        break;
      //}}}
      //{{{
      case 3:
        if (precon) {
          for (i = 0; i < bytewidth; i++)
            recon[i] = scanline[i] + precon[i] / 2;
          for (i = bytewidth; i < length; i++)
            recon[i] = scanline[i] + ((recon[i - bytewidth] + precon[i]) / 2);
          }
        else {
          for (i = 0; i < bytewidth; i++)
            recon[i] = scanline[i];
          for (i = bytewidth; i < length; i++)
            recon[i] = scanline[i] + recon[i - bytewidth] / 2;
          }
        break;
      //}}}
      //{{{
      case 4:
        if (precon) {
          for (i = 0; i < bytewidth; i++)
            recon[i] = (uint8_t)(scanline[i] + paeth_predictor (0, precon[i], 0));
          for (i = bytewidth; i < length; i++)
            recon[i] = (uint8_t)(scanline[i] + paeth_predictor (recon[i - bytewidth], precon[i], precon[i - bytewidth]));
          }
        else {
          for (i = 0; i < bytewidth; i++)
            recon[i] = scanline[i];
          for (i = bytewidth; i < length; i++)
            recon[i] = (uint8_t)(scanline[i] + paeth_predictor (recon[i - bytewidth], 0, 0));
          }
        break;
      //}}}
      //{{{
      default:
        mError = PNG_EMALFORMED;
        break;
      //}}}
      }
    }
  //}}}
  //{{{
  void unfilter (uint8_t* out, const uint8_t* in, uint32_t w, uint32_t h, uint32_t bpp) {
  //  For PNG filter method 0
  //   this function unfilters a single image (e.g. without interlacing this is called once, with Adam7 it's called 7 times)
  //   out must have enough bytes allocated already, in must have the scanlines + 1 filtertype byte per scanline
  //   w and h are image dimensions or dimensions of reduced image, bpp is bpp per pixel
  //   in and out are allowed to be the same memory address!

    uint8_t* prevline = 0;

    /*bytewidth is used for filtering, is 1 when bpp < 8, number of bytes per pixel otherwise */
    uint32_t bytewidth = (bpp + 7) / 8;
    uint32_t linebytes = (w * bpp + 7) / 8;

    for (uint32_t y = 0; y < h; y++) {
      uint32_t outindex = linebytes * y;
      uint32_t inindex = (1 + linebytes) * y;  /*the extra filterbyte added to each row */
      uint8_t filterType = in[inindex];

      unfilterScanline (&out[outindex], &in[inindex + 1], prevline, bytewidth, filterType, linebytes);
      if (mError != PNG_EOK)
        return;

      prevline = &out[outindex];
      }
    }
  //}}}

  //{{{
  void removePaddingBits (uint8_t* out, const uint8_t *in, uint32_t olinebits, uint32_t ilinebits, uint32_t h) {
  // After filtering there are still padding bpp if scanlines have non multiple of 8 bit amounts. They need to be removed (except at last scanline of (Adam7-reduced) image) before working with pure image buffers for the Adam7 code, the color convert code and the output to the user.
  // in and out are allowed to be the same buffer, in may also be higher but still overlapping; in must have >= ilinebits*h bpp, out must have >= olinebits*h bpp, olinebits must be <= ilinebits
  // also used to move bpp after earlier such operations happened, e.g. in a sequence of reduced images from Adam7
  // only useful if (ilinebits - olinebits) is a value in the range 1..7

    uint32_t ibp = 0;
    uint32_t obp = 0;
    uint32_t diff = ilinebits - olinebits;

    for (uint32_t y = 0; y < h; y++) {
      for (uint32_t x = 0; x < olinebits; x++) {
        uint8_t bit = (uint8_t)((in[(ibp) >> 3] >> (7 - ((ibp) & 0x7))) & 1);
        ibp++;

        if (bit == 0)
          out[(obp) >> 3] &= (uint8_t)(~(1 << (7 - ((obp) & 0x7))));
        else
          out[(obp) >> 3] |= (1 << (7 - ((obp) & 0x7)));
        ++obp;
        }
      ibp += diff;
      }
    }
  //}}}
  //{{{
  void postProcessScanlines (uint8_t* out, uint8_t* in) {
  // out must be buffer big enough to contain full image, and in must contain the full decompressed data from the IDAT chunks

    uint32_t bpp = getBpp();
    if (bpp == 0) {
      mError = PNG_EMALFORMED;
      return;
      }

    if ((bpp < 8) && (mWidth * bpp != ((mWidth * bpp + 7) / 8) * 8)) {
      unfilter (in, in, mWidth, mHeight, bpp);
      if (mError != PNG_EOK)
        return;
      removePaddingBits (out, in, mWidth * bpp, ((mWidth * bpp + 7) / 8) * 8, mHeight);
      }
    else
      // filter into the out buffer
      unfilter (out, in, mWidth, mHeight, bpp);
    }
  //}}}

  // private vars
  uint16_t mWidth = 0;
  uint16_t mHeight = 0;

  uint16_t mColourDepth = 8;
  ePngFormat mFormat = PNG_RGBA8;
  ePngColor mColourType = PNG_RGBA;

  uint8_t* mPicBuf = nullptr;
  uint32_t mPicSize = 0;

  ePngError mError = PNG_EOK;
  };
