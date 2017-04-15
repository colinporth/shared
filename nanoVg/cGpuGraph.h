#pragma once 
#include "cPerfGraph.h"

class cGpuGraph : public cPerfGraph {
public:
  //{{{
  cGpuGraph (eRenderStyle style, const char* name) : cPerfGraph(style, name) {

    mSupported = glfwExtensionSupported ("GL_ARB_timer_query");
    if (mSupported)
      glGenQueries (kGPU_QUERY_COUNT, mQueries);
    }
  //}}}

  //{{{
  bool getSupported() {
    return mSupported;
    }
  //}}}

  //{{{
  void start() {

    if (mSupported) {
      glBeginQuery (GL_TIME_ELAPSED, mQueries[mCur % kGPU_QUERY_COUNT] );
      mCur++;
      }
    }
  //}}}
  //{{{
  int stop (float* times, int maxTimes) {

    GLint available = 1;

    int n = 0;
    if (mSupported) {
      glEndQuery (GL_TIME_ELAPSED);
      while (available && mRet <= mCur) {
        // check for results if there are any
        glGetQueryObjectiv (mQueries[mRet % kGPU_QUERY_COUNT], GL_QUERY_RESULT_AVAILABLE, &available);
        if (available) {
          GLuint64 timeElapsed = 0;
          glGetQueryObjectui64v (mQueries[mRet % kGPU_QUERY_COUNT], GL_QUERY_RESULT, &timeElapsed);
          mRet++;
          if (n < maxTimes) {
            times[n] = (float)((double)timeElapsed * 1e-9);
            n++;
            }
          }
        }
      }

    return n;
    }
  //}}}

private:
  static const int kGPU_QUERY_COUNT = 5;

  bool mSupported = false;
  int mCur = 0;
  int mRet = 0;
  unsigned int mQueries[kGPU_QUERY_COUNT];
  };
