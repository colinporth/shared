// cLoaderPlayer.cpp - audio,video loader,player
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include "cLoaderPlayer.h"

#include <functional>

// utils
#include "../date/date.h"
#include "../utils/utils.h"
#include "../utils/cLog.h"

// container
#include "../utils/cSong.h"

// audio decode
#include "../decoders/cAudioDecode.h"

// audio device
#ifdef _WIN32
  #include "../audio/audioWASAPI.h"
  #include "../audio/cWinAudio16.h"
#else
  #include "../audio/cLinuxAudio.h"
#endif

// video decode
#include "../utils/cVideoDecode.h"

// net
#ifdef _WIN32
  #include "../net/cWinSockHttp.h"
#else
  #include "../net/cLinuxHttp.h"
#endif

#ifdef _WIN32
  //#include "../common/cJpegImage.h"
  #include "../../shared/utils/cFileList.h"
#endif

#include "readerWriterQueue.h"

using namespace std;
using namespace chrono;
//}}}

//{{{
class cTsParser {
// extract pes from ts
public:
  cTsParser (int pid, const string& name) : mPid(pid), mName(name) {}
  virtual ~cTsParser() {}

  virtual int getQueueSize() { return 0; }
  virtual float getQueueFrac() { return 0.f; }

  virtual void clear (int num) {}
  virtual void stop() {}

  //{{{
  void parse (uint8_t* ts, bool afterPlay) {

    bool payloadStart = ts[1] & 0x40;
    int continuityCount = ts[3] & 0x0F;
    int headerSize = 1 + ((ts[3] & 0x20) ? 4 + ts[4] : 3);
    ts += headerSize;
    int tsLeft = 188 - headerSize;

    processBody (ts, tsLeft, payloadStart, continuityCount, afterPlay);
    }
  //}}}
  virtual void processLast (bool afterPlay) {}

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
  virtual void processBody (uint8_t* ts, int tsLeft, bool payloadStart, int continuityCount, bool afterPlay) {
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
  };
//}}}
//{{{
class cPatParser : public cTsParser {
public:
  cPatParser (function<void (int programPid, int programSid)> callback)
    : cTsParser (0, "pat"), mCallback(callback) {}
  virtual ~cPatParser() {}

