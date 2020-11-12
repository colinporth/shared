// cLoader.cpp - audio,video loader, launches cSongPlayer
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <functional>

#include "cLoader.h"
#include "cSongPlayer.h"

// utils
#include "../date/date.h"
#include "../utils/utils.h"
#include "../utils/cLog.h"

// container
#include "../utils/cSong.h"

// audio decode
#include "../decoders/cAudioParser.h"
#include "../decoders/iAudioDecoder.h"
#include "../decoders/cFFmpegAudioDecoder.h"

// video pool
#include "../utils/iVideoPool.h"

// net
#ifdef _WIN32
  #include "../net/cWinSockHttp.h"
#else
  #include "../net/cLinuxHttp.h"
#endif

#ifdef _WIN32
  //#include "../common/cJpegImage.h"
  #include "../../shared/utils/cFileList.h"
#else
  #include <sys/mman.h>
#endif

#include "readerWriterQueue.h"

using namespace std;
using namespace chrono;
//}}}

namespace {
  //{{{
  iAudioDecoder* createAudioDecoder (eAudioFrameType frameType) {

    switch (frameType) {
      case eAudioFrameType::eMp3:
        cLog::log (LOGINFO, "createAudioDecoder ffmpeg mp3");
        return new cFFmpegAudioDecoder (frameType);

      case eAudioFrameType::eAacAdts:
        cLog::log (LOGINFO, "createAudioDecoder ffmpeg aacAdts");
        return new cFFmpegAudioDecoder (frameType);

      case eAudioFrameType::eAacLatm:
        cLog::log (LOGINFO, "createAudioDecoder ffmpeg aacLatm");
        return new cFFmpegAudioDecoder (frameType);

      default:
        cLog::log (LOGERROR, "createAudioDecoder frameType:%d", frameType);
      }

    return nullptr;
    }
  //}}}
  //{{{
  int64_t getPts (const uint8_t* ts) {
  // return 33 bits of pts,dts

    if ((ts[0] & 0x01) && (ts[2] & 0x01) && (ts[4] & 0x01)) {
      // valid marker bits
      int64_t pts = ts[0] & 0x0E;
      pts = (pts << 7) | ts[1];
      pts = (pts << 8) | (ts[2] & 0xFE);
      pts = (pts << 7) | ts[3];
      pts = (pts << 7) | (ts[4] >> 1);
      return pts;
      }
    else {
      cLog::log (LOGERROR, "getPts marker bits - %02x %02x %02x %02x 0x02", ts[0], ts[1], ts[2], ts[3], ts[4]);
      return 0;
      }
    }
  //}}}
  //{{{
  string getHlsHostName (bool radio) {
    return radio ? "as-hls-uk-live.bbcfmt.s.llnwi.net" : "vs-hls-uk-live.akamaized.net";
    }
  //}}}
  //{{{
  string getHlsPathName (bool radio, const string& channel, int audioRate, int videoRate) {

    string pathName = (radio ? "pool_904/live/uk/" : "pool_902/live/uk/") +
                      channel + "/" + channel + ".isml/" + channel;

    if (radio)
      pathName += "-audio=" + dec(audioRate);

    else {
      pathName += (audioRate < 128000) ? "-pa3=" : "-pa4=" + dec(audioRate);
      if (videoRate)
        pathName += "-video=" + dec(videoRate);
      }

    return pathName;
    }
  //}}}
  //{{{
  string getHlsM3u8Name() {
    return ".norewind.m3u8";
    }
  //}}}
  //{{{
  string getTagValue (uint8_t* buffer, const char* tag, char terminator) {
  // crappy get value from tag

    const char* tagPtr = strstr ((const char*)buffer, tag);
    const char* valuePtr = tagPtr + strlen (tag);
    const char* endPtr = strchr (valuePtr, terminator);

    return string (valuePtr, endPtr - valuePtr);
    }
  //}}}
  }

//{{{
class cService {
public:
  cService (int sid, bool selected) : mSid(sid), mSelected(selected) {}

  bool isSelected() { return mSelected; }
  int getAudioPid() { return mAudioPid; }
  int getVideoPid() { return mVideoPid; }
  int getSubtitlePid() { return mSubtitlePid; }

  void setSelected (bool selected) { mSelected = selected; }
  void setAudioPid (int pid) { mAudioPid = pid; }
  void setVideoPid (int pid) { mVideoPid = pid; }
  void setSubtitlePid (int pid) { mSubtitlePid = pid; }

private:
  const int mSid;

  int mAudioPid = 0;
  int mVideoPid = 0;
  int mSubtitlePid = 0;

  bool mSelected = false;
  };
//}}}
//{{{
class cPidParser {
public:
  cPidParser (int pid, const string& name) : mPid(pid), mName(name) {}
  virtual ~cPidParser() {}

  virtual int getQueueSize() { return 0; }
  virtual float getQueueFrac() { return 0.f; }

  virtual void stop() {}
  virtual void processLast (bool reuseFromFront) {}

