// iWindow.h -  windowinterface
#pragma once
#include <string>

class iWindow {
public:
  virtual ~iWindow() {}

  #ifdef USE_NANOVG
    virtual cVg* getContext() = 0;
  #endif

  virtual uint16_t getWidthPix() = 0;
  virtual uint16_t getHeightPix() = 0;

  virtual bool getShift() = 0;
  virtual bool getControl() = 0;
  virtual bool getMouseDown() = 0;
  };
