// cWinSock.h - winSock http
#pragma once
#include "cEsp8266Http.h"

class cWinEsp8266Http : public cEsp8266Http {
public:
  cWinEsp8266Http() : cEsp8266Http() {}
  virtual ~cWinEsp8266Http() {}

protected:
  //{{{
  virtual void initialiseComms() {

    mComHandle = CreateFile (L"COM6", GENERIC_READ | GENERIC_WRITE, 0,NULL, OPEN_EXISTING, 0, NULL);
    if (mComHandle == INVALID_HANDLE_VALUE)
      printf ("cEsp8266 - invalid COM port\n");

    // set timeouts
    COMMTIMEOUTS commTimeouts = { MAXDWORD, 100, 100, 100, 100};
    if (!SetCommTimeouts (mComHandle, &commTimeouts))
      printf ("cEsp8266 - error setCommTimeouts\n");

    DCB dcb;
    memset (&dcb, 0, sizeof(dcb));
    dcb.DCBlength = sizeof(dcb);
    //dcb.BaudRate = 230400;
    dcb.BaudRate = 921600;
    dcb.fBinary = 1;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.ByteSize = 8;

    if (!SetCommState(mComHandle, &dcb))
      printf ("cEsp8266 - error setting baud\n");
    }
  //}}}

  virtual void sendCommand (std::string command) {
    std::string fullCommand = command + "\r\n";
    DWORD numWritten;
    WriteFile (mComHandle, fullCommand.c_str(), (DWORD)fullCommand.size(), &numWritten, NULL);
    }

  virtual uint8_t readChar() {
    uint8_t ch = 0xFF;
    DWORD numRead;
    return ReadFile (mComHandle, &ch, 1, &numRead, NULL) ? ch : 0xFF;
    }

  virtual bool readChars (uint8_t* content, DWORD bytesLeft, DWORD& numRead) {
    return ReadFile (mComHandle, content, bytesLeft, &numRead, NULL) != 0;
    }


private:
  HANDLE mComHandle = 0;
  };