  //{{{
  void parse (uint8_t* ts, bool reuseFromFront) {

    bool payloadStart = ts[1] & 0x40;
    int continuityCount = ts[3] & 0x0F;

    int headerSize = 1 + ((ts[3] & 0x20) ? 4 + ts[4] : 3);
    bool hasPayload = ts[3] & 0x10;

    ts += headerSize;
    int tsLeft = 188 - headerSize;

    // ignore any leading payload before a payloadStart
    if (hasPayload && (payloadStart || mGotPayloadStart)) {
      mGotPayloadStart = true;
      processPayload (ts, tsLeft, payloadStart, continuityCount, reuseFromFront);
      }
    }
  //}}}

protected:
  //{{{
  static uint32_t getCrc32 (uint32_t crc, uint8_t* buf, unsigned int len) {

    //{{{
    // CRC32 lookup table for polynomial 0x04c11db7
    const unsigned long crcTable[256] = {
      0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
      0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
      0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
      0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
      0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
      0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
      0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
      0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
      0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
      0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
      0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
      0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
      0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
      0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
      0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
      0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
      0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
      0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
      0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
      0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
      0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
      0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
      0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
      0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
      0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
      0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
      0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
      0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
      0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
      0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
      0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
      0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
      0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
      0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
      0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
      0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
      0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
      0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
      0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
      0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
      0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
      0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
      0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4};
    //}}}

    for (auto i = 0u; i < len; i++)
      crc = (crc << 8) ^ crcTable[((crc >> 24) ^ *buf++) & 0xff];

    return crc;
    }
  //}}}
  //{{{
  virtual void processPayload (uint8_t* ts, int tsLeft, bool payloadStart, int continuityCount, bool reuseFromFront) {

    string info;
    for (int i = 0; i < tsLeft; i++) {
      int value = ts[i];
      info += hex (value, 2) + " ";
      }

    cLog::log (LOGINFO, mName + " " + string(payloadStart ? "start ": "") + dec (tsLeft,3) + ":" + info);
    }
  //}}}

  // vars
  const int mPid;
  const string mName;
  bool mGotPayloadStart = false;
  };
//}}}
//{{{
class cPatParser : public cPidParser {
public:
  cPatParser (function<void (int programPid, int programSid)> callback)
    : cPidParser (0, "pat"), mCallback(callback) {}
  virtual ~cPatParser() {}

  virtual void processPayload (uint8_t* ts, int tsLeft, bool payloadStart, int continuityCount, bool reuseFromFront) {

    if (payloadStart) {
      //int pointerField = ts[0]
      ts++;
      tsLeft--;

      //int tid = ts[0]; // could check it
      int sectionLength = ((ts[1] & 0x0F) << 8) + ts[2] + 3;
      if (getCrc32 (0xffffffff, ts, sectionLength) != 0) {
        //{{{  error return
        cLog::log (LOGERROR, mName + " crc error");
        return;
        }
        //}}}

      // transport stream id = ((ts[3] & 0xF) << 4) | ts[4]
      // currentNext,versionNumber = ts[5]
      // section number = ts[6]
      // last section number = ts[7]

      // skip past pat header
      ts += 8;
      tsLeft -= 8;
      sectionLength -= 8 + 4;
      if (sectionLength > tsLeft) {
        //{{{  error return
        cLog::log (LOGERROR, mName + " sectionLength " + dec(sectionLength) + " tsLeft" + dec(tsLeft));
        return;
        }
        //}}}

      // iterate pat programs
      while (sectionLength > 0) {
        int programSid = (ts[0] << 8) + ts[1];
        int programPid = ((ts[2] & 0x1F) << 8) + ts[3];
        mCallback (programPid, programSid);

        ts += 4;
        tsLeft -= 4;
        sectionLength -= 4;
        }
      }
    }

private:
  function <void (int programPid, int programSid)> mCallback;
  };
//}}}
//{{{
class cPmtParser : public cPidParser {
public:
  cPmtParser (int pid, int sid, function<void (int streamSid, int streamPid, int streamType)> callback)
    : cPidParser (pid, "pmt"), mSid(sid), mCallback(callback) {}
  virtual ~cPmtParser() {}

  //{{{  sPmt
  typedef struct {
     unsigned char table_id            :8;

     uint8_t section_length_hi         :4;
     uint8_t                           :2;
     uint8_t dummy                     :1; // has to be 0
     uint8_t section_syntax_indicator  :1;
     uint8_t section_length_lo         :8;

     uint8_t program_number_hi         :8;
     uint8_t program_number_lo         :8;
     uint8_t current_next_indicator    :1;
     uint8_t version_number            :5;
     uint8_t                           :2;
     uint8_t section_number            :8;
     uint8_t last_section_number       :8;
     uint8_t PCR_PID_hi                :5;
     uint8_t                           :3;
     uint8_t PCR_PID_lo                :8;
     uint8_t program_info_length_hi    :4;
     uint8_t                           :4;
     uint8_t program_info_length_lo    :8;
     //descrs
    } sPmt;

  //sPmtInfo
  typedef struct {
     uint8_t stream_type        :8;
     uint8_t elementary_PID_hi  :5;
     uint8_t                    :3;
     uint8_t elementary_PID_lo  :8;
     uint8_t ES_info_length_hi  :4;
     uint8_t                    :4;
     uint8_t ES_info_length_lo  :8;
     // descrs
    } sPmtInfo;
  //}}}

