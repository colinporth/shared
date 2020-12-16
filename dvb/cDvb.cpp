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
#endif

#ifdef __linux__
  //{{{  linux includes
  #include <unistd.h>
  #include <fcntl.h>
  #include <sys/ioctl.h>
  #include <sys/uio.h>
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
#include <thread>

#include "../fmt/core.h"
#include "../date/date.h"
#include "../utils/cLog.h"
#include "../utils/cBipBuffer.h"

#include "cDvb.h"

using namespace std;
using namespace fmt;
//}}}

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
    mDvbNetworkProvider.As (&cDvb::mScanningTuner);

    //{{{  setup dvbTuningSpace2 interface
    if (cDvb::mScanningTuner->get_TuningSpace (mTuningSpace.GetAddressOf()) != S_OK)
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
    if (cDvb::mScanningTuner->get_TuneRequest (mTuneRequest.GetAddressOf()) != S_OK)
      mTuningSpace->CreateTuneRequest (mTuneRequest.GetAddressOf());

    if (mTuneRequest->put_Locator (mDvbLocator.Get()) != S_OK)
      cLog::log (LOGERROR, "createGraphDvbT - put_Locator failed");

    if (cDvb::mScanningTuner->Validate (mTuneRequest.Get()) != S_OK)
      cLog::log (LOGERROR, "createGraphDvbT - Validate failed");

    if (cDvb::mScanningTuner->put_TuneRequest (mTuneRequest.Get()) != S_OK)
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
      mGraphBuilder.As (&cDvb::mMediaControl);

      return true;
      }
    else
      return false;
    }
  //}}}
#endif

#ifdef __linux__
  //{{{  vars
  int mFrontEnd = 0;
  int mDemux = 0;
  struct pollfd fds[1];
  cTsBlock* mBlockFreeList = NULL;
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
      cLog::log (LOGERROR, format ("Demux set filter pid:{} {}", pid, error));
    }
  //}}}
#endif
  }

// public:
//{{{
cDvb::cDvb (int frequency, int adapter) : mFrequency(frequency), mAdapter(adapter) {

  if (frequency) {
    #ifdef _WIN32
      // windows create and tune
      if (createGraph (frequency * 1000))
        mTuneStr = format ("tuned {}Mhz", frequency);
      else
        mTuneStr = format ("not tuned {}Mhz", frequency);
    #endif

    #ifdef __linux__
      // open frontend nonBlocking rw
      string frontend = format ("/dev/dvb/adapter{}/frontend{}", mAdapter, 0);
      mFrontEnd = open (frontend.c_str(), O_RDWR | O_NONBLOCK);
      if (mFrontEnd < 0){
        cLog::log (LOGERROR, "cDvb open frontend failed");
        return;
        }
      tune (frequency * 1000000);

      // open demux nonBlocking rw
      string demux = format ("/dev/dvb/adapter{}/demux{}", mAdapter, 0);
      mDemux = open (demux.c_str(), O_RDWR | O_NONBLOCK);
      if (mDemux < 0) {
        cLog::log (LOGERROR, "cDvb open demux failed");
        return;
        }
      setTsFilter (8192, DMX_PES_OTHER);

      // open dvr blocking reads, big buffer 50m
      string dvr = format ("/dev/dvb/adapter{}/dvr{}", mAdapter, 0);
      mDvr = open (dvr.c_str(), O_RDONLY);
      fds[0].fd = mDvr;
      fds[0].events = POLLIN;

      constexpr int kDvrBufferSize = 256 * 1024 * 188;
      if (ioctl (mDvr, DMX_SET_BUFFER_SIZE, kDvrBufferSize) < 0)
        cLog::log (LOGERROR, "cDvb dvr DMX_SET_BUFFER_SIZE failed");

    #endif
    }
  };
//}}}
//{{{
cDvb::~cDvb() {

  #ifdef __linux__
    if (mDvr)
      close (mDvr);
    if (mDemux)
      close (mDemux);
    if (mFrontEnd)
      close (mFrontEnd);
  #endif
  }
//}}}

//{{{
string cDvb::getStatusString() {
  #ifdef _WIN32
    return "";
  #endif

  #ifdef __linux__
    struct dtv_property props[] = {
      { .cmd = DTV_STAT_SIGNAL_STRENGTH },   // max 0xFFFF percentage
      { .cmd = DTV_STAT_CNR },               // 0.001db
      { .cmd = DTV_STAT_ERROR_BLOCK_COUNT },
      { .cmd = DTV_STAT_TOTAL_BLOCK_COUNT }, // count
      { .cmd = DTV_STAT_PRE_ERROR_BIT_COUNT },
      { .cmd = DTV_STAT_PRE_TOTAL_BIT_COUNT },
      { .cmd = DTV_STAT_POST_ERROR_BIT_COUNT },
      { .cmd = DTV_STAT_POST_TOTAL_BIT_COUNT },
      };

    struct dtv_properties cmdProperty = {
      .num = 8,
      .props = props
      };

    if ((ioctl (mFrontEnd, FE_GET_PROPERTY, &cmdProperty)) < 0)
      return "status failed";

    return format ("strength:{:5.2f}% snr:{:5.2f}db block:{:x},{:x}, pre:{:x},{:x} post:{:x},{:x}",
                   100.f * ((props[0].u.st.stat[0].uvalue & 0xFFFF) / float(0xFFFF)),
                   props[1].u.st.stat[0].svalue / 1000.f,
                   (__u64)props[2].u.st.stat[0].uvalue,
                   (__u64)props[3].u.st.stat[0].uvalue,
                   (__u64)props[4].u.st.stat[0].uvalue,
                   (__u64)props[5].u.st.stat[0].uvalue,
                   (__u64)props[6].u.st.stat[0].uvalue,
                   (__u64)props[7].u.st.stat[0].uvalue);
  #endif
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
  #endif

  #ifdef __linux__
    // linux tune
    bool t2 = frequency == 626000000;

    struct dvb_frontend_info fe_info;
    if (ioctl (mFrontEnd, FE_GET_INFO, &fe_info) < 0) {
      //{{{  error, return
      cLog::log (LOGERROR, "FE_GET_INFO failed");
      return;
      }
      //}}}
    cLog::log (LOGINFO, "tuning %s %s to %u ", fe_info.name, t2 ? "T2" : "T", frequency);

    // discard stale events
    struct dvb_frontend_event ev;
    while (ioctl (mFrontEnd, FE_GET_EVENT, &ev) >= 0)
      cLog::log (LOGINFO, "discarding stale event");

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
    for (int i = 0; i < 20; i++) {
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
        //mErrorStr = updateErrorStr (mDvbTransportStream->getErrors());

        mTuneStr = format ("{}Mhz", frequency/1000000);
        return;
        }
        //}}}
      else
        cLog::log (LOGINFO, "waiting for lock");
      }
    cLog::log (LOGERROR, "tuning failed");
  #endif
  }
