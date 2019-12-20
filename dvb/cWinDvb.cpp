// cWinDvb.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <chrono>
#include "../utils/date.h"
#include "../utils/utils.h"
#include "../utils/cLog.h"
#include "../utils/cBipBuffer.h"

#include "cDumpTransportStream.h"

#include "cWinDvb.h"
#include <bdamedia.h>

DEFINE_GUID (CLSID_DVBTLocator, 0x9CD64701, 0xBDF3, 0x4d14, 0x8E,0x03, 0xF1,0x29,0x83,0xD8,0x66,0x64);
DEFINE_GUID (CLSID_BDAtif, 0xFC772ab0, 0x0c7f, 0x11d3, 0x8F,0xf2, 0x00,0xa0,0xc9,0x22,0x4c,0xf4);
DEFINE_GUID (CLSID_Dump, 0x36a5f770, 0xfe4c, 0x11ce, 0xa8, 0xed, 0x00, 0xaa, 0x00, 0x2f, 0xea, 0xb5);

#pragma comment (lib,"strmiids")
//}}}

// public:
cDvb::cDvb (const std::string& root) : cDumpTransportStream (root, false) {};

//{{{
bool cDvb::createGraph (int frequency)  {

  if (!createGraphDvbT (frequency))
    return false;

  mFrequency = frequency;

  createFilter (mGrabberFilter, CLSID_SampleGrabber, L"grabber", mDvbCapture);
  mGrabberFilter.As (&mGrabber);

  auto hr = mGrabber->SetOneShot (false);
  if (hr != S_OK)
    cLog::log (LOGERROR, "SetOneShot failed " + dec(hr));

  hr = mGrabber->SetBufferSamples (true);
  if (hr != S_OK)
    cLog::log (LOGERROR, "SetBufferSamples failed " + dec(hr));

  mGrabberCB.allocateBuffer (128*240*188);
  hr = mGrabber->SetCallback (&mGrabberCB, 0);
  if (hr != S_OK)
    cLog::log (LOGERROR, "SetCallback failed " + dec(hr));

  createFilter (mMpeg2Demux, CLSID_MPEG2Demultiplexer, L"MPEG2demux", mGrabberFilter);
  createFilter (mBdaTif, CLSID_BDAtif, L"BDAtif", mMpeg2Demux);
  mGraphBuilder.As (&mMediaControl);

  return true;
  }
//}}}
//{{{
bool cDvb::createGraph (int frequency, const std::string& fileName) {

  if (!createGraphDvbT (frequency))
    return false;

  createFilter (mInfTeeFilter, CLSID_InfTee, L"infTee", mDvbCapture);

  auto wstr = std::wstring (fileName.begin(), fileName.end());
  createFilter (mDumpFilter, CLSID_Dump, L"dump", mInfTeeFilter);
  mDumpFilter.As (&mFileSinkFilter);
  mFileSinkFilter->SetFileName (wstr.c_str(), nullptr);

  createFilter (mMpeg2Demux, CLSID_MPEG2Demultiplexer, L"MPEG2demux", mInfTeeFilter);
  createFilter (mBdaTif, CLSID_BDAtif, L"BDAtif", mMpeg2Demux);
  mGraphBuilder.As (&mMediaControl);
  return true;
  }
//}}}

//{{{
void cDvb::tune (int frequency) {

  auto hr = mDvbLocator->put_CarrierFrequency (frequency);
  //{{{  error
  if (hr != S_OK)
    cLog::log (LOGERROR, "tune - put_CarrierFrequency" + dec(hr));
  //}}}

  hr = mDvbLocator->put_Bandwidth (8);
  //{{{  error
  if (hr != S_OK)
    cLog::log (LOGERROR, "tune - put_Bandwidth" + dec(hr));
  //}}}

  hr = mDvbTuningSpace2->put_DefaultLocator (mDvbLocator.Get());
  //{{{  error
  if (hr != S_OK)
    cLog::log (LOGERROR, "tune - put_DefaultLocator" + dec(hr));

  //}}}
  hr = mTuneRequest->put_Locator (mDvbLocator.Get());
  //{{{  error
  if (hr != S_OK)
    cLog::log (LOGERROR, "tune - put_Locator" + dec(hr));
  //}}}

  hr = mScanningTuner->Validate (mTuneRequest.Get());
  //{{{  error
  if (hr != S_OK)
    cLog::log (LOGERROR, "tune - Validate" + dec(hr));
  //}}}

  hr = mScanningTuner->put_TuneRequest (mTuneRequest.Get());
  //{{{  error
  if (hr != S_OK)
    cLog::log (LOGERROR, "tune - put_TuneRequest" + dec(hr));
  //}}}

  hr = mMediaControl->Run();
  //{{{  error
  if (hr != S_OK)
    cLog::log (LOGERROR, "tune - run" + dec(hr));
  //}}}

  mFrequency = frequency;
  }
