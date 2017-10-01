// iAudio.h
#pragma once

class iAudio {
public:
  iAudio() {}
  virtual ~iAudio() {}

  virtual float getVolume()  = 0;
  virtual void setVolume (float volume) = 0;

  virtual void audOpen (int sampleFreq, int bitsPerSample, int channels) = 0;
  virtual void audPlay (int16_t* src, int len, float pitch) = 0;
  virtual void audSilence() = 0;
  virtual void audClose() = 0;
  };
