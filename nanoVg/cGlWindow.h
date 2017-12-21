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
  void initialise (std::string title, int width, int height, unsigned char* sansFont);
  void run();

  virtual void onKey (int key, int scancode, int action, int mods) = 0;
  virtual void onChar (char ch, int mods) = 0;
  virtual void onMouseProx (bool inClient, int x, int y);
  virtual void onMouseDown (bool right, int x, int y);
  virtual void onMouseMove (bool right, int x, int y, int xInc, int yInc);
  virtual void onMouseUp (bool right, bool mouseMoved, int x, int y);
  virtual void onMouseWheel (int delta);

  static cGlWindow* mGlWindow;

  std::vector<std::string> getFiles (std::string fileName, std::string match);

  GLFWwindow* mWindow = nullptr;

protected:
  void toggleVsync();
  void togglePerf();
  void toggleStats();
  void toggleTests();

  cRootContainer* mRoot = nullptr;

  static float mMouseX;
  static float mMouseY;

private:
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

  static bool mMouseDown;
  static bool mMouseMoved;
  static int mMouseIntX;
  static int mMouseIntY;

  static bool mAlted;
  static bool mSupered;
  static bool mShifted;
  static bool mControlled;
  };