//}}}
//{{{
void cDvb::pause() {

  auto hr = mMediaControl->Pause();
  if (hr != S_OK)
    cLog::log (LOGERROR, "pause graph failed " + dec(hr));

  mGrabberCB.clear();
  }
//}}}
//{{{
void cDvb::stop() {

  auto hr = mMediaControl->Stop();
  if (hr != S_OK)
    cLog::log (LOGERROR, "stop graph failed " + dec(hr));

  mGrabberCB.clear();
  clear();
  }
//}}}

//{{{
void cDvb::grabThread() {

  CoInitializeEx (NULL, COINIT_MULTITHREADED);
  cLog::setThreadName ("grab");

  auto hr = mMediaControl->Run();
  if (hr == S_OK) {
    int64_t streamPos = 0;
    auto blockSize = 0;
    while (true) {
      auto ptr = getBlock (blockSize);
      if (blockSize) {
        streamPos += demux (ptr, blockSize, streamPos, false, -1);
        releaseBlock (blockSize);
        }
      else
        Sleep (1);
      }
    }
  else
    cLog::log (LOGERROR, "run graph failed " + dec(hr));

  cLog::log (LOGINFO, "exit");
  CoUninitialize();
  }
//}}}
//{{{
void cDvb::signalThread() {

  CoInitializeEx (NULL, COINIT_MULTITHREADED);
  cLog::setThreadName ("sig ");

  while (true) {
    if (mScanningTuner) {
      long signal = 0;
      mScanningTuner->get_SignalStrength (&signal);
      mSignal = signal / 0x10000;
      }
    Sleep (100);
    mSignalStr = dec(mSignal,3);
    }

  cLog::log (LOGINFO, "exit");
  CoUninitialize();
  }
//}}}
//{{{
void cDvb::readThread (const std::string& inTs) {

  cLog::setThreadName ("read");

  auto file = fopen (inTs.c_str(), "rb");
  if (!file) {
    //{{{  error, return
    cLog::log (LOGERROR, "no file " + inTs);
    return;
    }
    //}}}

  uint64_t streamPos = 0;
  auto blockSize = 188 * 8;
  auto buffer = (uint8_t*)malloc (blockSize);

  bool run = true;
  while (run) {
    int bytesRead = fread (buffer, 1, blockSize, file);
    if (bytesRead > 0)
      streamPos += demux (buffer, bytesRead, streamPos, false, -1);
    else
      break;
    mPacketStr = dec(getDiscontinuity());
    }

  fclose (file);
  free (buffer);

  cLog::log (LOGERROR, "exit");
  }
//}}}

// private:
//{{{
class cGrabberCB : public ISampleGrabberCB {
public:
  cGrabberCB() {}
  virtual ~cGrabberCB() {}

  void allocateBuffer (int bufSize) { mBipBuffer.allocateBuffer (bufSize); }
  void clear() { mBipBuffer.clear(); }

  uint8_t* getBlock (int& len) { return mBipBuffer.getContiguousBlock (len); }
  void releaseBlock (int len) { mBipBuffer.decommitBlock (len); }

private:
  //{{{
  class cBipBuffer {
  public:
      cBipBuffer() : mBuffer(NULL), ixa(0), sza(0), ixb(0), szb(0), buflen(0), ixResrv(0), szResrv(0) {}
      //{{{
      ~cBipBuffer() {
          // We don't call FreeBuffer, because we don't need to reset our variables - our object is dying
          if (mBuffer != NULL)
              ::VirtualFree (mBuffer, buflen, MEM_DECOMMIT);
      }
      //}}}

