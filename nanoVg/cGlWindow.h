// cGlWindow.h
#pragma once
//{{{  includes
#include <string>
#include <vector>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define NANOVG_GL3 1
#define NANOVG_GL_USE_UNIFORMBUFFER 1
#include "cVgGL.h"

typedef struct GLFWwindow GLFWwindow;
class cPerfGraph;
class cGpuGraph;

#define USE_NANOVG
#include "../../shared/widgets/cRootContainer.h"
//}}}

class cGlWindow : public cVgGL, public iDraw {
public:
  cGlWindow();
  virtual ~cGlWindow();

  static std::vector<std::string> getFiles (std::string fileName, std::string match);

  // iWindow
  cVg* getContext();
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

  virtual void onKey (int key, int scancode, int action, int mods) = 0;
  virtual void onChar (char ch, int mods) = 0;

  void toggleVsync();
  void togglePerf();
  void toggleStats();
  void toggleTests();

  static cGlWindow* mGlWindow;
  static float mMouseX;
  static float mMouseY;

  GLFWwindow* mWindow = nullptr;

private:
  void onProx (bool inClient, int x, int y);
  void onDown (bool right, int x, int y);
  void onMove (bool right, int x, int y, int xInc, int yInc);
  void onUp (bool right, bool mouseMoved, int x, int y);
  void onWheel (int delta);

  void drawSpinner (float cx, float cy, float r, float t);
  void drawEyes (float x, float y, float w, float h, float cursorX, float cursorY, float t);
  void drawLines (float x, float y, float w, float h, float t);
  void drawStats (float x, float y, std::string str);

  static void glfwKey (GLFWwindow* window, int key, int scancode, int action, int mods);
  static void glfwCharMods (GLFWwindow* window, unsigned int ch, int mods);
  static void glfwCursorPos (GLFWwindow* window, double xpos, double ypos);
  static void glfwMouseButton (GLFWwindow* window, int button, int action, int mods);
  static void glfMouseScroll (GLFWwindow* window, double xoffset, double yoffset);
  static void errorcb (int error, const char* desc);

  bool mVsync = true;
  bool mDrawPerf = false;
  bool mDrawStats = false;
  bool mDrawTests = false;

  cPerfGraph* mFpsGraph = nullptr;
  cPerfGraph* mCpuGraph = nullptr;
  cGpuGraph* mGpuGraph = nullptr;

  cRootContainer* mRoot = nullptr;

  static bool mMouseDown;
  static bool mMouseMoved;
  static int mMouseIntX;
  static int mMouseIntY;

  static bool mAlted;
  static bool mSupered;
  static bool mShifted;
  static bool mControlled;
  };
