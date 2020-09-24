// cUrl.h - url parser
#pragma once
#include <stdint.h>
#include <cstring>
#include <string>
//#include <sstream>
//#include <iomanip>

class cUrl {
public:
  std::string getScheme() { return mScheme; }
  std::string getHost() { return mHost; }
  std::string getPath() { return mPath; }
  std::string getPort() { return mPort; }
  std::string getUsername() { return mUsername; }
  std::string getPassword() { return mPassword; }
  std::string getQuery() { return mQuery; }
  std::string getFragment() { return mFragment; }

  void parse (const std::string& urlString) {
  // parseUrl, see RFC 1738, 3986

    auto url = urlString.c_str();
    size_t urlLen = urlString.size();
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
    mScheme = std::string (curstr, len);

    // Make the character to lower if it is upper case.
    for (unsigned int i = 0; i < mScheme.size(); i++)
      mScheme[i] = tolower (mScheme[i]);
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

      mUsername = std::string (curstr, tmpstr - curstr);
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

        mPassword  = std::string (curstr, tmpstr - curstr);
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

    mHost  = std::string (curstr, tmpstr - curstr);
    curstr = tmpstr;
    //}}}
    //{{{  parse port number
    if (':' == *curstr) {
      curstr++;

      // Read port number
      tmpstr = curstr;
      while ((tmpstr < url + urlLen) && ('/' != *tmpstr))
        tmpstr++;

      mPort = std::string (curstr, tmpstr - curstr);
      curstr = tmpstr;
      }
    //}}}

    // end of string?
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

    mPath = std::string (curstr, tmpstr - curstr);
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

      mQuery  = std::string (curstr, tmpstr - curstr);
      curstr = tmpstr;
      }
    //}}}
    //{{{  parse fragment
    if ('#' == *curstr) {
      // Skip '#'
      curstr++;

      // Read fragment
      tmpstr = curstr;
      while (tmpstr < url + urlLen)
        tmpstr++;
      mFragment  = std::string (curstr, tmpstr - curstr);
      curstr = tmpstr;
      }
    //}}}
    }

private:
  std::string mScheme;    // mandatory
  std::string mHost;      // mandatory
  std::string mPath;      // optional
  std::string mPort;      // optional
  std::string mUsername;  // optional
  std::string mPassword;  // optional
  std::string mQuery;     // optional
  std::string mFragment;  // optional
  };
