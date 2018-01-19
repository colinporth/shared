// cKeyboard.cpp - crude linux keyboard handler
//{{{  includes
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <fcntl.h>

#include "cKeyboard.h"
#include "cLog.h"

using namespace std;
//}}}

//{{{
cKeyboard::cKeyboard() {

  if (isatty (STDIN_FILENO)) {
    struct termios new_termios;
    tcgetattr (STDIN_FILENO, &mOrigTermios);

    new_termios = mOrigTermios;
    new_termios.c_lflag &= ~(ICANON | ECHO | ECHOCTL | ECHONL);
    new_termios.c_cflag |= HUPCL;
    new_termios.c_cc[VMIN] = 0;
    tcsetattr (STDIN_FILENO, TCSANOW, &new_termios);
    }

  else {
    mOrigFl = fcntl (STDIN_FILENO, F_GETFL);
    fcntl (STDIN_FILENO, F_SETFL, mOrigFl | O_NONBLOCK);
    }

  mAction = 0;
  }
//}}}
//{{{
cKeyboard::~cKeyboard() {
  restore_term();
  }
//}}}

//{{{
int cKeyboard::getEvent() {

  int ret = mAction;
  mAction = 0;
  return ret;
  }
//}}}

//{{{
void cKeyboard::run() {

  cLog::setThreadName ("key ");

  while (true) {
    int ch[8];
    int chnum = 0;
    while ((ch[chnum] = getchar()) != EOF)
      chnum++;

    if (chnum > 0) {
      if (chnum > 1)
        ch[0] = ch[chnum-1] | (ch[chnum-2] << 8);
      if (mKeymap[ch[0]] != 0)
        mAction = mKeymap[ch[0]];
      else
        cLog::log (LOGNOTICE, "cKeyboard - unconfigured key %x", ch[0]);
      }
    else
      msSleep (40);

    //cLog::log (LOGINFO, "cKeyboard::run - loop");
    }

  cLog::log (LOGERROR, "run - exit");
  }
//}}}

// private
//{{{
void cKeyboard::restore_term() {

  if (isatty (STDIN_FILENO))
    tcsetattr (STDIN_FILENO, TCSANOW, &mOrigTermios);
  else
    fcntl (STDIN_FILENO, F_SETFL, mOrigFl);
  }
//}}}
//{{{
void cKeyboard::msSleep (int ms) {

  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000;
  nanosleep (&ts, NULL);
  }
//}}}
