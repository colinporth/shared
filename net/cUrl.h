// cHttp.h - http base class based on tinyHttp parser
#pragma once
//{{{  includes
#include <stdint.h>
#include <string>
#include <sstream>
#include <iomanip>
//}}}

class cUrl {
public:
  ~cUrl() { clear(); }
  //{{{
  void clear() {
    free (scheme);
    scheme = nullptr;

    free (host);
    host = nullptr;

    free (port);
    port = nullptr;

    free (query);
    query = nullptr;

    free (fragment);
    fragment = nullptr;

    free (username);
    username = nullptr;

    free (password);
    password = nullptr;
    }
  //}}}

  std::string getScheme() { return scheme; }
  std::string getHost() { return host; }
  std::string getPath() { return path; }
  std::string getPort() { return port; }
  std::string getUsername() { return username; }
  std::string getPassword() { return password; }
  std::string getQuery() { return query; }
  std::string getFragment() { return fragment; }

  //{{{
  void parse (const std::string& urlString) {
  // parseUrl, see RFC 1738, 3986
  // !!! keep converting to string !!!

    clear();

    auto url = urlString.c_str();
    int urlLen = (int)urlString.size();
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
      username = (char*)malloc(len+1);
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
        password = (char*)malloc(len+1);
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
    host = (char*)malloc(len+1);
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
      port = (char*)malloc(len+1);
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
    path = (char*)malloc(len+1);
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

      query = (char*)malloc(len+1);
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

      fragment = (char*)malloc(len+1);
      strncpy (fragment, curstr, len);
      fragment[len] = '\0';

      curstr = tmpstr;
      }
    //}}}
    }
  //}}}

private:
  char* scheme = nullptr;    // mandatory
  char* host = nullptr;      // mandatory
  char* path = nullptr;      // optional
  char* port = nullptr;      // optional
  char* username = nullptr;  // optional
  char* password = nullptr;  // optional
  char* query = nullptr;     // optional
  char* fragment = nullptr;  // optional
  };
