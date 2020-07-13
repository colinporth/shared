// cRaspWindow.h - non X openGL
//{{{  includes
#pragma once

// Egl, gles2
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "GLES2/gl2.h"

// cVgGL
#include "cVgGL.h"
#include "cPerfGraph.h"

#include "../utils/iChange.h"
#include "../widgets/cRootContainer.h"
//}}}

class cRaspWindow : public cVg, public iChange, public iDraw {
public:
  cRaspWindow();
  ~cRaspWindow();

  // iWindow
  cVg* getContext () { return this; }
  float getWidth() { return mRoot->getWidth(); }
  float getHeight() { return mRoot->getHeight(); }
  uint16_t getWidthPix() { return mRoot->getPixWidth(); }
  uint16_t getHeightPix() { return mRoot->getPixHeight(); }

  bool getShift() { return false; }
  bool getControl() { return false; }
  bool getMouseDown() { return false; }

  // iDraw
  void pixel (uint32_t colour, int16_t x, int16_t y) {}
  void drawRect (uint32_t colour, int16_t x, int16_t y, uint16_t width, uint16_t height);
  void stamp (uint32_t colour, uint8_t* src, int16_t x, int16_t y, uint16_t width, uint16_t height) {}
  int drawText (uint32_t colour, uint16_t fontHeight, std::string str, int16_t x, int16_t y, uint16_t width, uint16_t height);
  void ellipseSolid (uint32_t colour, int16_t x, int16_t y, uint16_t xradius, uint16_t yradius);

  // iChange
  void changed() { mCountDown = 0; }
  void setChangeCountDown (int countDown) { mChangeCountDown = countDown; }

protected:
  cRootContainer* initialise (float scale, uint32_t alpha);

  cWidget* add (cWidget* widget) { return mRoot->add (widget); }
  cWidget* addAt (cWidget* widget, float x, float y) { return mRoot->addAt (widget,x,y); }
  cWidget* addAtPix (cWidget* widget, int16_t x, int16_t y) { return mRoot->addAtPix (widget,x,y); }
  cWidget* addTopLeft (cWidget* widget) { return mRoot->addTopLeft (widget); }
  cWidget* addTopRight (cWidget* widget) { return mRoot->addTopRight (widget); }
  cWidget* addBottomLeft (cWidget* widget) { return mRoot->addBottomLeft (widget); }
  cWidget* addBottomRight (cWidget* widget) { return mRoot->addBottomRight (widget); }
  cWidget* addBelow (cWidget* widget) { return mRoot->addBelow (widget); }
  cWidget* addLeft (cWidget* widget) { return mRoot->addLeft (widget); }
  cWidget* addAbove (cWidget* widget) { return mRoot->addAbove (widget); }
  void run();

  void toggleVsync() { mVsync = !mVsync; setVsync (mVsync); }
  void togglePerf()  { mDrawPerf = !mDrawPerf; mFpsGraph->reset(); mCpuGraph->reset(); }
  void toggleStats() { mDrawStats = !mDrawStats; }
  void toggleTests() { mDrawTests = !mDrawTests; }

  uint64_t getAbsoluteClock();
  void uSleep (uint64_t uSec);

  virtual void pollKeyboard() = 0;

  //  vars
  bool mExit = false;

  int mMouseX = 0;
  int mMouseY = 0;
  int mMouseButtons = 0;

  bool mVsync = true;
  bool mDrawPerf = false;
  bool mDrawStats = false;
  bool mDrawTests = false;

private:
  void pollMouse();

  void setVsync (bool vsync);
  void startFrame();
  void endSwapFrame();

  void drawMouse (int x, int y);
  void drawSpinner (float cx, float cy, float r);
  void drawEyes (float x, float y, float w, float h, float cursorX, float cursorY);
  void drawLines (float x, float y, float w, float h);

  //{{{  vars
  DISPMANX_DISPLAY_HANDLE_T mDispmanxDisplay = 0;
  DISPMANX_ELEMENT_HANDLE_T mDispmanxElement = 0;
  EGL_DISPMANX_WINDOW_T mNativeWindow;

  EGLDisplay mEglDisplay;
  EGLSurface mEglSurface;
  EGLContext mEglContext;

  int64_t mTime = 0;

  cPerfGraph* mFpsGraph = nullptr;
  cPerfGraph* mCpuGraph = nullptr;

  cRootContainer* mRoot = nullptr;
  uint32_t mScreenWidth = 0;
  uint32_t mScreenHeight = 0;

  int mMouseFd = -1;

  uint32_t mCountDown = 0;
  uint32_t mChangeCountDown = 100;
  //}}}
  };
