// cHttp.h - http base class based on tinyHttp parser
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <functional>

#include "cUrl.h"
//}}}

class cHttp {
public:
  cHttp();
  virtual ~cHttp();
  virtual void initialise() = 0;

  // gets
  int getResponse() { return mResponse; }
  uint8_t* getContent() { return mContent; }
  int getContentSize() { return mContentReceivedSize; }

  int getHeaderContentSize() { return mHeaderContentLength; }

  int get (const std::string& host, const std::string& path, const std::string& header = "",
           const std::function<void (const std::string& key, const std::string& value)>& headerCallback = [](const std::string&, const std::string&) noexcept {},
           const std::function<bool (const uint8_t* data, int len)>& dataCallback = [](const uint8_t*, int) noexcept { return true; });
  std::string getRedirect (const std::string& host, const std::string& path);
  void freeContent();

protected:
  virtual int connectToHost (const std::string& host) = 0;
  virtual bool getSend (const std::string& sendStr) = 0;
  virtual int getRecv (uint8_t* buffer, int bufferSize) { return 0; }

private:
  //{{{
  enum eState {
    eHeader,
    eExpectedData,
    eChunkHeader,
    eChunkData,
    eStreamData,
    eClose,
    eError,
    };
  //}}}
  //{{{
  enum eHeaderState {
    eHeaderDone,
    eHeaderContinue,
    eHeaderVersionCharacter,
    eHeaderCodeCharacter,
    eHeaderStatusCharacter,
    eHeaderKeyCharacter,
    eHeaderValueCharacter,
    eHeaderStoreKeyValue,
    };
  //}}}
  //{{{
  enum eContentState {
    eContentNone,
    eContentLength,
    eContentChunked,
    };
  //}}}

  void clear();
  eHeaderState parseHeaderChar (char ch);
  bool parseChunk (int& size, char ch);
  bool parseData (const uint8_t* data, int length, int& bytesParsed,
                  const std::function<void (const std::string& key, const std::string& value)>& headerCallback,
                  const std::function<bool (const uint8_t* data, int len)>& dataCallback);

  // vars
  eState mState = eHeader;

  eHeaderState mHeaderState = eHeaderDone;
  char* mHeaderBuffer = nullptr;
  int mHeaderBufferAllocSize = 0;
  int mKeyLen = 0;
  int mValueLen = 0;

  eContentState mContentState = eContentNone;
  int mHeaderContentLength = -1;
  int mContentLengthLeft = -1;
  int mContentReceivedSize = 0;
  uint8_t* mContent = nullptr;

  int mResponse = 0;
  cUrl mRedirectUrl;
  };