      //{{{
      bool allocateBuffer (int buffersize) {
      // Allocates a buffer in virtual memory, to the nearest page size (rounded up)
      //   int buffersize size of buffer to allocate, in bytes
      //   return bool true if successful, false if buffer cannot be allocated

        if (buffersize <= 0)
          return false;
        if (mBuffer != NULL)
          freeBuffer();

        SYSTEM_INFO si;
        ::GetSystemInfo(&si);

        // Calculate nearest page size
        buffersize = ((buffersize + si.dwPageSize - 1) / si.dwPageSize) * si.dwPageSize;

        mBuffer = (BYTE*)::VirtualAlloc (NULL, buffersize, MEM_COMMIT, PAGE_READWRITE);
        if (mBuffer == NULL)
          return false;

        buflen = buffersize;
        return true;
        }
      //}}}
      //{{{
      void clear() {
      // Clears the buffer of any allocations or reservations. Note; it
      // does not wipe the buffer memory; it merely resets all pointers,
      // returning the buffer to a completely empty state ready for new
      // allocations.

        ixa = sza = ixb = szb = ixResrv = szResrv = 0;
        }
      //}}}
      //{{{
      void freeBuffer() {
      // Frees a previously allocated buffer, resetting all internal pointers to 0.

        if (mBuffer == NULL)
          return;

        ixa = sza = ixb = szb = buflen = 0;

        ::VirtualFree(mBuffer, buflen, MEM_DECOMMIT);

        mBuffer = NULL;
        }
      //}}}

      //{{{
      uint8_t* reserve (int size, OUT int& reserved) {
      // Reserves space in the buffer for a memory write operation
      //   int size                 amount of space to reserve
      //   OUT int& reserved        size of space actually reserved
      // Returns:
      //   BYTE*                    pointer to the reserved block
      //   Will return NULL for the pointer if no space can be allocated.
      //   Can return any value from 1 to size in reserved.
      //   Will return NULL if a previous reservation has not been committed.

        // We always allocate on B if B exists; this means we have two blocks and our buffer is filling.
        if (szb) {
          int freespace = getBFreeSpace();
          if (size < freespace)
            freespace = size;
          if (freespace == 0)
            return NULL;

          szResrv = freespace;
          reserved = freespace;
          ixResrv = ixb + szb;
          return mBuffer + ixResrv;
          }
        else {
          // Block b does not exist, so we can check if the space AFTER a is bigger than the space
          // before A, and allocate the bigger one.
          int freespace = getSpaceAfterA();
          if (freespace >= ixa) {
            if (freespace == 0)
              return NULL;
            if (size < freespace)
              freespace = size;

            szResrv = freespace;
            reserved = freespace;
            ixResrv = ixa + sza;
            return mBuffer + ixResrv;
            }
          else {
            if (ixa == 0)
              return NULL;
            if (ixa < size)
              size = ixa;
            szResrv = size;
            reserved = size;
            ixResrv = 0;
            return mBuffer;
            }
          }
        }
      //}}}
      //{{{
      void commit (int size) {
      // Commits space that has been written to in the buffer
      // Parameters:
      //   int size                number of bytes to commit
      //   Committing a size > than the reserved size will cause an assert in a debug build;
      //   in a release build, the actual reserved size will be used.
      //   Committing a size < than the reserved size will commit that amount of data, and release
      //   the rest of the space.
      //   Committing a size of 0 will release the reservation.

        if (size == 0) {
          // decommit any reservation
          szResrv = ixResrv = 0;
          return;
          }

        // If we try to commit more space than we asked for, clip to the size we asked for.

        if (size > szResrv)
          size = szResrv;

        // If we have no blocks being used currently, we create one in A.
        if (sza == 0 && szb == 0) {
          ixa = ixResrv;
          sza = size;

          ixResrv = 0;
          szResrv = 0;
          return;
          }

        if (ixResrv == sza + ixa)
          sza += size;
        else
          szb += size;

        ixResrv = 0;
        szResrv = 0;
        }
      //}}}

      //{{{
      uint8_t* getContiguousBlock (OUT int& size) {
      // Gets a pointer to the first contiguous block in the buffer, and returns the size of that block.
      //   OUT int & size            returns the size of the first contiguous block
      // Returns:
      //   BYTE*                    pointer to the first contiguous block, or NULL if empty.

        if (sza == 0) {
          size = 0;
          return NULL;
          }

        size = sza;
        return mBuffer + ixa;
        }
      //}}}
      //{{{
      void decommitBlock (int size) {
      // Decommits space from the first contiguous block,  size amount of memory to decommit

        if (size >= sza) {
          ixa = ixb;
          sza = szb;
          ixb = 0;
          szb = 0;
          }
        else {
          sza -= size;
          ixa += size;
          }
        }
      //}}}
      //{{{
      int getCommittedSize() const {
      // Queries how much data (in total) has been committed in the buffer
      // Returns:
      //   int                    total amount of committed data in the buffer

        return sza + szb;
        }
      //}}}

