// cGlWindow.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#include "cGlWindow.h"

#include "../utils/utils.h"
#include "../utils/cLog.h"

using namespace std;
//}}}

//{{{
cGlWindow::~cGlWindow() {
  glfwTerminate();
  }
//}}}

// iWindow
cVg* cGlWindow::getContext() { return this; }
float cGlWindow::getWidthPix() { return mRootContainer->getPixWidth(); }
float cGlWindow::getHeightPix() { return mRootContainer->getPixHeight(); }

// iDraw
//{{{
void cGlWindow::drawRect (uint32_t colour, float x, float y, float width, float height) {

  fillColor (nvgRGBA32 (colour));

  beginPath();
  rect (x, y, width, height);
  triangleFill();
  }
//}}}
//{{{
float cGlWindow::drawText (uint32_t colour, float fontHeight, string str, float x, float y, float width, float height) {

  setFontSize (fontHeight);
  setTextAlign (cVg::ALIGN_LEFT | cVg::ALIGN_TOP);
  fillColor (nvgRGBA32 (colour));
  text (x+3.f, y+1.f, str);

  // get width
  float bounds[4];
  return getTextBounds (x+3.f, y+1.f, str, bounds);
  }
//}}}
//{{{
float cGlWindow::drawTextRight (uint32_t colour, float fontHeight, string str, float x, float y, float width, float height) {

  setFontSize (fontHeight);
  setTextAlign (cVg::ALIGN_RIGHT | cVg::ALIGN_TOP);
  fillColor (nvgRGBA32 (colour));
  text (x, y+1.f, str);

  // get width
  return getTextBounds (x, y+1.f, str, nullptr);
  }
//}}}
//{{{
void cGlWindow::drawEllipseSolid (uint32_t colour, float x, float y, float xradius, float yradius) {

  fillColor (nvgRGBA32 (colour));

  beginPath();
  ellipse (x, y, xradius, yradius);
  fill();
  }
//}}}

// protected
//{{{
cRootContainer* cGlWindow::initialise (const string& title, int width, int height,
                                       unsigned char* fontData, int fontDataSize) {

  mGlWindow = this;

  glfwSetErrorCallback (errorCallback);
  if (!glfwInit()) {
    //{{{  error, return
    cLog::log (LOGERROR, "Failed to init GLFW");
    return nullptr;
    }
    //}}}

  // glfw hints
  glfwWindowHint (GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 0);

  mWindow = glfwCreateWindow (width, height, title.c_str(), NULL, NULL);
  if (!mWindow) {
    //{{{  error, return
    cLog::log (LOGERROR, "failed to create Glfw window " + dec(width) + "x" + dec(height) + " " + title);
    glfwTerminate();
    return nullptr;
    }
    //}}}

  // setup callbacks
  glfwMakeContextCurrent (mWindow);
  gladLoadGLLoader ((GLADloadproc)glfwGetProcAddress);

  glfwSetKeyCallback (mWindow, glfwKey);
  glfwSetCharModsCallback (mWindow, glfwCharMods);

  glfwSetCursorPosCallback (mWindow, glfwCursorPos);
  glfwSetMouseButtonCallback (mWindow, glfwMouseButton);
  glfwSetScrollCallback (mWindow, glfMouseScroll);

  glfwSetWindowPosCallback (mWindow, glfWindowPos);
  glfwSetWindowSizeCallback (mWindow, glfWindowSize);

  glfwSwapInterval (1);

  cVg::initialise();
  createFont ("sans", (unsigned char*)fontData, fontDataSize);
  setFontByName ("sans");

  mRootContainer = new cRootContainer (width, height);

  // init timers
  glfwSetTime (0);
  mCpuGraph = new cPerfGraph (cPerfGraph::GRAPH_RENDER_MS, "cpu");
  mFpsGraph = new cPerfGraph (cPerfGraph::GRAPH_RENDER_FPS, "frame");

  return mRootContainer;
  }
