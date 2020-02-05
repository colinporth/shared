// cHttp.h - http base class based on tinyHttp parser
#pragma once
//{{{  includes
#include <stdint.h>

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <functional>
//}}}

//{{{
class cUrl {
public:
  //{{{
  cUrl() : scheme(nullptr), host(nullptr), path(nullptr), port(nullptr),
                 username(nullptr), password(nullptr), query(nullptr), fragment(nullptr) {}
  //}}}
  //{{{
  ~cUrl() {
    smallFree (scheme);
    smallFree(host);
    smallFree(port);
    smallFree(query);
    smallFree(fragment);
    smallFree(username);
    smallFree(password);
    }
  //}}}

  //{{{
  void parse (const char* url, int urlLen) {
  // parseUrl, see RFC 1738, 3986

    auto curstr = url;
    //{{{  parse scheme
    // <scheme>:<scheme-specific-part>
    // <scheme> := [a-z\+\-\.]+
    //             upper case = lower case for resiliency
    const char* tmpstr = strchr (curstr, ':');
    if (!tmpstr)
      return;
    auto len = tmpstr - curstr;

    // Check restrictions
    for (auto i = 0; i < len; i++)
      if (!isalpha (curstr[i]) && ('+' != curstr[i]) && ('-' != curstr[i]) && ('.' != curstr[i]))
        return;

    // Copy the scheme to the storage
    scheme = (char*)malloc (len+1);
    strncpy_s (scheme, len+1, curstr, len);
    scheme[len] = '\0';

    // Make the character to lower if it is upper case.
    for (auto i = 0; i < len; i++)
      scheme[i] = tolower (scheme[i]);
    //}}}

    // skip ':'
    tmpstr++;
    curstr = tmpstr;
    //{{{  parse user, password
    // <user>:<password>@<host>:<port>/<url-path>
    // Any ":", "@" and "/" must be encoded.
    // Eat "//" */
    for (auto i = 0; i < 2; i++ ) {
      if ('/' != *curstr )
        return;
      curstr++;
      }

    // Check if the user (and password) are specified
    auto userpass_flag = 0;
    tmpstr = curstr;
    while (tmpstr < url + urlLen) {
      if ('@' == *tmpstr) {
        // Username and password are specified
        userpass_flag = 1;
       break;
        }
      else if ('/' == *tmpstr) {
        // End of <host>:<port> specification
        userpass_flag = 0;
        break;
        }
      tmpstr++;
      }

    // User and password specification
    tmpstr = curstr;
    if (userpass_flag) {
      //{{{  Read username
      while ((tmpstr < url + urlLen) && (':' != *tmpstr) && ('@' != *tmpstr))
         tmpstr++;

      len = tmpstr - curstr;
      username = (char*)malloc(len+1);
      strncpy_s (username, len+1, curstr, len);
      username[len] = '\0';
      //}}}
      // Proceed current pointer
      curstr = tmpstr;
      if (':' == *curstr) {
        // Skip ':'
        curstr++;
        //{{{  Read password
        tmpstr = curstr;
        while ((tmpstr < url + urlLen) && ('@' != *tmpstr))
          tmpstr++;

        len = tmpstr - curstr;
        password = (char*)malloc(len+1);
        strncpy_s (password, len+1, curstr, len);
        password[len] = '\0';
        curstr = tmpstr;
        }
        //}}}

      // Skip '@'
      if ('@' != *curstr)
        return;
      curstr++;
      }
    //}}}

    auto bracket_flag = ('[' == *curstr) ? 1 : 0;
    //{{{  parse host
    tmpstr = curstr;
    while (tmpstr < url + urlLen) {
      if (bracket_flag && ']' == *tmpstr) {
        // End of IPv6 address
        tmpstr++;
        break;
        }
      else if (!bracket_flag && (':' == *tmpstr || '/' == *tmpstr))
        // Port number is specified
        break;
      tmpstr++;
      }

    len = tmpstr - curstr;
    host = (char*)malloc(len+1);
    strncpy_s (host, len+1, curstr, len);
    host[len] = '\0';
    curstr = tmpstr;
    //}}}
    //{{{  parse port number
    if (':' == *curstr) {
      curstr++;

      // Read port number
      tmpstr = curstr;
      while ((tmpstr < url + urlLen) && ('/' != *tmpstr))
        tmpstr++;

      len = tmpstr - curstr;
      port = (char*)malloc(len+1);
      strncpy_s (port, len+1, curstr, len);
      port[len] = '\0';
      curstr = tmpstr;
      }
    //}}}

    // end of string ?
    if (curstr >= url + urlLen)
      return;

    //{{{  Skip '/'
    if ('/' != *curstr)
      return;

    curstr++;
    //}}}
    //{{{  Parse path
    tmpstr = curstr;
    while ((tmpstr < url + urlLen) && ('#' != *tmpstr) && ('?' != *tmpstr))
      tmpstr++;

    len = tmpstr - curstr;
    path = (char*)malloc(len+1);
    strncpy_s (path, len+1, curstr, len);
    path[len] = '\0';
    curstr = tmpstr;
    //}}}
    //{{{  parse query
    if ('?' == *curstr) {
      // skip '?'
      curstr++;

      /* Read query */
      tmpstr = curstr;
      while ((tmpstr < url + urlLen) && ('#' != *tmpstr))
        tmpstr++;
      len = tmpstr - curstr;

      query = (char*)malloc(len+1);
      strncpy_s (query, len+1, curstr, len);
      query[len] = '\0';
      curstr = tmpstr;
      }
    //}}}
    //{{{  parse fragment
    if ('#' == *curstr) {
      // Skip '#'
      curstr++;

      /* Read fragment */
      tmpstr = curstr;
      while (tmpstr < url + urlLen)
        tmpstr++;
      len = tmpstr - curstr;

      fragment = (char*)malloc(len+1);
      strncpy_s (fragment, len+1, curstr, len);
      fragment[len] = '\0';

      curstr = tmpstr;
      }
    //}}}
    }
  //}}}

