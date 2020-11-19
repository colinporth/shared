// cGlWindow.cpp
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define WIN32_LEAN_AND_MEAN
  #define NOMINMAX
  #include <windows.h>
#endif

#include "cGlWindow.h"

#include "../utils/utils.h"
#include "../utils/cLog.h"

#include "../widgets/cRootContainer.h"

using namespace std;
using namespace chrono;
//}}}

cGlWindow::cGlWindow() {}
//{{{
cGlWindow::~cGlWindow() {
  glfwTerminate();
  }
//}}}

// iClockTime
//{{{
int cGlWindow::getDayLightSeconds() {
  return mDayLightSeconds;
  }
//}}}
//{{{
system_clock::time_point cGlWindow::getNowRaw() {
  return system_clock::now();
  }
//}}}
//{{{
system_clock::time_point cGlWindow::getNowDayLight() {
  return system_clock::now() + seconds (mDayLightSeconds);
  }
//}}}

// iDraw
//{{{
void cGlWindow::drawRect (const sColourF& colour, cPointF point, cPointF size) {

  setFillColour (colour);

  beginPath();
  rect (point, size);
  triangleFill();
  }
//}}}
//{{{
float cGlWindow::drawText (const sColourF& colour, float fontHeight, string str, cPointF point, cPointF size) {

  setFontSize (fontHeight);
  setTextAlign (cVg::eAlignLeft | cVg::eAlignTop);
  setFillColour (sColourF(colour));
  text (point + cPointF (3.f, 1.f), str);

  // get width
  float bounds[4];
  return getTextBounds (point + cPointF(3.f, 1.f), str, bounds);
  }
//}}}
//{{{
float cGlWindow::drawTextRight (const sColourF& colour, float fontHeight, string str, cPointF point, cPointF size) {

  setFontSize (fontHeight);
  setTextAlign (cVg::eAlignRight | cVg::eAlignTop);
  setFillColour (sColourF(colour));
  text (point + cPointF (0.f, 1.f), str);

  // get width
  float bounds[4];
  return getTextBounds (point + cPointF (0.f,1.f), str, bounds);
  }
//}}}
//{{{
void cGlWindow::drawEllipseSolid (const sColourF& colour, cPointF point, cPointF radius) {

  setFillColour (sColourF(colour));

  beginPath();
  ellipse (point, radius);
  fill();
  }
//}}}

// iWindow
cVg* cGlWindow::getVg() { return this; }
cPointF cGlWindow::getSize() { return mRootContainer->getSize(); }

// protected
//{{{
cRootContainer* cGlWindow::initialiseGui (const string& title, int width, int height, uint8_t* font, int fontSize) {

  mGlWindow = this;

  #ifdef _WIN32
    TIME_ZONE_INFORMATION timeZoneInfo;
    if (GetTimeZoneInformation (&timeZoneInfo) == TIME_ZONE_ID_DAYLIGHT)
      mDayLightSeconds = -timeZoneInfo.DaylightBias * 60;
  #endif

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

  mMonitor = glfwGetPrimaryMonitor();
  glfwGetWindowSize (mWindow, &mWindowSize[0], &mWindowSize[1]);
  glfwGetWindowPos (mWindow, &mWindowPos[0], &mWindowPos[1]);

  // setup callbacks
  glfwMakeContextCurrent (mWindow);
  gladLoadGLLoader ((GLADloadproc)glfwGetProcAddress);

  glfwSetWindowUserPointer (mWindow, this);

  glfwSetKeyCallback (mWindow, glfwKey);
  glfwSetCharModsCallback (mWindow, glfwCharMods);

  glfwSetCursorPosCallback (mWindow, glfwCursorPos);
  glfwSetMouseButtonCallback (mWindow, glfwMouseButton);
  glfwSetScrollCallback (mWindow, glfMouseScroll);

  glfwSetWindowPosCallback (mWindow, glfWindowPos);
  glfwSetWindowSizeCallback (mWindow, glfWindowSize);

  glfwSwapInterval (1);

  cVg::initialise();
  createFont ("sans", (unsigned char*)font, fontSize);
  setFontByName ("sans");

  mRootContainer = new cRootContainer (float(width), float(height));

  // init timers
  glfwSetTime (0);
  mCpuGraph = new cPerfGraph (cPerfGraph::eRenderMs, "cpu");
  mFpsGraph = new cPerfGraph (cPerfGraph::eRenderFps, "frame");

  return mRootContainer;
  }
