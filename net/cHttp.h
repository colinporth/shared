// cHttp.h - http base class based on tinyHttp parser
#pragma once
//{{{  includes
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
//}}}

class cHttp {
public:
  //{{{
  cHttp() {
    mScratch = (char*)bigMalloc (kMaxScratch, "httpScratch");
    }
  //}}}
  //{{{
  virtual ~cHttp() {

    bigFree (mContent);
    bigFree (mScratch);

    if (mRedirectUrl)
      delete mRedirectUrl;
    }
  //}}}
  //{{{  new, delete
  void* operator new (std::size_t size) { return bigMalloc (size, "cHttp"); }
  void operator delete (void* ptr) { bigFree (ptr); }
  //}}}
  virtual void initialise() = 0;

  // gets
  int getResponse() { return mResponse; }
  uint8_t* getContent() { return mContent; }
  int getContentSize() { return mContentSize; }
  std::string getLastHost() { return mLastHost; }
  std::string getLastPath() { return mLastPath; }

  //{{{
  int get (std::string host, std::string path) {
  // send http GET request to host, return response code

    mResponse = 0;

    mState = http_header;
    mParseHeaderState = http_parse_header_done;
    mChunked = 0;

    mKeyStrLen = 0;
    mValueStrLen = 0;
    mContentLen = -1;

    freeContent();

    auto connectToHostResult = connectToHost (host);
    if (connectToHostResult < 0)
      return connectToHostResult;

    if (!getSend ("GET /" + path + " HTTP/1.1\r\nHost: " + host + "\r\n\r\n"))
      return 0;

    getAllRecv();

    mLastPath = path;
    return mResponse;
    }
  //}}}
  //{{{
  std::string getRedirectable (std::string host, std::string path) {

    auto response = get (host, path);
    if (response == 302) {
      response = get (mRedirectUrl->host, path);
      if (response != 200)
        printf ("cHttp - redirect error\n");
      return response == 200 ? mRedirectUrl->host : host;
      }

    return host;
    }
  //}}}
  //{{{
  void freeContent() {
    bigFree (mContent);
    mContent = nullptr;
    mContentSize = 0;
    }
  //}}}

protected:
  virtual int connectToHost (std::string host) = 0;
  virtual bool getSend (std::string sendStr) = 0;
  //{{{
  virtual int getAllRecv() {

    auto buffer = (uint8_t*)bigMalloc (kRecvBuffer, "getBuf");

    auto needMoreData = true;
    while (needMoreData) {
      auto bufferPtr = buffer;
      int bufferBytesReceived = getRecv (buffer, kRecvBuffer);
      if (bufferBytesReceived <= 0) {
        bigFree (buffer);
        return bufferBytesReceived;
        }

      while (needMoreData && (bufferBytesReceived > 0)) {
        int bytesReceived;
        needMoreData = parseRecvData (bufferPtr, bufferBytesReceived, bytesReceived);
        bufferBytesReceived -= bytesReceived;
        bufferPtr += bytesReceived;
        }
      }

    bigFree (buffer);
    return 0;
    }
  //}}}
  virtual int getRecv (uint8_t* buffer, int bufferSize) { return 0; }

