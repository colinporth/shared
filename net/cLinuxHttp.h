// cLinuxHttp.h - linux http
#pragma once
//{{{  includes
#include "cHttp.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "../utils/cLog.h"
//}}}

class cPlatformHttp : public cHttp {
public:
  cPlatformHttp() : cHttp() {}
  //{{{
  virtual ~cPlatformHttp() {

    if (mSocket >= 0)
      close (mSocket);
    }
  //}}}

  virtual void initialise() {}

protected:
  //{{{
  virtual int connectToHost (const std::string& host) {

    if ((mSocket == -1) || (host != mLastHost)) {
      // notConnected or diff host
      if (mSocket >= 0)
        close (mSocket);

      mSocket = socket (AF_INET, SOCK_STREAM, 0);
      if (mSocket >= 0)
        cLog::log (LOGINFO, "connectToHost - successfully opened socket");
      else
        cLog::log (LOGINFO, "connectToHost - error opening socket");

      struct hostent* server = gethostbyname (host.c_str());
      if (server == NULL)
        cLog::log (LOGINFO, "connectToHost - gethostbyname() failed");
      else {
        cLog::log (LOGINFO, "connectToHost - %s ", server->h_name);
        unsigned int j = 0;
        while (server->h_addr_list[j] != NULL) {
          cLog::log (LOGINFO, "- %s", inet_ntoa(*(struct in_addr*)(server->h_addr_list[j])));
          j++;
          }
        }

      struct sockaddr_in serveraddr;
      memset (&serveraddr, 0, sizeof(sockaddr_in));
      serveraddr.sin_family = AF_INET;
      memcpy (&serveraddr.sin_addr.s_addr, server->h_addr, server->h_length);

      int port = 80;
      serveraddr.sin_port = htons (port);
      if (connect (mSocket, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) >= 0)
        cLog::log (LOGINFO, "connectToHost - connected socket:%d", mSocket);
      else
        cLog::log (LOGINFO, "connectToHost - Error Connecting");

      mLastHost = host;
      }

    return 0;
    }
  //}}}
  //{{{
  virtual bool getSend (const std::string& sendStr) {

    if (send (mSocket, sendStr.c_str(), (int)sendStr.size(), 0) < 0)  {
      close (mSocket);
      mSocket = -1;
      return false;
      }

    return true;
    }
  //}}}
  //{{{
  virtual int getRecv (uint8_t* buffer, int bufferSize) {

    int bufferBytesReceived = recv (mSocket,(char*)buffer, bufferSize, 0);
    if (bufferBytesReceived <= 0) {
      close (mSocket);
      mSocket = -1;
      return -5;
      }

    return bufferBytesReceived;
    }
  //}}}

private:
  int mSocket = -1;
  std::string mLastHost;
  };