  virtual void processBody (uint8_t* ts, int tsLeft, bool payloadStart, int continuityCount, bool afterPlay) {

    if (payloadStart) {
      //int pointerField = ts[0]
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
class cPmtParser : public cTsParser {
public:
  cPmtParser (int pid, int sid, function<void (int streamSid, int streamPid, int streamType)> callback)
    : cTsParser (pid, "pmt"), mSid(sid), mCallback(callback) {}
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

  virtual void processBody (uint8_t* ts, int tsLeft, bool payloadStart, int continuityCount, bool afterPlay) {

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
class cTdtParser : public cTsParser {
public:
  cTdtParser() : cTsParser (0x14, "tdt") {}
  virtual ~cTdtParser() {}
  };
//}}}
//{{{
class cPesParser : public cTsParser {
//{{{
class cPesItem {
public:
  //{{{
  cPesItem (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts)
      : mAfterPlay(afterPlay), mPesSize(size), mNum(num), mPts(pts) {
    mPes = (uint8_t*)malloc (size);
    memcpy (mPes, pes, size);
    }
  //}}}
  //{{{
  ~cPesItem() {
    free (mPes);
    }
  //}}}

  bool mAfterPlay;
  uint8_t* mPes;
  const int mPesSize;
  const int mNum;
  const int64_t mPts;
  };
//}}}
public:
  //{{{
  cPesParser (int pid, const string& name, bool useQueue = true) : cTsParser(pid, name), mUseQueue(useQueue) {

    mPes = (uint8_t*)malloc (kInitPesSize);
    if (useQueue)
      thread ([=](){ dequeThread(); }).detach();
    }
  //}}}
  virtual ~cPesParser() {}

  virtual int getQueueSize() { return (int)mQueue.size_approx(); }
  virtual float getQueueFrac() { return (float)mQueue.size_approx() / mQueue.max_capacity(); }

  virtual void clear (int num) = 0;
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
  virtual void processBody (uint8_t* ts, int tsLeft, bool payloadStart, int continuityCount, bool afterPlay) {

    if ((mContinuityCount >= 0) &&
        (continuityCount != ((mContinuityCount + 1) & 0xF)))
      cLog::log (LOGERROR, "continuity count error pid:%d %d %d", mPid, continuityCount, mContinuityCount);
    mContinuityCount = continuityCount;

    if (payloadStart) {
      // end of last pes, if any, process it
      processLast (afterPlay);

      if (ts[7] & 0x80)
        mPts = getPts (ts+9);

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
  virtual void dispatchDecode (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts) {
    if (mUseQueue)
      mQueue.enqueue (new cPesParser::cPesItem (afterPlay, pes, size, num, pts));
    else
      decode (afterPlay, pes, size, num, pts);
    }
  //}}}
  virtual void decode (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts) = 0;

protected:
  //{{{
  void dequeThread() {

    cLog::setThreadName (mName + "Q");

    mQueueRunning = true;

    while (!mQueueExit) {
      cPesItem* pesItem;
      if (mQueue.wait_dequeue_timed (pesItem, 40000)) {
        decode (pesItem->mAfterPlay, pesItem->mPes, pesItem->mPesSize, pesItem->mNum, pesItem->mPts);
        delete pesItem;
        }
      }

    mQueueRunning = false;
    }
  //}}}

  int mAllocSize = kInitPesSize;

  uint8_t* mPes;
  int mPesSize = 0;
  int mNum = 0;
  int64_t mPts = 0;
  int mContinuityCount = -1;

private:
  static constexpr int kInitPesSize = 4096;
  //{{{
  static int64_t getPts (const uint8_t* ts) {
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

  bool mUseQueue = true;
  bool mQueueExit = false;
  bool mQueueRunning = false;
  readerWriterQueue::cBlockingReaderWriterQueue <cPesItem*> mQueue;
  };
//}}}
//{{{
class cAudioPesParser : public cPesParser {
public:
  cAudioPesParser (int pid, cAudioDecode* audioDecode, int num,
                   function <void (bool afterPlay, float* samples, int num, int64_t pts)> callback)
      : cPesParser(pid, "aud"), mAudioDecode(audioDecode), mCallback(callback) {
    mNum = num;
    }
  virtual ~cAudioPesParser() {}

  virtual void clear (int num) {
  // num is cSong frameNum, audio frames since midnight
    mNum = num;
    mPesSize = 0;
    mPts = 0;
    }

  virtual void processLast (bool afterPlay) {
  // count audio frames in pes

    if (mPesSize) {
      int numFrames = 0;
      uint8_t* framePes = mPes;
      while (mAudioDecode->parseFrame (framePes, mPes + mPesSize)) {
        int framePesSize = mAudioDecode->getNextFrameOffset();
        framePes += framePesSize;
        numFrames++;
        }

      dispatchDecode (afterPlay, mPes, mPesSize, mNum, mPts);
      mNum += numFrames;
      mPesSize = 0;
      }
    }

  virtual void decode (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts) {
  // decode pes to audio frames

    uint8_t* framePes = pes;
    while (mAudioDecode->parseFrame (framePes, pes + size)) {
      // decode a single frame from pes
      int framePesSize = mAudioDecode->getNextFrameOffset();
      float* samples = mAudioDecode->decodeFrame (framePes, framePesSize, num, pts);
      if (samples)
        mCallback (afterPlay, samples, num, pts);
      else
        cLog::log (LOGERROR, "cAudioPesParser decode failed %d %d", size, num);

      // pts of next frame in pes, assumes 48000 sample rate
      pts += (mAudioDecode->getNumSamplesPerFrame() * 90) / 48;
      num++;

      // point to next frame in pes
      framePes += framePesSize;
      }
    }

private:
  cAudioDecode* mAudioDecode;
  function <void (bool afterPlay, float* samples, int num, int64_t pts)> mCallback;
  };
//}}}
//{{{
class cVideoPesParser : public cPesParser {
public:
  cVideoPesParser (int pid, cVideoDecode* videoDecode, function <void (int64_t pts)> callback)
    : cPesParser (pid, "vid"), mVideoDecode(videoDecode), mCallback(callback) {}
  virtual ~cVideoPesParser() {}

  virtual void clear (int num) {
  // use num as frame in chunk for ffmpeg pts synthesis

    mNum = 0;
    mPesSize = 0;
    mPts = 0;
    }

  virtual void processLast (bool afterPlay) {
    if (mPesSize) {
      int numFrames = 1;
      dispatchDecode (afterPlay, mPes, mPesSize, mNum, mPts);
      mNum += numFrames;
      mPesSize = 0;
      }

    }
  void decode (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts) {
    mVideoDecode->decodeFrame (afterPlay, pes, size, num, pts);
    mCallback (pts);
    }

private:
  cVideoDecode* mVideoDecode;
  function <void (int64_t pts)> mCallback;
  };
//}}}
//{{{
class cTestAudioPesParser : public cPesParser {
public:
  cTestAudioPesParser (int pid, cAudioDecode* audioDecode, int num,
                       function <void (bool afterPlay, float* samples, int num, int64_t pts)> callback)
      : cPesParser(pid, "aud"), mAudioDecode(audioDecode), mCallback(callback) {
    mNum = num;
    }
  virtual ~cTestAudioPesParser() {}

  virtual void clear (int num) {
  // num is cSong frameNum, audio frames since midnight
    mNum = num;
    mPesSize = 0;
    mPts = 0;
    }

  virtual void processLast (bool afterPlay) {

    if (mPesSize) {
      cLog::log (LOGINFO, "audio pes %d %d", mPesSize, mNum);
      mNum += 1;
      mPesSize = 0;
      }
    }

  virtual void decode (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts) {
    }

private:
  cAudioDecode* mAudioDecode;
  function <void (bool afterPlay, float* samples, int num, int64_t pts)> mCallback;
  };
//}}}
//{{{
class cTestVideoPesParser : public cPesParser {
public:
  cTestVideoPesParser(int pid, cVideoDecode* videoDecode, function <void (int64_t pts)> callback)
    : cPesParser (pid, "vid"), mVideoDecode(videoDecode), mCallback(callback) {}
  virtual ~cTestVideoPesParser() {}

  virtual void clear (int num) {
  // use num as frame in chunk for ffmpeg pts synthesis

    mNum = 0;
    mPesSize = 0;
    mPts = 0;
    }

  virtual void processLast (bool afterPlay) {
    if (mPesSize) {
      cLog::log (LOGINFO, "video pes %d %d", mPesSize, mNum);
      mNum += 1;
      mPesSize = 0;
      }
    }

  void decode (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts) {
    }

private:
  cVideoDecode* mVideoDecode;
  function <void (int64_t pts)> mCallback;
  };
//}}}
//{{{
//class cSdtParser : public cTsParser {
//public:
  //cSdtParser() : cTsParser (0x11, "sdt") {}
  //virtual ~cSdtParser() {}
  //};
//}}}
//{{{
//class cEitParser : public cTsParser {
//public:
  //cEitParser() : cTsParser (0x12, "eit") {}
  //virtual ~cEitParser() {}

  //virtual void processBody (uint8_t* ts, int tsLeft, bool payloadStart, int continuityCount, bool afterPlay) {
    ////cLog::log (LOGINFO, "EIT ts " + dec (tsLeft) + ":" + info);
    //}
  //};
//}}}

// public
cLoaderPlayer::cLoaderPlayer() {}
//{{{
cLoaderPlayer::~cLoaderPlayer() {
  clear();
  }
//}}}

//{{{
void cLoaderPlayer::getFrac (float& loadFrac, float& videoFrac, float& audioFrac) {
// return fracs for spinner graphic, true if ok to display

  loadFrac = mLoadFrac;
  //audioFrac = mParsers.size() > 0 ? mParsers[0]->getQueueFrac() : 0.f;
  //videoFrac = mParsers.size() > 1 ? mParsers[1]->getQueueFrac() : 0.f;
  audioFrac = 0.f;
  videoFrac = 0.f;
  }
//}}}
//{{{
void cLoaderPlayer::getSizes (int& loadSize, int& videoQueueSize, int& audioQueueSize) {
// return sizes

  loadSize = mLoadSize;
  audioQueueSize = 0;
  videoQueueSize = 0;
  //audioQueueSize = mParsers.size() > 0 ? mParsers[0]->getQueueSize() : 0;
  //videoQueueSize = mParsers.size() > 1 ? mParsers[1]->getQueueSize() : 0;
  }
//}}}

//{{{
void cLoaderPlayer::videoFollowAudio() {

  if (mVideoDecode) {
    // get play frame
    auto framePtr = mSong->getFramePtr (mSong->getPlayFrame());
    if (framePtr) {
      auto pts = framePtr->getPts();
      mVideoDecode->setPlayPts (pts);
      mVideoDecode->clear (pts);
      }
    }
  }
//}}}

//{{{
void cLoaderPlayer::hlsLoaderThread (bool radio, const string& channelName,
                                     int audBitrate, int vidBitrate, eLoaderFlags loaderFlags) {
// hls http chunk load and possible decode thread

  const string hostName = radio ? "as-hls-uk-live.bbcfmt.s.llnwi.net" : "vs-hls-uk-live.akamaized.net";
  const string kM3u8 = ".norewind.m3u8";
  constexpr int kVideoPoolSize = 128;

  cLog::setThreadName ("hlsL");
  mRunning = true;
  mExit = false;

  // audBitrate < 128000 use aacHE, more samplesPerframe, less framesPerChunk
  mSong = new cSong();
  mSong->initialise (cAudioDecode::eAac, 2, 48000, audBitrate < 128000 ? 2048 : 1024, 1000);
  mSong->setBitrateFramesPerChunk (audBitrate, audBitrate < 128000 ? (radio ? 150 : 180) : (radio ? 300 : 360));
  mSong->setChannel (channelName);

  int chunkNum;
  int frameNum;

  // parser callbacks
  auto addAudioFrameCallback = [&](bool afterPlay, float* samples, int num, int64_t pts) noexcept {
    //{{{  addAudioFrame lambda
    mSong->addFrame (afterPlay, num, samples, true, mSong->getNumFrames(), pts);
    startPlayer (true);
    };
    //}}}
  auto addVideoFrameCallback = [&](int64_t pts) noexcept {
    //{{{  addVideoFrame lambda
    // video frame decoded - nothing for now
    };
    //}}}
  auto addStreamCallback = [&](int sid, int pid, int type) noexcept {
    //{{{  addStream lambda
    if (mParsers.find (pid) == mParsers.end())
      // new stream seen, add stream parser
      switch (type) {
        case 15:
          mAudioDecode = new cAudioDecode (cAudioDecode::eAac);
          mParsers.insert (
            map<int,cTsParser*>::value_type (pid,
              new cAudioPesParser (pid, mAudioDecode, frameNum, addAudioFrameCallback)));
          break;

        case 27:
          if (vidBitrate) {
            mVideoDecode = cVideoDecode::create (loaderFlags & eFFmpeg, kVideoPoolSize);
            mParsers.insert (
              map<int,cTsParser*>::value_type (pid,
                new cVideoPesParser (pid, mVideoDecode, addVideoFrameCallback)));
            }
          break;

        default:
          cLog::log (LOGERROR, "unrecognised stream type %d %d", pid, type);
        }
    };
    //}}}
  auto addProgramCallback = [&](int pid, int sid) noexcept {
    //{{{  addProgram lambda
    if (mParsers.find (pid) == mParsers.end())
      mParsers.insert (
        map<int,cTsParser*>::value_type (pid,
          new cPmtParser (pid, sid, addStreamCallback)));
    };
    //}}}

  // add PAT parser
  mParsers.insert (map<int,cTsParser*>::value_type (0x00, new cPatParser (addProgramCallback)));

  while (!mExit && !mSong->getChanged()) {
    mSong->setChanged (false);
    cPlatformHttp http;
    string pathName = getHlsPathName (radio, vidBitrate);
    string redirectedHostName = http.getRedirect (hostName, pathName + kM3u8);
    if (http.getContent()) {
      //{{{  parse m3u8 for mediaSequence,programDateTimePoint
      int extXMediaSequence = stoi (getTagValue (http.getContent(), "#EXT-X-MEDIA-SEQUENCE:"));
      istringstream inputStream (getTagValue (http.getContent(), "#EXT-X-PROGRAM-DATE-TIME:"));

      system_clock::time_point extXProgramDateTimePoint;
      inputStream >> date::parse ("%FT%T", extXProgramDateTimePoint);

      http.freeContent();

      mSong->setHlsBase (extXMediaSequence, extXProgramDateTimePoint, -37s);
      //}}}
      while (!mExit && !mSong->getChanged()) {
        if (mSong->loadChunk (system_clock::now(), 2, chunkNum, frameNum)) {
          bool chunkAfterPlay = frameNum >= mSong->getPlayFrame();
          for (auto parser : mParsers)
            parser.second->clear (frameNum);
          int contentParsed = 0;
          if (http.get (redirectedHostName, pathName + '-' + dec(chunkNum) + ".ts", "",
                        [&] (const string& key, const string& value) noexcept {
                          //{{{  header lambda
                          if (key == "content-length")
                            cLog::log (LOGINFO, "chunk:" + dec(chunkNum) +
                                                " frame:" + dec(frameNum) +
                                                " size:" + dec(http.getHeaderContentSize()/1000) + "k");
                          },
                          //}}}
                        [&] (const uint8_t* data, int length) noexcept {
                          //{{{  data lambda
                          mLoadSize = http.getContentSize();
                          mLoadFrac = float(http.getContentSize()) / http.getHeaderContentSize();

                          if (!radio) {
                            // tsParse as we go
                            while (http.getContentSize() - contentParsed >= 188) {
                              uint8_t* ts = http.getContent() + contentParsed;
                              if (*ts == 0x47) {
                                int pid = ((ts[1] & 0x1F) << 8) | ts[2];
                                auto it = mParsers.find (pid);
                                if (it != mParsers.end())
                                  it->second->parse (ts, chunkAfterPlay);
                                else
                                  cLog::log (LOGERROR, "pid parser not found %d", pid);
                                ts += 188;
                                }
                              else
                                cLog::log (LOGERROR, "ts packet sync:%d", contentParsed);
                              contentParsed += 188;
                              }
                            }
                          return true;
                          }
                          //}}}
                        ) == 200) {
            // ??? why does pesParser fail for radio audio pes ???
            if (radio) {
              //{{{  parse chunk of ts
              // extract audio pes from chunk of ts packets, write it back crunched into ts, always gets smaller as ts stripped
              uint8_t* tsPtr = http.getContent();
              uint8_t* tsEndPtr = tsPtr + http.getContentSize();
              uint8_t* pesPtr = tsPtr;
              while ((tsPtr < tsEndPtr) && (*tsPtr++ == 0x47)) {
                // ts packet start
                auto pid = ((tsPtr[0] & 0x1F) << 8) | tsPtr[1];
                if (pid == 34) {
                  // audio pid
                  bool payStart = tsPtr[0] & 0x40;
                  int headerBytes = (tsPtr[2] & 0x20) ? 4 + tsPtr[3] : 3;
                  tsPtr += headerBytes;
                  int tsBodyBytes = 187 - headerBytes;
                  if (payStart) {
                    int pesHeaderBytes = 9 + tsPtr[8];
                    tsPtr += pesHeaderBytes;
                    tsBodyBytes -= pesHeaderBytes;
                    }

                  // copy ts payload back into same buffer, always copying to lower address in same buffer
                  memcpy (pesPtr, tsPtr, tsBodyBytes);
                  pesPtr += tsBodyBytes;
                  tsPtr += tsBodyBytes;
                  }
                else // PAT and PMT 0x20 pids expected
                  tsPtr += 187;
                }

              if (!mAudioDecode)
                mAudioDecode = new cAudioDecode (cAudioDecode::eAac);

              // parse audio pes for audio frames
              uint8_t* pesEnd = pesPtr;
              pesPtr = http.getContent();
              while (getAudioDecode()->parseFrame (pesPtr, pesEnd)) {
                float* samples = getAudioDecode()->decodeFrame (frameNum);
                if (samples) {
                  mSong->addFrame (chunkAfterPlay, frameNum++, samples, true, mSong->getNumFrames(), 0);
                  startPlayer (true);
                  }
                else
                  cLog::log (LOGERROR, "aud parser failed to decode %d", frameNum);

                pesPtr += getAudioDecode()->getNextFrameOffset();
                }
              }
              //}}}
              //{{{  pesParse chunk of ts - should work but fails !!! find out why !!!
              //uint8_t* ts = http.getContent();
              //uint8_t* tsEnd = ts + http.getContentSize();
              //while ((ts < tsEnd) && (*ts == 0x47)) {
                //mParsers[0]->parse (ts, chunkAfterPlay);
                //ts += 188;
                //}

              //mParsers[0]->process (afterPlay);
              //}
              //}}}
            else
              for (auto parser : mParsers)
                parser.second->processLast (chunkAfterPlay);
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

      // wait for playerThread to end
      mPlayer.join();
      }
    }

  clear();
  cLog::log (LOGINFO, "exit");
  mRunning = false;
  }
//}}}
//{{{
void cLoaderPlayer::icyLoaderThread (const string& url) {

  cLog::setThreadName ("icyL");

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
    mSong = new cSong();
    cAudioDecode decode (cAudioDecode::eAac);

    // init http
    cPlatformHttp http;
    cUrl parsedUrl;
    parsedUrl.parse (url);

    int frameNum = -1;
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
                addIcyInfo (frameNum, icyInfo);
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

          return !mExit && !mSong->getChanged();
          }
          //}}}

        if (frameNum == -1) {
          // enough data to determine frameType and sampleRate (wrong for aac sbr)
          frameNum = 0;
          int sampleRate;
          auto frameType = cAudioDecode::parseSomeFrames (bufferFirst, bufferEnd, sampleRate);
          mSong->initialise (frameType, 2, 44100, (frameType == cAudioDecode::eMp3) ? 1152 : 2048, 3000);
          }

        while (decode.parseFrame (buffer, bufferEnd)) {
          if (decode.getFrameType() == mSong->getFrameType()) {
            auto samples = decode.decodeFrame (frameNum);
            if (samples) {
              mSong->setFixups (decode.getNumChannels(), decode.getSampleRate(), decode.getNumSamplesPerFrame());
              mSong->addFrame (true, frameNum++, samples, true, mSong->getNumFrames()+1, 0);
              startPlayer (true);
              }
            }
          buffer += decode.getNextFrameOffset();
          }

        if ((buffer > bufferFirst) && (buffer < bufferEnd)) {
          // shuffle down last partial frame
          auto bufferLeft = int(bufferEnd - buffer);
          memcpy (bufferFirst, buffer, bufferLeft);
          bufferEnd = bufferFirst + bufferLeft;
          buffer = bufferFirst;
          }

        return !mExit && !mSong->getChanged();
        }
      //}}}
      );

    cLog::log (LOGINFO, "icyThread songChanged");
    mPlayer.join();
    }

  clear();
  cLog::log (LOGINFO, "exit");
  }
//}}}
//{{{
void cLoaderPlayer::fileLoaderThread (const string& filename) {

  cLog::setThreadName ("file");

  while (!mExit) {
    #ifdef _WIN32
      //{{{  windows file map
      HANDLE fileHandle = CreateFile (filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
      HANDLE fileMapping = CreateFileMapping (fileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
      uint8_t* fileFirst = (uint8_t*)MapViewOfFile (fileMapping, FILE_MAP_READ, 0, 0, 0);
      auto fileSize = GetFileSize (fileHandle, NULL);
      uint8_t* fileEnd = fileFirst + fileSize;
      //}}}
    #else
      //{{{  linux mmap to do
      uint8_t* fileFirst = nullptr;
      auto fileSize = 0;
      uint8_t* fileEnd = fileFirst + fileSize;
      //}}}
    #endif

    if ((fileFirst[0] == 0x47) && (fileFirst[188] == 0x47)) {
      //{{{  ts
      cLog::log (LOGINFO, "Ts file %d", fileSize);

      mSong = new cSong();
      mSong->initialise (cAudioDecode::eAac, 2, 48000, 1024, 1000);

      mAudioDecode = new cAudioDecode (cAudioDecode::eAac);
      mVideoDecode = cVideoDecode::create (true, 128);

      // parser callbacks
      auto addAudioFrameCallback = [&](bool afterPlay, float* samples, int num, int64_t pts) noexcept {
        //{{{  addAudioFrame lambda
        mSong->addFrame (afterPlay, num, samples, true, mSong->getNumFrames(), pts);
        startPlayer (true);
        };
        //}}}
      auto addVideoFrameCallback = [&](int64_t pts) noexcept {
        //{{{  addVideoFrame lambda
        // video frame decoded - nothing for now
        };
        //}}}
      auto addStreamCallback = [&](int sid, int pid, int type) noexcept {
        //{{{  addStream lambda
        if (mParsers.find (pid) == mParsers.end())
          switch (type) {
            case 6:  // subtitle
              break;

            case 15: // aac adts
              mParsers.insert (
                map<int,cTsParser*>::value_type (pid,
                  new cTestAudioPesParser (pid, mAudioDecode, 0, addAudioFrameCallback)));
              break;

            case 17: // aac latm
              mParsers.insert (
                map<int,cTsParser*>::value_type (pid,
                  new cTestAudioPesParser (pid, mAudioDecode, 0, addAudioFrameCallback)));
              break;

            case 27: // h264
              mParsers.insert (
                map<int,cTsParser*>::value_type (pid,
                  new cTestVideoPesParser (pid, mVideoDecode, addVideoFrameCallback)));
              break;

            default:
              cLog::log (LOGERROR, "unrecognised stream type %d %d", pid, type);
            }
        };
        //}}}
      auto addProgramCallback = [&](int pid, int sid) noexcept {
        //{{{  addProgram lambda
        if (mParsers.find (pid) == mParsers.end())
          mParsers.insert (
            map<int,cTsParser*>::value_type (pid,
              new cPmtParser (pid, sid, addStreamCallback)));
        };
        //}}}

      // add PAT parser
      mParsers.insert (map<int,cTsParser*>::value_type (0x00, new cPatParser (addProgramCallback)));
      mParsers.insert (map<int,cTsParser*>::value_type (0x14, new cTdtParser()));

      // parsers init
      for (auto parser : mParsers)
        parser.second->clear (0);

      uint8_t* ts = fileFirst;
      while (ts + 188 <= fileEnd) {
        if (ts[0] == 0x47) {
          int pid = ((ts[1] & 0x1F) << 8) | ts[2];
          auto it = mParsers.find (pid);
          if (it != mParsers.end())
            it->second->parse (ts, true);
          ts += 188;
          }
        else {
          cLog::log (LOGERROR, "ts packet nosync");
          ts++;
          }

        mLoadFrac = float(ts - fileFirst) / fileSize;
        }
      mLoadFrac = float(ts - fileFirst) / fileSize;

      // parsers finish
      for (auto parser : mParsers)
        parser.second->processLast (true);

      cLog::log (LOGINFO, "loaded");
      }
      //}}}
    else {
      //{{{  aac or mp3
      int sampleRate;
      auto fileFrameType = cAudioDecode::parseSomeFrames (fileFirst, fileEnd, sampleRate);
      cAudioDecode decode (fileFrameType);

      if (cAudioDecode::mJpegPtr) {
        //{{{  found jpegImage
        cLog::log (LOGINFO, "found jpeg piccy in file, load it");
        delete (cAudioDecode::mJpegPtr);
        cAudioDecode::mJpegPtr = nullptr;
        }
        //}}}

      mSong = new cSong();
      int frameNum = 0;
      if (fileFrameType == cAudioDecode::eWav) {
        // parse wav
        constexpr int kFrameSamples = 1024;
        mSong->initialise (fileFrameType, 2, sampleRate, kFrameSamples, 0);

        uint8_t* filePtr = fileFirst;
        decode.parseFrame (filePtr, fileEnd);
        auto samples = decode.getFramePtr();
        while (!mExit && !mSong->getChanged() && ((samples + (kFrameSamples * 2 * sizeof(float))) <= fileEnd)) {
          mSong->addFrame (true, frameNum++, (float*)samples, false, fileSize / (kFrameSamples * 2 * sizeof(float)), 0);
          samples += kFrameSamples * 2 * sizeof(float);
          startPlayer (false);
          mLoadFrac = float(samples - fileFirst) / fileSize;
          }
        mLoadFrac = float(samples - fileFirst) / fileSize;
        }

      else {
        // parse coded
        bool firstSamples = true;
        uint8_t* filePtr = fileFirst;
        while (!mExit && !mSong->getChanged() && decode.parseFrame (filePtr, fileEnd)) {
          // parsed frame
          if (decode.getFrameType() == fileFrameType) {
            // frame is expected type, will get id3 which we ignore
            float* samples = decode.decodeFrame (frameNum);
            if (samples) {
              if (firstSamples) // need to decode a frame to set sampleRate for aacHE, its wrong in the header
                mSong->initialise (fileFrameType,
                                   decode.getNumChannels(), decode.getSampleRate(), decode.getNumSamplesPerFrame(), 0);

              int numFrames = mSong->getNumFrames();
              int totalFrames = (numFrames > 0) ? (fileSize / (int(decode.getFramePtr() - fileFirst) / numFrames)) : 0;
              mSong->addFrame (true, frameNum++, samples, true, totalFrames+1, 0);

              if (!firstSamples)
                startPlayer (false);
              firstSamples = false;
              }
            }
          filePtr += decode.getNextFrameOffset();
          mLoadFrac = float(filePtr - fileFirst) / fileSize;
          }
        mLoadFrac = float(filePtr - fileFirst) / fileSize;
        }

      cLog::log (LOGINFO, "loaded");
      // wait for play to end or abort, wav uses the file mapping pointers to play
      mPlayer.join();
      }
      //}}}

    #ifdef _WIN32
      //{{{  windows file map close
      UnmapViewOfFile (fileFirst);
      CloseHandle (fileHandle);
      //}}}
    #else
      //{{{  linux mmap close to do
      //}}}
    #endif

    //{{{  next file
    //if (mSong->getChanged()) // use changed fileIndex
    //  mSong->setChanged (false);
    //else if (!mFileList->nextIndex())
    //}}}
    mExit = true;;
    }

  clear();
  cLog::log (LOGINFO, "exit");
  }
//}}}

// private
//{{{
void cLoaderPlayer::clear() {

  for (auto parser : mParsers) {
    parser.second->stop();
    delete parser.second;
    }
  mParsers.clear();

  // remove main container
  auto tempSong = mSong;
  mSong = nullptr;
  delete tempSong;

  auto tempAudioDecode = mAudioDecode;
  mAudioDecode = nullptr;
  delete tempAudioDecode;

  auto tempVideoDecode = mVideoDecode;
  mVideoDecode = nullptr;
  delete tempVideoDecode;
  }
//}}}

//{{{
string cLoaderPlayer::getHlsPathName (bool radio, int vidBitrate) {

  const string mPoolName = radio ? "pool_904/live/uk/" : "pool_902/live/uk/";

  string pathName = mPoolName + mSong->getChannel() +
                    "/" + mSong->getChannel() +
                    ".isml/" + mSong->getChannel();
  if (radio)
    pathName += "-audio=" + dec(mSong->getBitrate());
  else {
    pathName += mSong->getBitrate() < 128000 ? "-pa3=" : "-pa4=" + dec(mSong->getBitrate());
    if (vidBitrate)
      pathName += "-video=" + dec(vidBitrate);
    }

  return pathName;
  }
//}}}
//{{{
string cLoaderPlayer::getTagValue (uint8_t* buffer, const char* tag) {

  const char* tagPtr = strstr ((const char*)buffer, tag);
  const char* valuePtr = tagPtr + strlen (tag);
  const char* endPtr = strchr (valuePtr, '\n');

  return string (valuePtr, endPtr - valuePtr);
  }
//}}}
//{{{
void cLoaderPlayer::addIcyInfo (int frame, const string& icyInfo) {

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
        mSong->getSelect().addMark (frame, titleStr);
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
void cLoaderPlayer::startPlayer (bool streaming) {

  if (!mPlayer.joinable()) {
    mPlayer = thread ([=]() {
      // lambda - player
      cLog::setThreadName ("play");

      float silence [2048*2] = { 0.f };
      float samples [2048*2] = { 0.f };

      #ifdef _WIN32
        SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        //{{{  WSAPI player thread, video just follows play pts
        auto device = getDefaultAudioOutputDevice();

        if (device) {
          cLog::log (LOGINFO, "device @ %d", mSong->getSampleRate());
          device->setSampleRate (mSong->getSampleRate());
          device->start();

          cSong::cFrame* framePtr;
          while (!mExit && !mSong->getChanged()) {
            device->process ([&](float*& srcSamples, int& numSrcSamples) mutable noexcept {
              // lambda callback - load srcSamples
              shared_lock<shared_mutex> lock (mSong->getSharedMutex());

              framePtr = mSong->getFramePtr (mSong->getPlayFrame());
              if (mPlaying && framePtr && framePtr->getSamples()) {
                if (mSong->getNumChannels() == 1) {
                  //{{{  mono to stereo
                  auto src = framePtr->getSamples();
                  auto dst = samples;
                  for (int i = 0; i < mSong->getSamplesPerFrame(); i++) {
                    *dst++ = *src;
                    *dst++ = *src++;
                    }
                  }
                  //}}}
                else
                  memcpy (samples, framePtr->getSamples(), mSong->getSamplesPerFrame() * mSong->getNumChannels() * sizeof(float));
                srcSamples = samples;
                }
              else
                srcSamples = silence;
              numSrcSamples = mSong->getSamplesPerFrame();

              if (mVideoDecode && framePtr)
                mVideoDecode->setPlayPts (framePtr->getPts());
              if (mPlaying && framePtr)
                mSong->incPlayFrame (1, true);
              });

            if (!streaming && (mSong->getPlayFrame() > mSong->getLastFrame()))
              break;
            }

          device->stop();
          }
        //}}}
      #else
        //SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        //{{{  audio16 player thread, video just follows play pts
        cAudio audio (2, mSong->getSampleRate(), 40000, false);

        cSong::cFrame* framePtr;
        while (!mExit && !mSong->getChanged()) {
          float* playSamples = silence;
            {
            // scoped song mutex
            shared_lock<shared_mutex> lock (mSong->getSharedMutex());
            framePtr = mSong->getFramePtr (mSong->getPlayFrame());
            bool gotSamples = mPlaying && framePtr && framePtr->getSamples();
            if (gotSamples) {
              memcpy (samples, framePtr->getSamples(), mSong->getSamplesPerFrame() * 8);
              playSamples = samples;
              }
            }
          audio.play (2, playSamples, mSong->getSamplesPerFrame(), 1.f);

          if (mVideoDecode && framePtr)
            mVideoDecode->setPlayPts (framePtr->getPts());
          if (mPlaying && framePtr)
            mSong->incPlayFrame (1, true);

          if (!streaming && (mSong->getPlayFrame() > mSong->getLastFrame()))
            break;
          }
        //}}}
      #endif

      cLog::log (LOGINFO, "exit");
      });
    }
  }
//}}}
