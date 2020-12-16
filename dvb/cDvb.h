// cDvb.h
#pragma once
#ifdef _WIN32
  #include <wrl.h>
  #include <initguid.h>
  #include <DShow.h>
  #include <bdaiface.h>
  #include <ks.h>
  #include <ksmedia.h>
  #include <bdatif.h>
#endif

  #include "cDvbUtils.h"

class cDvb {
public:
  cDvb (int frequency, int adapter);
  virtual ~cDvb();

  std::string getStatusString();

  void tune (int frequency);
  void reset();
  int setFilter (uint16_t pid);
  void unsetFilter (int fd, uint16_t pid);

  int getBlock (uint8_t*& block, int& blockSize);
  cTsBlock* read (cTsBlockPool* blockPool);

  #ifdef _WIN32
    uint8_t* getBlockBDA (int& len);
    void releaseBlock (int len);

    inline static Microsoft::WRL::ComPtr<IMediaControl> mMediaControl;
    inline static Microsoft::WRL::ComPtr<IScanningTuner> mScanningTuner;
  #endif

  #ifdef __linux__
    std::string updateSignalStr();
    int mDvr = 0;
  #endif

  std::string mErrorStr = "waiting";
  std::string mTuneStr = "untuned";
  std::string mSignalStr = "no signal";

private:
  int mFrequency;
  int mAdapter;
  };