  //{{{
  bool parseRecvData (const uint8_t* data, int length, int& read) {

    auto initialLength = length;
    while (length) {
      switch (mState) {
        case http_header:
          switch (parseHeaderChar (*data)) {
            case http_parse_header_code_character:
              //{{{  code char
              mResponse = mResponse * 10 + *data - '0';
              break;
              //}}}
            case http_parse_header_done:
              //{{{  code done
              if (mParseHeaderState != 0)
                mState = http_error;

              else if (mChunked) {
                mContentLen = 0;
                mState = http_chunk_header;
                }

              else if (mContentLen == 0)
                mState = http_close;

              else if (mContentLen > 0)
                mState = http_raw_data;

              else
                mState = http_error;

              break;
              //}}}
            case http_parse_header_key_character:
              //{{{  header key char
              mScratch [mKeyStrLen] = tolower (*data);
              if (mKeyStrLen >= kMaxScratch)
                debug ("mScratch header key overflow " + dec (mKeyStrLen) + " > " + dec (kMaxScratch));
              else
                mKeyStrLen++;
              break;
              //}}}
            case http_parse_header_value_character:
              //{{{  header value char
              mScratch [mKeyStrLen + mValueStrLen] = *data;
              if (mKeyStrLen + mValueStrLen >= kMaxScratch)
                debug ("mScratch header value overflow " + dec(mKeyStrLen + mValueStrLen) + " > " + dec (kMaxScratch));
              else
                mValueStrLen++;
              break;
              //}}}
            case http_parse_header_store_keyvalue: {
              //{{{  key value done
              if ((mKeyStrLen == 17) && (strncmp (mScratch, "transfer-encoding", mKeyStrLen) == 0)) {
                mChunked = (mValueStrLen == 7) && (strncmp (mScratch + mKeyStrLen, "chunked", mValueStrLen) == 0);
                }
              else if ((mKeyStrLen == 14) && (strncmp (mScratch, "content-length", mKeyStrLen) == 0)) {
                mContentLen = 0;
                for (int ii = mKeyStrLen, end = mKeyStrLen + mValueStrLen; ii != end; ++ii)
                  mContentLen = mContentLen * 10 + mScratch[ii] - '0';
                mContent = (uint8_t*)bigMalloc (mContentLen, "httpContent");
                }

              else if ((mKeyStrLen == 8) && (strncmp (mScratch, "location", mKeyStrLen) == 0)) {
                if (!mRedirectUrl)
                  mRedirectUrl = new cUrl();
                mRedirectUrl->parse (mScratch + mKeyStrLen, mValueStrLen);
                }

              mKeyStrLen = 0;
              mValueStrLen = 0;
              break;
              }
              //}}}
            default:;
            }
          --length;
          ++data;
          break;
        //{{{
        case http_raw_data: {
          int chunksize = (length < mContentLen) ? length : mContentLen;

          if (mContent) {
            memcpy (mContent + mContentSize, data, chunksize);
            mContentSize += chunksize;
            }
          mContentLen -= chunksize;
          length -= chunksize;
          data += chunksize;

          if (mContentLen == 0)
            mState = http_close;
          }
          break;
        //}}}
        //{{{
        case http_chunk_header:
          if (!parseChunked (mContentLen, *data)) {
            if (mContentLen == -1)
              mState = http_error;
            else if (mContentLen == 0)
              mState = http_close;
            else
              mState = http_chunk_data;
            }

          --length;
          ++data;
          break;
        //}}}
        //{{{
        case http_chunk_data: {
          int chunksize = (length < mContentLen) ? length : mContentLen;

          if (!mContent) {
            mContent = (uint8_t*)bigMalloc (mContentLen, "httpContent");
            memcpy (mContent + mContentSize, data, chunksize);
            mContentSize += chunksize;
            }
          else
            printf ("implement more than 1 data chunk\n");

          mContentLen -= chunksize;
          length -= chunksize;
          data += chunksize;

          if (mContentLen == 0) {
            mContentLen = 1;
            mState = http_chunk_header;
            }
          }
          break;
        //}}}
        case http_close:
        case http_error:
          break;
        default:;
        }

      if (mState == http_error || mState == http_close) {
        read = initialLength - length;
        return false;
        }
      }

    read = initialLength - length;
    return true;
    }
  //}}}