  private:
    //{{{
    int getSpaceAfterA() const {
      return buflen - ixa - sza;
      }
    //}}}
    //{{{
    int getBFreeSpace() const {
      return ixa - ixb - szb;
      }
    //}}}

    uint8_t* mBuffer;
    int buflen;

    int ixa;
    int sza;

    int ixb;
    int szb;

    int ixResrv;
    int szResrv;
    };
  //}}}

  // ISampleGrabberCB methods
  STDMETHODIMP_(ULONG) AddRef() { return ++ul_cbrc; }
  STDMETHODIMP_(ULONG) Release() { return --ul_cbrc; }
  STDMETHODIMP QueryInterface (REFIID riid, void** p_p_object) { return E_NOTIMPL; }

  //{{{
  STDMETHODIMP BufferCB (double sampleTime, BYTE* samples, long sampleLen) {
    cLog::log (LOGERROR, "cSampleGrabberCB::BufferCB called");
    return S_OK;
    }
  //}}}
  //{{{
  STDMETHODIMP SampleCB (double sampleTime, IMediaSample* mediaSample) {

    if (mediaSample->IsDiscontinuity() == S_OK)
      cLog::log (LOGERROR, "cSampleGrabCB::SampleCB sample isDiscontinuity");

    int actualDataLength = mediaSample->GetActualDataLength();
    if (actualDataLength != 240*188)
     cLog::log (LOGERROR, "cSampleGrabCB::SampleCB - unexpected sampleLength");

    int bytesAllocated = 0;
    uint8_t* ptr = mBipBuffer.reserve (actualDataLength, bytesAllocated);

    if (!ptr || (bytesAllocated != actualDataLength))
      cLog::log (LOGERROR, "failed to reserve buffer");
    else {
      uint8_t* samples;
      mediaSample->GetPointer (&samples);
      memcpy (ptr, samples, actualDataLength);
      mBipBuffer.commit (actualDataLength);
      }

    return S_OK;
    }
  //}}}

  // vars
  ULONG ul_cbrc = 0;
  cBipBuffer mBipBuffer;
  };
//}}}

//{{{
bool cDvb::connectPins (Microsoft::WRL::ComPtr<IBaseFilter> fromFilter,
                  Microsoft::WRL::ComPtr<IBaseFilter> toFilter,
                  wchar_t* fromPinName, wchar_t* toPinName) {
// if name == NULL use first correct direction unconnected pin
// else use pin matching name

  Microsoft::WRL::ComPtr<IEnumPins> fromPins;
  fromFilter->EnumPins (&fromPins);

  Microsoft::WRL::ComPtr<IPin> fromPin;
  while (fromPins->Next (1, &fromPin, NULL) == S_OK) {
    Microsoft::WRL::ComPtr<IPin> connectedPin;
    fromPin->ConnectedTo (&connectedPin);
    if (!connectedPin) {
      // match fromPin info
      PIN_INFO fromPinInfo;
      fromPin->QueryPinInfo (&fromPinInfo);
      if ((fromPinInfo.dir == PINDIR_OUTPUT) &&
          (!fromPinName || !wcscmp (fromPinInfo.achName, fromPinName))) {
        // found fromPin, look for toPin
        Microsoft::WRL::ComPtr<IEnumPins> toPins;
        toFilter->EnumPins (&toPins);

        Microsoft::WRL::ComPtr<IPin> toPin;
        while (toPins->Next (1, &toPin, NULL) == S_OK) {
          Microsoft::WRL::ComPtr<IPin> connectedPin;
          toPin->ConnectedTo (&connectedPin);
          if (!connectedPin) {
            // match toPin info
            PIN_INFO toPinInfo;
            toPin->QueryPinInfo (&toPinInfo);
            if ((toPinInfo.dir == PINDIR_INPUT) &&
                (!toPinName || !wcscmp (toPinInfo.achName, toPinName))) {
              // found toPin
              if (mGraphBuilder->Connect (fromPin.Get(), toPin.Get()) == S_OK) {
                cLog::log (LOGINFO, "- connecting pin " + wstrToStr (fromPinInfo.achName) +
                                    " to " + wstrToStr (toPinInfo.achName));
                return true;
                }
              else {
                cLog::log (LOGINFO, "- connectPins failed");
                return false;
                }
              }
            }
          }
        cLog::log (LOGERROR, "connectPins - no toPin");
        return false;
        }
      }
    }

  cLog::log (LOGERROR, "connectPins - no fromPin");
  return false;
  }
