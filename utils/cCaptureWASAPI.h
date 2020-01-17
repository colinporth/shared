// cCaptureWASAPI.h
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>

#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <initguid.h>

class cCaptureWASAPI {
// simple big linear buffer for now
public:
  cCaptureWASAPI (uint32_t bufferSize);
  ~cCaptureWASAPI();

  int getChannels() { return kCaptureChannnels; }
  WAVEFORMATEX* getWaveFormatEx() { return mWaveFormatEx; }
  float* getFrame (int frameSize);

  void run();

private:
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
