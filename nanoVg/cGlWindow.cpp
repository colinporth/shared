// cGlWindow.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define WIN32_LEAN_AND_MEAN

#include "../utils/utils.h"
#include "../utils/cLog.h"

#include "cGlWindow.h"

using namespace std;
//}}}

//{{{
cGlWindow::~cGlWindow() {
  glfwTerminate();
  }
//}}}

//{{{  static var inits
cGlWindow* cGlWindow::mGlWindow = NULL;

bool cGlWindow::mMouseDown = false;
bool cGlWindow::mMouseMoved = false;
float cGlWindow::mMouseX = 0;
float cGlWindow::mMouseY = 0;
int cGlWindow::mMouseIntX = 0;
int cGlWindow::mMouseIntY = 0;

bool cGlWindow::mAlted = false;
bool cGlWindow::mSupered = false;
bool cGlWindow::mShifted = false;
bool cGlWindow::mControlled = false;
//}}}

//{{{  iWindow
cVg* cGlWindow::getContext() { return this; }
uint16_t cGlWindow::getWidthPix() { return mRoot->getPixWidth(); }
uint16_t cGlWindow::getHeightPix() { return mRoot->getPixHeight(); }
//}}}
//{{{  iDraw
//{{{
void cGlWindow::pixel (uint32_t colour, int16_t x, int16_t y) {
  rectClipped (colour, x, y, 1, 1);
  }
//}}}
//{{{
void cGlWindow::drawRect (uint32_t colour, int16_t x, int16_t y, uint16_t width, uint16_t height) {
  beginPath();
  rect (x, y, width, height);
  fillColor (nvgRGBA ((colour & 0xFF0000) >> 16, (colour & 0xFF00) >> 8, colour & 0xFF,255));
  triangleFill();
  }
//}}}
//{{{
void cGlWindow::stamp (uint32_t colour, uint8_t* src, int16_t x, int16_t y, uint16_t width, uint16_t height) {
  //rect (0xC0000000 | (colour & 0xFFFFFF), x,y, width, height);
  }
//}}}
//{{{
int cGlWindow::drawText (uint32_t colour, uint16_t fontHeight, string str, int16_t x, int16_t y, uint16_t width, uint16_t height) {

  fontSize ((float)fontHeight);
  textAlign (cVg::ALIGN_LEFT | cVg::ALIGN_TOP);
  fillColor (nvgRGBA ((colour & 0xFF0000) >> 16, (colour & 0xFF00) >> 8, colour & 0xFF,255));
  text ((float)x+3, (float)y+1, str);

  // get width
  return (int)textBounds ((float)x+3, (float)y+1, str, nullptr);
  }
//}}}
//{{{
void cGlWindow::ellipseSolid (uint32_t colour, int16_t x, int16_t y, uint16_t xradius, uint16_t yradius) {
  beginPath();
  ellipse (x, y, xradius, yradius);
  fillColor (nvgRGBA ((colour & 0xFF0000) >> 16, (colour & 0xFF00) >> 8, colour & 0xFF,255));
  fill();
  }
//}}}
//}}}

// protected
//{{{
cRootContainer* cGlWindow::initialise (string title, int width, int height, unsigned char* sansFont) {

  mGlWindow = this;

  if (!glfwInit()) {
    //{{{  error, return
    cLog::log (LOGERROR, "Failed to init GLFW");
    return nullptr;
    }
    //}}}
  glfwSetErrorCallback (errorcb);

  // glfw hints
#ifdef NANOVG_GL3
  glfwWindowHint (GLFW_OPENGL_DEBUG_CONTEXT, 1);
  glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
#elseif NANOVG_GL2
  glfwWindowHint (GLFW_OPENGL_DEBUG_CONTEXT, 1);
  glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 0);
#elseif NANOVG_GLES3
  glfwWindowHint (GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 0);
