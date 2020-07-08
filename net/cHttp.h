// cHttp.h - http base class based on tinyHttp parser
#pragma once
//{{{  includes
#include <stdint.h>

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <functional>

#include "cUrl.h"
//}}}

class cHttp {
public:
  //{{{
  cHttp() {
    mScratch = (char*)malloc (kInitialScratchSize);
    mScratchAllocSize = kInitialScratchSize;
    }
  //}}}
  //{{{
  virtual ~cHttp() {

    free (mContent);
    free (mScratch);
    }
  //}}}
  virtual void initialise() = 0;

  // gets
  int getResponse() { return mResponse; }
  uint8_t* getContent() { return mContent; }
  int getContentSize() { return mContentSize; }

  //{{{
  int get (const std::string& host, const std::string& path,
           const std::string& header = "",
           const std::function<void (const std::string& key, const std::string& value)>& headerCallback = [](const std::string&, const std::string&) noexcept {},
           const std::function<bool (const uint8_t* data, int len)>& dataCallback = [](const uint8_t*, int) noexcept { return true; }) {
  // send http GET request to host, return response code

    clear();

    auto connectToHostResult = connectToHost (host);
    if (connectToHostResult < 0)
      return connectToHostResult;

    std::string request = "GET /" + path + " HTTP/1.1\r\n" +
                          "Host: " + host + "\r\n" +
                          (header.empty() ? "" : (header + "\r\n")) +
                          "\r\n";
    if (getSend (request)) {
      uint8_t buffer[kRecvBufferSize];
      bool needMoreData = true;
      while (needMoreData) {
        auto bufferPtr = buffer;
        auto bufferBytesReceived = getRecv (buffer, sizeof(buffer));
        if (bufferBytesReceived <= 0)
          break;
        //cLog::log (LOGINFO, "getAllRecv %d", bufferBytesReceived);

        while (needMoreData && (bufferBytesReceived > 0)) {
          int bytesReceived;
          needMoreData = parseRecvData (bufferPtr, bufferBytesReceived, bytesReceived,
                                        headerCallback, dataCallback);
          bufferBytesReceived -= bytesReceived;
          bufferPtr += bytesReceived;
          }
        }
      }

    return mResponse;
    }
  //}}}
  //{{{
  std::string getRedirect (const std::string& host, const std::string& path) {
  // return redirected host if any

    auto response = get (host, path);
    if (response == 302) {
      cLog::log (LOGINFO, "getRedirect host " +  mRedirectUrl.getHost());
      response = get (mRedirectUrl.getHost(), path);
      if (response == 200)
        return mRedirectUrl.getHost();
      else
        cLog::log (LOGERROR, "cHttp - redirect - get error");
      }

    return host;
    }
  //}}}
  //{{{
  void freeContent() {
    free (mContent);
    mContent = nullptr;
    mContentSize = 0;
    }
  //}}}

protected:
  virtual int connectToHost (const std::string& host) = 0;
  virtual bool getSend (const std::string& sendStr) = 0;
  virtual int getRecv (uint8_t* buffer, int bufferSize) { return 0; }

private:
  constexpr static int kRecvBufferSize = 1024;
  constexpr static int kInitialScratchSize = 256;
  //{{{
  enum eState {
    eHttp_header,
    eHttp_chunk_header,
    eHttp_chunk_data,
    eHttp_raw_data,
    eHttp_stream_data,
    eHttp_close,
    eHttp_error,
    };
  //}}}
  //{{{
  enum eParseHeaderState {
    eHttp_parse_header_done,
    eHttp_parse_header_continue,
    eHttp_parse_header_version_character,
    eHttp_parse_header_code_character,
    eHttp_parse_header_status_character,
    eHttp_parse_header_key_character,
    eHttp_parse_header_value_character,
    eHttp_parse_header_store_keyvalue
    };
  //}}}