//}}}
//{{{
void cDvb::findFilter (Microsoft::WRL::ComPtr<IBaseFilter>& filter,
                 const CLSID& clsid, wchar_t* name,
                 Microsoft::WRL::ComPtr<IBaseFilter> fromFilter) {
// Find instance of filter of type CLSID by name, add to graphBuilder, connect fromFilter

  cLog::log (LOGINFO, "findFilter " + wstrToStr (name));

  Microsoft::WRL::ComPtr<ICreateDevEnum> systemDevEnum;
  CoCreateInstance (CLSID_SystemDeviceEnum, nullptr,
                    CLSCTX_INPROC_SERVER, IID_PPV_ARGS (&systemDevEnum));

  Microsoft::WRL::ComPtr<IEnumMoniker> classEnumerator;
  systemDevEnum->CreateClassEnumerator (clsid, &classEnumerator, 0);

  if (classEnumerator) {
    int i = 1;
    IMoniker* moniker = NULL;
    ULONG fetched;
    while (classEnumerator->Next (1, &moniker, &fetched) == S_OK) {
      IPropertyBag* propertyBag;
      if (moniker->BindToStorage (NULL, NULL, IID_IPropertyBag, (void**)&propertyBag) == S_OK) {
        VARIANT varName;
        VariantInit (&varName);
        propertyBag->Read (L"FriendlyName", &varName, 0);
        VariantClear (&varName);

        cLog::log (LOGINFO, "FindFilter " + wstrToStr (varName.bstrVal));

        // bind the filter
        moniker->BindToObject (NULL, NULL, IID_IBaseFilter, (void**)(&filter));

        mGraphBuilder->AddFilter (filter.Get(), name);
        propertyBag->Release();

        if (connectPins (fromFilter, filter)) {
          propertyBag->Release();
          break;
          }
        else
          mGraphBuilder->RemoveFilter (filter.Get());
        }
      propertyBag->Release();
      moniker->Release();
      }

    moniker->Release();
    }
  }
//}}}
//{{{
void cDvb::createFilter (Microsoft::WRL::ComPtr<IBaseFilter>& filter,
                   const CLSID& clsid, wchar_t* title,
                   Microsoft::WRL::ComPtr<IBaseFilter> fromFilter) {
// createFilter type clsid, add to graphBuilder, connect fromFilter

  cLog::log (LOGINFO, "createFilter " + wstrToStr (title));
  CoCreateInstance (clsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS (filter.GetAddressOf()));
  mGraphBuilder->AddFilter (filter.Get(), title);
  connectPins (fromFilter, filter);
  }
//}}}

