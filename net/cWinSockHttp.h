// cWinSock.h - winSock http
#pragma once
#include "cHttp.h"

class cWinSockHttp : public cHttp {
public:
  //{{{
  cWinSockHttp() : cHttp() {
    WSAStartup (MAKEWORD(2,2), &wsaData);
    }
  //}}}
  //{{{
  virtual ~cWinSockHttp() {
    if (mSocket != -1)
      closesocket (mSocket);
    }
  //}}}

  virtual void initialise() {}

protected:
  //{{{
  virtual int connectToHost (const std::string& host) {

    if ((mSocket == -1) || (host != mLastHost)) {
      // not connected or different host
      //{{{  close any open webSocket
      if (mSocket >= 0)
        closesocket (mSocket);
      //}}}
      //{{{  win32 find host ipAddress
      struct addrinfo hints;
      memset (&hints, 0, sizeof(hints));
      hints.ai_family = AF_INET;       // IPv4
      hints.ai_protocol = IPPROTO_TCP; // TCP
      hints.ai_socktype = SOCK_STREAM; // TCP so its SOCK_STREAM
      struct addrinfo* targetAddressInfo = NULL;
      unsigned long getAddrRes = getaddrinfo (host.c_str(), NULL, &hints, &targetAddressInfo);
      if (getAddrRes != 0 || targetAddressInfo == NULL) {
        //{{{  error
        return -1;
        }
        //}}}

      // form sockAddr
      struct sockaddr_in sockAddr;
      sockAddr.sin_addr = ((struct sockaddr_in*)targetAddressInfo->ai_addr)->sin_addr;
      sockAddr.sin_family = AF_INET; // IPv4
      sockAddr.sin_port = htons (80); // HTTP Port: 80

      // free targetAddressInfo from getaddrinfo
      freeaddrinfo (targetAddressInfo);
      //}}}

      // win32 create webSocket
      mSocket = (unsigned int)socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (mSocket < 0) {
        //{{{  error
        return -2;
        }
        //}}}

      // win32 connect webSocket
      if (connect (mSocket, (SOCKADDR*)&sockAddr, sizeof(sockAddr))) {
        //{{{  error
        closesocket (mSocket);
        mSocket = -1;
        return -3;
        }
        //}}}

      mLastHost = host;
      }

    return 0;
    }
  //}}}
  //{{{
  virtual bool getSend (const std::string& sendStr) {

    if (send (mSocket, sendStr.c_str(), (int)sendStr.size(), 0) < 0) {
      closesocket (mSocket);
      mSocket = -1;
      return false;
      }

    return true;
    }
  //}}}
  //{{{
  virtual int getRecv (uint8_t* buffer, int bufferSize) {

    int bufferBytesReceived = recv (mSocket, (char*)buffer, bufferSize, 0);
    if (bufferBytesReceived <= 0) {
      closesocket (mSocket);
      mSocket = -1;
      return -5;
      }

    return bufferBytesReceived;
    }
  //}}}

private:
  int mSocket = -1;
  WSADATA wsaData;
  };
