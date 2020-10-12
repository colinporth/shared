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
cVg* cGlWindow::getVg() { return this; }
float cGlWindow::getWidthPix() { return mRootContainer->getPixWidth(); }
float cGlWindow::getHeightPix() { return mRootContainer->getPixHeight(); }

// iDraw
//{{{
void cGlWindow::drawRect (const sColourF& colour, cPointF p, cPointF size) {

  setFillColour (colour);

  beginPath();
  rect (p, size);
  triangleFill();
  }
//}}}
//{{{
float cGlWindow::drawText (const sColourF& colour, float fontHeight, string str, cPointF p, cPointF size) {

  setFontSize (fontHeight);
  setTextAlign (cVg::eAlignLeft | cVg::eAlignTop);
  setFillColour (sColourF(colour));
  text (p + cPointF (3.f, 1.f), str);

  // get width
  float bounds[4];
  return getTextBounds (p + cPointF(3.f, 1.f), str, bounds);
  }
//}}}
//{{{
float cGlWindow::drawTextRight (const sColourF& colour, float fontHeight, string str, cPointF p, cPointF size) {

  setFontSize (fontHeight);
  setTextAlign (cVg::eAlignRight | cVg::eAlignTop);
  setFillColour (sColourF(colour));
  text (p + cPointF (0.f, 1.f), str);

  // get width
  float bounds[4];
  return getTextBounds (p + cPointF (0.f,1.f), str, bounds);
  }
//}}}
//{{{
void cGlWindow::drawEllipseSolid (const sColourF& colour, cPointF p, float xradius, float yradius) {

  setFillColour (sColourF(colour));

  beginPath();
  ellipse (p, cPointF (xradius, yradius));
  fill();
  }
//}}}

// protected
//{{{
cRootContainer* cGlWindow::initialise (const string& title, int width, int height, unsigned char* fontData, int fontDataSize) {

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
  mCpuGraph = new cPerfGraph (cPerfGraph::eRenderMs, "cpu");
  mFpsGraph = new cPerfGraph (cPerfGraph::eRenderFps, "frame");

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
    drawEyes (cPointF(winWidth*3.0f/4.0f, winHeight/2.0f), cPointF(winWidth/4.0f, winHeight/2.0f),
              mMouseX, mMouseY, (float)glfwGetTime());
    drawLines (cPointF(0.0f, 50.0f), cPointF((float)winWidth, (float)winHeight), (float)glfwGetTime());
    drawSpinner (cPointF(winWidth/2.f, winHeight/2.f), 20.f, 16.f, (float)glfwGetTime(),
                 sColourF(0,0,0,0), sColourF(1.f, 1.f, 1.f, .5f));
    }
    //}}}

  if (mDrawPerf) {
    //{{{  draw perf stats
    mFpsGraph->render (this, cPointF(0.f, winHeight-35.f), cPointF(winWidth/2.f -2.f, 35.f));
    mCpuGraph->render (this, cPointF(winWidth/2.f, winHeight-35.f), cPointF(winWidth/2.f - 2.f, 35.f));
    }
    //}}}

  if (mDrawStats) {
    //{{{  draw stats
    setFontSize (12.0f);
    setTextAlign (cVg::eAlignLeft | cVg::eAlignBottom);
    setFillColour (kWhiteF);
    text (cPointF(0.f, (float)winHeight), getFrameStats() + (mVsync ? " vsyncOn" : " vsyncOff"));
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
                             const sColourF& color1, const sColourF& color2) {

  saveState();

  beginPath();
  float angle0 = (frac * k2Pi);
  float angle1 = kPi + angle0;
  arc (centre, outer, angle0, angle1, cVg::eClockWise);
  arc (centre, inner, angle1, angle0, cVg::eCounterClockWise);
  closePath();

  cPointF a (centre.x + cosf (angle0) * (outer + inner) * 0.5f, centre.y + sinf (angle0) * (outer + inner) * 0.5f);
  cPointF b (centre.x + cosf (angle1) * (outer + inner) * 0.5f, centre.y + sinf (angle1) * (outer + inner) * 0.5f);
  auto paint = setLinearGradient (a, b, color1, color2);
  setFillPaint (paint);
  fill();

  restoreState();
  }
