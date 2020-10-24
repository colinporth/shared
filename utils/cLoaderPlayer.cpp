// cHlsPlayer.cpp - audio,video loader,player
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include "cLoaderPlayer.h"

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

#include "../utils/cPesParser.h"

using namespace std;
using namespace chrono;
//}}}

// public
//{{{
cLoaderPlayer::~cLoaderPlayer() {

  delete mSong;
  delete mVideoDecode;
  delete mAudioDecode;

  // !!!!! should clear down mPesParsers !!!!
  }
//}}}
//{{{
void cLoaderPlayer::initialise (bool radio,
                                const string& hostName, const string& poolName, const string& channelName,
                                int audBitrate, int vidBitrate,
                                int videoPoolSize, eLoader loader) {

  mRadio = radio;
  mHostName = hostName;
  mPoolName = poolName;
  mChannelName = channelName;
  mStreaming = true;
  mLoader = loader;

  // audio
  mAudBitrate = audBitrate;
  mSong = new cSong();
  mAudioDecode = new cAudioDecode (cAudioDecode::eAac);

  // video
  mVidBitrate = vidBitrate;
  if (vidBitrate)
    mVideoDecode = cVideoDecode::create (loader & eFFmpeg, loader & eBgra, videoPoolSize);

  // audio pesParser, !!! inited but not used by mRadio until I fix it !!!
  mPesParsers.push_back (
    new cPesParser (0x22, loader & eQueueAudio, "aud",
      [&](uint8_t* pes, int size, int num, int64_t pts, cPesParser* pesParser) noexcept {
        //{{{  audio pes process callback lambda
        uint8_t* framePes = pes;
        while (getAudioDecode()->parseFrame (framePes, pes + size)) {
          // decode a single frame from pes
          int framePesSize = getAudioDecode()->getNextFrameOffset();
          cLog::log (LOGINFO, "%d %d", num, framePesSize);
          pesParser->decode (framePes, framePesSize, num, pts);

          // pts of next frame in pes, assumes 48000 sample rate
          pts += (getAudioDecode()->getNumSamplesPerFrame() * 90) / 48;
          num++;

          // point to next frame in pes
          framePes += framePesSize;
          }

        return num;
        },
        //}}}
      [&](uint8_t* pes, int size, int num, int64_t pts) noexcept {
        //{{{  audio pes decode callback lambda
        float* samples = mAudioDecode->decodeFrame (pes, size, num, pts);
        if (samples) {
          mSong->addFrame (num, samples, true, mSong->getNumFrames(), pts);
          startPlayer();
          }
        else
          cLog::log (LOGERROR, "aud pesParser failed to decode %d %d", size, num);
        }
        //}}}
      )
    );

  if (vidBitrate) {
    // video pesParser
    mPesParsers.push_back (
      new cPesParser (0x21, loader & eQueueVideo, "vid",
        [&](uint8_t* pes, int size, int num, int64_t pts, cPesParser* pesParser) noexcept {
          //{{{  video pes process callback lambda
          pesParser->decode (pes, size, num, pts);
          num++;
          return num;
          },
          //}}}
        [&](uint8_t* pes, int size, int num, int64_t pts) noexcept {
          //{{{  video pes decode callback lambda
          mVideoDecode->decodeFrame (pes, size, num, pts);
          }
          //}}}
        )
      );
    }

  //{{{  PAT pesParser
  //mPesParsers.push_back (
    //new cPesParser (0x00, false, "pat",
      //[&] (uint8_t* pes, int size, int num, int64_t pts, cPesParser* pesParser) noexcept {
        //{{{  pat callback
        //string info;
        //for (int i = 0; i < size; i++) {
          //int value = pes[i];
          //info += hex (value, 2) + " ";
          //}
        //cLog::log (LOGINFO, "PAT process " + dec (size) + ":" + info);
        //return num;
        //},
        //}}}
      //[&] (uint8_t* pes, int size, int num, int64_t pts) noexcept {}
      //)
    //);
  //}}}
  //{{{  pgm 0x20 pesParser
  //mPesParsers.push_back (
    //new cPesParser (0x20, false, "pgm",
      //[&] (uint8_t* pes, int size, int num, int64_t pts, cPesParser* pesParser) noexcept {
        //{{{  pgm callback
        //string info;
        //for (int i = 0; i < size; i++) {
          //int value = pes[i];
          //info += hex (value, 2) + " ";
          //}
        //cLog::log (LOGINFO, "pgm process " + dec (size) + ":" + info);
        //return num;
        //},
        //}}}
      //[&] (uint8_t* pes, int size, int num, int64_t pts) noexcept {}
      //)
    //);
  //}}}
  }
