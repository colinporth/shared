// iWindow.h -  windowinterface
#pragma once

class iWindow {
public:
  virtual ~iWindow() {}

  virtual cVg* getVg() = 0;

  virtual cPointF getPixSize() = 0;

  virtual bool getShift() = 0;
  virtual bool getControl() = 0;
  virtual bool getMouseDown() = 0;
  };