  virtual void processPayload (uint8_t* ts, int tsLeft, bool payloadStart, int continuityCount, bool reuseFromFront) {
    if (payloadStart) {
      //int pointerField = ts[0];
      ts++;
      tsLeft--;

      //int tid = ts[0];
      int sectionLength = ((ts[1] & 0x0F) << 8) + ts[2] + 3;
      if (getCrc32 (0xffffffff, ts, sectionLength) != 0) {
        //{{{  error return
        cLog::log (LOGERROR, mName + " crc error");
        return;
        }
        //}}}

      int sid = (ts[3] << 8) + ts[4];
      //int versionNumber = ts[5];
      //int sectionNumber = ts[6];
      //int lastSectionNumber = ts[7];
      //int pcrPid = ((ts[8] & 0x1f) << 8) + ts[9];
      int programInfoLength = ((ts[10] & 0x0f) << 8) + ts[11];

      // skip past pmt header
      ts += 12 + programInfoLength;
      tsLeft -= 12 - programInfoLength;
      sectionLength -= 12 + 4 - programInfoLength;

      if (sectionLength > tsLeft) {
        //{{{  error return
        cLog::log (LOGERROR, mName + " sectionLength " + dec(sectionLength) + " tsLeft" + dec(tsLeft));
        return;
        }
        //}}}

      // iterate pmt streams
      while (sectionLength > 0) {
        int streamType = ts[0];
        int streamPid = ((ts[1] & 0x1F) << 8) + ts[2];
        int streamInfoLength = ((ts[3] & 0x0F) << 8) + ts[4];
        mCallback (sid, streamPid, streamType);

        ts += 5 + streamInfoLength;
        tsLeft -= 5 + streamInfoLength;
        sectionLength -= 5 + streamInfoLength;
        }
      }
    }

private:
  int mSid;
  function <void (int streamSid, int streamPid, int streamType)> mCallback;
  };
//}}}
//{{{
class cPesParser : public cPidParser {
//{{{
class cPesItem {
public:
  //{{{
  cPesItem (bool reuseFromFront, uint8_t* pes, int size, int64_t pts, int64_t dts)
      : mReuseFromFront(reuseFromFront), mPesSize(size), mPts(pts), mDts(dts) {
    mPes = (uint8_t*)malloc (size);
    memcpy (mPes, pes, size);
    }
  //}}}
  //{{{
  ~cPesItem() {
    free (mPes);
    }
  //}}}

  bool mReuseFromFront;
  uint8_t* mPes;
  const int mPesSize;
  const int64_t mPts;
  const int64_t mDts;
  };
//}}}
public:
  //{{{
  cPesParser (int pid, const string& name, bool useQueue) : cPidParser(pid, name), mUseQueue(useQueue) {

    mPes = (uint8_t*)malloc (kInitPesSize);
    if (useQueue)
      thread ([=](){ dequeThread(); }).detach();
    }
  //}}}
  virtual ~cPesParser() {}

  virtual int getQueueSize() { return (int)mQueue.size_approx(); }
  virtual float getQueueFrac() { return (float)mQueue.size_approx() / mQueue.max_capacity(); }

  //{{{
  virtual void stop() {

    if (mUseQueue) {
      mQueueExit = true;
      //while (mQueueRunning)
      this_thread::sleep_for (100ms);
      }
    }
  //}}}
  //{{{
  virtual void processLast (bool reuseFromFront) {

    if (mPesSize) {
      dispatchDecode (reuseFromFront, mPes, mPesSize, mPts, mDts);
      mPesSize = 0;
      }
    }
  //}}}

protected:
  //{{{
  virtual void processPayload (uint8_t* ts, int tsLeft, bool payloadStart, int continuityCount, bool reuseFromFront) {
  // ts[0],ts[1],ts[2],ts[3] = stream id 0x000001xx
  // ts[4],ts[5] = packetLength, 0 for video
  // ts[6] = 0x80 marker, 0x04 = dataAlignmentIndicator
  // ts[7] = 0x80 = pts, 0x40 = dts
  // ts[8] = optional header length

    if ((mContinuityCount >= 0) &&
        (continuityCount != ((mContinuityCount + 1) & 0xF))) {
      // !!! should abandon pes !!!!
      cLog::log (LOGERROR, "continuity count error pid:%d %d %d", mPid, continuityCount, mContinuityCount);
      }
    mContinuityCount = continuityCount;

    if (payloadStart) {
      bool dataAlignmentIndicator = (ts[6] & 0x84) == 0x84;
      if (dataAlignmentIndicator) {
        processLast (reuseFromFront);

        // get pts dts for next pes
        if (ts[7] & 0x80)
          mPts = getPts (ts+9);
        if (ts[7] & 0x40)
          mDts = getPts (ts+14);
        }

      int headerSize = 9 + ts[8];
      ts += headerSize;
      tsLeft -= headerSize;
      }

    if (mPesSize + tsLeft > mAllocSize) {
      mAllocSize *= 2;
      mPes = (uint8_t*)realloc (mPes, mAllocSize);
      cLog::log (LOGINFO, mName + " pes allocSize doubled to " + dec(mAllocSize));
      }

    memcpy (mPes + mPesSize, ts, tsLeft);
    mPesSize += tsLeft;
    }
  //}}}
  //{{{
  virtual void dispatchDecode (bool reuseFromFront, uint8_t* pes, int size, int64_t pts, int64_t dts) {

    if (mUseQueue)
      mQueue.enqueue (new cPesParser::cPesItem (reuseFromFront, pes, size, pts, dts));
    else
      decode (reuseFromFront, pes, size, pts, dts);
    }
  //}}}
  virtual void decode (bool reuseFromFront, uint8_t* pes, int size, int64_t pts, int64_t dts) = 0;
  //{{{
  void dequeThread() {

    cLog::setThreadName (mName + "Q");

    mQueueRunning = true;

    while (!mQueueExit) {
      cPesItem* pesItem;
      if (mQueue.wait_dequeue_timed (pesItem, 40000)) {
        decode (pesItem->mReuseFromFront, pesItem->mPes, pesItem->mPesSize, pesItem->mPts, pesItem->mDts);
        delete pesItem;
        }
      }

    mQueueRunning = false;
    }
  //}}}

  int mAllocSize = kInitPesSize;

  uint8_t* mPes;
  int mPesSize = 0;
  int64_t mPts = 0;
  int64_t mDts = 0;
  int mContinuityCount = -1;

private:
  static constexpr int kInitPesSize = 4096;

