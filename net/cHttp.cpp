// cHttp.cpp - http base class based on tinyHttp parser
#include "cHttp.h"
#include "../utils/cLog.h"

using namespace std;

constexpr static int kRecvBufferSize = 1024;
constexpr static int kInitialScratchSize = 256;

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
  mContentReceivedSize = 0;
  }
//}}}

// private
//{{{
void cHttp::clear() {

  mResponse = 0;

  mState = eHttp_header;
  mParseHeaderState = eHttp_parse_header_done;

  mChunked = false;
  mHeaderContentLength = -1;
  mHeaderContentLengthValid = false;

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
cHttp::eParseHeaderState cHttp::parseHeaderChar (char ch) {
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
bool cHttp::parseRecvData (const uint8_t* data, int length, int& bytesParsed,
                           const function<void (const string& key, const string& value)>& headerCallback,
                           const function<bool (const uint8_t* data, int len)>& dataCallback) {

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
            string key = string (mScratch, size_t (mKeyLen));
            string value = string (mScratch + mKeyLen, size_t (mValueLen));
            headerCallback (key, value);
            //cLog::log (LOGINFO, "header key:" + key + " value:" + value);

            if (key == "content-length") {
              mHeaderContentLength = stoi (value);
              mContent = (uint8_t*)malloc (mHeaderContentLength);
              mHeaderContentLengthValid = true;
              cLog::log (LOGINFO, "got mHeaderContentLength:%d", mHeaderContentLength);
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
              mHeaderContentLength = 0;
              mState = eHttp_chunk_header;
              }

            else if (!mHeaderContentLengthValid)
              mState = eHttp_stream_data;

            else if (mHeaderContentLength == 0)
              mState = eHttp_close;

            else if (mHeaderContentLength > 0)
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
        //cLog::log (LOGINFO, "eHttp_chunk_header contentLen:%d", mHeaderContentLength);
        if (!parseChunk (mHeaderContentLength, *data)) {
          if (mHeaderContentLength == -1)
            mState = eHttp_error;
          else if (mHeaderContentLength == 0)
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
        //cLog::log (LOGINFO, "eHttp_chunk_data len:%d mHeaderContentLength:%d mContentReceivedSize:%d",
        //                    length, mHeaderContentLength, mContentReceivedSize);
        int chunkSize = (length < mHeaderContentLength) ? length : mHeaderContentLength;
        cLog::log (LOGINFO, "chunked mHeaderContentLength:%d chunksize:%d mContent:%x",
                            mHeaderContentLength, chunkSize, mContent);
        if (!mContent) {
          mContent = (uint8_t*)malloc (mHeaderContentLength);
          memcpy (mContent + mContentReceivedSize, data, chunkSize);
          mContentReceivedSize += chunkSize;
          }
        else {
          cLog::log (LOGERROR, "implement more than 1 data chunk");
          }

        mHeaderContentLength -= chunkSize;
        length -= chunkSize;
        data += chunkSize;

        if (mHeaderContentLength == 0) {
          mHeaderContentLength = 1;
          mState = eHttp_chunk_header;
          }
        }

        break;
      //}}}
      //{{{
      case eHttp_raw_data: {
        //cLog::log (LOGINFO, "eHttp_raw_data len:%d mHeaderContentLength:%d mContentReceivedSize:%d",
        //                    length, mHeaderContentLength, mContentReceivedSize);
        int chunkSize = (length < mHeaderContentLength) ? length : mHeaderContentLength;

        if (mContent) {
          memcpy (mContent + mContentReceivedSize, data, chunkSize);
          mContentReceivedSize += chunkSize;
          }

        mHeaderContentLength -= chunkSize;
        length -= chunkSize;
        data += chunkSize;

        if (mHeaderContentLength == 0)
          mState = eHttp_close;

        break;
        }
      //}}}
      //{{{
      case eHttp_stream_data: {
        //cLog::log (LOGINFO, "eHttp_stream_data len:%d", length);

        if (!dataCallback (data, length))
          mState = eHttp_close;

        data += length;
        length = 0;

        break;
        }
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
