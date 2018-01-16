// cRaspWindow.cpp
//{{{  includes
#include <stdio.h>
#include <fcntl.h>

#include "bcm_host.h"

#include "../utils/utils.h"
#include "../utils/cLog.h"
#include "../fonts/FreeSansBold.h"
#include "../fonts/DroidSansMono1.h"

#include "cRaspWindow.h"

using namespace std;
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

// public:
//{{{
cRaspWindow::cRaspWindow() {
  bcm_host_init();

  mMouseFd = open ("/dev/input/mouse0", O_RDONLY | O_NONBLOCK);
  if (mMouseFd < 0) {
    cLog::log (LOGERROR, "no mouse");
    return;
    }

  }
//}}}
//{{{
cRaspWindow::~cRaspWindow() {

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

// iDraw
//{{{
void cRaspWindow::drawRect (uint32_t colour, int16_t x, int16_t y, uint16_t width, uint16_t height) {

  beginPath();
  rect (x, y, width, height);
  fillColor (nvgRGBA ((colour & 0xFF0000) >> 16, (colour & 0xFF00) >> 8, colour & 0xFF,255));
  triangleFill();
  }
//}}}
//{{{
int cRaspWindow::drawText (uint32_t colour, uint16_t fontHeight, std::string str, int16_t x, int16_t y, uint16_t width, uint16_t height) {

  fontSize ((float)fontHeight);
  textAlign (cVg::ALIGN_LEFT | cVg::ALIGN_TOP);
  fillColor (nvgRGBA ((colour & 0xFF0000) >> 16, (colour & 0xFF00) >> 8, colour & 0xFF,255));
  return (int)text ((float)x+3, (float)y+1, str);
  }
//}}}
//{{{
void cRaspWindow::ellipseSolid (uint32_t colour, int16_t x, int16_t y, uint16_t xradius, uint16_t yradius) {

  beginPath();
  ellipse (x, y, xradius, yradius);
  fillColor (nvgRGBA ((colour & 0xFF0000) >> 16, (colour & 0xFF00) >> 8, colour & 0xFF,255));
  fill();
  }
//}}}

// protected
//{{{
cRootContainer* cRaspWindow::initialise (float scale, uint32_t alpha) {

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
  mMouseX = mScreenWidth / 2;
  mMouseY = mScreenHeight / 2;

  mDispmanxDisplay = vc_dispmanx_display_open (0);
  DISPMANX_UPDATE_HANDLE_T dispmanxUpdate = vc_dispmanx_update_start (0);

  // create dispmanx element
  VC_RECT_T dstRect = { 0, 0, (int32_t)mScreenWidth, (int32_t)mScreenHeight };
  VC_RECT_T srcRect = { 0, 0, (int32_t)(mScreenWidth << 16), (int32_t)(mScreenHeight << 16) };

  VC_DISPMANX_ALPHA_T alphaSpec;
  if (alpha)
    alphaSpec = { DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS, alpha, 0 };
  else
    alphaSpec = { DISPMANX_FLAGS_ALPHA_FROM_SOURCE, 128, 0 };
  mDispmanxElement = vc_dispmanx_element_add (dispmanxUpdate, mDispmanxDisplay, 3000,
                                              &dstRect, 0, &srcRect,
                                              DISPMANX_PROTECTION_NONE, &alphaSpec, NULL,
                                              DISPMANX_NO_ROTATE);
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

  //createFontMem ("sans", (unsigned char*)freeSansBold, sizeof(freeSansBold), 0);
  createFontMem ("sans", (unsigned char*)droidSansMono, sizeof(droidSansMono), 0);
  fontFace ("sans");
  cLog::log (LOGINFO, "created sans font droidSansMono");

  glViewport (0.f, 0.f, mScreenWidth, mScreenHeight);
  glClearColor (0.f, 0.f, 0.f, 0.f);

  mRoot = new cRootContainer (mScreenWidth, mScreenHeight);
  return mRoot;
  }
//}}}
//{{{
void cRaspWindow::run() {

  while (!mExit) {
    if (mCountDown > 0) {
      mCountDown--;
      usleep (10000);
      }
    else {
      mCountDown = mChangeCountDown;
      mCpuGraph->start (getAbsoluteClock() / 1000000.f);
      startFrame();
      if (mRoot)
        mRoot->onDraw (this);
      drawMouse (mMouseX, mMouseY);
      if (mDrawTests) {
        //{{{  draw tests
        drawEyes (mScreenWidth*3.f/4.f, mScreenHeight/2.f, mScreenWidth/4.f, mScreenHeight/2.f,
                  mMouseX, mMouseY);
        //drawLines (0.f, 50.f, mScreenWidth, mScreenHeight);
        drawSpinner (mScreenWidth/2.f, mScreenHeight/2.f, 20.f);
        }
        //}}}
      if (mDrawStats)
        drawStats (mScreenWidth, mScreenHeight, getFrameStats() + (mVsync ? " vsync" : " free"));
      if (mDrawPerf) {
        //{{{  render perf stats
        mFpsGraph->render (this, 0.f, mScreenHeight-35.f, mScreenWidth/3.f -2.f, 35.f);
        mCpuGraph->render (this, mScreenWidth/3.f, mScreenHeight-35.f, mScreenWidth/3.f - 2.f, 35.f);
        }
        //}}}
      endSwapFrame();
      mFpsGraph->updateTime (getAbsoluteClock() / 1000000.f);
      mCpuGraph->updateTime (getAbsoluteClock() / 1000000.f);
      }

    pollKeyboard();
    if (mMouseFd >= 0)
      pollMouse();
    }

  cLog::log (LOGNOTICE, "cRaspWindow::run - exit");
  }
//}}}

//{{{
uint64_t cRaspWindow::getAbsoluteClock() {

  struct timespec now;
  clock_gettime (CLOCK_MONOTONIC, &now);
  return (((uint64_t)now.tv_sec * 1000000000L) + now.tv_nsec) / 1000;
  }
//}}}
//{{{
void cRaspWindow::uSleep (uint64_t uSec) {

  struct timespec req = { 0 };
  req.tv_sec = uSec / 1000000;
  req.tv_nsec = (uSec % 1000000) * 1000;

  while (nanosleep (&req, &req) == -1 && errno == EINTR && (req.tv_nsec > 0 || req.tv_sec > 0));
  }
//}}}

// private
//{{{
void cRaspWindow::pollMouse() {

  uint8_t packet[3];
  while (true) {
    int bytes = read (mMouseFd, packet, 3);
    if (bytes != 3)
      return;
    if (packet[0] & 8)
      break;
    read (mMouseFd, packet, 1);
    }

  mMouseX += (packet[0] & 0x10) ? packet[1]-0x100 : packet[1];
  mMouseX = max(0, min((int)mScreenWidth, mMouseX));

  mMouseY -= (packet[0] & 0x20) ? packet[2]-0x100 : packet[2];
  mMouseY = max(0, min((int)mScreenHeight, mMouseY));

  mMouseButtons = packet[0] & 0x03;

  cLog::log (LOGINFO3, "pollMouse x:%d y:%d buttons:%x", mMouseX, mMouseY, mMouseButtons);
  }
//}}}

//{{{
void cRaspWindow::setVsync (bool vsync) {
  eglSwapInterval (mEglDisplay, vsync ? 1 : 0);
  }
//}}}
//{{{
void cRaspWindow::startFrame() {

  mTime = getAbsoluteClock();
  glClear (GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  beginFrame (mScreenWidth, mScreenHeight, (float)mScreenWidth / (float)mScreenWidth);
  }
//}}}
//{{{
void cRaspWindow::endSwapFrame() {

  endFrame();
  eglSwapBuffers (mEglDisplay, mEglSurface);
  }
//}}}

//{{{
void cRaspWindow::drawMouse (int x, int y) {

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
void cRaspWindow::drawSpinner (float cx, float cy, float r) {

  float t = mTime / 1000000.f;
  float a0 = 0.f + t*6;
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
void cRaspWindow::drawEyes (float x, float y, float w, float h, float cursorX, float cursorY) {

  float t = mTime / 1000000.f;
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
  ellipse (lx+3.f, ly+16.f, ex,ey);
  ellipse (rx+3.f, ry+16.f, ex,ey);
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
  if (d > 1.f) {
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
  if (d > 1.f) {
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
void cRaspWindow::drawLines (float x, float y, float w, float h) {

  float t = mTime / 1000000.f;
  int i, j;
  float pad = 5.f;
  float s = w/9.f - pad*2;
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
      fx = x + s*0.5f + (i*3+j)/9.f*w + pad;
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
void cRaspWindow::drawStats (float x, float y, std::string str) {

  fontSize (12.f);
  textAlign (cVg::ALIGN_LEFT | cVg::ALIGN_BOTTOM);
  fillColor (kWhite);
  text (0.f, y, str);
  }
//}}}
