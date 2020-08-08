// cGlWindow.h
//{{{  includes
#pragma once
#include "cVg.h"

#include "cPerfGraph.h"

#include "../utils/iChange.h"
#include "../widgets/cRootContainer.h"
//}}}

class cGlWindow : public cVg, public iChange, public iDraw {
public:
  cGlWindow() {};
  virtual ~cGlWindow();

  // iWindow
  cVg* getContext();
  float getWidth() { return 800; }
  float getHeight() { return 480; }
  float getWidthPix();
  float getHeightPix();

  bool getShift() { return mShifted; }
  bool getControl() { return mControlled; }
  bool getMouseDown() { return mMouseDown; }

  // iDraw
  void drawRect (uint32_t colour, float x, float y, float width, float height);
  float drawText (uint32_t colour, float fontHeight, std::string str, float x, float y, float width, float height);
  float drawTextRight (uint32_t colour, float fontHeight, std::string str, float x, float y, float width, float height);
  void drawEllipseSolid (uint32_t colour, float x, float y, float xradius, float yradius);

  void drawPixel (uint32_t colour, float x, float y) {}
  void drawStamp (uint32_t colour, uint8_t* src, float x, float y, float width, float height) {}

  //{{{
  void drawPixelClipped (uint32_t colour, float x, float y) {
    drawRectClipped (colour, x, y, 1, 1);
    }
  //}}}
  //{{{
  void drawStampClipped (uint32_t colour, uint8_t* src, float x, float y, float width, float height) {

    drawStamp (colour, src, x, y, width, height);
    }
  //}}}
  //{{{
  void drawRectClipped (uint32_t colour, float x, float y, float width, float height) {
    drawRect (colour, x, y, width, height);
    }
  //}}}
  //{{{
  void drawRectOutline (uint32_t colour, float x, float y, float width, float height, float thickness) {

    drawRect(colour, x, y, width, thickness);
    drawRect(colour, x + width-thickness, y, thickness, height);
    drawRect(colour, x, y + height-thickness, width, thickness);
    drawRect(colour, x, y, thickness, height);
    }
  //}}}

  // iChange
  void changed() {}
  void setChangeCountDown (int countDown) {}

protected:
  cRootContainer* initialise (std::string title, int width, int height, unsigned char* sansFont);

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

  void toggleVsync();
  void togglePerf();
  void toggleStats() { mDrawStats = !mDrawStats; }
  void toggleTests() { mDrawTests = !mDrawTests; }

  virtual void onKey (int key, int scancode, int action, int mods) = 0;
  virtual void onChar (char ch, int mods) = 0;

  static inline cGlWindow* mGlWindow = nullptr;
  static inline float mMouseX = 0;
  static inline float mMouseY = 0;

  struct GLFWwindow* mWindow = nullptr;

private:
  void mouseProx (float x, float y);
  void mouseDown (bool right, float x, float y);
  void mouseMove (bool right, float x, float y, float xInc, float yInc);
  void mouseUp (bool right, float x, float y);
  void mouseWheel (float delta);

  void drawEyes (float x, float y, float w, float h, float cursorX, float cursorY, float t);
  void drawLines (float x, float y, float w, float h, float t);
  void drawStats (float x, float y, const std::string& str);
  void drawSpinner (float cx, float cy, float r, float t);

  static void glfwKey (GLFWwindow* window, int key, int scancode, int action, int mods);
  static void glfwCharMods (GLFWwindow* window, unsigned int ch, int mods);
  static void glfwCursorPos (GLFWwindow* window, double xpos, double ypos);
  static void glfwMouseButton (GLFWwindow* window, int button, int action, int mods);
  static void glfMouseScroll (GLFWwindow* window, double xoffset, double yoffset);
  static void errorCallback (int error, const char* desc);

  bool mVsync = true;
  bool mDrawPerf = false;
  bool mDrawStats = false;
  bool mDrawTests = false;

  cPerfGraph* mCpuGraph = nullptr;
  cPerfGraph* mFpsGraph = nullptr;

  cRootContainer* mRoot = nullptr;

  static inline bool mMouseDown =  false;
  static inline bool mMouseMoved = false;
  static inline bool mMouseRightButton = false;
  static inline float mMouseLastX = 0;
  static inline float mMouseLastY = 0;

  static inline bool mAlted = false;
  static inline bool mSupered = false;
  static inline bool mShifted = false;
  static inline bool mControlled = false;
  };