#else NANOVG_GLE23
  glfwWindowHint (GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

  mWindow = glfwCreateWindow (width, height, title.c_str(), NULL, NULL);
  if (!mWindow) {
    //{{{  error, return
    cLog::log (LOGERROR, "failed to create Glfw window " + dec(width) + "x" + dec(height) + " " + title);
    glfwTerminate();
    return nullptr;
    }
    //}}}

  glfwMakeContextCurrent (mWindow);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

  glfwSetKeyCallback (mWindow, glfwKey);
  glfwSetCharModsCallback (mWindow, glfwCharMods);
  glfwSetCursorPosCallback (mWindow, glfwCursorPos);
  glfwSetMouseButtonCallback (mWindow, glfwMouseButton);
  glfwSetScrollCallback (mWindow, glfMouseScroll);

  glfwSwapInterval (1);

  cVgGL::initialise();
  createFontMem ("sans", (unsigned char*)sansFont, sizeof(sansFont), 0);
  fontFace ("sans");

  mRoot = new cRootContainer (width, height);

  // init timers
  glfwSetTime (0);
  mCpuGraph = new cPerfGraph (cPerfGraph::GRAPH_RENDER_MS, "cpu");
  mFpsGraph = new cPerfGraph (cPerfGraph::GRAPH_RENDER_FPS, "frame");

  return mRoot;
  }
//}}}
//{{{
void cGlWindow::run () {

  while (!glfwWindowShouldClose (mWindow)) {
    mCpuGraph->start ((float)glfwGetTime());

    // Update and render
    int winWidth, winHeight;
    glfwGetWindowSize (mWindow, &winWidth, &winHeight);
    int frameBufferWidth, frameBufferHeight;
    glfwGetFramebufferSize (mWindow, &frameBufferWidth, &frameBufferHeight);

    glViewport (0, 0, frameBufferWidth, frameBufferHeight);
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    beginFrame (winWidth, winHeight, (float)frameBufferWidth / (float)winWidth);
    if (mRoot)
      mRoot->onDraw (this);
    if (mDrawTests) {
      //{{{  draw
      drawEyes (winWidth*3.0f/4.0f, winHeight/2.0f, winWidth/4.0f, winHeight/2.0f,
                mMouseX, mMouseY, (float)glfwGetTime());
      drawLines (0.0f, 50.0f, (float)winWidth, (float)winHeight, (float)glfwGetTime());
      drawSpinner (winWidth/2.0f, winHeight/2.0f, 20.0f, (float)glfwGetTime());
      }
      //}}}
    if (mDrawStats)
      drawStats ((float)winWidth, (float)winHeight, getFrameStats() + (mVsync ? " vsync" : " free"));
    if (mDrawPerf) {
      //{{{  render perf stats
      mFpsGraph->render (this, 0.0f, winHeight-35.0f, winWidth/3.0f -2.0f, 35.0f);
      mCpuGraph->render (this, winWidth/3.0f, winHeight-35.0f, winWidth/3.0f - 2.0f, 35.0f);
      }
      //}}}
    endFrame();
    glfwSwapBuffers (mWindow);

    mCpuGraph->updateTime ((float)glfwGetTime());
    mFpsGraph->updateTime ((float)glfwGetTime());

    glfwPollEvents();
    }
  }
//}}}

//{{{
void cGlWindow::toggleVsync() {
  mVsync = !mVsync;
  glfwSwapInterval (mVsync ? 1 : 0);
  }
//}}}
//{{{
void cGlWindow::togglePerf() {

  mDrawPerf = !mDrawPerf;

  mFpsGraph->reset();
  mCpuGraph->reset();
  }
//}}}
//{{{
void cGlWindow::toggleStats() {
  mDrawStats = !mDrawStats;
  }
//}}}
//{{{
void cGlWindow::toggleTests() {
  mDrawTests = !mDrawTests;
  }
//}}}

// private
//{{{
void cGlWindow::onProx (bool inClient, int x, int y) {
  mMouseX = (float)x;
  mMouseY = (float)y;
  }
//}}}
//{{{
void cGlWindow::onDown (bool right, int x, int y) {
  mRoot->onDown (0, x, y, 0,  0, 0);
  }
//}}}
//{{{
void cGlWindow::onMove (bool right, int x, int y, int xInc, int yInc) {
  mMouseX = (float)x;
  mMouseY = (float)y;
  mRoot->onDown (1, x, y, 0, xInc, yInc);
  }
//}}}
//{{{
void cGlWindow::onUp (bool right, bool mouseMoved, int x, int y) {
  mRoot->onUp();
  }
//}}}
//{{{
void cGlWindow::onWheel (int delta) {
  }
//}}}

//{{{
void cGlWindow::drawSpinner (float cx, float cy, float r, float t) {

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
  fillPaint ( paint);
  fill();

  restoreState();
  }