  bool mUseQueue = true;
  bool mQueueExit = false;
  bool mQueueRunning = false;
  readerWriterQueue::cBlockingReaderWriterQueue <cPesItem*> mQueue;
  };
//}}}
//{{{
class cAudioPesParser : public cPesParser {
public:
  cAudioPesParser (int pid, iAudioDecoder* audioDecoder, bool useQueue,
                   function <void (bool reuseFromFront, float* samples, int64_t pts)> callback)
      : cPesParser(pid, "aud", useQueue), mAudioDecoder(audioDecoder), mCallback(callback) {
    }
  virtual ~cAudioPesParser() {}

protected:
  virtual void decode (bool reuseFromFront, uint8_t* pes, int size, int64_t pts, int64_t dts) {
  // decode pes to audio frames

    uint8_t* framePes = pes;
    int frameSize;
    while (cAudioParser::parseFrame (framePes, pes + size, frameSize)) {
      // decode a single frame from pes
      float* samples = mAudioDecoder->decodeFrame (framePes, frameSize, pts);
      if (samples) {
        mCallback (reuseFromFront, samples, pts);
        // pts of next frame in pes, assumes 48000 sample rate
        pts += (mAudioDecoder->getNumSamplesPerFrame() * 90) / 48;
        }
      else
        cLog::log (LOGERROR, "cAudioPesParser decode failed %d %d", size, pts);

      // point to next frame in pes
      framePes += frameSize;
      }
    }

private:
  iAudioDecoder* mAudioDecoder;
  function <void (bool reuseFromFront, float* samples, int64_t pts)> mCallback;
  };
//}}}
//{{{
class cVideoPesParser : public cPesParser {
public:
  cVideoPesParser (int pid, iVideoPool* videoPool, bool useQueue)
    : cPesParser (pid, "vid", useQueue), mVideoPool(videoPool) {}
  virtual ~cVideoPesParser() {}

protected:
  void decode (bool reuseFromFront, uint8_t* pes, int size, int64_t pts, int64_t dts) {
    mVideoPool->decodeFrame (reuseFromFront, pes, size, pts, dts);
    }

private:
  iVideoPool* mVideoPool;
  };
//}}}

// public
//{{{
void cLoader::getFracs (float& loadFrac, float& audioFrac, float& videoFrac) {
// return fracs for spinner graphic, true if ok to display

  loadFrac = mLoadFrac;
  audioFrac = 0.f;
  videoFrac = 0.f;

  if (mCurSid > 0) {
    cService* service = mServices[mCurSid];
    int audioPid = service->getAudioPid();
    int videoPid = service->getVideoPid();
    //cLog::log (LOGINFO, "getSizes %d %d", audioPid, videoPid);

    auto audioIt = mPidParsers.find (audioPid);
    if (audioIt != mPidParsers.end())
      audioFrac = (*audioIt).second->getQueueFrac();

    auto videoIt = mPidParsers.find (videoPid);
    if (videoIt != mPidParsers.end())
      videoFrac = (*videoIt).second->getQueueFrac();
    }
  }
//}}}
//{{{
void cLoader::getSizes (int& loadSize, int& audioQueueSize, int& videoQueueSize) {
// return sizes

  loadSize = mLoadSize;
  audioQueueSize = 0;
  videoQueueSize = 0;

  if (mCurSid > 0) {
    cService* service = mServices[mCurSid];
    int audioPid = service->getAudioPid();
    int videoPid = service->getVideoPid();
    //cLog::log (LOGINFO, "getSizes %d %d", audioPid, videoPid);

    auto audioIt = mPidParsers.find (audioPid);
    if (audioIt != mPidParsers.end())
      audioQueueSize = (*audioIt).second->getQueueSize();

    auto videoIt = mPidParsers.find (videoPid);
    if (videoIt != mPidParsers.end())
      videoQueueSize = (*videoIt).second->getQueueSize();
    }
  }
//}}}