//{{{
bool cDvb::createGraphDvbT (int frequency) {

  auto hr = CoCreateInstance (CLSID_FilterGraph, nullptr,
                              CLSCTX_INPROC_SERVER, IID_PPV_ARGS (mGraphBuilder.GetAddressOf()));
  //{{{  error
  if (hr != S_OK) {
    cLog::log (LOGERROR, "createGraphDvbT - CoCreateInstance graph failed " + dec(hr));
    return false;
    }
  //}}}

  hr = CoCreateInstance (CLSID_DVBTNetworkProvider, nullptr,
                         CLSCTX_INPROC_SERVER, IID_PPV_ARGS (mDvbNetworkProvider.GetAddressOf()));
  //{{{  error
  if (hr != S_OK) {
    cLog::log (LOGERROR, "createGraphDvbT - CoCreateInstance dvbNetworkProvider failed " + dec(hr));
    return false;
    }
  //}}}

  hr = mGraphBuilder->AddFilter (mDvbNetworkProvider.Get(), L"dvbtNetworkProvider");
  //{{{  error
  if (hr != S_OK) {
    cLog::log (LOGERROR, "createGraphDvbT - AddFilter failed " + dec(hr));
    return false;
    }
  //}}}
  mDvbNetworkProvider.As (&mScanningTuner);

  //{{{  setup dvbTuningSpace2 interface
  hr = mScanningTuner->get_TuningSpace (mTuningSpace.GetAddressOf());
  //{{{  error
  if (hr != S_OK)
    cLog::log (LOGERROR, "createGraphDvbT - get_TuningSpace failed " + dec(hr));
  //}}}

  mTuningSpace.As (&mDvbTuningSpace2);
  hr = mDvbTuningSpace2->put__NetworkType (CLSID_DVBTNetworkProvider);
  //{{{  error
  if (hr != S_OK)
    cLog::log (LOGERROR, "createGraphDvbT - put__NetworkType failed " + dec(hr));
  //}}}
  hr = mDvbTuningSpace2->put_SystemType (DVB_Terrestrial);
  //{{{  error
  if (hr != S_OK)
    cLog::log (LOGERROR, "createGraphDvbT - put_SystemType failed " + dec(hr));
  //}}}
  hr = mDvbTuningSpace2->put_NetworkID (9018);
  //{{{  error
  if (hr != S_OK)
    cLog::log (LOGERROR, "createGraphDvbT - put_NetworkID failed " + dec(hr));
  //}}}
  hr = mDvbTuningSpace2->put_FrequencyMapping (L"");
  //{{{  error
  if (hr != S_OK)
    cLog::log (LOGERROR, "createGraphDvbT - put_FrequencyMapping failed " + dec(hr));
  //}}}
  hr = mDvbTuningSpace2->put_UniqueName (L"DTV DVB-T");
  //{{{  error
  if (hr != S_OK)
    cLog::log (LOGERROR, "createGraphDvbT - put_UniqueName failed " + dec(hr));
  //}}}
  hr = mDvbTuningSpace2->put_FriendlyName (L"DTV DVB-T");
  //{{{  error
  if (hr != S_OK)
    cLog::log (LOGERROR, "createGraphDvbT - put_FriendlyName failed " + dec(hr));
  //}}}
  //}}}
  //{{{  create dvbtLocator and setup in dvbTuningSpace2 interface
  hr = CoCreateInstance (CLSID_DVBTLocator, nullptr,
                         CLSCTX_INPROC_SERVER, IID_PPV_ARGS (mDvbLocator.GetAddressOf()));
  //{{{  error
  if (hr != S_OK)
    cLog::log (LOGERROR, "createGraphDvbT - CoCreateInstance dvbLocator failed " + dec(hr));
  //}}}
  hr = mDvbLocator->put_CarrierFrequency (frequency);
  //{{{  error
  if (hr != S_OK)
    cLog::log (LOGERROR, "createGraphDvbT - put_CarrierFrequency failed " + dec(hr));
  //}}}
  hr = mDvbLocator->put_Bandwidth (8);
  //{{{  error
  if (hr != S_OK)
    cLog::log (LOGERROR, "createGraphDvbT - put_Bandwidth failed " + dec(hr));
  //}}}
  hr = mDvbTuningSpace2->put_DefaultLocator (mDvbLocator.Get());
  //{{{  error
  if (hr != S_OK)
    cLog::log (LOGERROR, "createGraphDvbT - put_DefaultLocator failed " + dec(hr));
  //}}}
  //}}}
  //{{{  tuneRequest from scanningTuner
  if (mScanningTuner->get_TuneRequest (mTuneRequest.GetAddressOf()) != S_OK)
    mTuningSpace->CreateTuneRequest (mTuneRequest.GetAddressOf());
  hr = mTuneRequest->put_Locator (mDvbLocator.Get());
  //{{{  error
  if (hr != S_OK)
    cLog::log (LOGERROR, "createGraphDvbT - put_Locator failed " + dec(hr));
  //}}}
  hr = mScanningTuner->Validate (mTuneRequest.Get());
  //{{{  error
  if (hr != S_OK)
    cLog::log (LOGERROR, "createGraphDvbT - Validate failed " + dec(hr));
  //}}}
  hr = mScanningTuner->put_TuneRequest (mTuneRequest.Get());
  //{{{  error
  if (hr != S_OK)
    cLog::log (LOGERROR, "createGraphDvbT - put_TuneRequest failed " + dec(hr));
  //}}}
  //}}}

  // dvbtNetworkProvider -> dvbtTuner -> dvbtCapture -> sampleGrabberFilter -> mpeg2Demux -> bdaTif
  findFilter (mDvbTuner, KSCATEGORY_BDA_NETWORK_TUNER, L"DVBTtuner", mDvbNetworkProvider);
  if (!mDvbTuner) {
    //{{{  error
    cLog::log (LOGERROR, "createGraphDvbT - unable to find dvbtuner filter");
    return false;
    }
    //}}}

  findFilter (mDvbCapture, KSCATEGORY_BDA_RECEIVER_COMPONENT, L"DVBTcapture", mDvbTuner);
  return true;
  }
//}}}

//{{{
uint8_t* cDvb::getBlock (int& len) {

  return mGrabberCB.getBlock (len);
  }
//}}}
//{{{
void cDvb::releaseBlock (int len) {
  mGrabberCB.releaseBlock (len);
  }
//}}}
