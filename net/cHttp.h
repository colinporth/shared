// cHttp.h - http base class based on tinyHttp parser
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <functional>
//#include <sstream>
//#include <iostream>
//#include <iomanip>

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
  int getContentSize() { return mContentReceivedBytes; }

  int get (const std::string& host, const std::string& path,
           const std::string& header = "",
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

  // static const
  //{{{
  static inline const uint8_t kHeaderState[88] = {
  //  *    \t    \n   \r    ' '     ,     :   PAD
    0x80,    1, 0xC1, 0xC1,    1, 0x80, 0x80, 0xC1,  // state 0:  HTTP version
    0x81,    2, 0xC1, 0xC1,    2,    1,    1, 0xC1,  // state 1:  Response code
    0x82, 0x82,    4,    3, 0x82, 0x82, 0x82, 0xC1,  // state 2:  Response reason
    0xC1, 0xC1,    4, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1,  // state 3:  HTTP version newline
    0x84, 0xC1, 0xC0,    5, 0xC1, 0xC1,    6, 0xC1,  // state 4:  Start of header field
    0xC1, 0xC1, 0xC0, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1,  // state 5:  Last CR before end of header
    0x87,    6, 0xC1, 0xC1,    6, 0x87, 0x87, 0xC1,  // state 6:  leading whitespace before header value
    0x87, 0x87, 0xC4,   10, 0x87, 0x88, 0x87, 0xC1,  // state 7:  header field value
    0x87, 0x88,    6,    9, 0x88, 0x88, 0x87, 0xC1,  // state 8:  Split value field value
    0xC1, 0xC1,    6, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1,  // state 9:  CR after split value field
    0xC1, 0xC1, 0xC4, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1,  // state 10: CR after header value
    };
  //}}}
  //{{{
  static inline const uint8_t kChunkState[20] = {
  //  *    LF    CR    HEX
    0xC1, 0xC1, 0xC1,    1,  // s0: initial hex char
    0xC1, 0xC1,    2, 0x81,  // s1: additional hex chars, followed by CR
    0xC1, 0x83, 0xC1, 0xC1,  // s2: trailing LF
    0xC1, 0xC1,    4, 0xC1,  // s3: CR after chunk block
    0xC1, 0xC0, 0xC1, 0xC1,  // s4: LF after chunk block
    };
  //}}}

  // vars
  eState mState = eHeader;

  eHeaderState mHeaderState = eHeaderDone;
  char* mScratch = nullptr;
  int mScratchAllocSize = 0;
  int mKeyLen = 0;
  int mValueLen = 0;

  eContentState mContentState = eContentNone;
  int mHeaderContentLength = -1;
  int mContentLengthLeft = -1;
  int mContentReceivedBytes = 0;
  uint8_t* mContent = nullptr;

  int mResponse = 0;
  cUrl mRedirectUrl;
  };