//{{{
void cLoader::hls (bool radio, const string& channel, int audioRate, int videoRate, eFlags flags) {

  stopAndWait();

  thread ([=]() {
    cLog::setThreadName ("hlsL");

    cHlsSong* hlsSong = new cHlsSong (eAudioFrameType::eAacAdts, 2, 48000,
                                      (audioRate < 128000) ? 2048 : 1024,
                                      (audioRate < 128000) ? 3840 : 1920,
                                      1000,
                                      (audioRate < 128000) ? (radio ? 150 : 180) : (radio ? 300 : 360));
    mSong = hlsSong;

    iAudioDecoder* audioDecoder = nullptr;
    iVideoPool* videoPool = nullptr;
    //{{{  add parsers, callbacks
    auto addAudioFrameCallback = [&](bool reuseFromFront, float* samples, int64_t pts) noexcept {
      hlsSong->addFrame (reuseFromFront, pts, samples, true, hlsSong->getNumFrames()+1);
      if (!mSongPlayer)
        mSongPlayer = new cSongPlayer (hlsSong, true);
      };

    auto addStreamCallback = [&](int sid, int pid, int type) noexcept {
      if (mPidParsers.find (pid) == mPidParsers.end()) {
        // new stream, add stream parser, add stream pid to service
        auto it = mServices.find (sid);
        if (it == mServices.end())
          cLog::log (LOGERROR, "PMT:%d for unrecognised sid:%d", pid, sid);
        else {
          // should only be one
          cService* service = (*it).second;
          switch (type) {
            //{{{
            case 15: // aacAdts
              service->setAudioPid (pid);
              if (service->isSelected()) {
                audioDecoder = createAudioDecoder (eAudioFrameType::eAacAdts);
                mPidParsers.insert (
                  map<int,cPidParser*>::value_type (pid,
                    new cAudioPesParser (pid, audioDecoder, true, addAudioFrameCallback)));
                }
              break;
            //}}}
            //{{{
            case 27: // h264video
              if (videoRate) {
                service->setVideoPid (pid);
                if (service->isSelected()) {
                  videoPool = iVideoPool::create (flags & eFlags::eFFmpeg, 192, mSong);
                  mPidParsers.insert (map<int,cPidParser*>::value_type (pid, new cVideoPesParser (pid, videoPool, true)));
                  mVideoPool = videoPool;
                  }
                }
              break;
            //}}}
            //{{{
            default:
              cLog::log (LOGERROR, "hls - unrecognised stream pid:type %d:%d", pid, type);
            //}}}
            }
          }
        }
      };

    auto addProgramCallback = [&](int pid, int sid) noexcept {
      if (mPidParsers.find (pid) == mPidParsers.end()) {
        // new PMT, add parser and new service
        mPidParsers.insert (map<int,cPidParser*>::value_type (pid, new cPmtParser (pid, sid, addStreamCallback)));
        mServices.insert (map<int,cService*>::value_type (sid, new cService (sid, true)));
        mCurSid = sid;
        }
      };

    // add PAT parser
    mPidParsers.insert (map<int,cPidParser*>::value_type (0x00, new cPatParser (addProgramCallback)));
    //}}}

    mExit = false;
    mRunning = true;
    while (!mExit) {
      cPlatformHttp http;
      string redirectedHostName = http.getRedirect (
        getHlsHostName (radio), getHlsPathName (radio, channel, audioRate, videoRate) + getHlsM3u8Name());
      if (http.getContent()) {
        //{{{  parse m3u8 for #USP-X-TIMESTAMP-MAP:MPEGTS, #EXT-X-PROGRAM-DATE-TIME, #EXT-X-MEDIA-SEQUENCE tags
        // get value for tag #USP-X-TIMESTAMP-MAP:MPEGTS=
        int64_t mpegTimestamp = stoll (getTagValue (http.getContent(), "#USP-X-TIMESTAMP-MAP:MPEGTS=", ','));

        // get value for tag #EXT-X-PROGRAM-DATE-TIME:
        istringstream inputStream (getTagValue (http.getContent(), "#EXT-X-PROGRAM-DATE-TIME:", '\n'));
        system_clock::time_point extXProgramDateTimePoint;
        inputStream >> date::parse ("%FT%T", extXProgramDateTimePoint);

        // get value for tag #EXT-X-MEDIA-SEQUENCE:
        int extXMediaSequence = stoi (getTagValue (http.getContent(), "#EXT-X-MEDIA-SEQUENCE:", '\n'));

        // 37s is the magic number of seconds that extXProgramDateTimePoint is out from clockTime
        hlsSong->setBaseHls (mpegTimestamp, extXProgramDateTimePoint, -37s, extXMediaSequence);
        http.freeContent();
        //}}}
        while (!mExit) {
          int64_t loadPts;
          bool reuseFromFront;
          int chunkNum = hlsSong->getLoadChunkNum (loadPts, reuseFromFront);
          if (chunkNum > 0) {
            int contentParsed = 0;
            if (http.get (redirectedHostName,
                          getHlsPathName (radio, channel, audioRate, videoRate) + '-' + dec(chunkNum) + ".ts", "",
                          [&](const string& key, const string& value) noexcept {
                            //{{{  header lambda
                            if (key == "content-length")
                              cLog::log (LOGINFO, "chunk:" + dec(chunkNum) +
                                                  " pts:" + getPtsFramesString (loadPts, mSong->getFramePtsDuration()) +
                                                  " size:" + dec(http.getHeaderContentSize()/1000) + "k");
                            },
                            //}}}
                          [&](const uint8_t* data, int length) noexcept {
                            //{{{  data lambda
                            mLoadSize = http.getContentSize();
                            mLoadFrac = float(http.getContentSize()) / http.getHeaderContentSize();

                            // parse ts packets as we receive them
                            while (http.getContentSize() - contentParsed >= 188) {
                              uint8_t* ts = http.getContent() + contentParsed;
                              if (ts[0] == 0x47) {
                                auto it = mPidParsers.find (((ts[1] & 0x1F) << 8) | ts[2]);
                                if (it != mPidParsers.end())
                                  it->second->parse (ts, reuseFromFront);
                                ts += 188;
                                }
                              else
                                cLog::log (LOGERROR, "ts packet sync:%d", contentParsed);
                              contentParsed += 188;
                              }

                            return true;
                            }
                            //}}}
                          ) == 200) {
              for (auto parser : mPidParsers)
                parser.second->processLast (reuseFromFront);
              http.freeContent();
              }
            else {
              //{{{  failed to load expected to be available chunk, backoff for 250ms
              cLog::log (LOGERROR, "late " + dec(chunkNum));
              this_thread::sleep_for (250ms);
              }
              //}}}
            }
          else // no chunk available, back off for 100ms
            this_thread::sleep_for (100ms);
          }

        if (mSongPlayer)
          mSongPlayer->stopAndWait();
        }
      }

    //{{{  delete resources
    mSong = nullptr;
    mVideoPool = nullptr;

    for (auto parser : mPidParsers) {
      //{{{  stop and delete pidParsers
      parser.second->stop();
      delete parser.second;
      }
      //}}}
    mPidParsers.clear();

    delete mSongPlayer;
    delete hlsSong;
    delete audioDecoder;
    delete videoPool;
    //}}}
    cLog::log (LOGINFO, "exit");
    mRunning = false;
    } ).detach();
  }
