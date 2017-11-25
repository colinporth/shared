// iAudio.h
#pragma once

class iAudio {
public:
  virtual ~iAudio() {}

  virtual void audOpen (int srcChannels, int srcSampleRate) = 0;
  virtual void audPlay (int srcChannels, int16_t* src, int srcSamples, float pitch) = 0;
  virtual void audClose() = 0;

  virtual int getDstChannels() = 0;
  virtual int getDstSampleRate() = 0;
  virtual int getDstChannelMask() = 0;

  virtual int getSrcChannels() = 0;

  virtual float getVolume() = 0;
  virtual void setVolume (float volume) = 0;

  virtual int getMixDown() = 0;
  virtual void setMixDown (int mixDown) = 0;
  };