  std::string mLastHost;
  std::string mLastPath;

private:
  const int kMaxScratch = 600;
  const int kRecvBuffer = 220000;
  //{{{
  enum eState {
    http_header,
    http_chunk_header,
    http_chunk_data,
    http_raw_data,
    http_close,
    http_error,
    };
  //}}}
  //{{{
  enum eParseHeaderState {
    http_parse_header_done,
    http_parse_header_continue,
    http_parse_header_version_character,
    http_parse_header_code_character,
    http_parse_header_status_character,
    http_parse_header_key_character,
    http_parse_header_value_character,
    http_parse_header_store_keyvalue
    };
  //}}}
  //{{{
  const uint8_t kHeaderState[88] = {
  //  *    \t    \n   \r    ' '     ,     :   PAD
    0x80,    1, 0xC1, 0xC1,    1, 0x80, 0x80, 0xC1, // state 0: HTTP version
    0x81,    2, 0xC1, 0xC1,    2,    1,    1, 0xC1, // state 1: Response code
    0x82, 0x82,    4,    3, 0x82, 0x82, 0x82, 0xC1, // state 2: Response reason
    0xC1, 0xC1,    4, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, // state 3: HTTP version newline
    0x84, 0xC1, 0xC0,    5, 0xC1, 0xC1,    6, 0xC1, // state 4: Start of header field
    0xC1, 0xC1, 0xC0, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, // state 5: Last CR before end of header
    0x87,    6, 0xC1, 0xC1,    6, 0x87, 0x87, 0xC1, // state 6: leading whitespace before header value
    0x87, 0x87, 0xC4,   10, 0x87, 0x88, 0x87, 0xC1, // state 7: header field value
    0x87, 0x88,    6,    9, 0x88, 0x88, 0x87, 0xC1, // state 8: Split value field value
    0xC1, 0xC1,    6, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, // state 9: CR after split value field
    0xC1, 0xC1, 0xC4, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, // state 10:CR after header value
    };
  //}}}
  //{{{
  const uint8_t kChunkState[20] = {
  //  *    LF    CR    HEX
    0xC1, 0xC1, 0xC1,    1, // s0: initial hex char
    0xC1, 0xC1,    2, 0x81, // s1: additional hex chars, followed by CR
    0xC1, 0x83, 0xC1, 0xC1, // s2: trailing LF
    0xC1, 0xC1,    4, 0xC1, // s3: CR after chunk block
    0xC1, 0xC0, 0xC1, 0xC1, // s4: LF after chunk block
    };
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
      scheme = (char*)smallMalloc (len+1, "url");
      strncpy (scheme, curstr, len);
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
        username = (char*)smallMalloc(len+1, "url");
        strncpy (username, curstr, len);
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
          password = (char*)smallMalloc(len+1, "url");
          strncpy (password, curstr, len);
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
      host = (char*)smallMalloc(len+1, "url");
      strncpy (host, curstr, len);
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
        port = (char*)smallMalloc(len+1, "httpPort");
        strncpy (port, curstr, len);
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
      path = (char*)smallMalloc(len+1, "url");
      strncpy (path, curstr, len);
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

        query = (char*)smallMalloc(len+1, "url");
        strncpy (query, curstr, len);
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

        fragment = (char*)smallMalloc(len+1, "uel");
        strncpy (fragment, curstr, len);
        fragment[len] = '\0';

        curstr = tmpstr;
        }
      //}}}
      }
    //}}}
    //{{{  public vars
    char* scheme;    // mandatory
    char* host;      // mandatory
    char* path;      // optional
    char* port;      // optional
    char* username;  // optional
    char* password;  // optional
    char* query;     // optional
    char* fragment;  // optional
    //}}}
    };
  //}}}

  //{{{
  bool parseChunked (int& size, char ch) {
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
      case 0xC0: return http_parse_header_done;
      case 0xC1: return http_parse_header_done;
      case 0xC4: return http_parse_header_store_keyvalue;
      case 0x80: return http_parse_header_version_character;
      case 0x81: return http_parse_header_code_character;
      case 0x82: return http_parse_header_status_character;
      case 0x84: return http_parse_header_key_character;
      case 0x87: return http_parse_header_value_character;
      case 0x88: return http_parse_header_value_character;
      }

    return http_parse_header_continue;
    }
  //}}}

  //{{{  private vars
  int mResponse = 0;
  eState mState = http_header;
  eParseHeaderState mParseHeaderState = http_parse_header_done;
  int mChunked = 0;

  int mKeyStrLen = 0;
  int mValueStrLen = 0;

  char* mScratch;
  cUrl* mRedirectUrl = nullptr;

  uint8_t* mContent = nullptr;
  int mContentSize = 0;
  int mContentLen = -1;
  //}}}
  };