//}}}

//{{{
void cLoaderPlayer::getFrac (float& loadFrac, float& videoFrac, float& audioFrac) {
// return fracs for spinner graphic, true if ok to display

  loadFrac = mLoadFrac;
  audioFrac = mPesParsers.size() > 0 ? mPesParsers[0]->getQueueFrac() : 0.f;
  videoFrac = mPesParsers.size() > 1 ? mPesParsers[1]->getQueueFrac() : 0.f;
  }
//}}}
//{{{
void cLoaderPlayer::getSizes (int& loadSize, int& videoQueueSize, int& audioQueueSize) {
// return sizes

  loadSize = mLoadSize;
  audioQueueSize = mPesParsers.size() > 0 ? mPesParsers[0]->getQueueSize() : 0;
  videoQueueSize = mPesParsers.size() > 1 ? mPesParsers[1]->getQueueSize() : 0;
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
void cLoaderPlayer::hlsLoaderThread() {
// hls http chunk load and possible decode thread

  cLog::setThreadName ("hls ");

  // bitrates < 128 use aacHE, more samplesPerframe, less framesPerChunk
  mSong->initialise (cAudioDecode::eAac, 2, 48000, mAudBitrate < 128000 ? 2048 : 1024, 1000);
  mSong->setBitrateFramesPerChunk (mAudBitrate, mAudBitrate < 128000 ? (mRadio ? 150 : 180) : (mRadio ? 300 : 360));
  mSong->setChannel (mChannelName);

  while (!mExit && !mSong->getChanged()) {
    mSong->setChanged (false);
    cPlatformHttp http;
    string pathName = getHlsPathName();
    string redirectedHostName = http.getRedirect (mHostName, pathName + ".norewind.m3u8");

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
        int chunkNum;
        int frameNum;
        if (mSong->loadChunk (system_clock::now(), 2, chunkNum, frameNum)) {
          //{{{  pesParser init
          // always audio
          mPesParsers[0]->clear (frameNum);

          // video
          if (mPesParsers.size() > 1)
            mPesParsers[1]->clear (0);

          int contentParsed = 0;
          //}}}
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

                           if (!mRadio) {
                             // pesParse as we go
                             while (http.getContentSize() - contentParsed >= 188) {
                               uint8_t* ts = http.getContent() + contentParsed;
                               if (*ts == 0x47) {
                                 for (auto pesParser : mPesParsers)
                                   if (pesParser->parseTs (ts))
                                     break;
                                 ts += 188;
                                 }
                               else {
                                 cLog::log (LOGERROR, "ts packet sync:%d", contentParsed);
                                 return false;
                                 }
                               contentParsed += 188;
                               }
                             }

                           return true;
                           }
                           //}}}
                        ) == 200) {
            // ??? why does pesParser fail for radio audio pes only ???
            if (mRadio) {
              //{{{  parse chunk of ts
              // extract audio pes from chunk of ts packets, write it back crunched into ts, always gets smaller as ts stripped
              uint8_t* ts = http.getContent();
              uint8_t* tsEnd = ts + http.getContentSize();
              uint8_t* pesPtr = ts;
              while ((ts < tsEnd) && (*ts++ == 0x47)) {
                // ts packet start
                auto pid = ((ts[0] & 0x1F) << 8) | ts[1];
                if (pid == 34) {
                  bool payStart = ts[0] & 0x40;
                  int headerBytes = (ts[2] & 0x20) ? 4 + ts[3] : 3;
                  ts += headerBytes;
                  int tsBodyBytes = 187 - headerBytes;
                  if (payStart) {
                    int pesHeaderBytes = 9 + ts[8];
                    ts += pesHeaderBytes;
                    tsBodyBytes -= pesHeaderBytes;
                    }

                  // copy ts payload back into same buffer, always getting smaller
                  memcpy (pesPtr, ts, tsBodyBytes);
                  pesPtr += tsBodyBytes;
                  ts += tsBodyBytes;
                  }
                else
                  ts += 187;
                }

              // parse pes frame by frame
              uint8_t* pesEnd = pesPtr;
              pesPtr = http.getContent();
              while (getAudioDecode()->parseFrame (pesPtr, pesEnd)) {
                float* samples = getAudioDecode()->decodeFrame (frameNum);
                mSong->addFrame (frameNum++, samples, true, mSong->getNumFrames(), 0);
                startPlayer();
                pesPtr += getAudioDecode()->getNextFrameOffset();
                }
              }
              //}}}
            else {
              //{{{  pesParser finish
              for (auto pesParser : mPesParsers)
                pesParser->process();
              }
              //}}}
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

  cLog::log (LOGINFO, "exit");
  }
//}}}
//{{{
void cLoaderPlayer::icyLoaderThread (const string& url) {

  cLog::setThreadName ("icy ");

  while (!mExit) {
    int icySkipCount = 0;
    int icySkipLen = 0;
    int icyInfoCount = 0;
    int icyInfoLen = 0;
    char icyInfo[255] = { 0 };

    uint8_t bufferFirst[4096];
    uint8_t* bufferEnd = bufferFirst;
    uint8_t* buffer = bufferFirst;

    int frameNum = -1;
    cAudioDecode decode (cAudioDecode::eAac);

    thread player;
    mSong->setChanged (false);

    cPlatformHttp http;
    cUrl parsedUrl;
    parsedUrl.parse (url);
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
          mSong->initialise(frameType, 2, 44100, (frameType == cAudioDecode::eMp3) ? 1152 : 2048, 3000);
          }

        while (decode.parseFrame (buffer, bufferEnd)) {
          if (decode.getFrameType() == mSong->getFrameType()) {
            auto samples = decode.decodeFrame (frameNum);
            if (samples) {
              mSong->setFixups (decode.getNumChannels(), decode.getSampleRate(), decode.getNumSamplesPerFrame());
              mSong->addFrame (frameNum++, samples, true, mSong->getNumFrames()+1, 0);
              startPlayer();
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
    player.join();
    }

  cLog::log (LOGINFO, "exit");
  }
//}}}
//{{{
void cLoaderPlayer::fileLoaderThread() {

  cLog::setThreadName ("file");

  while (!mExit) {
    if (cAudioDecode::mJpegPtr) {
      //{{{  delete any jpegImage
      //mJpegImageView->setImage (nullptr);

      //delete (cAudioDecode::mJpegPtr);
      //cAudioDecode::mJpegPtr = nullptr;
      }
      //}}}

    #ifdef _WIN32
      HANDLE fileHandle = CreateFile (mFileList->getCurFileItem().getFullName().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
      HANDLE mapping = CreateFileMapping (fileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
      auto fileMapFirst = (uint8_t*)MapViewOfFile (mapping, FILE_MAP_READ, 0, 0, 0);
      auto fileMapSize = GetFileSize (fileHandle, NULL);
      auto fileMapEnd = fileMapFirst + fileMapSize;

      // sampleRate for aac-sbr wrong in header, fixup later, use a maxValue for samples alloc
      int sampleRate;
      auto frameType = cAudioDecode::parseSomeFrames (fileMapFirst, fileMapEnd, sampleRate);
      //if (cAudioDecode::mJpegPtr) // should delete old jpegImage, but we have memory to waste
      //  mJpegImageView->setImage (new cJpegImage (cAudioDecode::mJpegPtr, cAudioDecode::mJpegLen));

      thread player;

      int frameNum = 0;
      bool songDone = false;
      auto fileMapPtr = fileMapFirst;
      cAudioDecode decode (frameType);

      if (frameType == cAudioDecode::eWav) {
        //{{{  parse wav
        auto frameSamples = 1024;
        mSong->initialise (frameType, 2, sampleRate, frameSamples);
        decode.parseFrame (fileMapPtr, fileMapEnd);
        auto samples = decode.getFramePtr();
        while (!mExit && !mSong->getChanged() && ((samples + (frameSamples * 2 * sizeof(float))) <= fileMapEnd)) {
          mSong->addFrame (frameNum++, (float*)samples, false, fileMapSize / (frameSamples * 2 * sizeof(float)), 0);
          samples += frameSamples * 2 * sizeof(float);
          startPlayer();
          }
        }
        //}}}
      else {
        //{{{  parse coded
        mSong->initialise (frameType, 2, sampleRate, (frameType == cAudioDecode::eMp3) ? 1152 : 2048);
        while (!mExit && !mSong->getChanged() && decode.parseFrame (fileMapPtr, fileMapEnd)) {
          if (decode.getFrameType() == mSong->getFrameType()) {
            auto samples = decode.decodeFrame (frameNum);
            if (samples) {
              int numFrames = mSong->getNumFrames();
              int totalFrames = (numFrames > 0) ? int(fileMapEnd - fileMapFirst) / (int(decode.getFramePtr() - fileMapFirst) / numFrames) : 0;
              mSong->setFixups (decode.getNumChannels(), decode.getSampleRate(), decode.getNumSamplesPerFrame());
              mSong->addFrame (frameNum++, samples, true, totalFrames+1, 0);
              startPlayer();
              }
            }
          fileMapPtr += decode.getNextFrameOffset();
          }
        }
        //}}}
      cLog::log (LOGINFO, "loaded");

      // wait for play to end or abort
      player.join();
      //{{{  next file
      UnmapViewOfFile (fileMapFirst);
      CloseHandle (fileHandle);

      if (mSong->getChanged()) // use changed fileIndex
        mSong->setChanged (false);
      else if (!mFileList->nextIndex())
        mExit = true;;
      //}}}
    #else
      this_thread::sleep_for (100ms);
    #endif
    }

  cLog::log (LOGINFO, "exit");
  }
//}}}

// private
//{{{
string cLoaderPlayer::getHlsPathName() {

  string pathName = mPoolName + mSong->getChannel() +
                    "/" + mSong->getChannel() +
                    ".isml/" + mSong->getChannel();
  if (mRadio)
    pathName += "-audio=" + dec(mSong->getBitrate());
  else {
    pathName += mSong->getBitrate() < 128000 ? "-pa3=" : "-pa4=" + dec(mSong->getBitrate());
    if (mVidBitrate)
      pathName += "-video=" + dec(mVidBitrate);
    }

  return pathName;
  }
//}}}
//{{{
string cLoaderPlayer:: getTagValue (uint8_t* buffer, const char* tag) {

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
void cLoaderPlayer::startPlayer() {

  if (!mPlayer.joinable()) {
    mPlayer = std::thread ([=]() {
      // player thread lambda
      cLog::setThreadName ("play");

      float silence [2048*2] = { 0.f };
      float samples [2048*2] = { 0.f };

      cAudioDecode decode (mSong->getFrameType());

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

            if (!mStreaming && (mSong->getPlayFrame() > mSong->getLastFrame()))
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

          if (!mStreaming && (mSong->getPlayFrame() > mSong->getLastFrame()))
            break;
          }
        //}}}
      #endif

      cLog::log (LOGINFO, "exit");
      });
    }
  }
//}}}
