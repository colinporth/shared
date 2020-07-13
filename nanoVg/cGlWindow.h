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
  uint16_t getWidthPix();
  uint16_t getHeightPix();

  bool getShift() { return mShifted; }
  bool getControl() { return mControlled; }
  bool getMouseDown() { return mMouseDown; }

  // iDraw
  void pixel (uint32_t colour, int16_t x, int16_t y);
  void drawRect (uint32_t colour, int16_t x, int16_t y, uint16_t width, uint16_t height);
  void stamp (uint32_t colour, uint8_t* src, int16_t x, int16_t y, uint16_t width, uint16_t height);
  int drawText (uint32_t colour, uint16_t fontHeight, std::string str, int16_t x, int16_t y, uint16_t width, uint16_t height);
  void ellipseSolid (uint32_t colour, int16_t x, int16_t y, uint16_t xradius, uint16_t yradius);

  //{{{
  void pixelClipped (uint32_t colour, int16_t x, int16_t y) {

    drawRect(colour, x, y, 1, 1);
    }
  //}}}
  //{{{
  void stampClipped (uint32_t colour, uint8_t* src, int16_t x, int16_t y, uint16_t width, uint16_t height) {

    stamp (colour, src, x, y, width, height);
    }
  //}}}
  //{{{
  void rectClipped (uint32_t colour, int16_t x, int16_t y, uint16_t width, uint16_t height) {

    drawRect (colour, x, y, width, height);
    }
  //}}}
  //{{{
  void rectOutline (uint32_t colour, int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t thickness) {

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
  void onProx (bool inClient, int x, int y);
  void onDown (bool right, int x, int y);
  void onMove (bool right, int x, int y, int xInc, int yInc);
  void onUp (bool right, bool mouseMoved, int x, int y);
  void onWheel (int delta);

  void drawEyes (float x, float y, float w, float h, float cursorX, float cursorY, float t);
  void drawLines (float x, float y, float w, float h, float t);
  void drawStats (float x, float y, const std::string& str);
  void drawSpinner (float cx, float cy, float r, float t);

  static void glfwKey (struct GLFWwindow* window, int key, int scancode, int action, int mods);
  static void glfwCharMods (struct GLFWwindow* window, unsigned int ch, int mods);
  static void glfwCursorPos (struct GLFWwindow* window, double xpos, double ypos);
  static void glfwMouseButton (struct GLFWwindow* window, int button, int action, int mods);
  static void glfMouseScroll (struct GLFWwindow* window, double xoffset, double yoffset);
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
  static inline int mMouseIntX = 0;
  static inline int mMouseIntY = 0;

  static inline bool mAlted = false;
  static inline bool mSupered = false;
  static inline bool mShifted = false;
  static inline bool mControlled = false;
  };
