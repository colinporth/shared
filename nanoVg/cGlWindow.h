// cGlWindow.h
//{{{  includes
#pragma once
#include "cVg.h"

#include "cPerfGraph.h"

#include "../utils/iChange.h"
#include "../widgets/cRootContainer.h"
//}}}

class cGlWindow : public cVg, public iDraw {
public:
  cGlWindow() {};
  virtual ~cGlWindow();

  //{{{  iWindow gets
  cVg* getVg();
  float getWidth() { return 800; }
  float getHeight() { return 480; }
  float getWidthPix();
  float getHeightPix();

  bool getShift() { return mShifted; }
  bool getControl() { return mControlled; }
  bool getMouseDown() { return mMouseDown; }
  //}}}

  // iDraw
  void drawRect (uint32_t colour, cPoint p, cPoint size);
  float drawText (uint32_t colour, float fontHeight, std::string str, cPoint p, cPoint size);
  float drawTextRight (uint32_t colour, float fontHeight, std::string str, cPoint p, cPoint size);
  void drawEllipseSolid (uint32_t colour, cPoint p, float xradius, float yradius);
  void drawPixel (uint32_t colour, cPoint p) {}
  void drawStamp (uint32_t colour, uint8_t* src, cPoint p, cPoint size) {}
  //{{{
  void drawPixelClipped (uint32_t colour, cPoint p) {
    drawRectClipped (colour, p, cPoint(1, 1));
    }
  //}}}
  //{{{
  void drawStampClipped (uint32_t colour, uint8_t* src, cPoint p, cPoint size) {
    drawStamp (colour, src, p, size);
    }
  //}}}
  //{{{
  void drawRectClipped (uint32_t colour, cPoint p, cPoint size) {
    drawRect (colour, p, size);
    }
  //}}}
  //{{{
  void drawRectOutline (uint32_t colour, cPoint p, cPoint size, float thickness) {
    drawRect (colour, p, cPoint(size.x, thickness));
    drawRect (colour, cPoint(p.x + size.x -thickness, p.y), cPoint(thickness, size.y));
    drawRect (colour, cPoint(p.x, p.y + size.y -thickness), cPoint(size.x, thickness));
    drawRect (colour, p, cPoint(thickness, size.y));
    }
  //}}}

protected:
  cRootContainer* initialise (const std::string& title, int width, int height, unsigned char* sansFont, int sansFontSize);

  cWidget* add (cWidget* widget) { return mRootContainer->add (widget); }
  cWidget* addAt (cWidget* widget, float x, float y) { return mRootContainer->addAt (widget,x,y); }
  cWidget* addAtPix (cWidget* widget, int16_t x, int16_t y) { return mRootContainer->addAtPix (widget,x,y); }
  cWidget* addTopLeft (cWidget* widget) { return mRootContainer->addTopLeft (widget); }
  cWidget* addTopRight (cWidget* widget) { return mRootContainer->addTopRight (widget); }
  cWidget* addBottomLeft (cWidget* widget) { return mRootContainer->addBottomLeft (widget); }
  cWidget* addBottomRight (cWidget* widget) { return mRootContainer->addBottomRight (widget); }
  cWidget* addBelow (cWidget* widget) { return mRootContainer->addBelow (widget); }
  cWidget* addLeft (cWidget* widget) { return mRootContainer->addLeft (widget); }
  cWidget* addAbove (cWidget* widget) { return mRootContainer->addAbove (widget); }
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
  void draw();

  void drawSpinner (cPoint centre, float inner, float outer, float frac,
                    const sVgColour& color1, const sVgColour& color2);
  void drawEyes (cPoint p, cPoint size, float cursorX, float cursorY, float t);
  void drawLines (cPoint p, cPoint size, float t);

  //{{{  static glfw callbacks
  static void glfwKey (GLFWwindow* window, int key, int scancode, int action, int mods);
  static void glfwCharMods (GLFWwindow* window, unsigned int ch, int mods);
  static void glfwCursorPos (GLFWwindow* window, double xpos, double ypos);
  static void glfwMouseButton (GLFWwindow* window, int button, int action, int mods);
  static void glfMouseScroll (GLFWwindow* window, double xoffset, double yoffset);
  static void glfWindowPos (GLFWwindow* window, int xsize, int ysize);
  static void glfWindowSize (GLFWwindow* window, int xsize, int ysize);
  static void errorCallback (int error, const char* desc);
  //}}}

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
  static inline float mMouseLastX = 0;
  static inline float mMouseLastY = 0;

  static inline bool mAlted = false;
  static inline bool mSupered = false;
  static inline bool mShifted = false;
  static inline bool mControlled = false;
  //}}}
  };
