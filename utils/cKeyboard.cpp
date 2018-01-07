// cKeyboard.cpp - crude linux keyboard handler
//{{{  includes
#include "cKeyboard.h"

#include <stdio.h>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <map>

#include "cLog.h"

using namespace std;
//}}}

//{{{
cKeyboard::cKeyboard() {

  if (isatty (STDIN_FILENO)) {
    struct termios new_termios;
    tcgetattr (STDIN_FILENO, &orig_termios);

    new_termios = orig_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO | ECHOCTL | ECHONL);
    new_termios.c_cflag |= HUPCL;
    new_termios.c_cc[VMIN] = 0;
    tcsetattr (STDIN_FILENO, TCSANOW, &new_termios);
    }
  else {
    orig_fl = fcntl (STDIN_FILENO, F_GETFL);
    fcntl (STDIN_FILENO, F_SETFL, orig_fl | O_NONBLOCK);
    }

  m_action = -1;
  }
//}}}
//{{{
cKeyboard::~cKeyboard() {
  restore_term();
  }
//}}}

//{{{
int cKeyboard::getEvent() {
  int ret = m_action;
  m_action = -1;
  return ret;
  }
//}}}

//{{{
void cKeyboard::run() {

  cLog::setThreadName (" key");

  while (true) {
    int ch[8];
    int chnum = 0;

    while ((ch[chnum] = getchar()) != EOF)
      chnum++;

    if (chnum > 1)
      ch[0] = ch[chnum - 1] | (ch[chnum - 2] << 8);
    if (chnum > 0)
      cLog::log (LOGINFO, "cKeyboard char 0x%x %c ", ch[0], ch[0]);

    if (m_keymap[ch[0]] != 0)
      m_action = m_keymap[ch[0]];
    else
      sleep(20);
    }
  }
//}}}

// private
//{{{
void cKeyboard::restore_term() {
  if (isatty (STDIN_FILENO))
    tcsetattr (STDIN_FILENO, TCSANOW, &orig_termios);
  else
    fcntl (STDIN_FILENO, F_SETFL, orig_fl);
  }
//}}}
//{{{
void cKeyboard::sleep (unsigned int milliSeconds) {
  struct timespec req;
  req.tv_sec = milliSeconds / 1000;
  req.tv_nsec = (milliSeconds % 1000) * 1000000;
  while (nanosleep(&req, &req) == -1 && errno == EINTR && (req.tv_nsec > 0 || req.tv_sec > 0));
  }
//}}}
