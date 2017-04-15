// cLwipHttp.h - http base class based on tinyHttp parser
#pragma once
#include "cHttp.h"

class cLwipHttp : public cHttp {
public:
  cLwipHttp() : cHttp() {}
  virtual ~cLwipHttp() {}

  virtual void initialise() {}

protected:
  //{{{
  virtual int connectToHost (std::string host) {

    if (!mNetConn || (host != mLastHost)) {
      // delete any old connection
      if (mNetConn)
        netconn_delete (mNetConn);
      mNetConn = nullptr;

      // find host ipAddress,
      ip_addr_t ipAddr;
      if (netconn_gethostbyname (host.c_str(), &ipAddr) != ERR_OK)
        return -1;

      // make new connection
      mNetConn = netconn_new (NETCONN_TCP);
      if (mNetConn == nullptr)
        return -2;

      // open new connection
      if (netconn_connect (mNetConn, &ipAddr, 80) != ERR_OK) {
        //{{{  error return
        netconn_delete (mNetConn);
        return -3;
        }
        //}}}
      mLastHost = host;
      }

    return 0;
    }
  //}}}
  //{{{
  virtual bool getSend (std::string sendStr) {

    if (netconn_write (mNetConn, sendStr.c_str(), (int)sendStr.size(), NETCONN_NOCOPY) != ERR_OK) {
      netconn_delete (mNetConn);
      return false;
      }

    return true;
    }
  //}}}
  //{{{
  virtual int getAllRecv() {
    // recv
    struct netbuf* buf = NULL;

    bool needMoreData = true;
    while (needMoreData) {
      if (netconn_recv (mNetConn, &buf) != ERR_OK) {
        //{{{  error return;
        netconn_delete (mNetConn);
        return -5;
        }
        //}}}

      uint8_t* bufferPtr;
      uint16_t bufferBytesReceived;
      netbuf_data (buf, (void**)(&bufferPtr), &bufferBytesReceived);
      while (needMoreData && (bufferBytesReceived > 0)) {
        int bytesReceived;
        needMoreData = parseRecvData (bufferPtr, bufferBytesReceived, bytesReceived);
        bufferBytesReceived -= bytesReceived;
        bufferPtr += bytesReceived;
        }
      netbuf_delete (buf);
      }

    return 0;
    }
  //}}}

private:
  struct netconn* mNetConn = nullptr;
  };
