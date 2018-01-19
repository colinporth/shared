#pragma once
#include <termios.h>
#include <map>

class cKeyboard {
public:
  cKeyboard();
  ~cKeyboard();

  int getEvent();
  void setKeymap (std::map<int,int> keymap) { mKeymap = keymap; }

  void run();

private:
  void restore_term();
  void msSleep (int ms);

  struct termios mOrigTermios;
  int mOrigFl;
  int mAction;
  std::map<int,int> mKeymap;
  };
