// cEsp8266Http.h - http base class based on tinyHttp parser
#pragma once
#include "cHttp.h"

class cEsp8266Http : public cHttp {
public:
  //{{{
  virtual void initialise() {

    initialiseComms();

    if (!sendCommandWait ("AT+GMR", "OK")) {
      debug ("esp8266 failed - reset");
      reset();
      sendCommandWait ("AT+CWJAP=\"home\",\"FAFFFAFF\"", "OK"); // access point
      }

    sendCommandWait ("ATE0", "OK");         // noecho
    sendCommandWait ("AT+CIPMUX=1", "OK");  // multiple
    }
  //}}}

  //{{{
  void closeTcp() {
    sendCommandWait ("AT+CIPCLOSE=4", "OK");
    }
  //}}}
  //{{{
  void setTcpHost (const std::string& host) {
    sendCommandWait ("AT+CIPSTART=4,\"TCP\",\"" + host + "\",80", "OK");
    }
  //}}}
  //{{{
  void send (const std::string& path) {

    std::string httpSend = "AT+CIPSEND=4," + dec(path.size()+2);

    sendCommand (httpSend);
    while (readChar() != '>') {}

    sendCommand (path);
    readReplyUntil ("SEND OK");
    }
  //}}}
  //{{{
  int recv (uint8_t* body, int bodySize) {

    while (readChar() != '+') {}
    bool ok = readChar() == 'I';
    ok &= readChar() == 'P';
    ok &= readChar() == 'D';
    ok &= readChar() == ',';
    ok &= readChar() == '4';
    ok &= readChar() == ',';

    int rxSize = 0;
    if (ok) {
      while (true) {
        auto ch = readChar();
        if (ch < '0' || ch > '9')
          break;
        rxSize = rxSize*10 + (ch - 0x30);
        }
      if (rxSize > bodySize) {
        debug ("cEsp8266::recv - size error" + dec(rxSize) + " " + dec(bodySize));
        return 0;
        }
      //debug ("cEsp8266::recv size " + dec (rxSize));

      uint8_t* content = body;
      DWORD bytesLeft = rxSize;
      DWORD numRead = 0;
      while (bytesLeft > 0) {
        readChars (content, bytesLeft, numRead);
        content += numRead;
        bytesLeft -= numRead;
        }
      }
    else
      debug ("cEsp8266::recv - error");

    return rxSize;
    }
  //}}}

protected:
  //{{{
  virtual int connectToHost (const std::string& host) {

    if (host != mLastHost) {
      closeTcp();
      setTcpHost (host);
      mLastHost = host;
      }

    return 0;
    }
  //}}}
  //{{{
  virtual bool getSend (const std::string& sendStr) {
    send (sendStr);
    return true;
    }
  //}}}
  //{{{
  virtual int getRecv (uint8_t* buffer, int bufferSize) {
    return recv (buffer, bufferSize);
    }
  //}}}

  virtual void initialiseComms() = 0;
  virtual void sendCommand ((const std::string& command) = 0;
  virtual uint8_t readChar() = 0;
  virtual bool readChars (uint8_t* content, DWORD bytesLeft, DWORD& numRead) = 0;

private:
  //{{{
  bool sendCommandWait (const std::string& command, const std::string& str) {
    sendCommand (command);
    return readReplyUntil (str);
    }
  //}}}
  //{{{
  bool readReplyUntil (const std::string& str) {
    std::string reply;
    do {
      reply.clear();
      auto ch = readChar();
      while (ch != 0x0a) {
        if (ch != 0x0d)
          reply += ch;
        ch = readChar();
        }
      } while ((reply != str) && (reply != "ERROR"));

    return reply != "ERROR";
    }
  //}}}

  //{{{
  void reset() {
    sendCommandWait ("AT+RST", "ready");
    sendCommandWait ("AT+GMR", "OK");
    sendCommandWait ("AT+CWMODE=1", "OK");
    sendCommandWait ("AT+CWLAP", "OK");
    }
  //}}}
  //{{{
  void reportIP() {
    sendCommandWait ("AT+CIFSR", "OK");
    }
  //}}}
  };