  // vars
  char* scheme;    // mandatory
  char* host;      // mandatory
  char* path;      // optional
  char* port;      // optional
  char* username;  // optional
  char* password;  // optional
  char* query;     // optional
  char* fragment;  // optional
  };
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

    if (mRedirectUrl)
      delete mRedirectUrl;
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
           const std::function<void (const std::string&, const std::string&)>& headerCallback = [](const std::string&, const std::string&) noexcept {},
           const std::function<void (const uint8_t*, int)>& dataCallback = [](const uint8_t*, int) noexcept {}) {
  // send http GET request to host, return response code

    clear();

    auto connectToHostResult = connectToHost (host);
    if (connectToHostResult < 0)
      return connectToHostResult;

    if (getSend ("GET /" + path + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 (header.empty() ? "" : (header + "\r\n")) +
                 "\r\n")) {

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
      cLog::log (LOGINFO, "getRedirect host %s", mRedirectUrl->host);
      response = get (mRedirectUrl->host, path);
      if (response == 200)
        return mRedirectUrl->host;
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

    mKeyLen = 0;
    mValueLen = 0;
    mContentLen = -1;

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
                      const std::function<void (const std::string&, const std::string&)>& headerCallback,
                      const std::function<void (const uint8_t*, int)>& dataCallback) {

    auto initialLength = length;

    while (length) {
      switch (mState) {
        case eHttp_header:
          switch (parseHeaderChar (*data)) {
            case eHttp_parse_header_code_character:
              //{{{  code char
              mResponse = mResponse * 10 + *data - '0';
              break;
              //}}}
            case eHttp_parse_header_done:
              //{{{  code done
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
            case eHttp_parse_header_key_character:
              //{{{  header key char
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
              //{{{  header value char
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
              //{{{  key value done
              std::string key = std::string (mScratch, size_t (mKeyLen));
              std::string value = std::string (mScratch + mKeyLen, size_t (mValueLen));
              headerCallback (key, value);

              //cLog::log (LOGINFO, "header key:" + key + " value:" + value);

              if ((mKeyLen == 17) && (strncmp (mScratch, "transfer-encoding", mKeyLen) == 0))
                // transfer-encoding chunked
                mChunked = (mValueLen == 7) && (strncmp (mScratch + mKeyLen, "chunked", mValueLen) == 0);
              else if ((mKeyLen == 14) && (strncmp (mScratch, "content-length", mKeyLen) == 0)) {
                // content-length len
                mContentLen = 0;
                for (int ii = mKeyLen, end = mKeyLen + mValueLen; ii != end; ++ii)
                  mContentLen = mContentLen * 10 + mScratch[ii] - '0';
                mContent = (uint8_t*)malloc (mContentLen);
                mContentLenValid = true;
                }
              else if ((mKeyLen == 8) && (strncmp (mScratch, "location", mKeyLen) == 0)) {
                // location url redirect
                if (!mRedirectUrl)
                  mRedirectUrl = new cUrl();
                mRedirectUrl->parse (mScratch + mKeyLen, mValueLen);
                }

              mKeyLen = 0;
              mValueLen = 0;

              break;
              }
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
          dataCallback (data, length);

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

  int mKeyLen = 0;
  int mValueLen = 0;

  char* mScratch;
  int mScratchAllocSize = 0;

  cUrl* mRedirectUrl = nullptr;

  uint8_t* mContent = nullptr;
  int mContentSize = 0;
  //}}}
  };
