#pragma once
#include <termios.h>
#include <map>

class cKeyboard {
public:
  cKeyboard();
  ~cKeyboard();

  int getEvent();
  void setKeymap (std::map<int,int> keymap) { m_keymap = keymap; }

  void run();

private:
  void restore_term();
  void sleep (unsigned int milliSeconds);

  struct termios orig_termios;
  int orig_fl;
  int m_action;
  std::map<int,int> m_keymap;
  };