//}}}
//{{{
void cGlWindow::drawEyes (cPointF p, cPointF size, float cursorX, float cursorY, float t) {

  cPointF eyeSize (size.x * 0.23f, size.y * 0.5f);
  cPointF left = p + eyeSize;
  cPointF right = p + cPointF (size.x - eyeSize.x, eyeSize.y);
  float br = (eyeSize.x < eyeSize.y ? eyeSize.x : eyeSize.y) * 0.5f;
  float blink = 1 - powf (sinf(t*0.5f), 200) * 0.8f;

  beginPath();
  ellipse (left + cPointF(3.0f,16.0f), eyeSize);
  ellipse (right + cPointF(3.0f,16.0f), eyeSize);
  setFillPaint (setLinearGradient (p + cPointF(0.f,size.y *0.5f), p + cPointF(size.x *0.1f, size.y),
                                   sColourF(0.f,0.f,0.f,0.125f), sColourF(0.f,0.f,0.f,0.0625f)));
  fill();

  beginPath();
  ellipse (left, eyeSize);
  ellipse (right, eyeSize);
  setFillPaint (setLinearGradient (p + cPointF(0.f,size.y * 0.25f), p + cPointF(size.x * 0.1f, size.y),
                                   sColourF(0.9f,0.9f,0.9f,1.f), kGreyF));
  fill();

  cPointF diff1 ((cursorX - right.x) / (eyeSize.x * 10), (cursorY - right.y) / (eyeSize.y * 10));
  float d = diff1.magnitude();
  if (d > 1.0f)
    diff1 /= d;
  diff1 *= eyeSize;
  diff1 *= cPointF (0.4f, 0.5f);

  beginPath();
  ellipse (left + diff1 + cPointF(0.f, eyeSize.y * 0.25f * (1-blink)), cPointF(br, br*blink));
  setFillColour (kDarkerGreyF);
  fill();

  cPointF diff2 ((cursorX - right.x) / (eyeSize.x * 10), (cursorY - right.y) / (eyeSize.y * 10));
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
void cGlWindow::drawLines (cPointF p, cPointF size, float t) {

  int i, j;
  float pad = 5.0f;
  float s = size.x/9.0f - pad*2;
  float pts[4*2];
  float fx;
  float fy;

  cVg::eLineCap joins[3] = {cVg::eMiter, cVg::eRound, cVg::eBevel };
  cVg::eLineCap caps[3] = { cVg::eButt, cVg::eRound, cVg::eSquare };

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
      fx = p.x + s*0.5f + (i*3+j)/9.0f*size.x + pad;
      fy = p.y - s*0.5f + pad;

      setLineCap (caps[i]);
      setLineJoin (joins[j]);
      setStrokeWidth (s*0.3f);
      setStrokeColour (sColourF(0.f,0.f,0.f, 0.7f));
      beginPath();
      moveTo (cPointF(fx+pts[0], fy+pts[1]));
      lineTo (cPointF(fx+pts[2], fy+pts[3]));
      lineTo (cPointF(fx+pts[4], fy+pts[5]));
      lineTo (cPointF(fx+pts[6], fy+pts[7]));
      stroke();

      setLineCap (cVg::eButt);
      setLineJoin (cVg::eBevel);
      setStrokeWidth (1.5f);
      setStrokeColour (sColourF (0.f,0.75f,1.f,1.f));
      beginPath();
      moveTo (cPointF(fx+pts[0], fy+pts[1]));
      lineTo (cPointF(fx+pts[2], fy+pts[3]));
      lineTo (cPointF(fx+pts[4], fy+pts[5]));
      lineTo (cPointF(fx+pts[6], fy+pts[7]));
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