//}}}
//{{{
void cGlWindow::run() {
// runs in app main thread after initialising everything

  while (!glfwWindowShouldClose (mWindow)) {
    glfwPollEvents();
    draw();
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

  mFpsGraph->clear();
  mCpuGraph->clear();
  }
//}}}

// private
//{{{
void cGlWindow::draw() {

  mCpuGraph->start ((float)glfwGetTime());

  // Update and render
  int winWidth;
  int winHeight;
  glfwGetWindowSize (mWindow, &winWidth, &winHeight);

  int frameBufferWidth;
  int frameBufferHeight;
  glfwGetFramebufferSize (mWindow, &frameBufferWidth, &frameBufferHeight);

  glViewport (0, 0, frameBufferWidth, frameBufferHeight);
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  beginFrame (winWidth, winHeight, (float)frameBufferWidth / (float)winWidth);

  if (mRootContainer)
    mRootContainer->onDraw (this);

  if (mDrawTests) {
    //{{{  draw tests
    drawEyes (winWidth*3.0f/4.0f, winHeight/2.0f, winWidth/4.0f, winHeight/2.0f,
              mMouseX, mMouseY, (float)glfwGetTime());
    drawLines (0.0f, 50.0f, (float)winWidth, (float)winHeight, (float)glfwGetTime());
    drawSpinner (winWidth/2.0f, winHeight/2.0f, 20.0f, (float)glfwGetTime(),
                 nvgRGBA(0, 0, 0, 0), nvgRGBA(255, 255, 255, 128));
    }
    //}}}

  if (mDrawPerf) {
    //{{{  draw perf stats
    mFpsGraph->render (this, 0.f, winHeight-35.f, winWidth/2.f -2.f, 35.f);
    mCpuGraph->render (this, winWidth/2.f, winHeight-35.f, winWidth/2.f - 2.f, 35.f);
    }
    //}}}

  if (mDrawStats) {
    //{{{  draw stats
    setFontSize (12.0f);
    setTextAlign (cVg::ALIGN_LEFT | cVg::ALIGN_BOTTOM);
    fillColor (nvgRGBA (255, 255, 255, 255));
    text (0.0f, (float)winHeight, getFrameStats() + (mVsync ? " vsyncOn" : " vsyncOff"));
    }
    //}}}

  endFrame();
  glfwSwapBuffers (mWindow);

  mCpuGraph->updateTime ((float)glfwGetTime());
  mFpsGraph->updateTime ((float)glfwGetTime());
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

// static keyboard - route to mRootContainer
//{{{
void cGlWindow::glfwKey (GLFWwindow* window, int key, int scancode, int action, int mods) {

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
void cGlWindow::glfwCharMods (GLFWwindow* window, unsigned int ch, int mods) {
  mGlWindow->onChar (ch, mods);
  }
//}}}

// static mouse - route to mRootContainer
//{{{
void cGlWindow::glfwMouseButton (GLFWwindow* window, int button, int action, int mods) {

  if (action == GLFW_PRESS) {
    mMouseDown = true;
    mMouseMoved = false;
    mMouseRightButton = button == GLFW_MOUSE_BUTTON_RIGHT;
    mRootContainer->onDown (mMouseX, mMouseY);
    mMouseLastX = mMouseX;
    mMouseLastY = mMouseY;
    }

  else if (action == GLFW_RELEASE) {
    mMouseRightButton = button == GLFW_MOUSE_BUTTON_RIGHT;
    mRootContainer->onUp();
    mMouseMoved = false;
    mMouseDown = false;
    }
  }
//}}}
//{{{
void cGlWindow::glfwCursorPos (GLFWwindow* window, double xpos, double ypos) {

  mMouseX = (float)xpos;
  mMouseY = (float)ypos;

  if (mMouseDown) {
    float xinc = mMouseX - mMouseLastX;
    float yinc = mMouseY - mMouseLastY;
    if ((fabs(xinc) >  0.5f) || (fabs(yinc) > 0.5f)) {
      mMouseMoved = true;
      mRootContainer->onMove (mMouseX, mMouseY, xinc, yinc);
      mMouseLastX = mMouseX;
      mMouseLastY = mMouseY;
      }
    }
  else
    mRootContainer->onProx (mMouseX, mMouseY);
  }
//}}}
//{{{
void cGlWindow::glfMouseScroll (GLFWwindow* window, double xoffset, double yoffset) {
  mRootContainer->onWheel (float (yoffset));
  }
//}}}
//{{{
void cGlWindow::glfWindowPos (GLFWwindow* window, int xsize, int ysize) {
  mGlWindow->draw();
  }
//}}}
//{{{
void cGlWindow::glfWindowSize (GLFWwindow* window, int xsize, int ysize) {
  mRootContainer->layout ((float)xsize, (float)ysize);
  mGlWindow->draw();
  }
//}}}

//{{{
void cGlWindow::errorCallback (int error, const char* desc) {
  cLog::log (LOGERROR, "GLFW error %d: %s\n", error, desc);
  }
//}}}
