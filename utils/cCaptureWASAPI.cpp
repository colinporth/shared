// cCaptureWASAPI.cpp
#define _CRT_SECURE_NO_WARNINGS
#include "cCaptureWASAPI.h"
#include "cLog.h"

//{{{
cCaptureWASAPI::cCaptureWASAPI (uint32_t bufferSize) {

  IMMDeviceEnumerator* myMMDeviceEnumerator = NULL;
  if (FAILED (CoCreateInstance (__uuidof (MMDeviceEnumerator), NULL, CLSCTX_ALL,
                                __uuidof (IMMDeviceEnumerator), (void**)&myMMDeviceEnumerator)))
    cLog::log (LOGERROR, "create IMMDeviceEnumerator failed");

  else {
    // get default render endpoint
    if (FAILED (myMMDeviceEnumerator->GetDefaultAudioEndpoint (eRender, eMultimedia, &mMMDevice)))
      cLog::log (LOGERROR, "IMMDeviceEnumerator::GetDefaultAudioEndpoint failed");

    if (FAILED (mMMDevice->Activate (__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&mAudioClient)))
      cLog::log (LOGERROR, "IMMDevice::Activate IAudioClient failed");

    if (FAILED (mAudioClient->GetDevicePeriod (&mHhnsDefaultDevicePeriod, NULL)))
      cLog::log (LOGERROR, "audioClient GetDevicePeriod failed");

    // get the default device format
    if (FAILED (mAudioClient->GetMixFormat (&mWaveFormatEx)))
      cLog::log (LOGERROR, "audioClient GetMixFormat failed");

    // with AUDCLNT_STREAMFLAGS_LOOPBACK, AUDCLNT_STREAMFLAGS_EVENTCALLBACK "data ready" event never gets set
    if (FAILED (mAudioClient->Initialize (AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, mWaveFormatEx, 0)))
      cLog::log (LOGERROR, "audioClient initialize failed");

    // activate an IAudioCaptureClient
    if (FAILED (mAudioClient->GetService (__uuidof(IAudioCaptureClient), (void**)&mAudioCaptureClient)))
      cLog::log (LOGERROR, "create IAudioCaptureClient failed");

    // start audioClient
    if (FAILED (mAudioClient->Start()))
      cLog::log (LOGERROR, "audioClient start failed");

    if (myMMDeviceEnumerator)
      myMMDeviceEnumerator->Release();

    mStreamFirst = (float*)malloc (bufferSize);
    mStreamLast = mStreamFirst + (bufferSize / 4);
    mStreamReadPtr = mStreamFirst;
    mStreamWritePtr = mStreamFirst;

    cLog::log (LOGNOTICE, "Simple buffer lasts %d minutes", bufferSize / 4 / 2 / 48000 / 60);
    }

  }
//}}}
//{{{
cCaptureWASAPI::~cCaptureWASAPI() {

  if (mAudioClient) {
    mAudioClient->Stop();
    mAudioClient->Release();
    }

  if (mAudioCaptureClient)
    mAudioCaptureClient->Release();

  if (mWaveFormatEx)
    CoTaskMemFree (mWaveFormatEx);

  if (mMMDevice)
    mMMDevice->Release();

  free (mStreamFirst);
  }
//}}}

//{{{
float* cCaptureWASAPI::getFrame (int frameSize) {
// get frameSize worth of floats if available

  if (mStreamWritePtr - mStreamReadPtr >= frameSize * kCaptureChannnels) {
    auto ptr = mStreamReadPtr;
    mStreamReadPtr += frameSize * kCaptureChannnels;
    return ptr;
    }
  else
    return nullptr;
  }
//}}}

//{{{
void cCaptureWASAPI::run() {

  // create a periodic waitable timer, -ve relative time, convert to milliseconds
  HANDLE wakeUp = CreateWaitableTimer (NULL, FALSE, NULL);
  LARGE_INTEGER firstFire;
  firstFire.QuadPart = -mHhnsDefaultDevicePeriod / 2;
  LONG timeBetweenFires = (LONG)mHhnsDefaultDevicePeriod / 2 / (10 * 1000);
  SetWaitableTimer (wakeUp, &firstFire, timeBetweenFires, NULL, NULL, FALSE);

  bool done = false;
  while (!done) {
    UINT32 packetSize;
    mAudioCaptureClient->GetNextPacketSize (&packetSize);
    while (!done && (packetSize > 0)) {
      BYTE* data;
      UINT32 numFramesRead;
      DWORD dwFlags;
      if (FAILED (mAudioCaptureClient->GetBuffer (&data, &numFramesRead, &dwFlags, NULL, NULL))) {
        //{{{  exit
        cLog::log (LOGERROR, "IAudioCaptureClient::GetBuffer failed");
        done = true;
        break;
        }
        //}}}
      if (numFramesRead == 0) {
        //{{{  exit
        cLog::log (LOGERROR, "audioCaptureClient::GetBuffer read 0 frames");
        done = true;
        }
        //}}}
      if (dwFlags == AUDCLNT_BUFFERFLAGS_SILENT)
        cLog::log (LOGINFO, "audioCaptureClient::GetBuffer silent %d", numFramesRead);
      else {
        if (dwFlags == AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY)
          cLog::log (LOGINFO, "audioCaptureClient::GetBuffer discontinuity");

        //cLog::log (LOGINFO, "captured frames:%d bytes:%d", numFramesRead, numFramesRead * mWaveFormatEx->nBlockAlign);
        memcpy (mStreamWritePtr, data, numFramesRead * mWaveFormatEx->nBlockAlign);
        mStreamWritePtr += numFramesRead * kCaptureChannnels;
        }

      if (FAILED (mAudioCaptureClient->ReleaseBuffer (numFramesRead))) {
        //{{{  exit
        cLog::log (LOGERROR, "audioCaptureClient::ReleaseBuffer failed");
        done = true;
        break;
        }
        //}}}

      mAudioCaptureClient->GetNextPacketSize (&packetSize);
      }

    DWORD dwWaitResult = WaitForSingleObject (wakeUp, INFINITE);
    if (dwWaitResult != WAIT_OBJECT_0) {
      //{{{  exit
      cLog::log (LOGERROR, "WaitForSingleObject error %u", dwWaitResult);
      done = true;
      }
      //}}}
    }

  CancelWaitableTimer (wakeUp);
  CloseHandle (wakeUp);
  }
//}}}
