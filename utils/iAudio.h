// iAudio.h
#pragma once

class iAudio {
public:
  enum eMixDown { eBestMix, eFLFR, eBLBR, eCentre, eWoofer, eAllMix, eNoMix };

  virtual ~iAudio() {}

  virtual void audOpen (int srcChannels, int srcSampleRate) = 0;
  virtual void audPlay (int srcChannels, int16_t* src, int srcSamples, float pitch) = 0;
  virtual void audClose() = 0;

  virtual int getDstChannels() = 0;
  virtual int getDstSampleRate() = 0;
  virtual int getDstChannelMask() = 0;

  virtual int getSrcChannels() = 0;

  virtual float getVolume() = 0;
  virtual float getDefaultVolume() = 0;
  virtual float getMaxVolume() = 0;
  virtual void setVolume (float volume) = 0;

  virtual bool getMixedFL() = 0;
  virtual bool getMixedFR() = 0;
  virtual bool getMixedC() = 0;
  virtual bool getMixedW() = 0;
  virtual bool getMixedBL() = 0;
  virtual bool getMixedBR() = 0;

  virtual eMixDown getMixDown() = 0;
  virtual void setMixDown (eMixDown mixDown) = 0;
  };
