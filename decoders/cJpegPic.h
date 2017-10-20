// cJpeg.h - based on tiny jpeg decoder, jhead
#pragma once
#include "iPic.h"
//{{{  static const
static const int kBodyBufferSize = 0x200;
static const int kPoolBufferSize = 0xB00;
static const int kInputBufferSize = 0x10000;

static const int kBcB = (int)(1.772 * 1024);
static const int kGcB = (int)(0.344 * 1024);
static const int kGcR = (int)(0.714 * 1024);
static const int kRcR = (int)(1.402 * 1024);

static const int32_t M2  = (int32_t)(1.08239 * 4096);
static const int32_t M4  = (int32_t)(2.61313 * 4096);
static const int32_t M5  = (int32_t)(1.84776 * 4096);
static const int32_t M13 = (int32_t)(1.41421 * 4096);

//{{{
static const uint8_t  kClip8[1024] = {
  // 0..255
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
  64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
  96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
  128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
  160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
  192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
  224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,

  // 256..511
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,

  // -512..-257
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  // -256..-1
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
//}}}
//{{{
static const uint8_t  kZigZag[64] = {
   0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
  12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
  35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
  58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
  };
//}}}
//{{{
static const uint16_t kScaleFactor[64] = {
  (uint16_t)(1.00000*8192), (uint16_t)(1.38704*8192), (uint16_t)(1.30656*8192), (uint16_t)(1.17588*8192),
  (uint16_t)(1.00000*8192), (uint16_t)(0.78570*8192), (uint16_t)(0.54120*8192), (uint16_t)(0.27590*8192),
  (uint16_t)(1.38704*8192), (uint16_t)(1.92388*8192), (uint16_t)(1.81226*8192), (uint16_t)(1.63099*8192),
  (uint16_t)(1.38704*8192), (uint16_t)(1.08979*8192), (uint16_t)(0.75066*8192), (uint16_t)(0.38268*8192),
  (uint16_t)(1.30656*8192), (uint16_t)(1.81226*8192), (uint16_t)(1.70711*8192), (uint16_t)(1.53636*8192),
  (uint16_t)(1.30656*8192), (uint16_t)(1.02656*8192), (uint16_t)(0.70711*8192), (uint16_t)(0.36048*8192),
  (uint16_t)(1.17588*8192), (uint16_t)(1.63099*8192), (uint16_t)(1.53636*8192), (uint16_t)(1.38268*8192),
  (uint16_t)(1.17588*8192), (uint16_t)(0.92388*8192), (uint16_t)(0.63638*8192), (uint16_t)(0.32442*8192),
  (uint16_t)(1.00000*8192), (uint16_t)(1.38704*8192), (uint16_t)(1.30656*8192), (uint16_t)(1.17588*8192),
  (uint16_t)(1.00000*8192), (uint16_t)(0.78570*8192), (uint16_t)(0.54120*8192), (uint16_t)(0.27590*8192),
  (uint16_t)(0.78570*8192), (uint16_t)(1.08979*8192), (uint16_t)(1.02656*8192), (uint16_t)(0.92388*8192),
  (uint16_t)(0.78570*8192), (uint16_t)(0.61732*8192), (uint16_t)(0.42522*8192), (uint16_t)(0.21677*8192),
  (uint16_t)(0.54120*8192), (uint16_t)(0.75066*8192), (uint16_t)(0.70711*8192), (uint16_t)(0.63638*8192),
  (uint16_t)(0.54120*8192), (uint16_t)(0.42522*8192), (uint16_t)(0.29290*8192), (uint16_t)(0.14932*8192),
  (uint16_t)(0.27590*8192), (uint16_t)(0.38268*8192), (uint16_t)(0.36048*8192), (uint16_t)(0.32442*8192),
  (uint16_t)(0.27590*8192), (uint16_t)(0.21678*8192), (uint16_t)(0.14932*8192), (uint16_t)(0.07612*8192)
  };
//}}}

// exif
//{{{  tags x
// tags
#define TAG_INTEROP_INDEX          0x0001
#define TAG_INTEROP_VERSION        0x0002
//}}}
//{{{  tags 1xx
#define TAG_IMAGE_WIDTH            0x0100
#define TAG_IMAGE_LENGTH           0x0101
#define TAG_BITS_PER_SAMPLE        0x0102
#define TAG_COMPRESSION            0x0103
#define TAG_PHOTOMETRIC_INTERP     0x0106
#define TAG_FILL_ORDER             0x010A
#define TAG_DOCUMENT_NAME          0x010D
#define TAG_IMAGE_DESCRIPTION      0x010E
#define TAG_MAKE                   0x010F
#define TAG_MODEL                  0x0110
#define TAG_SRIP_OFFSET            0x0111
#define TAG_ORIENTATION            0x0112
#define TAG_SAMPLES_PER_PIXEL      0x0115
#define TAG_ROWS_PER_STRIP         0x0116
#define TAG_STRIP_BYTE_COUNTS      0x0117
#define TAG_X_RESOLUTION           0x011A
#define TAG_Y_RESOLUTION           0x011B
#define TAG_PLANAR_CONFIGURATION   0x011C
#define TAG_RESOLUTION_UNIT        0x0128
#define TAG_TRANSFER_FUNCTION      0x012D
#define TAG_SOFTWARE               0x0131
#define TAG_DATETIME               0x0132
#define TAG_ARTIST                 0x013B
#define TAG_WHITE_POINT            0x013E
#define TAG_PRIMARY_CHROMATICITIES 0x013F
#define TAG_TRANSFER_RANGE         0x0156
//}}}
//{{{  tags 2xx
#define TAG_JPEG_PROC              0x0200
#define TAG_THUMBNAIL_OFFSET       0x0201
#define TAG_THUMBNAIL_LENGTH       0x0202
#define TAG_Y_CB_CR_COEFFICIENTS   0x0211
#define TAG_Y_CB_CR_SUB_SAMPLING   0x0212
#define TAG_Y_CB_CR_POSITIONING    0x0213
#define TAG_REFERENCE_BLACK_WHITE  0x0214
//}}}
//{{{  tags 1xxx
#define TAG_RELATED_IMAGE_WIDTH    0x1001
#define TAG_RELATED_IMAGE_LENGTH   0x1002
//}}}
//{{{  tags 8xxx
#define TAG_CFA_REPEAT_PATTERN_DIM 0x828D
#define TAG_CFA_PATTERN1           0x828E
#define TAG_BATTERY_LEVEL          0x828F
#define TAG_COPYRIGHT              0x8298
#define TAG_EXPOSURETIME           0x829A
#define TAG_FNUMBER                0x829D
#define TAG_IPTC_NAA               0x83BB
#define TAG_EXIF_OFFSET            0x8769
#define TAG_INTER_COLOR_PROFILE    0x8773
#define TAG_EXPOSURE_PROGRAM       0x8822
#define TAG_SPECTRAL_SENSITIVITY   0x8824
#define TAG_GPSINFO                0x8825
#define TAG_ISO_EQUIVALENT         0x8827
#define TAG_OECF                   0x8828
//}}}
//{{{  tags 9xxx
#define TAG_EXIF_VERSION           0x9000
#define TAG_DATETIME_ORIGINAL      0x9003
#define TAG_DATETIME_DIGITIZED     0x9004
#define TAG_COMPONENTS_CONFIG      0x9101
#define TAG_CPRS_BITS_PER_PIXEL    0x9102
#define TAG_SHUTTERSPEED           0x9201
#define TAG_APERTURE               0x9202
#define TAG_BRIGHTNESS_VALUE       0x9203
#define TAG_EXPOSURE_BIAS          0x9204
#define TAG_MAXAPERTURE            0x9205
#define TAG_SUBJECT_DISTANCE       0x9206
#define TAG_METERING_MODE          0x9207
#define TAG_LIGHT_SOURCE           0x9208
#define TAG_FLASH                  0x9209
#define TAG_FOCALLENGTH            0x920A
#define TAG_SUBJECTAREA            0x9214
#define TAG_MAKER_NOTE             0x927C
#define TAG_USERCOMMENT            0x9286
#define TAG_SUBSEC_TIME            0x9290
#define TAG_SUBSEC_TIME_ORIG       0x9291
#define TAG_SUBSEC_TIME_DIG        0x9292

#define TAG_WINXP_TITLE            0x9c9b // Windows XP - not part of exif standard.
#define TAG_WINXP_COMMENT          0x9c9c // Windows XP - not part of exif standard.
#define TAG_WINXP_AUTHOR           0x9c9d // Windows XP - not part of exif standard.
#define TAG_WINXP_KEYWORDS         0x9c9e // Windows XP - not part of exif standard.
#define TAG_WINXP_SUBJECT          0x9c9f // Windows XP - not part of exif standard.
//}}}
//{{{  tags Axxx
#define TAG_FLASH_PIX_VERSION      0xA000
#define TAG_COLOR_SPACE            0xA001
#define TAG_PIXEL_X_DIMENSION      0xA002
#define TAG_PIXEL_Y_DIMENSION      0xA003
#define TAG_RELATED_AUDIO_FILE     0xA004
#define TAG_INTEROP_OFFSET         0xA005
#define TAG_FLASH_ENERGY           0xA20B
#define TAG_SPATIAL_FREQ_RESP      0xA20C
#define TAG_FOCAL_PLANE_XRES       0xA20E
#define TAG_FOCAL_PLANE_YRES       0xA20F
#define TAG_FOCAL_PLANE_UNITS      0xA210
#define TAG_SUBJECT_LOCATION       0xA214
#define TAG_EXPOSURE_INDEX         0xA215
#define TAG_SENSING_METHOD         0xA217
#define TAG_FILE_SOURCE            0xA300
#define TAG_SCENE_TYPE             0xA301
#define TAG_CFA_PATTERN            0xA302
#define TAG_CUSTOM_RENDERED        0xA401
#define TAG_EXPOSURE_MODE          0xA402
#define TAG_WHITEBALANCE           0xA403
#define TAG_DIGITALZOOMRATIO       0xA404
#define TAG_FOCALLENGTH_35MM       0xA405
#define TAG_SCENE_CAPTURE_TYPE     0xA406
#define TAG_GAIN_CONTROL           0xA407
#define TAG_CONTRAST               0xA408
#define TAG_SATURATION             0xA409
#define TAG_SHARPNESS              0xA40A
#define TAG_DISTANCE_RANGE         0xA40C
#define TAG_IMAGE_UNIQUE_ID        0xA420
//}}}
//{{{  format
#define FMT_BYTE       1
#define FMT_STRING     2
#define FMT_USHORT     3
#define FMT_ULONG      4
#define FMT_URATIONAL  5
#define FMT_SBYTE      6
#define FMT_UNDEFINED  7
#define FMT_SSHORT     8
#define FMT_SLONG      9
#define FMT_SRATIONAL 10
#define FMT_SINGLE    11
#define FMT_DOUBLE    12
//}}}

static const int kBytesPerFormat[] = { 0, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8 };
static const char* kWeekDay[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
//}}}
//{{{
class cExifInfo {
public:
  std::string mExifMake;
  std::string mExifModel;
  uint32_t mExifOrientation = 0;
  float mExifExposure = 0;
  float mExifFocalLength = 0;
  float mExifAperture = 0;
  tm mExifTm;
  std::string mExifTimeString;
  };
//}}}
//{{{
class cExifGpsInfo {
public:
  cExifGpsInfo() {}

  //{{{
  std::string getString() {

    std::stringstream stringstream;

    if (!mDatum.empty())
      stringstream << mDatum << ' ';
    if (mLatitudeDeg || mLatitudeMin || mLatitudeSec)
      stringstream << mLatitudeDeg << mLatitudeRef << mLatitudeMin << L"'" << mLatitudeSec << ' ';
    if (mLongitudeDeg || mLongitudeMin || mLongitudeSec)
      stringstream << mLongitudeDeg << mLongitudeRef << mLongitudeMin << L"'" << mLongitudeSec << ' ';

    if (mAltitude)
      stringstream << "alt:" << mAltitudeRef << mAltitude << ' ';
    if (mImageDirection)
      stringstream << "dir:" << mImageDirectionRef << mImageDirection << ' ';

    if (!mDatum.empty())
      stringstream << mDate << L' ';

    if (mHour || mMinute || mSecond)
      stringstream << mHour << ':' << mMinute << ':' << mSecond;

    return stringstream.str();
    }
  //}}}

  int mVersion = 0;

  wchar_t mLatitudeRef = 0;
  float mLatitudeDeg = 0;
  float mLatitudeMin = 0;
  float mLatitudeSec = 0;

  char mLongitudeRef = 0;
  float mLongitudeDeg = 0;
  float mLongitudeMin = 0;
  float mLongitudeSec = 0;

  wchar_t mAltitudeRef = 0;
  float mAltitude = 0;

  wchar_t mImageDirectionRef;
  float mImageDirection = 0;

  float mHour = 0;
  float mMinute = 0;
  float mSecond = 0;

  std::string mDate;
  std::string mDatum;
  };
//}}}

class cJpegPic : public iPic {
public:
  //{{{
  cJpegPic (uint16_t components, uint8_t* buffer) : mBuffer(buffer), mBufferPtr(buffer), mBytesPerPixel(components)  {

    memset (mQtable, 0, 4 * sizeof(int32_t));

    memset (mHuffBits, 0, 2 * 2);
    memset (mHuffData, 0, 2 * 2);
    memset (mHuffCode, 0, 2 * 2 * sizeof(uint16_t));

    mPoolBuffer = (uint8_t*)bigMalloc(kPoolBufferSize, "jpegPool");
    mInputBuffer = (uint8_t*)bigMalloc (kInputBufferSize, "jpegInputBuffer");
    }
  //}}}
  //{{{
  virtual ~cJpegPic() {
    bigFree (mPoolBuffer);
    bigFree (mInputBuffer);
    }
  //}}}
  void* operator new (std::size_t size) { return smallMalloc (size, "cJpegPic"); }
  void operator delete (void *ptr) { smallFree (ptr); }

  // iPic
  virtual uint16_t getWidth() { return mWidth; }
  virtual uint16_t getHeight() { return mHeight; }
  virtual uint16_t getComponents() { return mBytesPerPixel; }
  virtual uint8_t* getPic() { return mFrameBuffer; }
  virtual void setPic (const uint8_t* buffer, int size, uint16_t components) {};

  //{{{  gets
  uint32_t getPoolBytesLeft() { return mPoolBytesLeft; }

  int getThumbOffset() { return mThumbBytes > 0 ? mThumbOffset : 0; }
  int getThumbBytes() { return mThumbBytes; }
  //}}}
  //{{{
  bool readHeader() {

    mPoolPtr = mPoolBuffer;
    mPoolBytesLeft = kPoolBufferSize;
    mThumbOffset = 0;
    mThumbBytes = 0;

    //{{{  read first word, must be SOI marker
    if ((read (mInputBuffer, 2) != 2) || (mInputBuffer[0] != 0xFF) || (mInputBuffer[1] != 0xD8))
      return false;
    //}}}
    uint32_t offset = 2;
    while (true) {
      //{{{  read marker, length
      if (read (mInputBuffer, 4) != 4)
        return false;

      uint16_t marker = mInputBuffer[0]<<8 | mInputBuffer[1];
      uint16_t length = mInputBuffer[2]<<8 | mInputBuffer[3];
      if (((marker>>8) != 0xFF) || (length <= 2))
        return false;

      length -= 2;
      offset += 4;
      //}}}
      //{{{  read marker body into mInputBuffer
      if (read (mInputBuffer, length) != length)
        return false;

      offset += length;
      //}}}
      switch (marker & 0xFF) {
        case 0xE1: parseAPP (mInputBuffer, length); break;
        case 0xDD: parseDRI (mInputBuffer, length); break;
        case 0xC4: if (!parseHFT (mInputBuffer, length)) return false; break;
        case 0xDB: if (!parseDQT (mInputBuffer, length)) return false; break;
        case 0xC0: if (!parseSOF (mInputBuffer, length)) return false; break;
        case 0xDA: // SOS
          //{{{  parseSOS
          if (!parseSOS (mInputBuffer, length))
            return false;

          // Pre-load the JPEG data to extract it from the bit stream
          offset %= kBodyBufferSize;
          mDataCounter = offset ? read (mInputBuffer + length, kBodyBufferSize - offset) : 0;
          mDataPtr = offset ? mInputBuffer + length - 1 : mInputBuffer;
          mDataMask = 0;
          return true;
          //}}}
        case 0xC1: // SOF1
        case 0xC2: // SOF2
        case 0xC3: // SOF3
        case 0xC5: // SOF5
        case 0xC6: // SOF6
        case 0xC7: // SOF7
        case 0xC9: // SOF9
        case 0xCA: // SOF10
        case 0xCB: // SOF11
        case 0xCD: // SOF13
        case 0xCE: // SOF14
        case 0xCF: // SOF15
        case 0xD9: // EOI
          return false;
        default: ;  // ignore unknown marker
        }
      }

    return false;
    }
  //}}}

  //{{{
  uint8_t* decodeBody (uint8_t scaleShift) {

    mScaleShift = scaleShift;
    mFrameWidth = mWidth >> scaleShift;
    mFrameHeight = mHeight >> scaleShift;
    mFrameBuffer = (uint8_t*)bigMalloc (mFrameWidth * mFrameHeight * mBytesPerPixel, "jpegFrame");

    // Initialize DC values
    mDcValue[0] = 0;
    mDcValue[1] = 0;
    mDcValue[2] = 0;

    uint16_t restartCount = 0;
    uint16_t restartInterval = 0;

    // iterate MCUs
    for (uint32_t y = 0; y < mHeight; y += msy*8) {
      for (uint32_t x = 0; x < mWidth; x += msx*8) {
        if (mNumRst && restartInterval++ == mNumRst) {
          // process restart interval if DRI header found
          if (!restart (restartCount++))  {
            bigFree (mFrameBuffer);
            return nullptr;
            }
          restartInterval = 1;
          }

        // load MCU, decompress huffman coded stream and apply IDCT
        if (!mcuLoad()) {
          bigFree (mFrameBuffer);
          return nullptr;
          }

        // process MCU, color space conversion, scaling and output
        if (!mcuProcess (x, y)) {
          bigFree (mFrameBuffer);
          return nullptr;
          }
        }
      }

    //printf ("bytes left %d\n", mPoolBytesLeft);
    return mFrameBuffer;
    }
  //}}}
  // vars
  cExifInfo mExifInfo;
  cExifGpsInfo mExifGpsInfo;

protected:
  //{{{
  virtual uint32_t read (uint8_t* buffer, uint32_t bytes) {
    memcpy (buffer, mBufferPtr, bytes);
    mBufferPtr += bytes;
    return bytes;
    }
  //}}}
  uint8_t* mBuffer = nullptr;
  uint8_t* mBufferPtr = nullptr;

private:
  //{{{
  uint8_t* alloc (uint32_t bytes) {

    // align block size to the word boundary
    bytes = (bytes + 3) & ~3;
    mPoolBytesLeft -= bytes;
    auto allocation = mPoolPtr;
    mPoolPtr += bytes;

    // return allocated memory block
    return allocation;
    }
  //}}}

  //{{{
  uint16_t getExifWord (uint8_t* ptr, bool intelEndian) {

    return *(ptr + (intelEndian ? 1 : 0)) << 8 |  *(ptr + (intelEndian ? 0 : 1));
    }
  //}}}
  //{{{
  uint32_t getExifLong (uint8_t* ptr, bool intelEndian) {

    return getExifWord (ptr + (intelEndian ? 2 : 0), intelEndian) << 16 |
           getExifWord (ptr + (intelEndian ? 0 : 2), intelEndian);
    }
  //}}}
  //{{{
  float getExifSignedRational (uint8_t* ptr, bool intelEndian, uint32_t& numerator, uint32_t& denominator) {

    numerator = getExifLong (ptr, intelEndian);
    denominator = getExifLong (ptr+4, intelEndian);
    return (denominator == 0) ? 0 : (float)numerator / (float)denominator;
    }
  //}}}
  //{{{
  void getExifGpsInfo (uint8_t* ptr, uint8_t* offsetBasePtr, bool intelEndian) {

    auto numDirectoryEntries = getExifWord (ptr, intelEndian);
    ptr += 2;

    for (auto entry = 0; entry < numDirectoryEntries; entry++) {
      auto tag = getExifWord (ptr, intelEndian);
      auto format = getExifWord (ptr+2, intelEndian);
      auto components = getExifLong (ptr+4, intelEndian);
      auto offset = getExifLong (ptr+8, intelEndian);
      auto bytes = components * kBytesPerFormat[format];
      auto valuePtr = (bytes <= 4) ? ptr+8 : offsetBasePtr + offset;
      ptr += 12;

      uint32_t numerator;
      uint32_t denominator;
      switch (tag) {
        //{{{
        case 0x00:  // version
          mExifGpsInfo.mVersion = getExifLong (valuePtr, intelEndian);
          break;
        //}}}
        //{{{
        case 0x01:  // latitude ref
          mExifGpsInfo.mLatitudeRef = valuePtr[0];
          break;
        //}}}
        //{{{
        case 0x02:  // latitude
          mExifGpsInfo.mLatitudeDeg = getExifSignedRational (valuePtr, intelEndian, numerator, denominator);
          mExifGpsInfo.mLatitudeMin = getExifSignedRational (valuePtr + 8, intelEndian, numerator, denominator);
          mExifGpsInfo.mLatitudeSec = getExifSignedRational (valuePtr + 16, intelEndian, numerator, denominator);
          break;
        //}}}
        //{{{
        case 0x03:  // longitude ref
          mExifGpsInfo.mLongitudeRef = valuePtr[0];
          break;
        //}}}
        //{{{
        case 0x04:  // longitude
          mExifGpsInfo.mLongitudeDeg = getExifSignedRational (valuePtr, intelEndian, numerator, denominator);
          mExifGpsInfo.mLongitudeMin = getExifSignedRational (valuePtr + 8, intelEndian, numerator, denominator);
          mExifGpsInfo.mLongitudeSec = getExifSignedRational (valuePtr + 16, intelEndian, numerator, denominator);
          break;
        //}}}
        //{{{
        case 0x05:  // altitude ref
          mExifGpsInfo.mAltitudeRef = valuePtr[0];
          break;
        //}}}
        //{{{
        case 0x06:  // altitude
          mExifGpsInfo.mAltitude = getExifSignedRational (valuePtr, intelEndian, numerator, denominator);
          break;
        //}}}
        //{{{
        case 0x07:  // timeStamp
          mExifGpsInfo.mHour = getExifSignedRational (valuePtr, intelEndian, numerator, denominator),
          mExifGpsInfo.mMinute = getExifSignedRational (valuePtr + 8, intelEndian, numerator, denominator),
          mExifGpsInfo.mSecond = getExifSignedRational (valuePtr + 16, intelEndian, numerator, denominator);
          break;
        //}}}
        //{{{
        case 0x08:  // satellites
          printf ("TAG_gps_Satellites %s \n", valuePtr);
          break;
        //}}}
        //{{{
        case 0x0B:  // dop
          printf ("TAG_gps_DOP %f\n", getExifSignedRational (valuePtr, intelEndian, numerator, denominator));
          break;
        //}}}
        //{{{
        case 0x10:  // direction ref
          mExifGpsInfo.mImageDirectionRef = valuePtr[0];
          break;
        //}}}
        //{{{
        case 0x11:  // direction
          mExifGpsInfo.mImageDirection = getExifSignedRational (valuePtr, intelEndian, numerator, denominator);
          break;
        //}}}
        //{{{
        case 0x12:  // datum
          mExifGpsInfo.mDatum = (char*)valuePtr;
          break;
        //}}}
        //{{{
        case 0x1D:  // date
          mExifGpsInfo.mDate = (char*)valuePtr;
          break;
        //}}}
        //{{{
        case 0x1B:
          //printf ("TAG_gps_ProcessingMethod\n");
          break;
        //}}}
        //{{{
        case 0x1C:
          //printf ("TAG_gps_AreaInformation\n");
          break;
        //}}}
        case 0x09: printf ("TAG_gps_status\n"); break;
        case 0x0A: printf ("TAG_gps_MeasureMode\n"); break;
        case 0x0C: printf ("TAG_gps_SpeedRef\n"); break;
        case 0x0D: printf ("TAG_gps_Speed\n"); break;
        case 0x0E: printf ("TAG_gps_TrackRef\n"); break;
        case 0x0F: printf ("TAG_gps_Track\n"); break;
        case 0x13: printf ("TAG_gps_DestLatitudeRef\n"); break;
        case 0x14: printf ("TAG_gps_DestLatitude\n"); break;
        case 0x15: printf ("TAG_gps_DestLongitudeRef\n"); break;
        case 0x16: printf ("TAG_gps_DestLongitude\n"); break;
        case 0x17: printf ("TAG_gps_DestBearingRef\n"); break;
        case 0x18: printf ("TAG_gps_DestBearing\n"); break;
        case 0x19: printf ("TAG_gps_DestDistanceRef\n"); break;
        case 0x1A: printf ("TAG_gps_DestDistance\n"); break;
        case 0x1E: printf ("TAG_gps_Differential\n"); break;
        default: printf ("unknown gps tag %x\n", tag); break;
        }
      }
    }
  //}}}
  //{{{
  std::string getExifTime (uint8_t* ptr, struct tm* tmPtr) {
  // Convert exif time to Unix time structure

    // Check for format: YYYY:MM:DD HH:MM:SS format.
    // Date and time normally separated by a space, but also seen a ':' there, so
    // skip the middle space with '%*c' so it can be any character.
    tmPtr->tm_wday = -1;
    tmPtr->tm_sec = 0;
    sscanf ((char*)ptr, "%d%*c%d%*c%d%*c%d:%d:%d",
            &tmPtr->tm_year, &tmPtr->tm_mon, &tmPtr->tm_mday,
            &tmPtr->tm_hour, &tmPtr->tm_min, &tmPtr->tm_sec);

    tmPtr->tm_isdst = -1;
    tmPtr->tm_mon -= 1;     // Adjust for unix zero-based months
    tmPtr->tm_year -= 1900; // Adjust for year starting at 1900

    // find day of week
    mktime (tmPtr);

    std::string str = (char*)ptr;
    if ((tmPtr->tm_wday >= 0) && (tmPtr->tm_wday <= 6)) {
      str += " ";
      str += kWeekDay[tmPtr->tm_wday];
      }

    return str;
    }
  //}}}
  //{{{
  void parseExifDirectory (uint8_t* offsetBasePtr, uint8_t* ptr, bool intelEndian) {

    auto numDirectoryEntries = getExifWord (ptr, intelEndian);
    ptr += 2;

    for (auto entry = 0; entry < numDirectoryEntries; entry++) {
      auto tag = getExifWord (ptr, intelEndian);
      auto format = getExifWord (ptr+2, intelEndian);
      auto components = getExifLong (ptr+4, intelEndian);
      auto offset = getExifLong (ptr+8, intelEndian);
      auto bytes = components * kBytesPerFormat[format];
      auto valuePtr = (bytes <= 4) ? ptr+8 : offsetBasePtr + offset;
      ptr += 12;

      uint32_t numerator;
      uint32_t denominator;
      switch (tag) {
        case TAG_EXIF_OFFSET:
          parseExifDirectory (offsetBasePtr, offsetBasePtr + offset, intelEndian); break;
        case TAG_ORIENTATION:
          mExifInfo.mExifOrientation = offset; break;
        case TAG_APERTURE:
          mExifInfo.mExifAperture = (float)exp(getExifSignedRational (valuePtr, intelEndian, numerator, denominator)*log(2)*0.5); break;
        case TAG_FOCALLENGTH:
          mExifInfo.mExifFocalLength = getExifSignedRational (valuePtr, intelEndian, numerator, denominator); break;
        case TAG_EXPOSURETIME:
          mExifInfo.mExifExposure = getExifSignedRational (valuePtr, intelEndian, numerator, denominator); break;
        case TAG_MAKE:
          if (mExifInfo.mExifMake.empty())
            mExifInfo.mExifMake = (char*)valuePtr;
          break;
        case TAG_MODEL:
          if (mExifInfo.mExifModel.empty())
            mExifInfo.mExifModel = (char*)valuePtr;
          break;
        case TAG_DATETIME:
        case TAG_DATETIME_ORIGINAL:
        case TAG_DATETIME_DIGITIZED:
          if (mExifInfo.mExifTimeString.empty())
            mExifInfo.mExifTimeString = getExifTime (valuePtr, &mExifInfo.mExifTm);
          break;
        case TAG_THUMBNAIL_OFFSET:
          mThumbOffset = offset; break;
        case TAG_THUMBNAIL_LENGTH:
          mThumbBytes = offset; break;
        case TAG_GPSINFO:
          getExifGpsInfo (offsetBasePtr + offset, offsetBasePtr, intelEndian); break;
        //case TAG_MAXAPERTURE:
        //  printf ("TAG_MAXAPERTURE\n"); break;
        //case TAG_SHUTTERSPEED:
        //  printf ("TAG_SHUTTERSPEED\n"); break;
        default:;
        //  printf ("TAG %x\n", tag);
        }
      }

    auto extraDirectoryOffset = getExifLong (ptr, intelEndian);
    if (extraDirectoryOffset > 4)
      parseExifDirectory (offsetBasePtr, offsetBasePtr + extraDirectoryOffset, intelEndian);
    }
  //}}}

  //{{{
  bool parseAPP (uint8_t* ptr, uint32_t length) {
  // find and read APP1 EXIF marker, return true if thumb, valid mThumbBuffer, mThumbLength

    // check exifId
    if (getExifLong (ptr, false) != 0x45786966)
      return false;
    ptr += 4;

    // check 0 word
    if (getExifWord (ptr, false) != 0x0000)
      return false;
    ptr += 2;

    auto offsetBasePtr = ptr;

    // endian
    bool intelEndian = getExifWord (ptr, false) == 0x4949;
    ptr += 2;

    // 002a word
    if (getExifWord (ptr, intelEndian) != 0x002a)
      return false;
    ptr += 2;

    // firstOffset 8
    if (getExifLong (ptr, intelEndian) != 8)
      return false;
    ptr += 4;

    parseExifDirectory (offsetBasePtr, ptr, intelEndian);

    mThumbOffset += 6 + 6; // SOImarker(2), APP1marker(2), APP1length(2), EXIF00marker(6)
    return mThumbBytes > 0;
    }
  //}}}
  //{{{
  bool parseDQT (uint8_t* ptr, uint32_t length) {
  // create de-quantization and prescaling tables with a DQT segment

    while (length) {
      if (length < 65)
        return false;
      length -= 65;

      // table property
      auto d = *ptr++;
      if (d & 0xF0) // not 8-bit resolution
        return false;

      // table ID
      uint32_t i = d & 3;

      // alloc memory for table
      auto tablePtr = (int32_t*)alloc (64 * sizeof (int32_t));

      // register table
      mQtable[i] = tablePtr;
      for (uint32_t i = 0; i < 64; i++) {
        // load table, apply scale factor of Arai algorithm to the de-quantizers
        auto z = kZigZag[i];
        tablePtr[z] = (int32_t)((uint32_t)*ptr++ * kScaleFactor[z]);
        }
      }

    return true;
    }
  //}}}
  //{{{
  bool parseHFT (uint8_t* ptr, uint32_t length) {
  // Create huffman code tables with a DHT segment

    while (length) {
      if (length < 17)
        return false;
      length -= 17;

      // table number and class
      uint8_t d = *ptr++;
      uint32_t cls = (d >> 4);

      // class = dc(0)/ac(1), table number = 0/1
      uint32_t num = d & 0x0F;
      if (d & 0xEE)
        return false;

      auto bitDistributionTablePtr = (uint8_t*)alloc (16);
      mHuffBits[num][cls] = bitDistributionTablePtr;

      uint32_t np = 0;
      for (uint32_t i = 0; i < 16; i++) {
        // load number of patterns for 1 to 16-bit code
        uint32_t b = *ptr++;
        bitDistributionTablePtr[i] = b;
        // get sum of code words for each code
        np += b;
        }

      auto codeWordTablePtr = (uint16_t*)alloc (np * sizeof (uint16_t));
      mHuffCode[num][cls] = codeWordTablePtr;

      uint16_t hc = 0;
      uint32_t j = 0;
      for (uint32_t i = 0; i < 16; i++, hc <<= 1) {
        // rebuild huffman code word table
        uint32_t b = bitDistributionTablePtr[i];
        while (b--)
          codeWordTablePtr[j++] = hc++;
        }

      if (length < np)
        return false;  // Err: wrong ptr size
      length -= np;

      auto decodedDataPtr = (uint8_t*)alloc (np);
      mHuffData[num][cls] = decodedDataPtr;
      for (uint32_t i = 0; i < np; i++) {
        // Load decoded data corresponds to each code ward
        uint8_t d = *ptr++;
        if (!cls && d > 11)
          return false;
        *decodedDataPtr++ = d;
        }
      }

    return true;
    }
  //}}}
  //{{{
  bool parseDRI (uint8_t* ptr, uint32_t length) {
    mNumRst = ptr[0]<<8 | ptr[1];
    return length >= 2;
    }
  //}}}
  //{{{
  bool parseSOF (uint8_t* ptr, uint32_t length) {

    mHeight = ptr[1]<<8 | ptr[2];
    mWidth =  ptr[3]<<8 | ptr[4];

    // only Y/Cb/Cr format
    if (ptr[5] != 3)
      return false;

    // Check three image components
    for (auto i = 0; i < 3; i++) {
      // Get sampling factor
      uint8_t b = ptr[7 + 3 * i];
      if (!i) {
        // Y component
        if (b != 0x11 && b != 0x22 && b != 0x21) //  Check sampling factor, only 4:4:4, 4:2:0 or 4:2:2
          return false;

        // Size of MCU [blocks]
        msx = b >> 4;
        msy = b & 15;
        }
      else if (b != 0x11) // Cb/Cr component
        return false;

      // Get dequantizer table ID for this component
      b = ptr[8 + 3 * i];
      if (b > 3)
        return false;

      mQtableId[i] = b;
      }

    return true;
    }
  //}}}
  //{{{
  bool parseSOS (uint8_t* ptr, uint32_t length) {

    if (!mWidth || !mHeight)
      return false;
    if (ptr[0] != 3)
      return false;

    // Check if all tables corresponding to each components have been loaded
    for (auto i = 0; i < 3; i++) {
      uint8_t b = ptr[2 + 2 * i]; // Get huffman table ID

      // Different table number for DC/AC element
      if (b != 0x00 && b != 0x11)
        return false;

      b = i ? 1 : 0;
      // Check huffman table for this component
      if (!mHuffBits[b][0] || !mHuffBits[b][1])
        return false;
      if (!mQtable [mQtableId [i]])
        return false;
      }

    // Allocate working buffer for MCU and RGB
    uint32_t numYblocks = msy * msx; // Number of Y blocks in the MCU
    if (!numYblocks) // SOF0 not loaded
      return false;
    mMcuBuffer = (uint8_t*)alloc ((numYblocks + 2) * 64);

    // allocate buffer for IDCT, RGB, at least 256 bytes for IDCT
    auto idctRgbBufferSize = numYblocks * 64 * 2 + 64;
    if (idctRgbBufferSize < 256)
      idctRgbBufferSize = 256;
    mIdctRgbBuffer = (uint8_t*)alloc (idctRgbBufferSize);

    // Initialization succeeded. Ready to decompress the JPEG image
    return true;
    }
  //}}}

  //{{{
  int bitEtract (uint32_t nBits) {
  // Extract N bits from input stream

    auto msk = mDataMask;
    uint32_t dc = mDataCounter;
    auto dp = mDataPtr; // Bit mask, number of data available, read ptr
    auto s = *dp;
    uint32_t v = 0;
    uint32_t f = 0;
    do {
      if (!msk) {
        if (!dc) {
          // No input data is available, re-fill input buffer
          dp = mInputBuffer; // Top of input buffer
          dc = read (dp, kBodyBufferSize);
          if (!dc)
            return false;
          }
        else
          dp++;     // Next data ptr

        dc--;       // Decrement number of available bytes
        if (f) {
          // In flag sequence?
          f = 0;      // Exit flag sequence
          if (*dp != 0)
            return false; // Err: unexpected flag is detected (may be collapted data)
          *dp = s = 0xFF;     // The flag is a data 0xFF
          }
        else {
          s = *dp;        // Get next data byte
          if (s == 0xFF) {
            // start of flag sequence?
            f = 1;
            continue;  // Enter flag sequence
            }
          }
        msk = 0x80;   // Read from MSB
        }

      v <<= 1;  // Get a bit
      if (s & msk)
        v++;
      msk >>= 1;
      nBits--;
      } while (nBits);

    mDataMask = msk;
    mDataCounter = dc;
    mDataPtr = dp;
    return (int)v;
    }
  //}}}
  //{{{
  int huffExtract (const uint8_t* hbits, const uint16_t* hcode, const uint8_t* hdata) {
  // Extract a huffman decoded data from input stream

    auto msk = mDataMask;
    uint32_t dc = mDataCounter;
    auto dp = mDataPtr; // Bit mask, number of data available, read ptr
    auto s = *dp;
    uint32_t v = 0;
    uint32_t f = 0;
    uint32_t bl = 16;  // Max code lengthgth
    do {
      if (!msk) {
        if (!dc) {
          // No input data is available, re-fill input buffer
          dp = mInputBuffer; // Top of input buffer
          dc = read (dp, kBodyBufferSize);
          if (!dc)
            return false; // read error or wrong stream termination
          }
        else
          dp++; // Next data ptr

        dc--;   // Decrement number of available bytes
        if (f) {
          // In flag sequence?
          f = 0;    // Exit flag sequence
          if (*dp != 0)
            return false; // unexpected flag is detected (may be collapted data)
          *dp = s = 0xFF; // The flag is a data 0xFF
          }
        else {
          s = *dp; // Get next data byte
          if (s == 0xFF) {
            // start of flag sequence
            f = 1;
            continue;  // Enter flag sequence, get trailing byte
            }
          }
        msk = 0x80;
        }

      // Get a bit
      v <<= 1;
      if (s & msk)
        v++;
      msk >>= 1;

      for (uint32_t nd = *hbits++; nd; nd--) {
        // Search the code word in this bit lengthgth
        if (v == *hcode++) {
          // Matched
          mDataMask = msk;
          mDataCounter = dc;
          mDataPtr = dp;
          return *hdata;
          }
        hdata++;
        }
      bl--;
      } while (bl);

    return true;
    }
  //}}}
  //{{{
  void idct (int32_t* src, uint8_t* dst) {
  // Apply Inverse-DCT in Arai Algorithm (see also aa_idct.png)

    for (uint32_t i = 0; i < 8; i++, src++) {
      //{{{  process columns
      int32_t v0 = *src;
      int32_t v5 = *(src+8);
      int32_t v1 = *(src+16);
      int32_t v7 = *(src+24);
      int32_t v2 = *(src+32);
      int32_t v6 = *(src+40);
      int32_t v3 = *(src+48);
      int32_t v4 = *(src+56);

      // process even elements
      int32_t t10 = v0 + v2;
      int32_t t12 = v0 - v2;
      int32_t t11 = (v1 - v3) * M13 >> 12;

      v3 += v1;
      t11 -= v3;
      v0 = t10 + v3;
      v3 = t10 - v3;
      v1 = t11 + t12;
      v2 = t12 - t11;

      // process odd elements
      t10 = v5 - v4;
      t11 = v5 + v4;
      t12 = v6 - v7;
      v7 += v6;
      v5 = (t11 - v7) * M13 >> 12;
      v7 += t11;

      int32_t t13 = (t10 + t12) * M5 >> 12;
      v4 = t13 - (t10 * M2 >> 12);
      v6 = t13 - (t12 * M4 >> 12) - v7;
      v5 -= v6;
      v4 -= v5;

      // writeback transformed values
      *src = v0 + v7;
      *(src+8) = v1 + v6;
      *(src+16) = v2 + v5;
      *(src+24) = v3 + v4;
      *(src+32) = v3 - v4;
      *(src+40) = v2 - v5;
      *(src+48) = v1 - v6;
      *(src+56) = v0 - v7;
      }
      //}}}

    src -= 8;
    for (uint32_t i = 0; i < 8; i++) {
      //{{{  process rows
      int32_t v0 = *src++ + (128L << 8);
      int32_t v5 = *src++;
      int32_t v1 = *src++;
      int32_t v7 = *src++;
      int32_t v2 = *src++;
      int32_t v6 = *src++;
      int32_t v3 = *src++;
      int32_t v4 = *src++;

      // process even elements
      int32_t t10 = v0 + v2;
      int32_t t12 = v0 - v2;
      int32_t t11 = (v1 - v3) * M13 >> 12;
      v3 += v1;
      t11 -= v3;
      v0 = t10 + v3;
      v3 = t10 - v3;
      v1 = t11 + t12;
      v2 = t12 - t11;

      // process odd elements
      t10 = v5 - v4;
      t11 = v5 + v4;
      t12 = v6 - v7;
      v7 += v6;
      v5 = (t11 - v7) * M13 >> 12;
      v7 += t11;

      int32_t t13 = (t10 + t12) * M5 >> 12;
      v4 = t13 - (t10 * M2 >> 12);
      v6 = t13 - (t12 * M4 >> 12) - v7;
      v5 -= v6;
      v4 -= v5;

      // descale the transformed values 8 bits and output
      *dst++ = kClip8 [(v0 + v7) >> 8];
      *dst++ = kClip8 [(v1 + v6) >> 8];
      *dst++ = kClip8 [(v2 + v5) >> 8];
      *dst++ = kClip8 [(v3 + v4) >> 8];
      *dst++ = kClip8 [(v3 - v4) >> 8];
      *dst++ = kClip8 [(v2 - v5) >> 8];
      *dst++ = kClip8 [(v1 - v6) >> 8];
      *dst++ = kClip8 [(v0 - v7) >> 8];
      }
      //}}}
    }
  //}}}

  //{{{
  bool restart (uint16_t rstn) {
  // Process restart interval

    // Discard padding bits and get two bytes from the input stream
    auto dp = mDataPtr;
    uint32_t dc = mDataCounter;
    uint16_t d = 0;

    for (uint32_t i = 0; i < 2; i++) {
      if (!dc) {
        // No input data is available, re-fill input buffer
        dp = mInputBuffer;
        dc = read (dp, kBodyBufferSize);
        if (!dc)
          return false;
        }
      else
        dp++;
      dc--;
      // Get a byte
      d = (d << 8) | *dp;
      }

    mDataPtr = dp;
    mDataCounter = dc;
    mDataMask = 0;

    // Check the marker
    if ((d & 0xFFD8) != 0xFFD0 || (d & 7) != (rstn & 7)) // expected RSTn marker is not detected
      return false;

    // Reset DC offset
    mDcValue[0] = 0;
    mDcValue[1] = 0;
    mDcValue[2] = 0;

    return true;
    }
  //}}}
  //{{{
  bool mcuLoad() {

    uint32_t nby = msx * msy;  // Number of Y blocks (1, 2 or 4)
    uint32_t nbc = 2;          // Number of C blocks (2)
    auto mcuBuf = mMcuBuffer;  // Pointer to the first block
    auto idctBuf = (int32_t*)mIdctRgbBuffer;

    for (uint32_t blk = 0; blk < nby + nbc; blk++, mcuBuf += 64) {
      uint32_t cmp = (blk < nby) ? 0 : blk - nby + 1;  // Component number 0:Y, 1:Cb, 2:Cr
      uint32_t id = cmp ? 1 : 0;                       // Huffman table ID of the component
      auto dqf = mQtable[mQtableId[cmp]];   // De-quantize table ID for this component
      //{{{  huffman decode DC from input
      auto hb = mHuffBits[id][0];  // Huffman table for the DC element
      auto hc = mHuffCode[id][0];
      auto hd = mHuffData[id][0];

      // Extract a huffman coded data (bit length)
      int b = huffExtract (hb, hc, hd);
      if (b < 0)
        return false;  // invalid code or input

      int d = mDcValue[cmp];         // DC value of previous block
      if (b) {                  // If there is any difference from previous block
        int e = bitEtract (b);  // Extract data bits
        if (e < 0)
          return false;

        // MSB position
        b = 1 << (b - 1);
        if (!(e & b)) // Restore sign if needed
          e -= (b << 1) - 1;

        // Get current value
        d += e;
        mDcValue[cmp] = (int16_t)d;  // Save current DC value for next block
        }

      // De-quantize, apply scale factor of Arai algorithm and descale 8 bits
      idctBuf[0] = d * dqf[0] >> 8;
      //}}}
      //{{{  huffman decode 63 AC from input
      memset (idctBuf+1, 0, 63*sizeof(uint32_t));

      hb = mHuffBits[id][1];
      hc = mHuffCode[id][1];
      hd = mHuffData[id][1];

      // Top of the AC elements
      uint32_t i = 1;
      do {
        // Extract a huffman coded value (zero runs and bit lengthgth)
        int b = huffExtract (hb, hc, hd);
        if (b == 0) // EOB
          break;
        if (b < 0)
          return false;

        // Number of leading zero elements
        auto z = (uint32_t)b >> 4;
        if (z) {
          // Skip zero elements
          i += z;
          if (i >= 64) // Too long zero run
            return false;
          }

        // Bit length
        if (b &= 0x0F) {
          // Extract data bits
          d = bitEtract (b);
          if (d < 0)
            return false;

          // MSB position
          b = 1 << (b - 1);
          if (!(d & b)) // Restore negative value if needed
            d -= (b << 1) - 1;

          // De-quantize, apply scale factor of Arai algorithm and descale 8 bits
          z = kZigZag[i];
          idctBuf[z] = d * dqf[z] >> 8;
          }
        } while (++i < 64);  // next AC element
      //}}}

      if (mScaleShift == 3)  // only use DC element
        *mcuBuf = uint8_t ((*idctBuf / 256) + 128);
      else
        idct (idctBuf, mcuBuf);
      }

    return true;
    }
  //}}}
  //{{{
  bool mcuProcess (uint32_t x, uint32_t y) {

    uint32_t mx = msx * 8;
    uint32_t my = msy * 8;

    uint32_t rx = (x + mx <= mWidth) ? mx : mWidth - x;
    uint32_t ry = (y + my <= mHeight) ? my : mHeight - y;
    if (!rx || !ry)
      return true;

    if (mScaleShift) {
      // scaled
      x >>= mScaleShift;
      y >>= mScaleShift;
      rx >>= mScaleShift;
      ry >>= mScaleShift;
      if (!rx || !ry)
        return true;

      if (mScaleShift == 3) {
        //{{{  1/8 scaling, just use DC value
        auto dstPtr = mFrameBuffer + ((y * mFrameWidth) + x) * mBytesPerPixel;
        auto stride = (mFrameWidth - rx) * mBytesPerPixel;

        auto chromaPtr = mMcuBuffer + (mx * my);
        int32_t cb = *chromaPtr - 128;
        int32_t cr = *(chromaPtr + 64) - 128;

        for (uint32_t iy = 0; iy < my; iy += 8, dstPtr += stride) {
          auto lumaPtr = mMcuBuffer;
          if (iy == 8)
            lumaPtr += 64 * 2;
          for (uint32_t ix = 0; ix < mx; ix += 8, lumaPtr += 64) {
            // convert YCbCr to BGRA
            *dstPtr++ = kClip8 [*lumaPtr +  ((kBcB * cb) >> 10)];
            *dstPtr++ = kClip8 [*lumaPtr - (((kGcB * cb) + (kGcR * cr)) >> 10)];
            *dstPtr++ = kClip8 [*lumaPtr +  ((kRcR * cr) >> 10)];
            if (mBytesPerPixel == 4)
              *dstPtr++ = 0xFF;
            }
          }
        }
        //}}}
      else {
        //{{{  1/2, 1/4 scaling
        auto bgra = mIdctRgbBuffer;
        for (uint32_t iy = 0; iy < my; iy++) {
          auto chromaPtr = mMcuBuffer;
          auto lumaPtr = chromaPtr + iy * 8;
          if (my == 16) {
            // Double block height?
            chromaPtr += 64 * 4 + (iy >> 1) * 8;
            if (iy >= 8)
              lumaPtr += 64;
            }
          else
            // Single block height
            chromaPtr += mx * 8 + iy * 8;

          for (uint32_t ix = 0; ix < mx; ix++) {
            // Get CbCr component and restore right level
            int32_t cb = *chromaPtr - 128;
            int32_t cr = *(chromaPtr + 64) - 128;
            if (mx == 16) {
              // Double block width
              if (ix == 8) // Jump to next block if double block heigt
                lumaPtr += 64 - 8;

              // Increase chroma pointer every two pixels
              chromaPtr += ix & 1;
              }
            else // Single block width, Increase chroma pointer every pixel
              chromaPtr++;

            // Convert YCbCr to BGRA
            *bgra++ = kClip8 [*lumaPtr +  ((kBcB * cb) >> 10)];
            *bgra++ = kClip8 [*lumaPtr - (((kGcB * cb) + (kGcR * cr)) >> 10)];
            *bgra++ = kClip8 [*lumaPtr +  ((kRcR * cr) >> 10)];
            }
          }

        //{{{  average scaleShift 1,2
        uint32_t s = mScaleShift * 2;   // number of shifts for averaging
        uint32_t w = 1 << mScaleShift;  // Width of square
        uint32_t a = (mx - w) * 3;      // Bytes to skip for next line in the square

        auto output = (uint8_t*)mIdctRgbBuffer;
        for (uint32_t iy = 0; iy < my; iy += w) {
          for (uint32_t ix = 0; ix < mx; ix += w) {
            bgra = (uint8_t*)mIdctRgbBuffer + (iy * mx + ix) * 3;
            uint32_t v1 = 0;
            uint32_t v2 = 0;
            uint32_t v3 = 0;
            for (uint32_t y = 0; y < w; y++) {
              // Accumulate BGR values in the square
              for (uint32_t x = 0; x < w; x++) {
                v1 += *bgra++;
                v2 += *bgra++;
                v3 += *bgra++;
                }
              bgra += a;
              }

            // Put the averaged value as a pixel
            *output++ = (uint8_t)(v1 >> s);
            *output++ = (uint8_t)(v2 >> s);
            *output++ = (uint8_t)(v3 >> s);
            }
          }
        //}}}
        //{{{  squeeze up pixel table if a part of MCU is to be truncated
        mx >>= mScaleShift;
        if (rx < mx) {
          auto src = (uint8_t*)mIdctRgbBuffer;
          auto dst = (uint8_t*)mIdctRgbBuffer;
          for (uint32_t y = 0; y < ry; y++, src += (mx - rx) * 3) {
            for (uint32_t x = 0; x < rx; x++) {
              *dst++ = *src++;
              *dst++ = *src++;
              *dst++ = *src++;
              }
            }
          }
        //}}}

        auto src = mIdctRgbBuffer;
        auto dstPtr = mFrameBuffer + ((y * mFrameWidth) + x) * mBytesPerPixel;
        auto stride = (mFrameWidth - rx) * mBytesPerPixel;
        for (uint32_t j = 0; j < ry; j++, dstPtr += stride)
          for (uint32_t i = 0; i < rx; i++) {
            *dstPtr++ = *src++; // B
            *dstPtr++ = *src++; // G
            *dstPtr++ = *src++; // R
            if (mBytesPerPixel == 4)
              *dstPtr++ = 0xFF;
            }
        }
        //}}}
      }
    else {
      //{{{  1/1 scaling
      auto dstPtr = mFrameBuffer + ((y * mFrameWidth) + x) * mBytesPerPixel;
      auto stride = (mFrameWidth - rx) * mBytesPerPixel;

      for (uint32_t iy = 0; iy < ry; iy++, dstPtr += stride) {
        auto chromaPtr = mMcuBuffer;
        auto lumaPtr = chromaPtr + iy * 8;
        if (my == 16) {
          // Double block height
          chromaPtr += 64 * 4 + (iy >> 1) * 8;
          if (iy >= 8)
            lumaPtr += 64;
          }
        else
          // Single block height
          chromaPtr += mx * 8 + iy * 8;

        for (uint32_t ix = 0; ix < rx; ix++, lumaPtr++) {
          // get CbCr components as signed int
          int32_t cb = *chromaPtr - 128;
          int32_t cr = *(chromaPtr + 64) - 128;
          if (mx == 16) {
            // double block width, increase chroma pointer every two pixels
            if (ix == 8) // jump to next block if double block height
              lumaPtr += 64 - 8;
            chromaPtr += ix & 1;
            }
          else // single block width, increase chroma pointer every pixel
            chromaPtr++;

          // convert YCbCr to BGRA
          *dstPtr++ = kClip8 [*lumaPtr +  ((kBcB * cb) >> 10)];
          *dstPtr++ = kClip8 [*lumaPtr - (((kGcB * cb) + (kGcR * cr)) >> 10)];
          *dstPtr++ = kClip8 [*lumaPtr +  ((kRcR * cr) >> 10)];
          if (mBytesPerPixel == 4)
            *dstPtr++ = 0xFF;
          }
        }
      }
      //}}}

    return y + ry <= mFrameHeight;
    }
  //}}}

  //{{{  private vars
  uint32_t  mWidth;          // Size of the input image
  uint32_t  mHeight;         // Size of the input image
  uint8_t   mScaleShift;     // Output scaling ratio
  uint8_t   msx;             // MCU size in unit of block (width, height)
  uint8_t   msy;             // MCU size in unit of block (width, height)
  uint16_t  mNumRst = 0;     // Restart inverval in MCUs

  uint8_t*  mPoolBuffer;
  uint8_t*  mPoolPtr;
  uint32_t  mPoolBytesLeft;  // size of momory pool (bytes available)
  uint8_t*  mMcuBuffer;      // working buffer for the MCU
  uint8_t*  mIdctRgbBuffer;  // working buffer for IDCT and RGB output

  uint8_t*  mInputBuffer;    // bit stream input buffer
  uint32_t  mDataCounter;    // Number of bytes available in the input buffer
  uint8_t*  mDataPtr;        // Current data read ptr
  uint8_t   mDataMask;       // Current bit in the current read byte

  int16_t   mDcValue[3];     // Previous DC element of each component
  uint8_t   mQtableId[3];    // Quantization table ID of each component
  int32_t*  mQtable[4];      // Dequaitizer tables [id]

  uint8_t*  mHuffBits[2][2]; // Huffman bit distribution tables [id][dcac]
  uint8_t*  mHuffData[2][2]; // Huffman decoded data tables [id][dcac]
  uint16_t* mHuffCode[2][2]; // Huffman code word tables [id][dcac]

  uint32_t mFrameWidth = 0;
  uint32_t mFrameHeight = 0;
  uint8_t* mFrameBuffer = nullptr;

  // thumb
  uint32_t mThumbOffset = 0;
  uint32_t mThumbBytes = 0;

  uint16_t mBytesPerPixel;
  //}}}
  };
