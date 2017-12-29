// cRaspWindow.h
#define NANOVG
#define NANOVG_GLES2 1
//{{{  includes
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Egl, gles2
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "GLES2/gl2.h"

// cVgGL
#include "cVgGL.h"
#include "cPerfGraph.h"
#include "../fonts/FreeSansBold.h"
#include "../fonts/DroidSansMono1.h"

#include "../utils/cLog.h"

#include "../utils/iChange.h"
#include "../widgets/cRootContainer.h"
//}}}
//{{{
const EGLint kConfigAttributeList[] = {
  EGL_RED_SIZE,        8,
  EGL_GREEN_SIZE,      8,
  EGL_BLUE_SIZE,       8,
  EGL_ALPHA_SIZE,      8,
  //EGL_DEPTH_SIZE,      8,
  EGL_STENCIL_SIZE,    8,
  EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
  EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
  EGL_NONE
  };
//}}}
//{{{
const EGLint kContextAttributeList[] = {
  EGL_CONTEXT_CLIENT_VERSION, 2,
  EGL_NONE
  };
//}}}

class cRaspWindow : public cVgGL, public iChange, public iDraw {
public:
  //{{{
  cRaspWindow (float scale, uint32_t alpha) {

    // get EGL display
    mEglDisplay = eglGetDisplay (EGL_DEFAULT_DISPLAY);
    cLog::log (LOGINFO, "eglGetDisplay display:%p", mEglDisplay);

    // init EGL display
    auto result = eglInitialize (mEglDisplay, NULL, NULL);
    cLog::log (LOGINFO, "eglInitialize display:%d", result);

    // get EGLconfig frameBuffer, BRCM extension gets closest match
    EGLConfig config;
    EGLint numConfig;
    eglSaneChooseConfigBRCM (mEglDisplay, kConfigAttributeList, &config, 1, &numConfig);
    cLog::log (LOGINFO, "eglSaneChooseConfigBRCM numConfig:%d", numConfig);

    result = eglBindAPI (EGL_OPENGL_ES_API);
    cLog::log (LOGINFO, "eglBindAPI:%d", result);

    // create EGL rendering context
    mEglContext = eglCreateContext (mEglDisplay, config, EGL_NO_CONTEXT, kContextAttributeList);
    cLog::log (LOGINFO, "eglCreateContext context:%p", mEglContext);

    // create EGL window surface
    graphics_get_display_size (0, &mScreenWidth, &mScreenHeight);
    cLog::log (LOGINFO, "graphics_get_display_size %d:%d", mScreenWidth, mScreenHeight);

    mScreenWidth = int(mScreenWidth * scale);
    mScreenHeight = int(mScreenHeight * scale);

    mDispmanxDisplay = vc_dispmanx_display_open (0);
    DISPMANX_UPDATE_HANDLE_T dispmanxUpdate = vc_dispmanx_update_start (0);

    // create dispmanx element
    VC_RECT_T dstRect = { 0, 0, (int32_t)mScreenWidth, (int32_t)mScreenHeight };
    VC_RECT_T srcRect = { 0,0, (int32_t)(mScreenWidth << 16), (int32_t)(mScreenHeight << 16) };
    VC_DISPMANX_ALPHA_T kAlpha = { DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS, alpha, 0 };
    //VC_DISPMANX_ALPHA_T kAlpha = { DISPMANX_FLAGS_ALPHA_FROM_SOURCE, 200, 0 };
    mDispmanxElement = vc_dispmanx_element_add (dispmanxUpdate, mDispmanxDisplay, 3000,
                                                &dstRect, 0, &srcRect,
                                                DISPMANX_PROTECTION_NONE, &kAlpha, NULL, DISPMANX_NO_ROTATE);
    mNativeWindow.element = mDispmanxElement;
    mNativeWindow.width = mScreenWidth;
    mNativeWindow.height = mScreenHeight;
    vc_dispmanx_update_submit_sync (dispmanxUpdate);

    mEglSurface = eglCreateWindowSurface (mEglDisplay, config, &mNativeWindow, NULL);
    cLog::log (LOGINFO, "eglCreateWindowSurface surface:%p", mEglSurface);

    // connect the context to the surface
    result = eglMakeCurrent (mEglDisplay, mEglSurface, mEglSurface, mEglContext);
    cLog::log (LOGINFO, "eglMakeCurrent %d", result);

    mFpsGraph = new cPerfGraph (cPerfGraph::GRAPH_RENDER_FPS, "Frame Time");
    mCpuGraph = new cPerfGraph (cPerfGraph::GRAPH_RENDER_MS, "CPU Time");

    // set
    setVsync (true);

    cVgGL::initialise();
    cLog::log (LOGINFO, "cRaspWindow");

    //createFontMem ("sans", (unsigned char*)freeSansBold, sizeof(freeSansBold), 0);
    createFontMem ("sans", (unsigned char*)droidSansMono, sizeof(droidSansMono), 0);
    fontFace ("sans");
    cLog::log (LOGINFO, "created sans font droidSansMono");

    glViewport (0, 0, mScreenWidth, mScreenHeight);
    glClearColor (0, 0, 0, 1.0f);
    }
  //}}}
  //{{{
  ~cRaspWindow() {

    // clear screen
    glClear (GL_COLOR_BUFFER_BIT);
    eglSwapBuffers (mEglDisplay, mEglSurface);
    eglDestroySurface (mEglDisplay, mEglSurface);

    DISPMANX_UPDATE_HANDLE_T dispmanxUpdate = vc_dispmanx_update_start (0);
    vc_dispmanx_element_remove (dispmanxUpdate, mDispmanxElement);

    vc_dispmanx_update_submit_sync (dispmanxUpdate);
    vc_dispmanx_display_close (mDispmanxDisplay);

    // Release OpenGL resources
    eglMakeCurrent (mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext (mEglDisplay, mEglContext);
    eglTerminate (mEglDisplay);
    }
  //}}}

  // iWindow
  cVg* getContext () { return this; }
  uint16_t getWidthPix() { return mRoot->getPixWidth(); }
  uint16_t getHeightPix() { return mRoot->getPixHeight(); }
  bool getShift() { return false; }
  bool getControl() { return false; }
  bool getMouseDown() { return false; }

  // iDraw
  void pixel (uint32_t colour, int16_t x, int16_t y) {}
  //{{{
  void drawRect (uint32_t colour, int16_t x, int16_t y, uint16_t width, uint16_t height) {

    beginPath();
    rect (x, y, width, height);
    fillColor (nvgRGBA ((colour & 0xFF0000) >> 16, (colour & 0xFF00) >> 8, colour & 0xFF,255));
    triangleFill();
    }
  //}}}
  //{{{
  int drawText (uint32_t colour, uint16_t fontHeight, std::string str, int16_t x, int16_t y, uint16_t width, uint16_t height) {

    fontSize ((float)fontHeight);
    textAlign (cVg::ALIGN_LEFT | cVg::ALIGN_TOP);
    fillColor (nvgRGBA ((colour & 0xFF0000) >> 16, (colour & 0xFF00) >> 8, colour & 0xFF,255));
    return (int)text ((float)x+3, (float)y+1, str);
    }
  //}}}
  //{{{
  void ellipseSolid (uint32_t colour, int16_t x, int16_t y, uint16_t xradius, uint16_t yradius) {

    beginPath();
    ellipse (x, y, xradius, yradius);
    fillColor (nvgRGBA ((colour & 0xFF0000) >> 16, (colour & 0xFF00) >> 8, colour & 0xFF,255));
    fill();
    }
  //}}}

  // iChange
  void changed() {}

protected:
  //{{{
  cRootContainer* initialise() {
    mRoot = new cRootContainer (mScreenWidth, mScreenHeight);
    return mRoot;
    }
  //}}}
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
  //{{{
  void run() {

    int mouseX = mScreenWidth/2;
    int mouseY = mScreenHeight/2;
    while (!mEscape) {
      mMouseButtons = getMouse (&mouseX, &mouseY);

      mCpuGraph->start (getAbsoluteClock() / 1000000.0f);

      startFrame();
      if (mRoot)
        mRoot->onDraw (this);
      drawMouse (mouseX, mouseY);
      if (mDrawTests) {
        //{{{  draw tests
        drawEyes (mScreenWidth*3.0f/4.0f, mScreenHeight/2.0f, mScreenWidth/4.0f, mScreenHeight/2.0f,
                  mouseX, mouseY);
        drawLines (0.0f, 50.0f, mScreenWidth, mScreenHeight);
        drawSpinner (mScreenWidth/2.0f, mScreenHeight/2.0f, 20.0f);
        }
        //}}}
      if (mDrawStats)
        drawStats (mScreenWidth, mScreenHeight, getFrameStats() + (mVsync ? " vsync" : " free"));
      if (mDrawPerf) {
        //{{{  render perf stats
        mFpsGraph->render (this, 0.0f, mScreenHeight-35.0f, mScreenWidth/3.0f -2.0f, 35.0f);
        mCpuGraph->render (this, mScreenWidth/3.0f, mScreenHeight-35.0f, mScreenWidth/3.0f - 2.0f, 35.0f);
        }
        //}}}
      endSwapFrame();

      mFpsGraph->updateTime (getAbsoluteClock() / 1000000.0f);
      mCpuGraph->updateTime (getAbsoluteClock() / 1000000.0f);

      pollKeyboard();
      }

    cLog::log (LOGNOTICE, "run escaped");
    }
  //}}}

  //{{{
  int getMouse (int* outx, int* outy) {

    struct sMousePacket {
      char buttons;
      char dx;
      char dy;
      };

    if (mMouseFd < 0)
      mMouseFd = open ("/dev/input/mouse0", O_RDONLY | O_NONBLOCK);

    if (mMouseFd >= 0) {
      struct sMousePacket mousePacket;
      while (true) {
        int bytes = read (mMouseFd, &mousePacket, sizeof(mousePacket));
        if (bytes < (int)sizeof(mousePacket))
          // not enough bytes yet
          return mMouseButtons;

        if (mousePacket.buttons & 8)
          break; // This bit should always be set
        read (mMouseFd, &mousePacket, 1); // Try to sync up again
        }

      int mouseX = (mousePacket.buttons & 0x10) ? mousePacket.dx - 256 : mousePacket.dx;
      if (mouseX < 0)
        mouseX = 0;
      else if (mouseX > (int)mScreenWidth)
        mouseX = mScreenWidth;

      int mouseY = - ((mousePacket.buttons & 0x20) ? mousePacket.dy - 256 : mousePacket.dy);
      if (mouseY < 0)
        mouseY = 0;
      else if (mouseY > (int)mScreenHeight)
        mouseY = mScreenHeight;

      mMouseButtons = mousePacket.buttons & 0x03;
      *outx = mouseX;
      *outy = mouseY;
      return mMouseButtons;
      }

    return 0;
    }
  //}}}
  //{{{
  uint64_t getAbsoluteClock() {

    struct timespec now;
    clock_gettime (CLOCK_MONOTONIC, &now);
    return (((uint64_t)now.tv_sec * 1000000000L) + now.tv_nsec) / 1000;
    }
  //}}}
  //{{{
  void uSleep (uint64_t uSec) {

    struct timespec req = { 0 };
    req.tv_sec = uSec / 1000000;
    req.tv_nsec = (uSec % 1000000) * 1000;

    while (nanosleep (&req, &req) == -1 && errno == EINTR && (req.tv_nsec > 0 || req.tv_sec > 0));
    }
  //}}}

  void toggleVsync() { mVsync = !mVsync; setVsync (mVsync); }
  void togglePerf() { mDrawPerf = !mDrawPerf; mFpsGraph->reset(); mCpuGraph->reset(); }
  void toggleStats() { mDrawStats = !mDrawStats; }
  void toggleTests() { mDrawTests = !mDrawTests; }
  virtual void pollKeyboard() = 0;
  //{{{  vars
  bool mEscape = false;
  bool mVsync = true;
  bool mDrawPerf = false;
  bool mDrawStats = false;
  bool mDrawTests = false;
  //}}}

private:
  //{{{
  void setVsync (bool vsync) {
    eglSwapInterval (mEglDisplay, vsync ? 1 : 0);
    }
  //}}}
  //{{{
  void startFrame() {

    mTime = getAbsoluteClock();
    glClear (GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    beginFrame (mScreenWidth, mScreenHeight, (float)mScreenWidth / (float)mScreenWidth);
    }
  //}}}
  //{{{
  void endSwapFrame() {

    endFrame();
    eglSwapBuffers (mEglDisplay, mEglSurface);
    }
  //}}}

  //{{{
  void drawMouse (int x, int y) {

    beginPath();
    ellipse (x, y, 4, 4);

    if (mMouseButtons & 1)
      fillColor (nvgRGBA (0, 200, 200, 255));
    else if (mMouseButtons & 2)
      fillColor (nvgRGBA (200, 200, 0, 255));
    else if (mMouseButtons & 2)
      fillColor (nvgRGBA (0, 200, 128, 192));
    else
      fillColor (nvgRGBA (0, 200, 0, 255));

    fill();
    }
  //}}}
  //{{{
  void drawSpinner (float cx, float cy, float r) {

    float t = mTime / 1000000.0f;
    float a0 = 0.0f + t*6;
    float a1 = PI + t*6;
    float r0 = r;
    float r1 = r * 0.75f;

    saveState();

    beginPath();
    arc (cx,cy, r0, a0, a1, cVg::eHOLE);
    arc (cx,cy, r1, a1, a0, cVg::eSOLID);
    closePath();

    float ax = cx + cosf(a0) * (r0+r1)*0.5f;
    float ay = cy + sinf(a0) * (r0+r1)*0.5f;
    float bx = cx + cosf(a1) * (r0+r1)*0.5f;
    float by = cy + sinf(a1) * (r0+r1)*0.5f;

    auto paint = linearGradient (ax,ay, bx,by, nvgRGBA(0,0,0,0), nvgRGBA(255,255,255,128));
    fillPaint (paint);
    fill();

    restoreState();
    }
  //}}}
  //{{{
  void drawEyes (float x, float y, float w, float h, float cursorX, float cursorY) {

    float t = mTime / 1000000.0f;
    float ex = w *0.23f;
    float ey = h * 0.5f;
    float lx = x + ex;
    float ly = y + ey;
    float rx = x + w - ex;
    float ry = y + ey;
    float dx,dy,d;
    float br = (ex < ey ? ex : ey) * 0.5f;
    float blink = 1 - powf (sinf(t*0.5f),200)*0.8f;

    auto bg = linearGradient (x,y+h*0.5f,x+w*0.1f,y+h, nvgRGBA(0,0,0,32), nvgRGBA(0,0,0,16));
    beginPath();
    ellipse (lx+3.0f,ly+16.0f, ex,ey);
    ellipse (rx+3.0f,ry+16.0f, ex,ey);
    fillPaint (bg);
    fill();

    bg = linearGradient (x,y+h*0.25f,x+w*0.1f,y+h, nvgRGBA(220,220,220,255), nvgRGBA(128,128,128,255));
    beginPath();
    ellipse (lx,ly, ex,ey);
    ellipse (rx,ry, ex,ey);
    fillPaint (bg);
    fill();

    dx = (cursorX - rx) / (ex * 10);
    dy = (cursorY - ry) / (ey * 10);
    d = sqrtf (dx*dx+dy*dy);
    if (d > 1.0f) {
      dx /= d; dy /= d;
      }
    dx *= ex*0.4f;
    dy *= ey*0.5f;

    beginPath();
    ellipse (lx+dx,ly+dy+ey*0.25f*(1-blink), br,br*blink);
    fillColor (nvgRGBA(32,32,32,255));
    fill();

    dx = (cursorX - rx) / (ex * 10);
    dy = (cursorY - ry) / (ey * 10);
    d = sqrtf (dx*dx+dy*dy);
    if (d > 1.0f) {
      dx /= d;
      dy /= d;
      }
    dx *= ex*0.4f;
    dy *= ey*0.5f;

    beginPath();
    ellipse (rx+dx,ry+dy+ey*0.25f*(1-blink), br,br*blink);
    fillColor (nvgRGBA(32,32,32,255));
    fill();

    auto gloss = radialGradient (lx-ex*0.25f,ly-ey*0.5f, ex*0.1f,ex*0.75f, nvgRGBA(255,255,255,128), nvgRGBA(255,255,255,0));
    beginPath();
    ellipse (lx,ly, ex,ey);
    fillPaint (gloss);
    fill();

    gloss = radialGradient (rx-ex*0.25f,ry-ey*0.5f, ex*0.1f,ex*0.75f, nvgRGBA(255,255,255,128), nvgRGBA(255,255,255,0));
    beginPath();
    ellipse (rx,ry, ex,ey);
    fillPaint (gloss);
    fill();
    }
  //}}}
  //{{{
  void drawLines (float x, float y, float w, float h) {

    float t = mTime / 1000000.0f;
    int i, j;
    float pad = 5.0f;
    float s = w/9.0f - pad*2;
    float pts[4*2];
    float fx;
    float fy;

    cVg::eLineCap joins[3] = {cVg::eMITER, cVg::eROUND, cVg::eBEVEL};
    cVg::eLineCap caps[3] = { cVg::eBUTT, cVg::eROUND, cVg::eSQUARE};

    saveState();
    pts[0] = -s*0.25f + cosf(t*0.3f) * s*0.5f;
    pts[1] = sinf(t*0.3f) * s*0.5f;
    pts[2] = -s*0.25f;
    pts[3] = 0;
    pts[4] = s*0.25f;
    pts[5] = 0;
    pts[6] = s*0.25f + cosf(-t*0.3f) * s*0.5f;
    pts[7] = sinf(-t*0.3f) * s*0.5f;

    for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) {
        fx = x + s*0.5f + (i*3+j)/9.0f*w + pad;
        fy = y - s*0.5f + pad;

        lineCap (caps[i]);
        lineJoin (joins[j]);
        strokeWidth (s*0.3f);
        strokeColor (nvgRGBA(0,0,0,160));
        beginPath();
        moveTo (fx+pts[0], fy+pts[1]);
        lineTo (fx+pts[2], fy+pts[3]);
        lineTo (fx+pts[4], fy+pts[5]);
        lineTo (fx+pts[6], fy+pts[7]);
        stroke();

        lineCap (cVg::eBUTT);
        lineJoin (cVg::eBEVEL);
        strokeWidth (1.5f);
        strokeColor (nvgRGBA (0,192,255,255));
        beginPath();
        moveTo (fx+pts[0], fy+pts[1]);
        lineTo (fx+pts[2], fy+pts[3]);
        lineTo (fx+pts[4], fy+pts[5]);
        lineTo (fx+pts[6], fy+pts[7]);
        stroke();
        }
      }
    restoreState();
    }
  //}}}
  //{{{
  void drawStats (float x, float y, std::string str) {

    fontSize (12.0f);
    textAlign (cVg::ALIGN_LEFT | cVg::ALIGN_BOTTOM);
    fillColor (kWhite);
    text (0.0f, y, str);
    }
  //}}}

  //{{{  vars
  DISPMANX_DISPLAY_HANDLE_T mDispmanxDisplay = 0;
  DISPMANX_ELEMENT_HANDLE_T mDispmanxElement = 0;

  EGLDisplay mEglDisplay;
  EGLSurface mEglSurface;
  EGLContext mEglContext;

  EGL_DISPMANX_WINDOW_T mNativeWindow;
  int64_t mTime = 0;

  cPerfGraph* mFpsGraph = nullptr;
  cPerfGraph* mCpuGraph = nullptr;

  cRootContainer* mRoot = nullptr;
  uint32_t mScreenWidth = 0;
  uint32_t mScreenHeight = 0;

  int mMouseFd = -1;
  int mMouseButtons = 0;
  //}}}
  };
