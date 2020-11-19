// cGlWindow.h
//{{{  includes
#pragma once
#include "cVg.h"

#include <chrono>
#include <array>
#include "cPerfGraph.h"

#include "../utils/iChange.h"

#include "../widgets/iClockTime.h"
#include "../widgets/iDraw.h"

class cWidget;
class cRootContainer;
//}}}

class cGlWindow : public cVg, public iClockTime, public iDraw {
public:
  cGlWindow();
  virtual ~cGlWindow();

  //{{{  iClockTime
  int getDayLightSeconds();
  std::chrono::system_clock::time_point getNowRaw();
  std::chrono::system_clock::time_point getNowDayLight();
  //}}}
  //{{{  iDraw
  void drawRect (const sColourF& colour, cPointF p, cPointF size);
  void drawPixel (const sColourF& colour, cPointF p) {}
  //{{{
  void drawRectOutline (const sColourF& colour, cPointF p, cPointF size, float thickness) {
    drawRect (colour, p, cPointF(size.x, thickness));
    drawRect (colour, cPointF(p.x + size.x -thickness, p.y), cPointF(thickness, size.y));
    drawRect (colour, cPointF(p.x, p.y + size.y -thickness), cPointF(size.x, thickness));
    drawRect (colour, p, cPointF(thickness, size.y));
    }
  //}}}

  float drawText (const sColourF& colour, float fontHeight, std::string str, cPointF p,  cPointF size);
  float drawTextRight (const sColourF& colour, float fontHeight, std::string str, cPointF p, cPointF size);
  void drawEllipseSolid (const sColourF& colour, cPointF p, cPointF radius);
  void drawStamp (const sColourF& colour, uint8_t* src, cPointF p, cPointF size) {}
  //}}}
  //{{{  iWindow
  cVg* getVg();

  cPointF getSize();

  bool getShift() { return mShifted; }
  bool getControl() { return mControlled; }
  bool getMouseDown() { return mMouseDown; }
  //}}}

protected:
  cRootContainer* initialiseGui (const std::string& title, int width, int height, uint8_t* font, int fontSize);
  cWidget* addTopLeft (cWidget* widget);
  cWidget* add (cWidget* widget);
  cWidget* add (cWidget* widget, float offset);
  cWidget* add (cWidget* widget, cPointF point);
  cWidget* addBelowLeft (cWidget* widget);
  void runGui (bool clear);

  // keystroke dispatch
  virtual void onKey (int key, int scancode, int action, int mods) = 0;
  virtual void onChar (char ch, int mods) = 0;

  // actions
  void toggleFullScreen();
  void toggleVsync();
  void togglePerf();
  void toggleStats() { mDrawStats = !mDrawStats; }
  void toggleTests() { mDrawTests = !mDrawTests; }

  // vars
  static inline cGlWindow* mGlWindow = nullptr;
  static inline cPointF mMousePos { 0.f,0.f };

  GLFWwindow* mWindow = nullptr;
  int mWinWidth = 0;
  int mWinHeight = 0;
  cPointF mWinSize  { 0.f,0.f };
  float mAspectRatio = 0.f;

  GLFWmonitor* mMonitor = nullptr;
  std::array <int, 2> mWindowPos { 0,0 };
  std::array <int, 2> mWindowSize { 0,0 };

private:
  //{{{  static glfw callbacks
  static void glfWindowPos (GLFWwindow* window, int xsize, int ysize);
  static void glfWindowSize (GLFWwindow* window, int xsize, int ysize);
  static void errorCallback (int error, const char* desc);

  static void glfwKey (GLFWwindow* window, int key, int scancode, int action, int mods);
  static void glfwCharMods (GLFWwindow* window, unsigned int ch, int mods);

  static void glfwCursorPos (GLFWwindow* window, double xpos, double ypos);
  static void glfwMouseButton (GLFWwindow* window, int button, int action, int mods);
  static void glfMouseScroll (GLFWwindow* window, double xoffset, double yoffset);
  //}}}

  void updateWindowSize();
  void draw (bool clear);
  void drawSpinner (cPointF centre, float inner, float outer, float frac,
                    const sColourF& colour1, const sColourF& colour2);
  void drawEyes (cPointF point, cPointF size, cPointF mousePos, float t);
  void drawLines (cPointF point, cPointF size, float t);

  //{{{  vars
  bool mVsync = true;
  bool mDrawPerf = false;
  bool mDrawStats = false;
  bool mDrawTests = false;

  cPerfGraph* mCpuGraph = nullptr;
  cPerfGraph* mFpsGraph = nullptr;

  static inline cRootContainer* mRootContainer = nullptr;

  static inline bool mMouseDown =  false;
  static inline bool mMouseMoved = false;
  static inline bool mMouseRightButton = false;
  static inline cPointF mMouseLastPos = { 0.f,0.f };

  static inline bool mAlted = false;
  static inline bool mSupered = false;
  static inline bool mShifted = false;
  static inline bool mControlled = false;


  static inline int mDayLightSeconds = 0;
  //}}}
  };
