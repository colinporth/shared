// iWindow.h -  windowinterface
#pragma once

class iWindow {
public:
  virtual ~iWindow() {}

  virtual cVg* getContext() = 0;

  virtual uint16_t getWidthPix() = 0;
  virtual uint16_t getHeightPix() = 0;

  virtual bool getShift() = 0;
  virtual bool getControl() = 0;
  virtual bool getMouseDown() = 0;
  };
