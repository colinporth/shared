// cHttp.cpp - http base class based on tinyHttp parser
#include "cHttp.h"
#include "../utils/cLog.h"

using namespace std;

constexpr int kInitialHeaderBufferSize = 256;
constexpr int kRecvBufferSize = 1024;

// public
//{{{
cHttp::cHttp() {
  mHeaderBuffer = (char*)malloc (kInitialHeaderBufferSize);
  mHeaderBufferAllocSize = kInitialHeaderBufferSize;
  }
//}}}
//{{{
cHttp::~cHttp() {

  free (mContent);
  free (mHeaderBuffer);
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
        needMoreData = parseData (bufferPtr, bufferBytesReceived, bytesReceived, headerCallback, dataCallback);
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

  mHeaderContentLength = -1;
  mContentLengthLeft = 0;
  mContentReceivedBytes = 0;

  mContentState = eContentNone;
  }
//}}}

// private
//{{{
void cHttp::clear() {

  mState = eHeader;

  mHeaderState = eHeaderDone;
  mKeyLen = 0;
  mValueLen = 0;

  freeContent();

  mResponse = 0;
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
    case 0xC0: return eHeaderDone;
    case 0xC1: return eHeaderDone;
    case 0xC4: return eHeaderStoreKeyValue;
    case 0x80: return eHeaderVersionCharacter;
    case 0x81: return eHeaderCodeCharacter;
    case 0x82: return eHeaderStatusCharacter;
    case 0x84: return eHeaderKeyCharacter;
    case 0x87: return eHeaderValueCharacter;
    case 0x88: return eHeaderValueCharacter;
    }

  return eHeaderContinue;
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
bool cHttp::parseData (const uint8_t* data, int length, int& bytesParsed,
                       const function<void (const string& key, const string& value)>& headerCallback,
                       const function<bool (const uint8_t* data, int len)>& dataCallback) {

  int initialLength = length;

  while (length) {
    switch (mState) {
      case eHeader:
        switch (parseHeaderChar (*data)) {
          case eHeaderCodeCharacter:
            //{{{  response char
            mResponse = mResponse * 10 + *data - '0';
            break;
            //}}}
          case eHeaderKeyCharacter:
            //{{{  key char
            if (mKeyLen >= mHeaderBufferAllocSize) {
              mHeaderBufferAllocSize *= 2;
              mHeaderBuffer = (char*)realloc (mHeaderBuffer, mHeaderBufferAllocSize);
              cLog::log (LOGINFO, "mHeaderBuffer key realloc %d %d", mKeyLen, mHeaderBufferAllocSize);
              }

            mHeaderBuffer [mKeyLen] = tolower (*data);
            mKeyLen++;

            break;
            //}}}
          case eHeaderValueCharacter:
            //{{{  value char
            if (mKeyLen + mValueLen >= mHeaderBufferAllocSize) {
              mHeaderBufferAllocSize *= 2;
              mHeaderBuffer = (char*)realloc (mHeaderBuffer, mHeaderBufferAllocSize);
              cLog::log (LOGINFO, "mHeaderBuffer value realloc %d %d", mKeyLen + mValueLen, mHeaderBufferAllocSize);
              }

            mHeaderBuffer [mKeyLen + mValueLen] = *data;
            mValueLen++;

            break;
            //}}}
          case eHeaderStoreKeyValue: {
            //{{{  key value
            string key = string (mHeaderBuffer, size_t (mKeyLen));
            string value = string (mHeaderBuffer + mKeyLen, size_t (mValueLen));
            headerCallback (key, value);

            //cLog::log (LOGINFO, "header key:" + key + " value:" + value);
            if (key == "content-length") {
              mHeaderContentLength = stoi (value);
              mContent = (uint8_t*)malloc (mHeaderContentLength);
              mContentLengthLeft = mHeaderContentLength;
              mContentState = eContentLength;
              //cLog::log (LOGINFO, "got mHeaderContentLength:%d", mHeaderContentLength);
              }

            else if (key == "transfer-encoding")
              mContentState = value == "chunked" ? eContentChunked : eContentNone;

            else if (key == "location")
              mRedirectUrl.parse (value);

            mKeyLen = 0;
            mValueLen = 0;

            break;
            }
            //}}}
          case eHeaderDone:
            //{{{  done
            if (mHeaderState != eHeaderDone)
              mState = eError;

            else if (mContentState == eContentChunked) {
              mState = eChunkHeader;
              mHeaderContentLength = 0;
              mContentLengthLeft = 0;
              mContentReceivedBytes = 0;
              }

            else if (mContentState == eContentNone)
              mState = eStreamData;

            // mContentState == eContentLength
            else if (mHeaderContentLength > 0)
              mState = eExpectedData;
            else if (mHeaderContentLength == 0)
              mState = eClose;

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
      case eExpectedData: {
        // declared and allocated in content-length header
        //cLog::log (LOGINFO, "eExpectedData - length:%d mHeaderContentLength:%d left:%d mContentReceivedSize:%d",
        //                    length, mHeaderContentLength, mContentLengthLeft, mContentReceivedBytes);
        int chunkSize = (length <= mContentLengthLeft) ? length : mContentLengthLeft;
        if (length > mContentLengthLeft)
          cLog::log (LOGERROR, "eExpectedData - too large %d %d", length, mContentLengthLeft);

        if (dataCallback (data, length)) {
          // data accepted by callback
          if (mContent) {
            memcpy (mContent + mContentReceivedBytes, data, chunkSize);
            mContentReceivedBytes += chunkSize;

            length -= chunkSize;
            data += chunkSize;
            mContentLengthLeft -= chunkSize;
            if (mContentLengthLeft <= 0)
              mState = eClose;
            }
          else {
            cLog::log (LOGERROR, "eExpectedData - content not allocated");
            mState = eClose;
            }
          }
        else // data not accepted by callback, bomb out
          mState = eClose;

        break;
        }
      //}}}
      //{{{
      case eChunkHeader:
        //cLog::log (LOGINFO, "eHttp_chunk_header contentLen:%d left:%d", mHeaderContentLength, mContentLengthLeft);
        if (!parseChunk (mHeaderContentLength, *data)) {
          if (mHeaderContentLength == -1)
            mState = eError;
          else if (mHeaderContentLength == 0)
            mState = eClose;
          else {
            mState = eChunkData;
            mContentLengthLeft = mHeaderContentLength;
            }
          }

        data++;
        length--;

        break;
      //}}}
      //{{{
      case eChunkData: {

        int chunkSize = (length < mContentLengthLeft) ? length : mContentLengthLeft;

        if (dataCallback (data, length)) {
          //log (LOGINFO, "eChunkData - mHeaderContentLength:%d left:%d chunksize:%d mContent:%x",
          //                    mHeaderContentLength, mContentLengthLeft, chunkSize, mContent);
          mContent = (uint8_t*)realloc (mContent, mContentReceivedBytes + chunkSize);
          memcpy (mContent + mContentReceivedBytes, data, chunkSize);
          mContentReceivedBytes += chunkSize;

          length -= chunkSize;
          data += chunkSize;
          mContentLengthLeft -= chunkSize;
          if (mContentLengthLeft <= 0) {
            // finished chunk, get ready for next chunk
            mHeaderContentLength = 1;
            mState = eChunkHeader;
            }
          }
        else // refused data in callback, bomb out early ???
          mState = eClose;

        break;
        }
      //}}}
      //{{{
      case eStreamData: {
        //cLog::log (LOGINFO, "eStreamData - length:%d", length);
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