//}}}
//{{{
void cGlWindow::drawEyes (float x, float y, float w, float h, float cursorX, float cursorY, float t) {

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
void cGlWindow::drawLines (float x, float y, float w, float h, float t) {

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
void cGlWindow::drawStats (float x, float y, string str) {

  fontSize (12.0f);
  textAlign (cVg::ALIGN_LEFT | cVg::ALIGN_BOTTOM);
  fillColor (nvgRGBA(255, 255, 255, 255));
  text (0.0f, y, str);
  }
//}}}

//{{{
void cGlWindow::glfwKey (struct GLFWwindow* window, int key, int scancode, int action, int mods) {

  if ((key == GLFW_KEY_LEFT_SHIFT) && (action == GLFW_PRESS))
    mShifted = true;
  else if ((key == GLFW_KEY_LEFT_SHIFT) && (action == GLFW_RELEASE))
    mShifted = false;
  else if ((key == GLFW_KEY_RIGHT_SHIFT) && (action == GLFW_PRESS))
    mShifted = true;
  else if ((key == GLFW_KEY_RIGHT_SHIFT) && (action == GLFW_RELEASE))
    mShifted = false;
  else if ((key == GLFW_KEY_LEFT_CONTROL) && (action == GLFW_PRESS))
    mControlled = true;
  else if ((key == GLFW_KEY_LEFT_CONTROL) && (action == GLFW_RELEASE))
    mControlled = false;
  else if ((key == GLFW_KEY_RIGHT_CONTROL) && (action == GLFW_PRESS))
    mControlled = true;
  else if ((key == GLFW_KEY_RIGHT_CONTROL) && (action == GLFW_RELEASE))
    mControlled = false;
  else if ((key == GLFW_KEY_LEFT_ALT) && (action == GLFW_PRESS))
    mAlted = true;
  else if ((key == GLFW_KEY_LEFT_ALT) && (action == GLFW_RELEASE))
    mAlted = false;
  else if ((key == GLFW_KEY_RIGHT_ALT) && (action == GLFW_PRESS))
    mAlted = true;
  else if ((key == GLFW_KEY_RIGHT_ALT) && (action == GLFW_RELEASE))
    mAlted = false;
  else if ((key == GLFW_KEY_LEFT_SUPER) && (action == GLFW_PRESS))
    mSupered = true;
  else if ((key == GLFW_KEY_LEFT_SUPER) && (action == GLFW_RELEASE))
    mSupered = false;
  else if ((key == GLFW_KEY_RIGHT_SUPER) && (action == GLFW_PRESS))
    mSupered = true;
  else if ((key == GLFW_KEY_RIGHT_SUPER) && (action == GLFW_RELEASE))
    mSupered = false;

  mGlWindow->onKey (key, scancode, action, mods);
  }
//}}}
//{{{
void cGlWindow::glfwCharMods (struct GLFWwindow* window, unsigned int ch, int mods) {
  mGlWindow->onChar (ch, mods);
  }
//}}}
//{{{
void cGlWindow::glfwCursorPos (struct GLFWwindow* window, double xpos, double ypos) {
  if (mMouseDown) {
    int xinc = int(xpos) - mMouseIntX;
    int yinc = int(ypos) - mMouseIntY;
    if ((xinc != 0) || (yinc != 0)) {
      mMouseMoved = true;
      mGlWindow->onMove (false, int(xpos), int(ypos), xinc, yinc);
      mMouseIntX = int(xpos);
      mMouseIntY = int(ypos);
      }
    }
  else
    mGlWindow->onProx (true, int(xpos), int(ypos));

  mMouseX = float(xpos);
  mMouseY = float(ypos);
  }
//}}}
//{{{
void cGlWindow::glfwMouseButton (struct GLFWwindow* window, int button, int action, int mods) {

  if (action == GLFW_PRESS) {
    mMouseMoved = false;
    mGlWindow->onDown (button == GLFW_MOUSE_BUTTON_RIGHT, (int)mMouseX, (int)mMouseY);
    mMouseDown = true;
    }

  else if (action == GLFW_RELEASE) {
    mGlWindow->onUp (button == GLFW_MOUSE_BUTTON_RIGHT, mMouseMoved, (int)mMouseX, (int)mMouseY);
    mMouseDown = false;
    mMouseMoved = false;
    }

  }
//}}}
//{{{
void cGlWindow::glfMouseScroll (struct GLFWwindow* window,  double xoffset, double yoffset) {
  mGlWindow->onWheel (int (yoffset));
  }
//}}}

//{{{
void cGlWindow::errorcb (int error, const char* desc) {
  cLog::log (LOGERROR, "GLFW error %d: %s\n", error, desc);
  }
//}}}