//}}}
//{{{
void cLoader::file (const string& filename, eFlags flags) {

  stopAndWait();

  thread ([=]() {
    // lambda
    cLog::setThreadName ("file");
    mRunning = true;

    while (!mExit) {
      #ifdef _WIN32
        // windows file map
        HANDLE fileHandle = CreateFile (filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        HANDLE fileMapping = CreateFileMapping (fileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
        uint8_t* first = (uint8_t*)MapViewOfFile (fileMapping, FILE_MAP_READ, 0, 0, 0);
        auto size = GetFileSize (fileHandle, NULL);
      #else
        // linux mmap
        int fd = open (filename.c_str(), O_RDONLY);
        auto size = lseek (fd, 0, SEEK_END);
        uint8_t* first = (uint8_t*)mmap (NULL, size, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0);
      #endif

      // load file
      if ((first[0] == 0x47) && (first[188] == 0x47))
        loadTsFile (first, size, flags);
      else
        loadAudioFile (first, size, flags);

      #ifdef _WIN32
        // windows file map close
        UnmapViewOfFile (first);
        CloseHandle (fileHandle);
      #else
        // linux mmap close to do
        munmap (first, size);
        close (fd);
      #endif

      //{{{  next file
      //if (mSong->getChanged()) // use changed fileIndex
        //mSong->setChanged (false);
      //else if (!mFileList->nextIndex())
        //mExit = true;
      //}}}
      }

    cLog::log (LOGINFO, "exit");
    mRunning = false;
    } ).detach();
  }
//}}}
//{{{
void cLoader::icycast (const string& url) {

  stopAndWait();

  thread ([=]() {
    // lambda
    cLog::setThreadName ("icyL");
    mRunning = true;

    iAudioDecoder* audioDecoder = nullptr;
    while (!mExit) {
      int icySkipCount = 0;
      int icySkipLen = 0;
      int icyInfoCount = 0;
      int icyInfoLen = 0;
      char icyInfo[255] = { 0 };

      uint8_t bufferFirst[4096];
      uint8_t* bufferEnd = bufferFirst;
      uint8_t* buffer = bufferFirst;

      // init container and audDecode

      // init http
      cPlatformHttp http;
      cUrl parsedUrl;
      parsedUrl.parse (url);

      int64_t pts = -1;
      http.get (parsedUrl.getHost(), parsedUrl.getPath(), "Icy-MetaData: 1",
        //{{{  headerCallback lambda
        [&](const string& key, const string& value) noexcept {
          if (key == "icy-metaint")
            icySkipLen = stoi (value);
          },
        //}}}
        //{{{  dataCallback lambda
        [&] (const uint8_t* data, int length) noexcept {
          // return false to exit

          // cLog::log (LOGINFO, "callback %d", length);
          if ((icyInfoCount >= icyInfoLen) && (icySkipCount + length <= icySkipLen)) {
            //{{{  simple copy of whole body, no metaInfo
            //cLog::log (LOGINFO1, "body simple copy len:%d", length);
            memcpy (bufferEnd, data, length);
            bufferEnd += length;
            icySkipCount += length;
            }
            //}}}
          else {
            //{{{  dumb copy for metaInfo straddling body, could be much better
            //cLog::log (LOGINFO1, "body split copy length:%d info:%d:%d skip:%d:%d ",
                                  //length, icyInfoCount, icyInfoLen, icySkipCount, icySkipLen);
            for (int i = 0; i < length; i++) {
              if (icyInfoCount < icyInfoLen) {
                icyInfo [icyInfoCount] = data[i];
                icyInfoCount++;
                if (icyInfoCount >= icyInfoLen)
                  addIcyInfo (pts, icyInfo);
                }
              else if (icySkipCount >= icySkipLen) {
                icyInfoLen = data[i] * 16;
                icyInfoCount = 0;
                icySkipCount = 0;
                //cLog::log (LOGINFO1, "body icyInfo len:", data[i] * 16);
                }
              else {
                icySkipCount++;
                *bufferEnd = data[i];
                bufferEnd++;
                }
              }

            return !mExit;
            }
            //}}}

          int sampleRate = 0;
          int numChannels =  0;
          auto frameType = cAudioParser::parseSomeFrames (bufferFirst, bufferEnd, numChannels, sampleRate);
          audioDecoder = createAudioDecoder (frameType);

          int frameSize;
          while (cAudioParser::parseFrame (buffer, bufferEnd, frameSize)) {
            auto samples = audioDecoder->decodeFrame (buffer, frameSize, pts);
            if (samples) {
              if (!mSong) {
                // enough data to determine frameType and sampleRate (wrong for aac sbr)
                pts = 0;
                mSong = new cSong (frameType, audioDecoder->getNumChannels(),
                                   audioDecoder->getSampleRate(), audioDecoder->getNumSamplesPerFrame(),
                                   1000);
                }

              mSong->addFrame (true, pts, samples, true, mSong->getNumFrames() + 1);
              pts += mSong->getFramePtsDuration();

              if (!mSongPlayer)
                mSongPlayer = new cSongPlayer(mSong, true);
              }
            buffer += frameSize;
            }

          if ((buffer > bufferFirst) && (buffer < bufferEnd)) {
            //{{{  shuffle down last partial frame
            auto bufferLeft = int(bufferEnd - buffer);
            memcpy (bufferFirst, buffer, bufferLeft);
            bufferEnd = bufferFirst + bufferLeft;
            buffer = bufferFirst;
            }
            //}}}

          return !mExit;
          }
        //}}}
        );

      cLog::log (LOGINFO, "icyThread");
      if (mSongPlayer)
        mSongPlayer->stopAndWait();
      }

    //{{{  delete resources
    delete mSongPlayer;

    auto tempSong = mSong;
    mSong = nullptr;
    delete tempSong;

    delete audioDecoder;
    //}}}
    cLog::log (LOGINFO, "exit");
    mRunning = false;
    } ).detach();
  }
//}}}

//{{{
void cLoader::skipped() {
  }
//}}}
//{{{
void cLoader::stopAndWait() {

  if (mRunning) {
    mExit = true;

    if (mSongPlayer)
      mSongPlayer->stopAndWait();

    while (mRunning) {
      this_thread::sleep_for (100ms);
      cLog::log(LOGINFO, "waiting to exit");
      }
    }
  }
//}}}

// private
//{{{
void cLoader::addIcyInfo (int64_t pts, const string& icyInfo) {

  cLog::log (LOGINFO, "addIcyInfo " + icyInfo);

  string icysearchStr = "StreamTitle=\'";
  string searchStr = "StreamTitle=\'";
  auto searchStrPos = icyInfo.find (searchStr);
  if (searchStrPos != string::npos) {
    auto searchEndPos = icyInfo.find ("\';", searchStrPos + searchStr.size());
    if (searchEndPos != string::npos) {
      string titleStr = icyInfo.substr (searchStrPos + searchStr.size(), searchEndPos - searchStrPos - searchStr.size());
      if (titleStr != mLastTitleStr) {
        cLog::log (LOGINFO1, "addIcyInfo found title = " + titleStr);
        mSong->getSelect().addMark (pts, titleStr);
        mLastTitleStr = titleStr;
        }
      }
    }

  string urlStr = "no url";
  searchStr = "StreamUrl=\'";
  searchStrPos = icyInfo.find (searchStr);
  if (searchStrPos != string::npos) {
    auto searchEndPos = icyInfo.find ('\'', searchStrPos + searchStr.size());
    if (searchEndPos != string::npos) {
      urlStr = icyInfo.substr (searchStrPos + searchStr.size(), searchEndPos - searchStrPos - searchStr.size());
      cLog::log (LOGINFO1, "addIcyInfo found url = " + urlStr);
      }
    }
  }
//}}}

//{{{
void cLoader::loadTsFile (uint8_t* first, int size, eFlags flags) {

  auto timePoint = system_clock::now();

  cPtsSong* ptsSong = new cPtsSong (eAudioFrameType::eAacAdts, 2, 48000, 1024, 1920, 0);
  mSong = ptsSong;

  int64_t loadPts = -1;
  iVideoPool* videoPool = nullptr;
  iAudioDecoder* audioDecoder = nullptr;
  //{{{  init parsers, callbacks
  auto addAudioFrameCallback = [&](bool reuseFromFront, float* samples, int64_t pts) noexcept {
    if (loadPts < 0)
      ptsSong->setBasePts (pts);
    loadPts = pts;
    mSong->addFrame (reuseFromFront, pts, samples, true, mSong->getNumFrames()+1);
    if (!mSongPlayer)
      mSongPlayer = new cSongPlayer (mSong, true);
    };

  auto addStreamCallback = [&](int sid, int pid, int type) noexcept {
    if (mPidParsers.find (pid) == mPidParsers.end()) {
      // new stream pid
      auto it = mServices.find (sid);
      if (it != mServices.end()) {
        cService* service = (*it).second;
        switch (type) {
          //{{{
          case 15: // aacAdts
            service->setAudioPid (pid);

            if (service->isSelected()) {
              audioDecoder = createAudioDecoder (eAudioFrameType::eAacAdts);
              mPidParsers.insert (map<int,cPidParser*>::value_type (pid,
                new cAudioPesParser (pid, audioDecoder, true, addAudioFrameCallback)));
              }

            break;
          //}}}
          //{{{
          case 17: // aacLatm
            service->setAudioPid (pid);

            if (service->isSelected()) {
              audioDecoder = createAudioDecoder (eAudioFrameType::eAacLatm);
              mPidParsers.insert (map<int,cPidParser*>::value_type (pid,
                new cAudioPesParser (pid, audioDecoder, true, addAudioFrameCallback)));
              }

            break;
          //}}}
          //{{{
          case 27: // h264video
            service->setVideoPid (pid);

            if (service->isSelected()) {
              videoPool = iVideoPool::create (flags & eFlags::eFFmpeg, 100, mSong);
              mPidParsers.insert (map<int,cPidParser*>::value_type (pid, new cVideoPesParser (pid, videoPool, true)));
              mVideoPool = videoPool;
              }

            break;
          //}}}
          //{{{
          case 6:  // do nothing - subtitle
            //cLog::log (LOGINFO, "subtitle %d %d", pid, type);
            break;
          //}}}
          //{{{
          case 2:  // do nothing - ISO 13818-2 video
            //cLog::log (LOGERROR, "mpeg2 video %d", pid, type);
            break;
          //}}}
          //{{{
          case 3:  // do nothing - ISO 11172-3 audio
            //cLog::log (LOGINFO, "mp2 audio %d %d", pid, type);
            break;
          //}}}
          //{{{
          case 5:  // do nothing - private mpeg2 tabled data
            break;
          //}}}
          //{{{
          case 11: // do nothing - dsm cc u_n
            break;
          //}}}
          default:
            cLog::log (LOGERROR, "loadTs - unrecognised stream type %d %d", pid, type);
          }
        }
      else
        cLog::log (LOGERROR, "loadTs - PMT:%d for unrecognised sid:%d", pid, sid);
      }
    };

  auto addProgramCallback = [&](int pid, int sid) noexcept {
    if ((sid > 0) && (mPidParsers.find (pid) == mPidParsers.end())) {
      cLog::log (LOGINFO, "PAT adding pid:service %d::%d", pid, sid);
      mPidParsers.insert (map<int,cPidParser*>::value_type (pid, new cPmtParser (pid, sid, addStreamCallback)));

      // select first service in PAT
      mServices.insert (map<int,cService*>::value_type (sid, new cService (sid, mCurSid == -1)));
      if (mCurSid == -1)
        mCurSid = sid;
      }
    };

  mPidParsers.insert (map<int,cPidParser*>::value_type (0x00, new cPatParser (addProgramCallback)));
  //}}}

  uint8_t* ts = first;
  uint8_t* last = first + size;
  while (!mExit && (ts + 188 <= last)) {
    if (ts[0] == 0x47) {
      auto it = mPidParsers.find (((ts[1] & 0x1F) << 8) | ts[2]);
      if (it != mPidParsers.end())
        it->second->parse (ts, true);
      ts += 188;
      }
    else {
      //{{{  sync error
      cLog::log (LOGERROR, "ts packet nosync");
      ts++;
      }
      //}}}
    mLoadFrac = float(ts - first) / size;

    int64_t playPts = mSong->getPlayPts();

    if ((loadPts >= 0) && (loadPts < playPts))
      cLog::log (LOGINFO, "skip back ", getPtsFramesString (loadPts, mSong->getFramePtsDuration()));
    if ((loadPts >= 0) && (loadPts > playPts + (3 * 90000)))
      cLog::log (LOGINFO, "skip forward ", getPtsFramesString (loadPts, mSong->getFramePtsDuration()));

    // block loading when load pts is >100 audio frames ahead of play frameNum
    while (!mExit && (loadPts > mSong->getPlayPts() + (100 * mSong->getFramePtsDuration())))
      this_thread::sleep_for (20ms);
    }
  // finish parsers
  for (auto parser : mPidParsers)
    parser.second->processLast (true);
  mLoadFrac = 0.f;

  cLog::log (LOGINFO, "load took %dms", duration_cast<milliseconds>(system_clock::now() - timePoint).count());
  if (mSongPlayer)
    mSongPlayer->wait();

  //{{{  delete resources
  for (auto parser : mPidParsers) {
    //{{{  stop and delete pidParsers
    parser.second->stop();
    delete parser.second;
    }
    //}}}
  mPidParsers.clear();

  mSong = nullptr;
  mVideoPool = nullptr;

  delete ptsSong;
  delete videoPool;
  delete audioDecoder;
  delete mSongPlayer;
  //}}}
  }
//}}}
//{{{
void cLoader::loadAudioFile (uint8_t* first, int size, eFlags flags) {
// wav,aac,mp3

  auto timePoint = system_clock::now();

  int numChannels = 0;
  int sampleRate = 0;
  uint8_t* last = first + size;
  eAudioFrameType fileFrameType = cAudioParser::parseSomeFrames (first, last, numChannels, sampleRate);
  iAudioDecoder* decoder = createAudioDecoder (fileFrameType);

  //{{{  jpeg
  //int jpegLen;
  //if (getJpeg (jpegLen)) {
    //{{{  found jpegImage
    //cLog::log (LOGINFO, "found jpeg piccy in file, load it");
    ////delete (cAudioParser::mJpegPtr);
    ////cAudioParser::mJpegPtr = nullptr;
    //}
    //}}}
  //}}}
  int64_t pts = 0;
  uint8_t* frame = first;
  if (fileFrameType == eAudioFrameType::eWav) {
    // wav - samples point into mmap file directly
    constexpr int kWavFrameSamples = 1024;
    mSong = new cSong (fileFrameType, numChannels, sampleRate, kWavFrameSamples, 0);

    int frameSize = 0;
    frame = cAudioParser::parseFrame (frame, last, frameSize);
    while (!mExit &&
           ((frame + (kWavFrameSamples * numChannels * 4)) <= last)) {
      // samples = frame
      mSong->addFrame (true, pts, (float*)frame, false, size / (kWavFrameSamples * numChannels * 4));
      if (!mSongPlayer)
        mSongPlayer = new cSongPlayer (mSong, false);

      pts += mSong->getFramePtsDuration();
      frame += kWavFrameSamples * 2 * sizeof(float);
      mLoadFrac = float(frame - first) / size;
      }
    }

  else {
    // aacAdts, mp3
    int frameSize = 0;
    while (!mExit &&
           cAudioParser::parseFrame (frame, last, frameSize)) {
      float* samples = decoder->decodeFrame (frame, frameSize, pts);
      if (samples) {
        if (!mSong) // first decoded frame gives aacHE sampleRate,samplesPerFrame
          mSong = new cSong (fileFrameType, decoder->getNumChannels(), decoder->getSampleRate(),
                             decoder->getNumSamplesPerFrame(), 0);
        int64_t totalFrames = 1 + (mSong->getNumFrames() > 0 ? (size / (int(frame - first) / mSong->getNumFrames())) : 0);
        mSong->addFrame (true, pts, samples, true, totalFrames);
        if (!mSongPlayer)
          mSongPlayer = new cSongPlayer (mSong, false);
        pts += mSong->getFramePtsDuration();
        }

      frame += frameSize;
      mLoadFrac = float(frame - first) / size;
      }
    }
  mLoadFrac = 0.f;

  // duration info
  cLog::log (LOGINFO, "load took %dms", duration_cast<milliseconds>(system_clock::now() - timePoint).count());
  mSongPlayer->wait();

  //{{{  delete resource
  auto tempSong = mSong;
  mSong = nullptr;
  delete mSongPlayer;
  delete tempSong;
  delete decoder;
  //}}}
  }
//}}}