//}}}
void cDvb::reset() {}
//{{{
int cDvb::setFilter (uint16_t pid) {

#ifdef _WIN32
  return 0;
#endif

#ifdef __linux__
  int mAdapter = 0;
  int mFeNum = 0;
  string filter = format ("/dev/dvb/adapter{}/demux{}", mAdapter, mFeNum);

  int fd = open (filter.c_str(), O_RDWR);
  if (fd < 0) {
    // error return
    cLog::log (LOGERROR, format ("dvbSetFilter - open failed pid:{}", pid));
    return -1;
    }

  struct dmx_pes_filter_params s_filter_params;
  s_filter_params.pid = pid;
  s_filter_params.input = DMX_IN_FRONTEND;
  s_filter_params.output = DMX_OUT_TS_TAP;
  s_filter_params.flags = DMX_IMMEDIATE_START;
  s_filter_params.pes_type = DMX_PES_OTHER;
  if (ioctl (fd, DMX_SET_PES_FILTER, &s_filter_params) < 0) {
    // error return
    cLog::log (LOGERROR, format ("dvbSetFilter - set_pesFilter failed pid:{} {}", pid, strerror (errno)));
    close (fd);
    return -1;
    }

  cLog::log (LOGINFO1, format ("dvbSetFilter pid:{}", pid));
  return fd;
#endif
  }
//}}}
//{{{
void cDvb::unsetFilter (int fd, uint16_t pid) {

#ifdef __linux__
  if (ioctl (fd, DMX_STOP) < 0)
    cLog::log (LOGERROR, format("dvbUnsetFilter - stop failed {}", strerror (errno)));
  else
    cLog::log (LOGINFO1, format ("dvbUnsetFilter - unsetting pid:{}", pid));
  close (fd);
#endif
  }
//}}}

#ifdef _WIN32
  //{{{
  uint8_t* cDvb::getBlockBDA (int& len) {

    return mGrabberCB.getBlock (len);
    }
  //}}}
  //{{{
  void cDvb::releaseBlock (int len) {
    mGrabberCB.releaseBlock (len);
    }
  //}}}
#endif

#ifdef __linux__
  //{{{
  string cDvb::updateSignalStr() {

    fe_status_t feStatus;
    if (ioctl (mFrontEnd, FE_READ_STATUS, &feStatus) < 0) {
      cLog::log (LOGERROR, "FE_READ_STATUS failed");
      return "dvb readStatus failed";
      }

    return format ("{}{}{}{}{}{} {}",
                   feStatus & FE_TIMEDOUT ? "timeout " : "",
                   feStatus & FE_HAS_LOCK ? "lock " : "",
                   feStatus & FE_HAS_SIGNAL ? "s" : "",
                   feStatus & FE_HAS_CARRIER ? "c": "",
                   feStatus & FE_HAS_VITERBI ? "v": " ",
                   feStatus & FE_HAS_SYNC ? "s" : "",
                   getStatusString());

    }
  //}}}
#endif

//{{{
int cDvb::getBlock (uint8_t*& block, int& blockSize) {

  #ifdef _WIN32
    cLog::log (LOGERROR, "cDvbSimple::getTsBlock not implemented");
    return 0;
  #endif

  #ifdef __linux__
    return ::read (mDvr, block, blockSize);
  #endif
  }
//}}}
//{{{
cTsBlock* cDvb::read (cTsBlockPool* blockPool) {

  constexpr int kMaxRead = 50;
  struct iovec iovecs[kMaxRead];

  cTsBlock* block = mBlockFreeList;
  cTsBlock** current = &block;

  for (int i = 0; i < kMaxRead; i++) {
    if (!(*current))
      *current = blockPool->newBlock();
    iovecs[i].iov_base = (*current)->mTs;
    iovecs[i].iov_len = 188;
    current = &(*current)->mNextBlock;
    }

  while (poll (fds, 1, -1) <= 0)
    cLog::log (LOGINFO, "poll waiting");

  int size = readv (fds[0].fd, iovecs, kMaxRead);
  if (size < 0) {
    cLog::log (LOGERROR, format ("readv DVR failed {}", strerror(errno)));
    size = 0;
    }
  size /= 188;

  current = &block;
  while (size && *current) {
    current = &(*current)->mNextBlock;
    size--;
    }

  mBlockFreeList = *current;
  *current = NULL;

  return block;
  }
//}}}