//}}}
cWidget* cGlWindow::addTopLeft (cWidget* widget) { return mRootContainer->addTopLeft (widget); }
cWidget* cGlWindow::add (cWidget* widget) { return mRootContainer->add (widget); }
cWidget* cGlWindow::add (cWidget* widget, float offset) { return mRootContainer->add (widget, offset); }
cWidget* cGlWindow::add (cWidget* widget, cPointF point) { return mRootContainer->add (widget, point); }
cWidget* cGlWindow::addBelowLeft (cWidget* widget) { return mRootContainer->addBelowLeft (widget); }
//{{{
void cGlWindow::runGui (bool clear) {
// usually runs in app main thread

  updateWindowSize();

  glClearColor (0, 0, 0, 1.f);
  while (!glfwWindowShouldClose (mWindow)) {
    glfwPollEvents();
    draw (clear);
    }
  }
//}}}

// actions
//{{{
void cGlWindow::toggleFullScreen() {

  if (glfwGetWindowMonitor (mWindow) == nullptr) {
    // backup window position and window size
    glfwGetWindowPos (mWindow, &mWindowPos[0], &mWindowPos[1]);
    glfwGetWindowSize (mWindow, &mWindowSize[0], &mWindowSize[1]);

    // get resolution of monitor
    const GLFWvidmode* mode = glfwGetVideoMode (glfwGetPrimaryMonitor());

    // switch to full screen
    glfwSetWindowMonitor  (mWindow, mMonitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
  else
    // restore last window size and position
    glfwSetWindowMonitor (mWindow, nullptr, mWindowPos[0], mWindowPos[1], mWindowSize[0], mWindowSize[1], 0);
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

// private static callbacks
//{{{
void cGlWindow::glfWindowPos (GLFWwindow* window, int xsize, int ysize) {

  mGlWindow->draw (true);
  }
//}}}
//{{{
void cGlWindow::glfWindowSize (GLFWwindow* window, int xsize, int ysize) {

  mGlWindow->updateWindowSize();
  mGlWindow->draw (true);
  }
//}}}
//{{{
void cGlWindow::errorCallback (int error, const char* desc) {
  cLog::log (LOGERROR, "GLFW error %d: %s\n", error, desc);
  }
//}}}
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
//{{{
void cGlWindow::glfwMouseButton (GLFWwindow* window, int button, int action, int mods) {

  if (action == GLFW_PRESS) {
    mMouseDown = true;
    mMouseMoved = false;
    mMouseRightButton = button == GLFW_MOUSE_BUTTON_RIGHT;
    mRootContainer->onDown (mMousePos);
    mMouseLastPos = mMousePos;
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

  mMousePos = { (float)xpos, (float)ypos };

  if (mMouseDown) {
    cPointF inc = mMousePos - mMouseLastPos;
    if ((fabs(inc.x) > 0.5f) || (fabs(inc.y) > 0.5f)) {
      mMouseMoved = true;
      mRootContainer->onMove (mMousePos, inc);
      mMouseLastPos = mMousePos;
      }
    }
  else
    mRootContainer->onProx (mMousePos);
  }
//}}}
//{{{
void cGlWindow::glfMouseScroll (GLFWwindow* window, double xoffset, double yoffset) {
  mRootContainer->onWheel (float (yoffset));
  }
//}}}

// private members
//{{{
void cGlWindow::updateWindowSize() {

  glfwGetWindowSize (mWindow, &mWinWidth, &mWinHeight);

  mWinSize = { (float)mWinWidth, (float)mWinHeight };
  mRootContainer->setSize (mWinSize);

  int frameBufferWidth;
  int frameBufferHeight;
  glfwGetFramebufferSize (mWindow, &frameBufferWidth, &frameBufferHeight);
  glViewport (0, 0, frameBufferWidth, frameBufferHeight);

  mAspectRatio = (float)frameBufferWidth / (float)mWinWidth;
  }
//}}}
//{{{
void cGlWindow::draw (bool clear) {

  mCpuGraph->start ((float)glfwGetTime());

  if (clear)
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  beginFrame (mWinWidth, mWinHeight, mAspectRatio);

  if (mRootContainer)
    mRootContainer->onDraw (this);

  if (mDrawTests) {
    //{{{  draw tests
    drawEyes (mWinSize * cPointF(0.75f, 0.5f), mWinSize * cPointF(0.25f, 0.5f), mMousePos, (float)glfwGetTime());
    drawLines (cPointF(0.f, 70.f), mWinSize, (float)glfwGetTime());
    drawSpinner (mWinSize / 2.f, 20.f, 16.f, (float)glfwGetTime(), kOpaqueBlackF, kSemiOpaqueWhiteF);
    }
    //}}}

  if (mDrawPerf) {
    //{{{  draw perf stats
    mFpsGraph->render (this, cPointF(0.f, mWinSize.y - 35.f), cPointF((mWinSize.x / 2.f) - 2.f, 35.f));
    mCpuGraph->render (this, cPointF(mWinSize.x / 2.f, mWinSize.y - 35.f), cPointF((mWinSize.x / 2.f) - 2.f, 35.f));
    }
    //}}}

  if (mDrawStats) {
    //{{{  draw stats
    setFontSize (16.f);
    setTextAlign (cVg::eAlignLeft | cVg::eAlignBottom);
    setFillColour (kWhiteF);
    text (cPointF(0.f, mWinSize.y), getFrameStats() + (mVsync ? " vsyncOn" : " vsyncOff"));
    }
    //}}}

  endFrame();
  mCpuGraph->updateTime ((float)glfwGetTime());
  glfwSwapBuffers (mWindow);

  mFpsGraph->updateTime ((float)glfwGetTime());
  }
//}}}

//{{{
void cGlWindow::drawSpinner (cPointF centre, float inner, float outer, float frac,
                             const sColourF& colour1, const sColourF& colour2) {

  saveState();

  beginPath();
  float angle0 = (frac * k2Pi);
  float angle1 = kPi + angle0;
  arc (centre, outer, angle0, angle1, cVg::eClockWise);
  arc (centre, inner, angle1, angle0, cVg::eCounterClockWise);
  closePath();

  float mid = (outer + inner) * 0.5f;
  auto paint = setLinearGradient (centre + cPointF(cosf (angle0) * mid, sinf (angle0) * mid),
                                  centre + cPointF(cosf (angle1) * mid, sinf (angle1) * mid),
                                  colour1, colour2);
  setFillPaint (paint);
  fill();

  restoreState();
  }
//}}}
//{{{
void cGlWindow::drawEyes (cPointF point, cPointF size, cPointF mousePos, float t) {

  cPointF eyeSize (size.x * 0.23f, size.y * 0.5f);
  cPointF left = point + eyeSize;
  cPointF right = point + cPointF (size.x - eyeSize.x, eyeSize.y);
  float br = (eyeSize.x < eyeSize.y ? eyeSize.x : eyeSize.y) * 0.5f;
  float blink = 1 - powf (sinf(t*0.5f), 200) * 0.8f;

  beginPath();
  ellipse (left + cPointF(3.0f,16.0f), eyeSize);
  ellipse (right + cPointF(3.0f,16.0f), eyeSize);
  setFillPaint (setLinearGradient (point + cPointF(0.f,size.y *0.5f), point + cPointF(size.x *0.1f, size.y),
                                   sColourF(0.f,0.f,0.f,0.125f), sColourF(0.f,0.f,0.f,0.0625f)));
  fill();

  beginPath();
  ellipse (left, eyeSize);
  ellipse (right, eyeSize);
  setFillPaint (setLinearGradient (point + cPointF(0.f,size.y * 0.25f), point + cPointF(size.x * 0.1f, size.y),
                                   sColourF(0.9f,0.9f,0.9f,1.f), kGreyF));
  fill();

  cPointF diff1 ((mousePos.x - right.x) / (eyeSize.x * 10), (mousePos.y - right.y) / (eyeSize.y * 10));
  float d = diff1.magnitude();
  if (d > 1.0f)
    diff1 /= d;
  diff1 *= eyeSize;
  diff1 *= cPointF (0.4f, 0.5f);

  beginPath();
  ellipse (left + diff1 + cPointF(0.f, eyeSize.y * 0.25f * (1-blink)), cPointF(br, br*blink));
  setFillColour (kDarkerGreyF);
  fill();

  cPointF diff2 ((mousePos.x - right.x) / (eyeSize.x * 10), (mousePos.y - right.y) / (eyeSize.y * 10));
  d = diff2.magnitude();
  if (d > 1.0f)
    diff2 /= d;
  diff2 *= eyeSize;
  diff2 *= cPointF (0.4f, 0.5f);

  beginPath();
  ellipse (right + diff2 + cPointF(0.f, eyeSize.y * 0.25f * (1-blink)), cPointF(br, br*blink));
  setFillColour (kDarkerGreyF);
  fill();

  beginPath();
  ellipse (left, eyeSize);
  setFillPaint (setRadialGradient (left + cPointF(eyeSize.x * 0.25f, eyeSize.y * 0.5f),
                                   eyeSize.x * 0.1f, eyeSize.x * 0.75f,
                                   sColourF(1.f,1.f,1.f,0.5f), sColourF(1.f,1.f,1.f,0.f)));
  fill();

  beginPath();
  ellipse (right, eyeSize);
  setFillPaint (setRadialGradient (right + cPointF(eyeSize.x*0.25f,-eyeSize.y*0.5f),
                                   eyeSize.x * 0.1f, eyeSize.x * 0.75f,
                                   sColourF(1.f,1.f,1.f,0.5f), sColourF(1.f,1.f,1.f,0.f)));
  fill();
  }
//}}}
//{{{
void cGlWindow::drawLines (cPointF point, cPointF size, float t) {

  float pad = 5.0f;
  float s = size.x/9.0f - pad*2;


  cVg::eLineCap joins[3] = { cVg::eMiter, cVg::eRound, cVg::eBevel };
  cVg::eLineCap caps[3] =  { cVg::eButt, cVg::eRound, cVg::eSquare };


  cPointF pts[4];
  pts[0] = {  s*0.25f + cosf(t*0.3f) * s*0.5f, sinf(t*0.3f) * s*0.5f };
  pts[1] = { -s*0.25f, 0.f };
  pts[2] = {  s*0.25f, 0.f };
  pts[3] = {  s*0.25f + cosf(-t*0.3f) * s*0.5f, sinf(-t*0.3f) * s*0.5f };

  saveState();

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      cPointF f = point + cPointF (s*0.5f + (i*3+j)/9.0f*size.x + pad,  s*0.5f + pad);

      setLineCap (caps[i]);
      setLineJoin (joins[j]);
      setStrokeWidth (s*0.3f);
      setStrokeColour (sColourF(0.f,0.f,0.f, 0.7f));

      beginPath();
      moveTo (f + pts[0]);
      lineTo (f + pts[1]);
      lineTo (f + pts[2]);
      lineTo (f + pts[3]);
      stroke();

      setLineCap (cVg::eButt);
      setLineJoin (cVg::eBevel);
      setStrokeWidth (1.5f);
      setStrokeColour (sColourF (0.f,0.75f,1.f,1.f));

      beginPath();
      moveTo (f + pts[0]);
      lineTo (f + pts[1]);
      lineTo (f + pts[2]);
      lineTo (f + pts[3]);
      stroke();
      }
    }

  restoreState();
  }
//}}}
