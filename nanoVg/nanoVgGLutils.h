//{{{
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//}}}
#ifndef NANOVG_GL_UTILS_H
#define NANOVG_GL_UTILS_H
  //{{{
  struct NVGLUframebuffer {
    NVGcontext* context;
    GLuint fbo;
    GLuint rbo;
    GLuint texture;
    int image;
  };
  //}}}
  typedef struct NVGLUframebuffer NVGLUframebuffer;

  void nvgluBindFramebuffer (NVGLUframebuffer* fb);
  NVGLUframebuffer* nvgluCreateFramebuffer (NVGcontext* context, int w, int h, int imageFlags);
  void nvgluDeleteFramebuffer (NVGLUframebuffer* fb);
#endif // NANOVG_GL_UTILS_H


#ifdef NANOVG_GL_IMPLEMENTATION
  #if defined(NANOVG_GL3) || defined(NANOVG_GLES2) || defined(NANOVG_GLES3)
    // FBO is core in OpenGL 3>.
    #define NANOVG_FBO_VALID 1
  #endif

  static GLint defaultFBO = -1;

  //{{{
  NVGLUframebuffer* nvgluCreateFramebuffer (NVGcontext* context, int w, int h, int imageFlags)
  {
  #ifdef NANOVG_FBO_VALID
    GLint defaultFBO;
    GLint defaultRBO;
    NVGLUframebuffer* fb = NULL;

    glGetIntegerv (GL_FRAMEBUFFER_BINDING, &defaultFBO);
    glGetIntegerv (GL_RENDERBUFFER_BINDING, &defaultRBO);

    fb = (NVGLUframebuffer*)malloc (sizeof(NVGLUframebuffer));
    if (fb == NULL)
      error;
    memset(fb, 0, sizeof(NVGLUframebuffer));

    fb->image = nvgCreateImageRGBA (context, w, h, imageFlags | NVG_IMAGE_FLIPY | NVG_IMAGE_PREMULTIPLIED, NULL);
    fb->texture = nvglImageHandleGL (context, fb->image);
    fb->context = context;

    // frame buffer object
    glGenFramebuffers (1, &fb->fbo);
    glBindFramebuffer (GL_FRAMEBUFFER, fb->fbo);

    // render buffer object
    glGenRenderbuffers (1, &fb->rbo);
    glBindRenderbuffer (GL_RENDERBUFFER, fb->rbo);
    glRenderbufferStorage (GL_RENDERBUFFER, GL_STENCIL_INDEX8, w, h);

    // combine all
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb->texture, 0);
    glFramebufferRenderbuffer (GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fb->rbo);

    if (glCheckFramebufferStatus (GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      glBindFramebuffer( GL_FRAMEBUFFER, defaultFBO);
      glBindRenderbuffer (GL_RENDERBUFFER, defaultRBO);
      nvgluDeleteFramebuffer (fb);
      return NULL;
      }

    glBindFramebuffer (GL_FRAMEBUFFER, defaultFBO);
    glBindRenderbuffer (GL_RENDERBUFFER, defaultRBO);
    return fb;

  #else
    NVG_NOTUSED(context);
    NVG_NOTUSED(w);
    NVG_NOTUSED(h);
    NVG_NOTUSED(imageFlags);
    return NULL;
  #endif
  }
  //}}}
  //{{{
  void nvgluBindFramebuffer (NVGLUframebuffer* fb) {

  #ifdef NANOVG_FBO_VALID
    if (defaultFBO == -1)
      glGetIntegerv (GL_FRAMEBUFFER_BINDING, &defaultFBO);
    glBindFramebuffer (GL_FRAMEBUFFER, fb != NULL ? fb->fbo : defaultFBO);
  #else
    NVG_NOTUSED(fb);
  #endif
    }
  //}}}
  //{{{
  void nvgluDeleteFramebuffer (NVGLUframebuffer* fb) {

  #ifdef NANOVG_FBO_VALID
    if (fb == NULL)
      return;
    if (fb->fbo != 0)
      glDeleteFramebuffers (1, &fb->fbo);
    if (fb->rbo != 0)
      glDeleteRenderbuffers (1, &fb->rbo);
    if (fb->image >= 0)
      nvgDeleteImage (fb->context, fb->image);
    fb->context = NULL;
    fb->fbo = 0;
    fb->rbo = 0;
    fb->texture = 0;
    fb->image = -1;
    free(fb);
  #else
    NVG_NOTUSED(fb);
  #endif
    }
  //}}}

#endif
