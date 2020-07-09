// cWinDvb.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#include <locale>
#include <codecvt>

#include <chrono>
#include "../date/date.h"
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

using namespace std;
//}}}

//{{{
inline string wstrToStr (const wstring& wstr) {
  wstring_convert<codecvt_utf8<wchar_t>, wchar_t> converter;
  return converter.to_bytes (wstr);
  }
//}}}

// public:
//{{{
cDvb::cDvb (int frequency, const string& root, bool recordAll) : cDumpTransportStream (root, recordAll) {
  createGraph (frequency);
  };
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
  }
//}}}

//{{{
void cDvb::grabThread() {

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
        if (getDiscontinuity())
          mErrorStr = "errors " + dec (getDiscontinuity()) + " of " + dec(streamPos/1000000) + "m";
        else
          mErrorStr = dec(streamPos/1000000) + "m";
        }
      else
        this_thread::sleep_for (1ms);

      if (mScanningTuner) {
        long signal = 0;
        mScanningTuner->get_SignalStrength (&signal);
        mSignalStr = "signal " + dec(signal / 0x10000, 3);
        }
      }
    }
  else
    cLog::log (LOGERROR, "run graph failed " + dec(hr));

  cLog::log (LOGINFO, "exit");
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
bool cDvb::createGraph (int frequency)  {

  if (!createGraphDvbT (frequency))
    return false;

  mTuneStr = "tuned " + dec (frequency / 1000, 3) + "Mhz";

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
uint8_t* cDvb::getBlock (int& len) {

  return mGrabberCB.getBlock (len);
  }
//}}}
//{{{
void cDvb::releaseBlock (int len) {
  mGrabberCB.releaseBlock (len);
  }
//}}}
