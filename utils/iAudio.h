// iAudio.h
#pragma once

class iAudio {
public:
  virtual ~iAudio() {}

  virtual void audOpen (int sampleRate, int bitsPerSample, int channels) = 0;
  virtual void audPlay (int16_t* src, int len, float pitch) = 0;
  virtual void audSilence (int samples) = 0;
  virtual void audClose() = 0;

  virtual int getOutChannels() = 0;
  virtual int getOutSampleRate() = 0;
 virtual  int getOutChannelMask() = 0;

  virtual float getVolume() = 0;
  virtual void setVolume (float volume) = 0;
  };
