// cWinDvb.h
//{{{  includes
#pragma once

#include <wrl.h>

#include <initguid.h>
#include <DShow.h>
#include <bdaiface.h>
#include <ks.h>
#include <ksmedia.h>
#include <bdatif.h>

#include "cDumpTransportStream.h"

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

//}}}

class cDvb : public cDumpTransportStream {
public:
  cDvb (const std::string& root);
  virtual ~cDvb() {}

  int getSignal() { return mSignal; }
  int getFrequency() { return mFrequency; }

  bool createGraph (int frequency);
  bool createGraph (int frequency, const std::string& fileName);

  void tune (int frequency);
  void pause();
  void stop();

  void grabThread();
  void signalThread();
  void readThread (const std::string& inTs);

  // vars - public for widget
  int mSignal = 0;

  std::string mTuneStr = "untuned";
  std::string mSignalStr = "signal";
  std::string mPacketStr = "packet";

private:
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

  bool connectPins (Microsoft::WRL::ComPtr<IBaseFilter> fromFilter,
                    Microsoft::WRL::ComPtr<IBaseFilter> toFilter,
                    wchar_t* fromPinName = NULL, wchar_t* toPinName = NULL);
  void findFilter (Microsoft::WRL::ComPtr<IBaseFilter>& filter,
                   const CLSID& clsid, wchar_t* name,
                   Microsoft::WRL::ComPtr<IBaseFilter> fromFilter);
  void createFilter (Microsoft::WRL::ComPtr<IBaseFilter>& filter,
                     const CLSID& clsid, wchar_t* title,
                     Microsoft::WRL::ComPtr<IBaseFilter> fromFilter);
  bool createGraphDvbT (int frequency);

  uint8_t* getBlock (int& len);
  void releaseBlock (int len);

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

  int mFrequency = 0;
  //}}}
  };
