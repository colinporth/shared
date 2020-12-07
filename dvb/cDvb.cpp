// cDvb.cpp
#ifdef _WIN32
  //{{{  windows includes
  #define _CRT_SECURE_NO_WARNINGS
  #define NOMINMAX
  #define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

  #include <wrl.h>
  #include <initguid.h>
  #include <DShow.h>
  #include <bdaiface.h>
  #include <ks.h>
  #include <ksmedia.h>
  #include <bdatif.h>

  #include <locale>
  #include <codecvt>

  MIDL_INTERFACE ("0579154A-2B53-4994-B0D0-E773148EFF85")
  ISampleGrabberCB : public IUnknown {
  public:
    virtual HRESULT STDMETHODCALLTYPE SampleCB (double SampleTime, IMediaSample* pSample) = 0;
    virtual HRESULT STDMETHODCALLTYPE BufferCB (double SampleTime, BYTE* pBuffer, long BufferLen) = 0;
    };

  MIDL_INTERFACE ("6B652FFF-11FE-4fce-92AD-0266B5D7C78F")
  ISampleGrabber : public IUnknown {
  public:
    virtual HRESULT STDMETHODCALLTYPE SetOneShot (BOOL OneShot) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMediaType (const AM_MEDIA_TYPE* pType) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType (AM_MEDIA_TYPE* pType) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetBufferSamples (BOOL BufferThem) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer (long* pBufferSize, long* pBuffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentSample (IMediaSample** ppSample) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCallback (ISampleGrabberCB* pCallback, long WhichMethodToCallback) = 0;
    };
  EXTERN_C const CLSID CLSID_SampleGrabber;

  #include <bdamedia.h>
  DEFINE_GUID (CLSID_DVBTLocator, 0x9CD64701, 0xBDF3, 0x4d14, 0x8E,0x03, 0xF1,0x29,0x83,0xD8,0x66,0x64);
  DEFINE_GUID (CLSID_BDAtif, 0xFC772ab0, 0x0c7f, 0x11d3, 0x8F,0xf2, 0x00,0xa0,0xc9,0x22,0x4c,0xf4);
  DEFINE_GUID (CLSID_Dump, 0x36a5f770, 0xfe4c, 0x11ce, 0xa8, 0xed, 0x00, 0xaa, 0x00, 0x2f, 0xea, 0xb5);

  #pragma comment (lib,"strmiids")
  //}}}
#else
  //{{{  linux includes
  #include <unistd.h>
  #include <signal.h>
  #include <fcntl.h>

  #include <sys/ioctl.h>
  #include <sys/time.h>
  #include <sys/poll.h>

  #include <linux/dvb/version.h>
  #include <linux/dvb/dmx.h>
  #include <linux/dvb/frontend.h>
  //}}}
#endif
//{{{  common includes
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <thread>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../fmt/core.h"
#include "../date/date.h"
#include "../utils/utils.h"
#include "../utils/cLog.h"
#include "../utils/cBipBuffer.h"

#include "cTransportStream.h"
#include "cSubtitle.h"

#include "cDvb.h"

using namespace std;
using namespace fmt;
//}}}

constexpr bool kDebug = false;

namespace { // anonymous
#ifdef _WIN32
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
  //{{{  vars
  Microsoft::WRL::ComPtr<IGraphBuilder> mGraphBuilder;

  Microsoft::WRL::ComPtr<IBaseFilter> mDvbNetworkProvider;
  Microsoft::WRL::ComPtr<IBaseFilter> mDvbTuner;
  Microsoft::WRL::ComPtr<IBaseFilter> mDvbCapture;

  Microsoft::WRL::ComPtr<IDVBTLocator> mDvbLocator;
  Microsoft::WRL::ComPtr<IScanningTuner> mScanningTuner;
  Microsoft::WRL::ComPtr<ITuningSpace> mTuningSpace;
  Microsoft::WRL::ComPtr<IDVBTuningSpace2> mDvbTuningSpace2;
  Microsoft::WRL::ComPtr<ITuneRequest> mTuneRequest;

  Microsoft::WRL::ComPtr<IBaseFilter> mMpeg2Demux;
  Microsoft::WRL::ComPtr<IBaseFilter> mBdaTif;

  Microsoft::WRL::ComPtr<IBaseFilter> mGrabberFilter;
  Microsoft::WRL::ComPtr<ISampleGrabber> mGrabber;
  cGrabberCB mGrabberCB;

  Microsoft::WRL::ComPtr<IBaseFilter> mInfTeeFilter;

  Microsoft::WRL::ComPtr<IBaseFilter> mDumpFilter;
  Microsoft::WRL::ComPtr<IFileSinkFilter> mFileSinkFilter;

  Microsoft::WRL::ComPtr<IMediaControl> mMediaControl;
  //}}}
  //{{{
  string wstrToStr (const wstring& wstr) {
    wstring_convert<codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.to_bytes (wstr);
    }
  //}}}

  //{{{
  bool connectPins (Microsoft::WRL::ComPtr<IBaseFilter> fromFilter,
                    Microsoft::WRL::ComPtr<IBaseFilter> toFilter,
                    wchar_t* fromPinName = NULL, wchar_t* toPinName = NULL) {
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
                  cLog::log (LOGINFO3, "- connecting pin " + wstrToStr (fromPinInfo.achName) +
                                       " to " + wstrToStr (toPinInfo.achName));
                  return true;
                  }
                else {
                  cLog::log (LOGINFO3, "- connectPins failed");
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
  void findFilter (Microsoft::WRL::ComPtr<IBaseFilter>& filter,
                   const CLSID& clsid, const wchar_t* name,
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

          cLog::log (LOGINFO, "- " + wstrToStr (varName.bstrVal));

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
  void createFilter (Microsoft::WRL::ComPtr<IBaseFilter>& filter,
                     const CLSID& clsid, const wchar_t* title,
                     Microsoft::WRL::ComPtr<IBaseFilter> fromFilter) {
  // createFilter type clsid, add to graphBuilder, connect fromFilter

    cLog::log (LOGINFO, "createFilter " + wstrToStr (title));
    CoCreateInstance (clsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS (filter.GetAddressOf()));
    mGraphBuilder->AddFilter (filter.Get(), title);
    connectPins (fromFilter, filter);
    }
  //}}}
  //{{{
  bool createGraphDvbT (int frequency) {

    if (CoCreateInstance (CLSID_FilterGraph, nullptr,
                          CLSCTX_INPROC_SERVER, IID_PPV_ARGS (mGraphBuilder.GetAddressOf())) != S_OK) {
      //{{{  error, exit
      cLog::log (LOGERROR, "createGraphDvbT - CoCreateInstance graph failed");
      return false;
      }
      //}}}

    if (CoCreateInstance (CLSID_DVBTNetworkProvider, nullptr,
                          CLSCTX_INPROC_SERVER, IID_PPV_ARGS (mDvbNetworkProvider.GetAddressOf())) != S_OK) {
      //{{{  error, exit
      cLog::log (LOGERROR, "createGraphDvbT - CoCreateInstance dvbNetworkProvider failed");
      return false;
      }
      //}}}

    if (mGraphBuilder->AddFilter (mDvbNetworkProvider.Get(), L"dvbtNetworkProvider") != S_OK) {
      //{{{  error,exit
      cLog::log (LOGERROR, "createGraphDvbT - AddFilter failed");
      return false;
      }
      //}}}
    mDvbNetworkProvider.As (&mScanningTuner);

    //{{{  setup dvbTuningSpace2 interface
    if (mScanningTuner->get_TuningSpace (mTuningSpace.GetAddressOf()) != S_OK)
      cLog::log (LOGERROR, "createGraphDvbT - get_TuningSpace failed");

    mTuningSpace.As (&mDvbTuningSpace2);
    if (mDvbTuningSpace2->put__NetworkType (CLSID_DVBTNetworkProvider) != S_OK)
      cLog::log (LOGERROR, "createGraphDvbT - put__NetworkType failed");

    if (mDvbTuningSpace2->put_SystemType (DVB_Terrestrial) != S_OK)
      cLog::log (LOGERROR, "createGraphDvbT - put_SystemType failed");

    if (mDvbTuningSpace2->put_NetworkID (9018) != S_OK)
      cLog::log (LOGERROR, "createGraphDvbT - put_NetworkID failed");

    if (mDvbTuningSpace2->put_FrequencyMapping (L"") != S_OK)
      cLog::log (LOGERROR, "createGraphDvbT - put_FrequencyMapping failed");

    if (mDvbTuningSpace2->put_UniqueName (L"DTV DVB-T") != S_OK)
      cLog::log (LOGERROR, "createGraphDvbT - put_UniqueName failed");

    if (mDvbTuningSpace2->put_FriendlyName (L"DTV DVB-T") != S_OK)
      cLog::log (LOGERROR, "createGraphDvbT - put_FriendlyName failed");
    //}}}
    //{{{  create dvbtLocator and setup in dvbTuningSpace2 interface
    if (CoCreateInstance (CLSID_DVBTLocator, nullptr,
                          CLSCTX_INPROC_SERVER, IID_PPV_ARGS (mDvbLocator.GetAddressOf())) != S_OK)
      cLog::log (LOGERROR, "createGraphDvbT - CoCreateInstance dvbLocator failed");

    if (mDvbLocator->put_CarrierFrequency (frequency) != S_OK)
      cLog::log (LOGERROR, "createGraphDvbT - put_CarrierFrequency failed");

    if (mDvbLocator->put_Bandwidth (8) != S_OK)
      cLog::log (LOGERROR, "createGraphDvbT - put_Bandwidth failed");

    if (mDvbTuningSpace2->put_DefaultLocator (mDvbLocator.Get()) != S_OK)
      cLog::log (LOGERROR, "createGraphDvbT - put_DefaultLocator failed");
    //}}}
    //{{{  tuneRequest from scanningTuner
    if (mScanningTuner->get_TuneRequest (mTuneRequest.GetAddressOf()) != S_OK)
      mTuningSpace->CreateTuneRequest (mTuneRequest.GetAddressOf());

    if (mTuneRequest->put_Locator (mDvbLocator.Get()) != S_OK)
      cLog::log (LOGERROR, "createGraphDvbT - put_Locator failed");

    if (mScanningTuner->Validate (mTuneRequest.Get()) != S_OK)
      cLog::log (LOGERROR, "createGraphDvbT - Validate failed");

    if (mScanningTuner->put_TuneRequest (mTuneRequest.Get()) != S_OK)
      cLog::log (LOGERROR, "createGraphDvbT - put_TuneRequest failed");
    //}}}

    // dvbtNetworkProvider -> dvbtTuner -> dvbtCapture -> sampleGrabberFilter -> mpeg2Demux -> bdaTif
    findFilter (mDvbTuner, KSCATEGORY_BDA_NETWORK_TUNER, L"DVBTtuner", mDvbNetworkProvider);
    if (!mDvbTuner) {
      //{{{  error, exit
      cLog::log (LOGERROR, "createGraphDvbT - unable to find dvbtuner filter");
      return false;
      }
      //}}}

    findFilter (mDvbCapture, KSCATEGORY_BDA_RECEIVER_COMPONENT, L"DVBTcapture", mDvbTuner);
    return true;
    }
  //}}}
  //{{{
  bool createGraph (int frequency)  {

    if (createGraphDvbT (frequency)) {
      createFilter (mGrabberFilter, CLSID_SampleGrabber, L"grabber", mDvbCapture);
      mGrabberFilter.As (&mGrabber);

      if (mGrabber->SetOneShot (false) != S_OK)
        cLog::log (LOGERROR, "SetOneShot failed");

      if (mGrabber->SetBufferSamples (true) != S_OK)
        cLog::log (LOGERROR, "SetBufferSamples failed");

      mGrabberCB.allocateBuffer (128*240*188);

      if (mGrabber->SetCallback (&mGrabberCB, 0) != S_OK)
        cLog::log (LOGERROR, "SetCallback failed");

      createFilter (mMpeg2Demux, CLSID_MPEG2Demultiplexer, L"MPEG2demux", mGrabberFilter);
      createFilter (mBdaTif, CLSID_BDAtif, L"BDAtif", mMpeg2Demux);
      mGraphBuilder.As (&mMediaControl);

      return true;
      }
    else
      return false;
    }
  //}}}

  //{{{
  uint8_t* getBlock (int& len) {

    return mGrabberCB.getBlock (len);
    }
  //}}}
  //{{{
  void releaseBlock (int len) {
    mGrabberCB.releaseBlock (len);
    }
  //}}}
#else
  //{{{  vars
  int mFrontEnd = 0;
  int mDemux = 0;
  int mDvr = 0 ;

  uint64_t mLastErrors = 0;
  int mLastBlockSize = 0;
  int mMaxBlockSize = 0;
  //}}}
  //{{{
  void setTsFilter (uint16_t pid, dmx_pes_type_t pestype) {

    struct dmx_pes_filter_params pesFilterParams;
    pesFilterParams.pid = pid;
    pesFilterParams.input = DMX_IN_FRONTEND;
    pesFilterParams.output = DMX_OUT_TS_TAP;
    pesFilterParams.pes_type = pestype;
    pesFilterParams.flags = DMX_IMMEDIATE_START;

    auto error = ioctl (mDemux, DMX_SET_PES_FILTER, &pesFilterParams);
    if (error < 0)
      cLog::log (LOGERROR, "Demux set filter pid " + dec(pid) + " " + dec(error));
    }
  //}}}
  //{{{
  string updateSignalStr() {

    fe_status_t feStatus;
    if (ioctl (mFrontEnd, FE_READ_STATUS, &feStatus) < 0) {
      cLog::log (LOGERROR, "FE_READ_STATUS failed");
      return "dvb readStatus failed";
      }
    else {
      // not sure why these fail, only a problem with xbox one usb tuner
      uint32_t strength = 0;
      ioctl (mFrontEnd, FE_READ_SIGNAL_STRENGTH, &strength);

      uint32_t snr = 0;
      ioctl (mFrontEnd, FE_READ_SNR, &snr);

      return format ("{}{}{}{}{}{} strength:{:.1f} snr:{:.1f}",
                     feStatus & FE_TIMEDOUT ? "timeout " : "",
                     feStatus & FE_HAS_LOCK ? "lock " : "",
                     feStatus & FE_HAS_SIGNAL ? "s" : "",
                     feStatus & FE_HAS_CARRIER ? "c": "",
                     feStatus & FE_HAS_VITERBI ? "v": " ",
                     feStatus & FE_HAS_SYNC ? "s" : "",
                     (strength & 0xFFFF) / 1000.f,
                     (snr & 0xFFFF) / 1000.f);
      }
    }
  //}}}
  //{{{
  string updateErrorStr (int errors) {
    return format ("err:{} max:{}", errors, mMaxBlockSize);
    }
  //}}}
  //{{{
  void readMonitorFe() {

    struct dtv_property getProps[] = {
        { .cmd = DTV_STAT_SIGNAL_STRENGTH },
        { .cmd = DTV_STAT_CNR },
        { .cmd = DTV_STAT_PRE_ERROR_BIT_COUNT },
        { .cmd = DTV_STAT_PRE_TOTAL_BIT_COUNT },
        { .cmd = DTV_STAT_POST_ERROR_BIT_COUNT },
        { .cmd = DTV_STAT_POST_TOTAL_BIT_COUNT },
        { .cmd = DTV_STAT_ERROR_BLOCK_COUNT },
        { .cmd = DTV_STAT_TOTAL_BLOCK_COUNT },
      };

    struct dtv_properties cmdGet = {
      .num = sizeof(getProps) / sizeof (getProps[0]),
      .props = getProps
      };

    if ((ioctl (mFrontEnd, FE_GET_PROPERTY, &cmdGet)) < 0)
      cLog::log (LOGERROR, "FE_GET_PROPERTY failed");
    else
      for (int i = 0; i < 8; i++) {
        auto uvalue = getProps[i].u.st.stat[0].uvalue;
        cLog::log (LOGINFO, "stats %d len:%d scale:%d uvalue:%d",
                   i, getProps[i].u.st.len, getProps[i].u.st.stat[0].scale, int(uvalue));
        }
    //__s64 svalue;

    // need to decode further
    }
  //}}}
#endif
  //{{{
  class cDvbTransportStream : public cTransportStream {
  public:
    //{{{
    cDvbTransportStream (const string& rootName,
                         const vector <string>& channelStrings, const vector <string>& saveStrings,
                         bool decodeSubtitle)
      : mRootName(rootName),
        mChannelStrings(channelStrings), mSaveStrings(saveStrings),
        mRecordAll ((channelStrings.size() == 1) && (channelStrings[0] == "all")),
        mDecodeSubtitle(decodeSubtitle) {}
    //}}}
    //{{{
    virtual ~cDvbTransportStream() {

      for (auto& subtitle : mSubtitleMap)
        delete (subtitle.second);

      mSubtitleMap.clear();
      }
    //}}}

    //{{{
    cSubtitle* getSubtitleBySid (int sid) {

      auto it = mSubtitleMap.find (sid);
      return (it == mSubtitleMap.end()) ? nullptr : (*it).second;
      }
    //}}}

  protected:
    //{{{
    virtual void start (cService* service, const string& name,
                        chrono::system_clock::time_point time,
                        chrono::system_clock::time_point starttime,
                        bool selected) {

      lock_guard<mutex> lockGuard (mFileMutex);

      service->closeFile();

      bool record = selected || mRecordAll;
      string saveName;

      if (!mRecordAll) {
        // filter and rename channel prefix
        size_t i = 0;
        for (auto& channelString : mChannelStrings) {
          if (channelString == service->getChannelString()) {
            record = true;
            if (i < mSaveStrings.size())
              saveName = mSaveStrings[i] +  " ";
            break;
            }
          i++;
          }
        }

      saveName += date::format ("%d %b %y %a %H.%M.%S ", date::floor<chrono::seconds>(time));

      if (record) {
        if ((service->getVidPid() > 0) &&
            (service->getAudPid() > 0) &&
            (service->getSubPid() > 0)) {
          auto validName = validFileString (name, "<>:/|?*\"\'\\");
          auto fileNameStr = mRootName + "/" + saveName + validName + ".ts";
          service->openFile (fileNameStr, 0x1234);
          cLog::log (LOGINFO, fileNameStr);
          }
        }
      }
    //}}}
    //{{{
    virtual void pesPacket (int sid, int pid, uint8_t* ts) {
    // look up service and write it

      lock_guard<mutex> lockGuard (mFileMutex);

      auto serviceIt = mServiceMap.find (sid);
      if (serviceIt != mServiceMap.end())
        serviceIt->second.writePacket (ts, pid);
      }
    //}}}
    //{{{
    virtual void stop (cService* service) {

      lock_guard<mutex> lockGuard (mFileMutex);

      service->closeFile();
      }
    //}}}

    //{{{
    virtual bool audDecodePes (cPidInfo* pidInfo, bool skip) {
      //cLog::log (LOGINFO, getPtsString (pidInfo->mPts) + " a - " + dec(pidInfo->getBufUsed());
      return false;
      }
    //}}}
    //{{{
    virtual bool vidDecodePes (cPidInfo* pidInfo, bool skip) {
      //cLog::log (LOGINFO, getPtsString (pidInfo->mPts) + " v - " + dec(pidInfo->getBufUsed());
      return false;
      }
    //}}}
    //{{{
    virtual bool subDecodePes (cPidInfo* pidInfo) {

      if (kDebug)
        cLog::log (LOGINFO1, "subtitle pid:" + dec(pidInfo->mPid,4) +
                             " sid:" + dec(pidInfo->mSid,5) +
                             " size:" + dec(pidInfo->getBufUsed(),4) +
                             " " + getFullPtsString (pidInfo->mPts) +
                             " " + getChannelStringBySid (pidInfo->mSid));

      if (mDecodeSubtitle) {
        // find or create sid service cSubtitleContext
        auto it = mSubtitleMap.find (pidInfo->mSid);
        if (it == mSubtitleMap.end()) {
          auto insertPair = mSubtitleMap.insert (map <int, cSubtitle*>::value_type (pidInfo->mSid, new cSubtitle()));
          it = insertPair.first;
          cLog::log (LOGINFO1, "cDvb::subDecodePes - create serviceStuff sid:" + dec(pidInfo->mSid));
          }
        auto subtitle = it->second;

        subtitle->decode (pidInfo->mBuffer, pidInfo->getBufUsed());
        if (kDebug)
          subtitle->debug ("- ");
        }

      return false;
      }
    //}}}

  private:
    string mRootName;

    vector<string> mChannelStrings;
    vector<string> mSaveStrings;
    bool mRecordAll;
    bool mDecodeSubtitle;

    mutex mFileMutex;

    map <int, cSubtitle*> mSubtitleMap; // indexed by sid
    };
  //}}}
  cDvbTransportStream* mDvbTransportStream;
  }


// public:
//{{{
cDvb::cDvb (int frequency, const string& root,
            const vector <string>& channelNames, const vector <string>& recordNames,
            bool decodeSubtitle) : mDecodeSubtitle(decodeSubtitle) {

  mDvbTransportStream = new cDvbTransportStream (root, channelNames, recordNames, decodeSubtitle);

  if (frequency) {
    #ifdef _WIN32
      // windows create and tune
      if (createGraph (frequency * 1000))
        mTuneStr = "tuned " + dec (frequency, 3) + "Mhz";
      else
        mTuneStr = "not tuned " + dec (frequency, 3) + "Mhz";

    #else
      // frontend nonBlocking rw
      mFrontEnd = open ("/dev/dvb/adapter0/frontend0", O_RDWR | O_NONBLOCK);
      if (mFrontEnd < 0){
        cLog::log (LOGERROR, "open frontend failed");
        return;
        }
      tune (frequency * 1000000);

      // demux nonBlocking rw
      mDemux = open ("/dev/dvb/adapter0/demux0", O_RDWR | O_NONBLOCK);
      if (mDemux < 0) {
        cLog::log (LOGERROR, "open demux failed");
        return;
        }
      setTsFilter (8192, DMX_PES_OTHER);

      // dvr blocking reads
      mDvr = open ("/dev/dvb/adapter0/dvr0", O_RDONLY);

      constexpr int kDvrBufferSize = 256 * 1024 * 188;
      if (ioctl (mDvr, DMX_SET_BUFFER_SIZE, kDvrBufferSize) < 0)

      if (mDvr < 0)
        cLog::log (LOGERROR, "open dvr failed");

    #endif
    }
  };
//}}}
//{{{
cDvb::~cDvb() {

  #ifndef _WIN32
    // linux
    if (mDvr)
      close (mDvr);
    if (mDemux)
      close (mDemux);
    if (mFrontEnd)
      close (mFrontEnd);
  #endif

  delete mDvbTransportStream;
  }
//}}}

//{{{
cSubtitle* cDvb::getSubtitleBySid (int sid) {
  return mDvbTransportStream->getSubtitleBySid (sid);
  }
//}}}
//{{{
cTransportStream* cDvb::getTransportStream() {
  return mDvbTransportStream;
  }
//}}}

//{{{
void cDvb::tune (int frequency) {

  #ifdef _WIN32
    // windows tune
    if (mDvbLocator->put_CarrierFrequency (frequency) != S_OK)
      cLog::log (LOGERROR, "tune - put_CarrierFrequency");
    if (mDvbLocator->put_Bandwidth (8) != S_OK)
      cLog::log (LOGERROR, "tune - put_Bandwidth");
    if (mDvbTuningSpace2->put_DefaultLocator (mDvbLocator.Get()) != S_OK)
      cLog::log (LOGERROR, "tune - put_DefaultLocator");
    if (mTuneRequest->put_Locator (mDvbLocator.Get()) != S_OK)
      cLog::log (LOGERROR, "tune - put_Locator");
    if (mScanningTuner->Validate (mTuneRequest.Get()) != S_OK)
      cLog::log (LOGERROR, "tune - Validate");
    if (mScanningTuner->put_TuneRequest (mTuneRequest.Get()) != S_OK)
      cLog::log (LOGERROR, "tune - put_TuneRequest");
    if (mMediaControl->Run() != S_OK)
      cLog::log (LOGERROR, "tune - run");

  #else
    // linux tune
    bool t2 = frequency == 626000000;

    struct dvb_frontend_info fe_info;
    if (ioctl (mFrontEnd, FE_GET_INFO, &fe_info) < 0) {
      //{{{  error, exit
      cLog::log (LOGERROR, "FE_GET_INFO failed");
      return;
      }
      //}}}
    cLog::log (LOGINFO, "tuning %s %s to %u ", fe_info.name, t2 ? "T2" : "T", frequency);

    //{{{  discard stale events
    struct dvb_frontend_event ev;
    while (true)
      if (ioctl (mFrontEnd, FE_GET_EVENT, &ev) < 0)
        break;
      else
        cLog::log (LOGINFO, "discarding stale event");
    //}}}

    //{{{
    struct dtv_property tuneProps[10];

    tuneProps[0].cmd = DTV_CLEAR;

    tuneProps[1].cmd = DTV_DELIVERY_SYSTEM;
    tuneProps[1].u.data = t2 ? SYS_DVBT2 : SYS_DVBT;

    tuneProps[2].cmd = DTV_FREQUENCY;
    tuneProps[2].u.data = frequency;

    tuneProps[3].cmd = DTV_MODULATION;
    tuneProps[3].u.data = QAM_AUTO;

    tuneProps[4].cmd = DTV_CODE_RATE_HP;
    tuneProps[4].u.data = FEC_AUTO;

    tuneProps[5].cmd = DTV_CODE_RATE_LP;
    tuneProps[5].u.data = 0;

    tuneProps[6].cmd = DTV_INVERSION;
    tuneProps[6].u.data = INVERSION_AUTO;

    tuneProps[7].cmd = DTV_TRANSMISSION_MODE;
    tuneProps[7].u.data = TRANSMISSION_MODE_AUTO;

    tuneProps[8].cmd = DTV_BANDWIDTH_HZ;
    tuneProps[8].u.data = 8000000;

    tuneProps[9].cmd = DTV_TUNE;
    //}}}
    struct dtv_properties cmdTune = { .num = 10, .props = tuneProps };

    if ((ioctl (mFrontEnd, FE_SET_PROPERTY, &cmdTune)) < 0) {
      //{{{  error, exit
      cLog::log (LOGERROR, "FE_SET_PROPERTY TUNE failed");
      return;
      }
      //}}}

    //{{{  wait for zero status indicating start of tuning
    do {
      ioctl (mFrontEnd, FE_GET_EVENT, &ev);
      cLog::log (LOGINFO, "waiting to tune");
      } while (ev.status != 0);
    //}}}

    // wait for tuning
    for (int i = 0; i < 10; i++) {
      this_thread::sleep_for (100ms);

      if (ioctl (mFrontEnd, FE_GET_EVENT, &ev) < 0) // no answer, consider it as not locked situation
        ev.status = FE_NONE;

      if (ev.status & FE_HAS_LOCK) {
        //{{{  tuning locked
        struct dtv_property getProps[] = {
            { .cmd = DTV_DELIVERY_SYSTEM },
            { .cmd = DTV_MODULATION },
            { .cmd = DTV_INNER_FEC },
            { .cmd = DTV_CODE_RATE_HP },
            { .cmd = DTV_CODE_RATE_LP },
            { .cmd = DTV_INVERSION },
            { .cmd = DTV_GUARD_INTERVAL },
            { .cmd = DTV_TRANSMISSION_MODE },
          };

        struct dtv_properties cmdGet = { .num = sizeof(getProps) / sizeof (getProps[0]), .props = getProps };
        if ((ioctl (mFrontEnd, FE_GET_PROPERTY, &cmdGet)) < 0) {
          //{{{  error, exit
          cLog::log (LOGERROR, "FE_GET_PROPERTY failed");
          return;
          }
          //}}}

        //{{{
        const char* deliveryTab[] = {
          "UNDEFINED",
          "DVBC_ANNEX_A",
          "DVBC_ANNEX_B",
          "DVBT",
          "DSS",
          "DVBS",
          "DVBS2",
          "DVBH",
          "ISDBT",
          "ISDBS",
          "ISDBC",
          "ATSC",
          "ATSCMH",
          "DTMB",
          "CMMB",
          "DAB",
          "DVBT2",
          "TURBO",
          "DVBC_ANNEX_C"
          };
        //}}}
        //{{{
        const char* modulationTab[] =  {
          "QPSK",
          "QAM_16",
          "QAM_32",
          "QAM_64",
          "QAM_128",
          "QAM_256",
          "QAM_AUTO",
          "VSB_8",
          "VSB_16",
          "PSK_8",
          "APSK_16",
          "APSK_32",
          "DQPSK"
        };
        //}}}
        //{{{
        const char* inversionTab[] = {
          "INV_OFF",
          "INV_ON",
          "INV_AUTO"
          };
        //}}}
        //{{{
        const char* codeRateTab[] = {
          "FEC_NONE",
          "FEC_1_2",
          "FEC_2_3",
          "FEC_3_4",
          "FEC_4_5",
          "FEC_5_6",
          "FEC_6_7",
          "FEC_7_8",
          "FEC_8_9",
          "FEC_AUTO",
          "FEC_3_5",
          "FEC_9_10",
          };
        //}}}
        //{{{
        const char* transmissionModeTab[] = {
          "TM_2K",
          "TM_8K",
          "TM_AUTO",
          "TM_4K",
          "TM_1K",
          "TM_16K",
          "TM_32K"
          };
        //}}}
        //{{{
        const char* guardTab[] = {
          "GUARD_1_32",
          "GUARD_1_16",
          "GUARD_1_8",
          "GUARD_1_4",
          "GUARD_AUTO",
          "GUARD_1_128",
          "GUARD_19_128",
          "GUARD_19_256"
          };
        //}}}

        cLog::log (LOGINFO, "locked using %s %s %s %s %s %s %s ",
                   deliveryTab [getProps[0].u.buffer.data[0]],
                   modulationTab [getProps[1].u.buffer.data[0]],
                   codeRateTab [getProps[2].u.buffer.data[0]],
                   codeRateTab [getProps[3].u.buffer.data[0]],
                   codeRateTab [getProps[4].u.buffer.data[0]],
                   inversionTab [getProps[5].u.buffer.data[0]],
                   guardTab [getProps[6].u.buffer.data[0]],
                   transmissionModeTab [getProps[7].u.buffer.data[0]]);

        mSignalStr = updateSignalStr();
        mErrorStr = updateErrorStr (mDvbTransportStream->getErrors());
        readMonitorFe();

        mTuneStr = string(fe_info.name) + " " + dec(frequency/1000000) + "Mhz";
        return;
        }
        //}}}
      else
        cLog::log (LOGINFO, "waiting for lock");
      }

    cLog::log (LOGERROR, "tuning failed\n");

  #endif
  }
//}}}

//{{{
void cDvb::grabThread (const string& root, const string& multiplexName) {

  cLog::setThreadName ("grab");

  string allName = root + "/all" + multiplexName + ".ts";
  FILE* mFile = root.empty() ? nullptr : fopen (allName.c_str(), "wb");

  #ifdef _WIN32 // windows
    auto hr = mMediaControl->Run();
    if (hr == S_OK) {
      int64_t streamPos = 0;
      auto blockSize = 0;
      while (true) {
        auto ptr = getBlock (blockSize);
        if (blockSize) {
          //{{{  read and demux block
          if (mFile)
            fwrite (ptr, 1, blockSize, mFile);

          streamPos += mDvbTransportStream->demux ({}, ptr, blockSize, streamPos, false);
          releaseBlock (blockSize);

          mErrorStr.clear();
          if (mDvbTransportStream->getErrors())
            mErrorStr += dec(mDvbTransportStream->getErrors()) + " err:";
          if (streamPos < 1000000)
            mErrorStr = dec(streamPos/1000) + "k";
          else
            mErrorStr = dec(streamPos/1000000) + "m";
          }
          //}}}
        else
          this_thread::sleep_for (1ms);
        if (mScanningTuner) {
          //{{{  update mSignalStr
          long signal = 0;
          mScanningTuner->get_SignalStrength (&signal);
          mSignalStr = "signal " + dec (signal / 0x10000);
          }
          //}}}
        }
      }
    else
      cLog::log (LOGERROR, "run graph failed " + dec(hr));

  #else // linux
    constexpr int kDvrReadBufferSize = 1024 * 188;
    auto buffer = (uint8_t*)malloc (kDvrReadBufferSize);

    uint64_t streamPos = 0;
    while (true) {
      int bytesRead = read (mDvr, buffer, kDvrReadBufferSize);
      if (bytesRead > 0) {
        streamPos += mDvbTransportStream->demux ({}, buffer, bytesRead, 0, false);
        if (mFile)
          fwrite (buffer, 1, bytesRead, mFile);

        bool show = (mDvbTransportStream->getErrors() != mLastErrors) || (bytesRead > mLastBlockSize);
        mLastErrors = mDvbTransportStream->getErrors();
        if (bytesRead > mLastBlockSize)
          mLastBlockSize = bytesRead;
        if (bytesRead > mMaxBlockSize)
          mMaxBlockSize = bytesRead;

        mSignalStr = updateSignalStr();
        if (show) {
          mErrorStr = updateErrorStr (mDvbTransportStream->getErrors());
          cLog::log (LOGINFO, mErrorStr + " " + mSignalStr);
          }
        }
      else
        cLog::log (LOGINFO, "cDvb grabThread no bytes read");
      }
    free (buffer);
  #endif

  if (mFile)
    fclose (mFile);

  cLog::log (LOGINFO, "exit");
  }
//}}}
//{{{
void cDvb::readThread (const string& fileName) {

  cLog::setThreadName ("read");

  auto file = fopen (fileName.c_str(), "rb");
  if (!file) {
    //{{{  error, return
    cLog::log (LOGERROR, "no file " + fileName);
    return;
    }
    //}}}

  uint64_t streamPos = 0;
  auto blockSize = 188 * 8;
  auto buffer = (uint8_t*)malloc (blockSize);

  int i = 0;
  bool run = true;
  while (run) {
    i++;
    if (!(i % 200)) // throttle read rate
      this_thread::sleep_for (20ms);

    size_t bytesRead = fread (buffer, 1, blockSize, file);
    if (bytesRead > 0)
      streamPos += mDvbTransportStream->demux ({}, buffer, bytesRead, streamPos, false);
    else
      break;
    mErrorStr = dec(mDvbTransportStream->getErrors());
    }

  fclose (file);
  free (buffer);

  cLog::log (LOGERROR, "exit");
  }
//}}}