  //{{{
  const uint8_t kHeaderState[88] = {
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
  const uint8_t kChunkState[20] = {
  //  *    LF    CR    HEX
    0xC1, 0xC1, 0xC1,    1,  // s0: initial hex char
    0xC1, 0xC1,    2, 0x81,  // s1: additional hex chars, followed by CR
    0xC1, 0x83, 0xC1, 0xC1,  // s2: trailing LF
    0xC1, 0xC1,    4, 0xC1,  // s3: CR after chunk block
    0xC1, 0xC0, 0xC1, 0xC1,  // s4: LF after chunk block
    };
  //}}}

  //{{{
  void clear() {

    mResponse = 0;

    mState = eHttp_header;
    mParseHeaderState = eHttp_parse_header_done;

    mChunked = false;
    mContentLen = -1;
    mContentLenValid = false;

    mKeyLen = 0;
    mValueLen = 0;

    freeContent();
    }
  //}}}
  //{{{
  bool parseChunk (int& size, char ch) {
  // Parses the size out of a chunk-encoded HTTP response. Returns non-zero if it
  // needs more data. Retuns zero success or error. When error: size == -1 On
  // success, size = size of following chunk data excluding trailing \r\n. User is
  // expected to process or otherwise seek past chunk data up to the trailing \r\n.
  // The state parameter is used for internal state and should be
  // initialized to zero the first call.

    auto code = 0;
    switch (ch) {
      case '\n': code = 1; break;
      case '\r': code = 2; break;
      case '0': case '1': case '2': case '3':
      case '4': case '5': case '6': case '7':
      case '8': case '9': case 'a': case 'b':
      case 'c': case 'd': case 'e': case 'f':
      case 'A': case 'B': case 'C': case 'D':
      case 'E': case 'F': code = 3; break;
      }

    auto newstate = kChunkState [mParseHeaderState * 4 + code];
    mParseHeaderState = (eParseHeaderState)(newstate & 0xF);

    switch (newstate) {
      case 0xC0:
        return size != 0;

      case 0xC1: /* error */
        size = -1;
        return false;

      case 0x01: /* initial char */
        size = 0;
        /* fallthrough */
      case 0x81: /* size char */
        if (ch >= 'a')
          size = size * 16 + (ch - 'a' + 10);
        else if (ch >= 'A')
          size = size * 16 + (ch - 'A' + 10);
        else
          size = size * 16 + (ch - '0');
        break;

      case 0x83:
        return size == 0;
      }

    return true;
    }
  //}}}
  //{{{
  eParseHeaderState parseHeaderChar (char ch) {
  // Parses a single character of an HTTP header stream. The state parameter is
  // used as internal state and should be initialized to zero for the first call.
  // Return value is a value from the http_parse_header enuemeration specifying
  // the semantics of the character. If an error is encountered,
  // http_parse_header_done will be returned with a non-zero state parameter. On
  // success http_parse_header_done is returned with the state parameter set to zero.

    auto code = 0;
    switch (ch) {
      case '\t': code = 1; break;
      case '\n': code = 2; break;
      case '\r': code = 3; break;
      case  ' ': code = 4; break;
      case  ',': code = 5; break;
      case  ':': code = 6; break;
      }

    auto newstate = kHeaderState [mParseHeaderState * 8 + code];
    mParseHeaderState = (eParseHeaderState)(newstate & 0xF);

    switch (newstate) {
      case 0xC0: return eHttp_parse_header_done;
      case 0xC1: return eHttp_parse_header_done;
      case 0xC4: return eHttp_parse_header_store_keyvalue;
      case 0x80: return eHttp_parse_header_version_character;
      case 0x81: return eHttp_parse_header_code_character;
      case 0x82: return eHttp_parse_header_status_character;
      case 0x84: return eHttp_parse_header_key_character;
      case 0x87: return eHttp_parse_header_value_character;
      case 0x88: return eHttp_parse_header_value_character;
      }

    return eHttp_parse_header_continue;
    }
  //}}}
  //{{{
  bool parseRecvData (const uint8_t* data, int length, int& bytesParsed,
                      const std::function<void (const std::string& key, const std::string& value)>& headerCallback,
                      const std::function<bool (const uint8_t* data, int len)>& dataCallback) {

    auto initialLength = length;

    while (length) {
      switch (mState) {
        case eHttp_header:
          switch (parseHeaderChar (*data)) {
            case eHttp_parse_header_code_character:
              //{{{  respone char
              mResponse = mResponse * 10 + *data - '0';
              break;
              //}}}
            case eHttp_parse_header_key_character:
              //{{{  key char
              if (mKeyLen >= mScratchAllocSize) {
                mScratchAllocSize *= 2;
                mScratch = (char*)realloc (mScratch, mScratchAllocSize);
                cLog::log (LOGINFO, "mScratch key realloc %d %d", mKeyLen, mScratchAllocSize);
                }

              mScratch [mKeyLen] = tolower (*data);
              mKeyLen++;

              break;
              //}}}
            case eHttp_parse_header_value_character:
              //{{{  value char
              if (mKeyLen + mValueLen >= mScratchAllocSize) {
                mScratchAllocSize *= 2;
                mScratch = (char*)realloc (mScratch, mScratchAllocSize);
                cLog::log (LOGINFO, "mScratch value realloc %d %d", mKeyLen + mValueLen, mScratchAllocSize);
                }

              mScratch [mKeyLen + mValueLen] = *data;
              mValueLen++;

              break;
              //}}}
            case eHttp_parse_header_store_keyvalue: {
              //{{{  key value
              std::string key = std::string (mScratch, size_t (mKeyLen));
              std::string value = std::string (mScratch + mKeyLen, size_t (mValueLen));
              headerCallback (key, value);
              //cLog::log (LOGINFO, "header key:" + key + " value:" + value);

              if (key == "content-length") {
                mContentLen = stoi (value);
                mContent = (uint8_t*)malloc (mContentLen);
                mContentLenValid = true;
                }
              else if (key == "location")
                mRedirectUrl.parse (value);
              else if (key == "transfer-encoding")
                mChunked = value == "chunked";

              mKeyLen = 0;
              mValueLen = 0;

              break;
              }
              //}}}
            case eHttp_parse_header_done:
              //{{{  done
              if (mParseHeaderState != 0)
                mState = eHttp_error;

              else if (mChunked) {
                mContentLen = 0;
                mState = eHttp_chunk_header;
                }

              else if (!mContentLenValid)
                mState = eHttp_stream_data;

              else if (mContentLen == 0)
                mState = eHttp_close;

              else if (mContentLen > 0)
                mState = eHttp_raw_data;

              else
                mState = eHttp_error;

              break;
              //}}}
            default:;
            }
          data++;
          length--;
          break;

        //{{{
        case eHttp_chunk_header:
          cLog::log (LOGINFO, "eHttp_chunk_header contentLen:%d", mContentLen);

          if (!parseChunk (mContentLen, *data)) {
            if (mContentLen == -1)
              mState = eHttp_error;
            else if (mContentLen == 0)
              mState = eHttp_close;
            else
              mState = eHttp_chunk_data;
            }

          data++;
          length--;

          break;
        //}}}
        //{{{
        case eHttp_chunk_data: {
          cLog::log (LOGINFO, "eHttp_chunk_data len:%d contentLen:%d contentSize:%d", length, mContentLen, mContentSize);

          int chunkSize = (length < mContentLen) ? length : mContentLen;

          cLog::log (LOGINFO, "chunked data %d chunksize %d", mContentLen, chunkSize);
          if (!mContent) {
            mContent = (uint8_t*)malloc (mContentLen);
            memcpy (mContent + mContentSize, data, chunkSize);
            mContentSize += chunkSize;
            }
          else
            cLog::log (LOGERROR, "implement more than 1 data chunk");

          mContentLen -= chunkSize;
          length -= chunkSize;
          data += chunkSize;

          if (mContentLen == 0) {
            mContentLen = 1;
            mState = eHttp_chunk_header;
            }
          }

          break;
        //}}}

        //{{{
        case eHttp_raw_data: {
          //cLog::log (LOGINFO, "eHttp_raw_data len:%d contentLen:%d contentSize:%d", length, mContentLen, mContentSize);
          int chunkSize = (length < mContentLen) ? length : mContentLen;

          if (mContent) {
            memcpy (mContent + mContentSize, data, chunkSize);
            mContentSize += chunkSize;
            }

          mContentLen -= chunkSize;
          length -= chunkSize;
          data += chunkSize;

          if (mContentLen == 0)
            mState = eHttp_close;
          }

          break;
        //}}}
        //{{{
        case eHttp_stream_data: {
          //cLog::log (LOGINFO, "eHttp_stream_data len:%d", length);
          if (!dataCallback (data, length))
            mState = eHttp_close;

          //int chunkSize = (length < mContentLen) ? length : mContentLen;
          //memcpy (mContent + mContentSize, data, chunkSize);
          //mContentSize += chunkSize;
          //mContentLen -= chunkSize;

          data += length;
          length = 0;
          }

          break;
        //}}}

        case eHttp_close:
        case eHttp_error:
          break;

        default:;
        }

      if ((mState == eHttp_error) || (mState == eHttp_close)) {
        bytesParsed = initialLength - length;
        return false;
        }
      }

    bytesParsed = initialLength - length;

    return true;
    }
  //}}}

  //{{{  private vars
  int mResponse = 0;

  eState mState = eHttp_header;
  eParseHeaderState mParseHeaderState = eHttp_parse_header_done;

  bool mChunked = 0;
  bool mContentLenValid = false;
  int mContentLen = -1;
  uint8_t* mContent = nullptr;
  int mContentSize = 0;

  char* mScratch;
  int mScratchAllocSize = 0;
  int mKeyLen = 0;
  int mValueLen = 0;

  cUrl mRedirectUrl;
  //}}}
  };
