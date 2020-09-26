// cCaptureWASAPI.h
#pragma once
//{{{  includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <Strmif.h>

#include <stdint.h>

struct IMMDevice;
struct IAudioClient;
struct IAudioCaptureClient;
//}}}

class cCaptureWASAPI {
// simple big linear buffer for now
public:
  cCaptureWASAPI (uint32_t bufferSize);
  ~cCaptureWASAPI();

  int getChannels() { return kCaptureChannnels; }
  WAVEFORMATEX* getWaveFormatEx() { return mWaveFormatEx; }

  float* getFrame (int frameSize);

private:
  void run();
  void launch();

  // static singleton ref
  static cCaptureWASAPI* mCaptureWASAPI;

  const int kCaptureChannnels = 2;

  IMMDevice* mMMDevice = NULL;
  WAVEFORMATEX* mWaveFormatEx = NULL;

  IAudioClient* mAudioClient = NULL;
  IAudioCaptureClient* mAudioCaptureClient = NULL;
  REFERENCE_TIME mHhnsDefaultDevicePeriod = 0;

  float* mStreamFirst = nullptr;
  float* mStreamLast = nullptr;
  float* mStreamReadPtr = nullptr;
  float* mStreamWritePtr = nullptr;
  };
