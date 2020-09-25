// cHttp.cpp - http base class based on tinyHttp parser
#include "cHttp.h"
#include "../utils/cLog.h"

using namespace std;

constexpr int kRecvBufferSize = 1024;
constexpr int kInitialScratchSize = 256;

// public
//{{{
cHttp::cHttp() {
  mScratch = (char*)malloc (kInitialScratchSize);
  mScratchAllocSize = kInitialScratchSize;
  }
//}}}
//{{{
cHttp::~cHttp() {

  free (mContent);
  free (mScratch);
  }
//}}}

//{{{
int cHttp::get (const string& host, const string& path,
                const string& header,
                const function<void (const string& key, const string& value)>& headerCallback,
                const function<bool (const uint8_t* data, int len)>& dataCallback) {
// send http GET request to host, return response code

  clear();

  auto connectToHostResult = connectToHost (host);
  if (connectToHostResult < 0)
    return connectToHostResult;

  string request = "GET /" + path + " HTTP/1.1\r\n" +
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
string cHttp::getRedirect (const string& host, const string& path) {
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
void cHttp::freeContent() {

  free (mContent);
  mContent = nullptr;
  mContentReceivedBytes = 0;
  }
//}}}

// private
//{{{
void cHttp::clear() {

  mResponse = 0;

  mState = eHeader;
  mHeaderState = eHeader_done;

  mDataState = eDataNone;
  mHeaderContentLength = -1;
  mContentLengthLeft = -1;

  mKeyLen = 0;
  mValueLen = 0;

  freeContent();
  }
//}}}
//{{{
bool cHttp::parseChunk (int& size, char ch) {
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
    case '0': case '1':
    case '2': case '3':
    case '4': case '5':
    case '6': case '7':
    case '8': case '9':
    case 'a': case 'b':
    case 'c': case 'd':
    case 'e': case 'f':
    case 'A': case 'B':
    case 'C': case 'D':
    case 'E': case 'F': code = 3; break;
    }

  auto newstate = kChunkState [mHeaderState * 4 + code];
  mHeaderState = (eHeaderState)(newstate & 0xF);

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
cHttp::eHeaderState cHttp::parseHeaderChar (char ch) {
// Parses a single character of an HTTP header stream. The state parameter is
// used as internal state and should be initialized to zero for the first call.
// Return value is a value from the http_header enuemeration specifying
// the semantics of the character. If an error is encountered,
// http_header_done will be returned with a non-zero state parameter. On
// success http_header_done is returned with the state parameter set to zero.

  auto code = 0;
  switch (ch) {
    case '\t': code = 1; break;
    case '\n': code = 2; break;
    case '\r': code = 3; break;
    case  ' ': code = 4; break;
    case  ',': code = 5; break;
    case  ':': code = 6; break;
    }

  auto newstate = kHeaderState [mHeaderState * 8 + code];
  mHeaderState = (eHeaderState)(newstate & 0xF);

  switch (newstate) {
    case 0xC0: return eHeader_done;
    case 0xC1: return eHeader_done;
    case 0xC4: return eHeader_store_keyvalue;
    case 0x80: return eHeader_version_character;
    case 0x81: return eHeader_code_character;
    case 0x82: return eHeader_status_character;
    case 0x84: return eHeader_key_character;
    case 0x87: return eHeader_value_character;
    case 0x88: return eHeader_value_character;
    }

  return eHeader_continue;
  }
//}}}
//{{{
bool cHttp::parseRecvData (const uint8_t* data, int length, int& bytesParsed,
                           const function<void (const string& key, const string& value)>& headerCallback,
                           const function<bool (const uint8_t* data, int len)>& dataCallback) {

  auto initialLength = length;

  while (length) {
    switch (mState) {
      case eHeader:
        switch (parseHeaderChar (*data)) {
          case eHeader_code_character:
            //{{{  respone char
            mResponse = mResponse * 10 + *data - '0';
            break;
            //}}}
          case eHeader_key_character:
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
          case eHeader_value_character:
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
          case eHeader_store_keyvalue: {
            //{{{  key value
            string key = string (mScratch, size_t (mKeyLen));
            string value = string (mScratch + mKeyLen, size_t (mValueLen));
            headerCallback (key, value);
            //cLog::log (LOGINFO, "header key:" + key + " value:" + value);

            if (key == "content-length") {
              mHeaderContentLength = stoi (value);
              mContent = (uint8_t*)malloc (mHeaderContentLength);
              mContentLengthLeft = mHeaderContentLength;
              mDataState = eDataContentLength;
              cLog::log (LOGINFO, "got mHeaderContentLength:%d", mHeaderContentLength);
              }
            else if (key == "transfer-encoding")
              mDataState = value == "chunked" ? eDataChunked : eDataNone;
            else if (key == "location")
              mRedirectUrl.parse (value);

            mKeyLen = 0;
            mValueLen = 0;

            break;
            }
            //}}}
          case eHeader_done:
            //{{{  done
            if (mHeaderState != 0)
              mState = eError;
            else if (mDataState == eDataChunked) {
              mHeaderContentLength = 0;
              mContentLengthLeft = 0;
              mState = eChunkHeader;
              }
            else if (mDataState == eDataNone)
              mState = eStreamData;
            else if (mHeaderContentLength == 0)
              mState = eClose;
            else if (mHeaderContentLength > 0)
              mState = eRawData;
            else
              mState = eError;

            break;
            //}}}
          default:;
          }
        data++;
        length--;
        break;

      //{{{
      case eChunkHeader:
        //cLog::log (LOGINFO, "eHttp_chunk_header contentLen:%d left:%d",
        //                    mHeaderContentLength, mContentLengthLeft);
        if (!parseChunk (mHeaderContentLength, *data)) {
          if (mHeaderContentLength == -1)
            mState = eError;
          else if (mHeaderContentLength == 0)
            mState = eClose;
          else
            mState = eChunkData;
          }

        data++;
        length--;

        break;
      //}}}
      //{{{
      case eChunkData: {
        //cLog::log (LOGINFO, "eHttp_chunk_data len:%d mHeaderContentLength:%d left:%d mContentReceivedSize:%d",
        //                    length, mHeaderContentLength, mContentLengthLeft, mContentReceivedSize);
        int chunkSize = (length < mContentLengthLeft) ? length : mContentLengthLeft;
        cLog::log (LOGINFO, "chunked mHeaderContentLength:%d left:%d chunksize:%d mContent:%x",
                            mHeaderContentLength, mContentLengthLeft, chunkSize, mContent);
        if (!mContent) {
          mContent = (uint8_t*)malloc (mHeaderContentLength);
          memcpy (mContent + mContentReceivedBytes, data, chunkSize);
          mContentReceivedBytes += chunkSize;
          }
        else {
          cLog::log (LOGERROR, "implement more than 1 data chunk");
          }

        mContentLengthLeft -= chunkSize;
        length -= chunkSize;
        data += chunkSize;

        if (mContentLengthLeft == 0) {
          mHeaderContentLength = 1;
          mState = eChunkHeader;
          }
        }

        break;
      //}}}
      //{{{
      case eRawData: {
        //cLog::log (LOGINFO, "eHttp_raw_data len:%d mHeaderContentLength:%d left:%d mContentReceivedSize:%d",
        //                    length, mHeaderContentLength, mContentLengthLeft, mContentReceivedSize);
        int chunkSize = (length < mContentLengthLeft) ? length : mContentLengthLeft;

        if (mContent) {
          memcpy (mContent + mContentReceivedBytes, data, chunkSize);
          mContentReceivedBytes += chunkSize;
          }

        mContentLengthLeft -= chunkSize;
        length -= chunkSize;
        data += chunkSize;

        if (mContentLengthLeft == 0)
          mState = eClose;

        break;
        }
      //}}}
      //{{{
      case eStreamData: {
        //cLog::log (LOGINFO, "eHttp_stream_data len:%d", length);

        if (!dataCallback (data, length))
          mState = eClose;

        data += length;
        length = 0;

        break;
        }
      //}}}

      case eClose:
      case eError:
        break;

      default:;
      }

    if ((mState == eError) || (mState == eClose)) {
      bytesParsed = initialLength - length;
      return false;
      }
    }

  bytesParsed = initialLength - length;

  return true;
  }
//}}}
