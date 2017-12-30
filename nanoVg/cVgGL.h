// cVgGL.h - based on Mikko Mononen memon@inside.org nanoVg
//{{{  includes
#pragma once

#include <stdio.h>
#include "cVg.h"
//}}}

class cVgGL : public cVg {
public:
  cVgGL() : cVg(0) {}
  cVgGL (int flags) : cVg(flags) {}
  //{{{
  virtual ~cVgGL() {

    glDisableVertexAttribArray (0);
    glDisableVertexAttribArray (1);

    #ifdef NANOVG_GL3
      glBindVertexArray (0);

      #ifdef NANOVG_UNIFORMBUFFER
        if (mFragBuffer)
          glDeleteBuffers (1, &mFragBuffer);
      #endif

      if (mVertexArray)
        glDeleteVertexArrays (1, &mVertexArray);
    #endif

    if (mVertexBuffer)
      glDeleteBuffers (1, &mVertexBuffer);

    glDisable (GL_CULL_FACE);
    glBindBuffer (GL_ARRAY_BUFFER, 0);
    glUseProgram (0);
    setBindTexture (0);

    for (int i = 0; i < mNumTextures; i++)
      if (mTextures[i].tex && (mTextures[i].flags & IMAGE_NODELETE) == 0)
        glDeleteTextures (1, &mTextures[i].tex);

    free (mTextures);
    free (mPathVertices);
    free (mFrags);
    free (mDraws);
    }
  //}}}
  //{{{
  void initialise() {

    mShader.create ("#define EDGE_AA 1\n");
    mShader.getUniforms();

    // Create dynamic vertex array
  #ifdef NANOVG_GL3
    glGenVertexArrays (1, &mVertexArray);
  #endif

    glGenBuffers (1, &mVertexBuffer);

  #ifdef NANOVG_UNIFORMBUFFER
    // Create UBOs
    mShader.bindUniformBlock();
    glGenBuffers (1, &mFragBuffer);
    int align = 4;
    glGetIntegerv (GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &align);
  #endif

    // removed because of strange startup time
    //glFinish();

    cVg::initialise();
    }
  //}}}

  //{{{
  GLuint imageHandle (int image) {
    return findTexture (image)->tex;
    }
  //}}}
  //{{{
  int createImageFromHandle (GLuint textureId, int w, int h, int imageFlags) {

    auto texture = allocTexture();
    if (texture == nullptr)
      return 0;

    texture->type = TEXTURE_RGBA;
    texture->tex = textureId;
    texture->flags = imageFlags;
    texture->width = w;
    texture->height = h;

    return texture->id;
    }
  //}}}

protected:
  //{{{
  void renderViewport (int width, int height, float devicePixelRatio) {
    mViewport[0] = (float)width;
    mViewport[1] = (float)height;
    }
  //}}}
  //{{{
  void renderText (int firstVertexIndex, int numVertices, cPaint& paint, cScissor& scissor) {

    auto draw = allocDraw();
    draw->set (cDraw::TEXT, paint.image, 0, 0, allocFrags (1), firstVertexIndex, numVertices);
    mFrags[draw->mFirstFragIndex].setImage (&paint, &scissor, findTexture (paint.image));
    }
  //}}}
  //{{{
  void renderTriangles (int firstVertexIndex, int numVertices, cPaint& paint, cScissor& scissor) {

    auto draw = allocDraw();
    draw->set (cDraw::TRIANGLE, paint.image, 0, 0, allocFrags (1), firstVertexIndex, numVertices);
    mFrags[draw->mFirstFragIndex].setFill (&paint, &scissor, 1.0f, 1.0f, -1.0f, findTexture (paint.image));
    }
  //}}}
  //{{{
  void renderFill (cShape& shape, cPaint& paint, cScissor& scissor, float fringe) {

    auto draw = allocDraw();
    if ((shape.mNumPaths == 1) && shape.mPaths[0].mConvex) {
      // convex
      draw->set (cDraw::CONVEX_FILL, paint.image, allocPathVertices (shape.mNumPaths), shape.mNumPaths,
                 allocFrags (1), 0,0);
      mFrags[draw->mFirstFragIndex].setFill (&paint, &scissor, fringe, fringe, -1.0f, findTexture (paint.image));
      }
    else {
      // stencil
      draw->set (cDraw::STENCIL_FILL, paint.image, allocPathVertices (shape.mNumPaths), shape.mNumPaths,
                 allocFrags (2), shape.mBoundsVertexIndex, 4);
      mFrags[draw->mFirstFragIndex].setSimple();
      mFrags[draw->mFirstFragIndex+1].setFill (&paint, &scissor, fringe, fringe, -1.0f, findTexture (paint.image));
      }

    auto fromPath = shape.mPaths;
    auto toPathVertices = &mPathVertices[draw->mFirstPathVerticesIndex];
    while (fromPath < shape.mPaths + shape.mNumPaths)
      *toPathVertices++ = fromPath++->mPathVertices;
    }
  //}}}
  //{{{
  void renderStroke (cShape& shape, cPaint& paint, cScissor& scissor, float fringe, float strokeWidth) {
  // only uses toPathVertices firstStrokeVertexIndex, strokeVertexCount, no fill

    auto draw = allocDraw();
    draw->set (cDraw::STROKE, paint.image, allocPathVertices (shape.mNumPaths), shape.mNumPaths, allocFrags (2), 0,0);
    mFrags[draw->mFirstFragIndex].setFill (&paint, &scissor, strokeWidth, fringe, -1.0f, findTexture (paint.image));
    mFrags[draw->mFirstFragIndex+1].setFill (&paint, &scissor, strokeWidth, fringe, 1.0f - 0.5f/255.0f, findTexture (paint.image));

    auto fromPath = shape.mPaths;
    auto toPathVertices = &mPathVertices[draw->mFirstPathVerticesIndex];
    while (fromPath < shape.mPaths + shape.mNumPaths)
      *toPathVertices++ = fromPath++->mPathVertices;
    }
  //}}}
  //{{{
  void renderFrame (c2dVertices& vertices, cCompositeOpState compositeOp) {

    mDrawArrays = 0;
    //{{{  init gl blendFunc
    GLenum srcRGB = convertBlendFuncFactor (compositeOp.srcRGB);
    GLenum dstRGB = convertBlendFuncFactor (compositeOp.dstRGB);
    GLenum srcAlpha = convertBlendFuncFactor (compositeOp.srcAlpha);
    GLenum dstAlpha = convertBlendFuncFactor (compositeOp.dstAlpha);

    if (srcRGB == GL_INVALID_ENUM || dstRGB == GL_INVALID_ENUM ||
        srcAlpha == GL_INVALID_ENUM || dstAlpha == GL_INVALID_ENUM)
      glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    else
      glBlendFuncSeparate (srcRGB, dstRGB, srcAlpha, dstAlpha);
    //}}}
    //{{{  init gl draw style
    glEnable (GL_CULL_FACE);
    glCullFace (GL_BACK);

    glFrontFace (GL_CCW);
    glEnable (GL_BLEND);

    glDisable (GL_DEPTH_TEST);
    glDisable (GL_SCISSOR_TEST);
    //}}}
    //{{{  init gl stencil buffer
    mStencilMask = 0xFF;
    glStencilMask (mStencilMask);

    mStencilFunc = GL_ALWAYS;
    mStencilFuncRef = 0;
    mStencilFuncMask = 0xFF;
    glStencilFunc (mStencilFunc, mStencilFuncRef, mStencilFuncMask);

    glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);
    glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    //}}}
    //{{{  init gl texture
    glActiveTexture (GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_2D, 0);
    mBindTexture = 0;
    //}}}
    //{{{  init gl uniforms
    mShader.setTex (0);
    mShader.setViewport (mViewport);

    #ifdef NANOVG_UNIFORMBUFFER
      // Upload ubo for frag shaders
      glBindBuffer (GL_UNIFORM_BUFFER, mFragBuffer);
      glBufferData (GL_UNIFORM_BUFFER, mNumFrags * sizeof(cFrag), mFrags, GL_STREAM_DRAW);
    #endif
    //}}}
    //{{{  init gl vertices
    #ifdef NANOVG_GL3
      // Upload vertex data
      glBindVertexArray (mVertexArray);
    #endif

    glBindBuffer (GL_ARRAY_BUFFER, mVertexBuffer);
    glBufferData (GL_ARRAY_BUFFER, vertices.getNumVertices() * sizeof(c2dVertex), vertices.getVertexPtr(0), GL_STREAM_DRAW);

    glEnableVertexAttribArray (0);
    glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, sizeof(c2dVertex), (const GLvoid*)(size_t)0);

    glEnableVertexAttribArray (1);
    glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, sizeof(c2dVertex), (const GLvoid*)(0 + 2*sizeof(float)));
    //}}}

    for (auto draw = mDraws; draw < mDraws + mNumDraws; draw++)
      switch (draw->mType) {
        case cDraw::TEXT:
          //{{{  text triangles
          setUniforms (draw->mFirstFragIndex, draw->mImage);
          glDrawArrays (GL_TRIANGLES, draw->mTriangleFirstVertexIndex, draw->mNumTriangleVertices);
          mDrawArrays++;
          break;
          //}}}
        case cDraw::TRIANGLE:
          //{{{  fill triangles
          if (mDrawSolid) {
            setUniforms (draw->mFirstFragIndex, draw->mImage);
            glDrawArrays (GL_TRIANGLES, draw->mTriangleFirstVertexIndex, draw->mNumTriangleVertices);
            mDrawArrays++;
            }
          break;
          //}}}
        case cDraw::CONVEX_FILL: {
          //{{{  convexFill
          setUniforms (draw->mFirstFragIndex, draw->mImage);

          auto pathVertices = &mPathVertices[draw->mFirstPathVerticesIndex];

          if (mDrawSolid)
            for (int i = 0; i < draw->mNumPaths; i++) {
              glDrawArrays (GL_TRIANGLE_FAN, pathVertices[i].mFirstFillVertexIndex, pathVertices[i].mNumFillVertices);
              mDrawArrays++;
              }

          if (mDrawEdges)
            for (int i = 0; i < draw->mNumPaths; i++)
              if (pathVertices[i].mNumStrokeVertices) {
                glDrawArrays (GL_TRIANGLE_STRIP, pathVertices[i].mFirstStrokeVertexIndex, pathVertices[i].mNumStrokeVertices);
                mDrawArrays++;
                }

          break;
          }
          //}}}
        case cDraw::STENCIL_FILL: {
          //{{{  stencilFill
          glEnable (GL_STENCIL_TEST);

          auto pathVertices = &mPathVertices[draw->mFirstPathVerticesIndex];

          if (mDrawSolid) {
            glDisable (GL_CULL_FACE);

            glStencilOpSeparate (GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
            glStencilOpSeparate (GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
            glColorMask (GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

            setStencilFunc (GL_ALWAYS, 0x00, 0xFF);
            setUniforms (draw->mFirstFragIndex, 0);
            for (int i = 0; i < draw->mNumPaths; i++) {
              glDrawArrays (GL_TRIANGLE_FAN, pathVertices[i].mFirstFillVertexIndex, pathVertices[i].mNumFillVertices);
              mDrawArrays++;
              }

            glEnable (GL_CULL_FACE);
            glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            }

          setUniforms (draw->mFirstFragIndex + 1, draw->mImage);
          if (mDrawEdges) {
            glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);
            setStencilFunc (GL_EQUAL, 0x00, 0xFF);
            for (int i = 0; i < draw->mNumPaths; i++)
              if (pathVertices[i].mNumStrokeVertices) {
                glDrawArrays (GL_TRIANGLE_STRIP, pathVertices[i].mFirstStrokeVertexIndex, pathVertices[i].mNumStrokeVertices);
                mDrawArrays++;
                }
            }

          // draw bounding rect as triangleStrip
          if (mDrawSolid || mDrawEdges) {
            glStencilOp (GL_ZERO, GL_ZERO, GL_ZERO);
            setStencilFunc (GL_NOTEQUAL, 0x00, 0xFF);
            glDrawArrays (GL_TRIANGLE_STRIP, draw->mTriangleFirstVertexIndex, draw->mNumTriangleVertices);
            mDrawArrays++;
            }

          glDisable (GL_STENCIL_TEST);
          break;
          }
          //}}}
        case cDraw::STROKE: {
          //{{{  stroke
          glEnable (GL_STENCIL_TEST);

          auto pathVertices = &mPathVertices[draw->mFirstPathVerticesIndex];

          // fill stroke base without overlap
          if (mDrawSolid) {
            glStencilOp (GL_KEEP, GL_KEEP, GL_INCR);
            setStencilFunc (GL_EQUAL, 0x00, 0xFF);
            setUniforms (draw->mFirstFragIndex + 1, draw->mImage);
            for (int i = 0; i < draw->mNumPaths; i++) {
              glDrawArrays (GL_TRIANGLE_STRIP, pathVertices[i].mFirstStrokeVertexIndex, pathVertices[i].mNumStrokeVertices);
              mDrawArrays++;
              }
            }

          setUniforms (draw->mFirstFragIndex, draw->mImage);
          if (mDrawEdges) {
            glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);
            setStencilFunc (GL_EQUAL, 0x00, 0xFF);
            for (int i = 0; i < draw->mNumPaths; i++)  {
              glDrawArrays (GL_TRIANGLE_STRIP, pathVertices[i].mFirstStrokeVertexIndex, pathVertices[i].mNumStrokeVertices);
              mDrawArrays++;
              }
            }

          // clear stencilBuffer
          if (mDrawSolid || mDrawEdges) {
            glColorMask (GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glStencilOp (GL_ZERO, GL_ZERO, GL_ZERO);
            setStencilFunc (GL_ALWAYS, 0x00, 0xFF);
            for (int i = 0; i < draw->mNumPaths; i++) {
              glDrawArrays (GL_TRIANGLE_STRIP, pathVertices[i].mFirstStrokeVertexIndex, pathVertices[i].mNumStrokeVertices);
              mDrawArrays++;
              }
            glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            }

          glDisable (GL_STENCIL_TEST);
          break;
          }
          //}}}
        }

    // reset counts
    mNumDraws = 0;
    mNumPathVertices = 0;
    mNumFrags = 0;
    }
  //}}}

  //{{{
  bool renderGetTextureSize (int image, int* w, int* h) {

    auto texture = findTexture (image);
    if (texture == nullptr)
      return false;

    *w = texture->width;
    *h = texture->height;

    return true;
    }
  //}}}
  //{{{
  int renderCreateTexture (int type, int w, int h, int imageFlags, const unsigned char* data) {

    auto texture = allocTexture();
    if (texture == nullptr)
      return 0;

  #ifdef NANOVG_GLES2
    // Check for non-power of 2.
    if (nearestPow2 (w) != (unsigned int)w || nearestPow2(h) != (unsigned int)h) {
      if ((imageFlags & IMAGE_REPEATX) != 0 || (imageFlags & IMAGE_REPEATY) != 0) {
        printf ("Repeat X/Y is not supported for non power-of-two textures (%d x %d)\n", w, h);
        imageFlags &= ~(IMAGE_REPEATX | IMAGE_REPEATY);
        }

      if (imageFlags & IMAGE_GENERATE_MIPMAPS) {
        printf ("Mip-maps is not support for non power-of-two textures (%d x %d)\n", w, h);
        imageFlags &= ~IMAGE_GENERATE_MIPMAPS;
        }
      }
  #endif

    glGenTextures (1, &texture->tex);
    texture->width = w;
    texture->height = h;
    texture->type = type;
    texture->flags = imageFlags;
    setBindTexture (texture->tex);

    glPixelStorei (GL_UNPACK_ALIGNMENT,1);

  #ifndef NANOVG_GLES2
    glPixelStorei (GL_UNPACK_ROW_LENGTH, texture->width);
    glPixelStorei (GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei (GL_UNPACK_SKIP_ROWS, 0);
  #endif

  #ifdef NANOVG_GL2
    // GL 1.4 and later has support for generating mipmaps using a tex parameter.
    if (imageFlags & IMAGE_GENERATE_MIPMAPS)
      glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
  #endif

    if (type == TEXTURE_RGBA)
      glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    else
    #ifdef NANOVG_GLES2
      glTexImage2D (GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
    #elif NANOVG_GLES3
      glTexImage2D (GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, data);
    #else
      glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, data);
    #endif

    if (imageFlags & cVg::IMAGE_GENERATE_MIPMAPS)
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
       imageFlags & cVg::IMAGE_NEAREST ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR);

    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, imageFlags &  cVg::IMAGE_NEAREST ? GL_NEAREST : GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, imageFlags &  cVg::IMAGE_NEAREST ? GL_NEAREST : GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, imageFlags &  cVg::IMAGE_REPEATX ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, imageFlags &  cVg::IMAGE_REPEATY ? GL_REPEAT : GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

  #ifndef NANOVG_GLES2
    glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei (GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei (GL_UNPACK_SKIP_ROWS, 0);
  #endif

    // The new way to build mipmaps on GLES and GL3
  #ifndef NANOVG_GL2
    if (imageFlags &  cVg::IMAGE_GENERATE_MIPMAPS)
      glGenerateMipmap (GL_TEXTURE_2D);
  #endif

    setBindTexture (0);

    return texture->id;
    }
  //}}}
  //{{{
  bool renderUpdateTexture (int image, int x, int y, int w, int h, const unsigned char* data) {

    auto texture = findTexture (image);
    if (texture == nullptr)
      return false;
    setBindTexture (texture->tex);

    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

  #ifndef NANOVG_GLES2
    glPixelStorei (GL_UNPACK_ROW_LENGTH, texture->width);
    glPixelStorei (GL_UNPACK_SKIP_PIXELS, x);
    glPixelStorei (GL_UNPACK_SKIP_ROWS, y);
  #else
    // No support for all of skip, need to update a whole row at a time.
    if (texture->type == TEXTURE_RGBA)
      data += y * texture->width * 4;
    else
      data += y * texture->width;
    x = 0;
    w = texture->width;
  #endif

    if (texture->type == TEXTURE_RGBA)
      glTexSubImage2D (GL_TEXTURE_2D, 0, x,y, w,h, GL_RGBA, GL_UNSIGNED_BYTE, data);
    else
    #ifdef NANOVG_GLES2
      glTexSubImage2D (GL_TEXTURE_2D, 0, x,y, w,h, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
    #else
      glTexSubImage2D (GL_TEXTURE_2D, 0, x,y, w,h, GL_RED, GL_UNSIGNED_BYTE, data);
    #endif

    glPixelStorei (GL_UNPACK_ALIGNMENT, 4);

  #ifndef NANOVG_GLES2
    glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei (GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei (GL_UNPACK_SKIP_ROWS, 0);
  #endif

    setBindTexture (0);
    return true;
    }
  //}}}
  //{{{
  bool renderDeleteTexture (int image) {

    for (int i = 0; i < mNumTextures; i++) {
      if (mTextures[i].id == image) {
        if (mTextures[i].tex != 0 && (mTextures[i].flags & IMAGE_NODELETE) == 0)
          glDeleteTextures (1, &mTextures[i].tex);
        memset (&mTextures[i], 0, sizeof(mTextures[i]));
        return true;
        }
      }

    return false;
    }
  //}}}

private:
  //{{{  enums
  enum eShaderType { SHADER_FILL_GRADIENT = 0, SHADER_FILL_IMAGE, SHADER_SIMPLE, SHADER_IMAGE };
  enum eUniformLocation { LOCATION_VIEWSIZE, LOCATION_TEX, LOCATION_FRAG, MAX_LOCATIONS };
  enum eUniformBindings { FRAG_BINDING };
  //}}}
  //{{{
  class cDraw {
  public:
    enum eType { TEXT, TRIANGLE, CONVEX_FILL, STENCIL_FILL, STROKE };
    //{{{
    void set (eType type, int image, int firstPathVerticesIndex, int numPaths, int firstFragIndex,
              int firstVertexIndex, int numVertices) {
      mType = type;
      mImage = image;

      mFirstPathVerticesIndex = firstPathVerticesIndex;
      mNumPaths = numPaths;

      mFirstFragIndex = firstFragIndex;

      mTriangleFirstVertexIndex = firstVertexIndex;
      mNumTriangleVertices = numVertices;
      }
    //}}}

    eType mType;
    int mImage = 0;

    int mFirstPathVerticesIndex = 0;
    int mNumPaths = 0;

    int mFirstFragIndex = 0;

    int mTriangleFirstVertexIndex = 0;
    int mNumTriangleVertices = 0;
    };
  //}}}
  //{{{
  class cTexture {
  public:
    int id = 0;
    GLuint tex = 0;

    int width = 0;
    int height = 0;
    int type = 0;
    int flags = 0;
    };
  //}}}
  //{{{
  class cFrag {
  public:
    //{{{
    void setFill (cPaint* paint, cScissor* scissor, float width, float fringe, float strokeThreshold, cTexture* tex) {

      innerColor = nvgPremulColor (paint->innerColor);
      outerColor = nvgPremulColor (paint->outerColor);

      if ((scissor->extent[0] < -0.5f) || (scissor->extent[1] < -0.5f)) {
        memset (scissorMatrix, 0, sizeof(scissorMatrix));
        scissorExt[0] = 1.0f;
        scissorExt[1] = 1.0f;
        scissorScale[0] = 1.0f;
        scissorScale[1] = 1.0f;
        }
      else {
        cTransform inverse;
        scissor->mTransform.getInverse (inverse);
        inverse.getMatrix3x4 (scissorMatrix);
        scissorExt[0] = scissor->extent[0];
        scissorExt[1] = scissor->extent[1];
        scissorScale[0] = scissor->mTransform.getAverageScaleX() / fringe;
        scissorScale[1] = scissor->mTransform.getAverageScaleY() / fringe;
        }

      memcpy (extent, paint->extent, sizeof(extent));
      strokeMult = (width * 0.5f + fringe * 0.5f) / fringe;
      this->strokeThreshold = strokeThreshold;

      cTransform inverse;
      if (paint->image) {
        type = SHADER_FILL_IMAGE;
        if ((tex->flags & cVg::IMAGE_FLIPY) != 0) {
          //{{{  flipY
          cTransform m1;
          m1.setTranslate (0.0f, extent[1] * 0.5f);
          m1.multiply (paint->mTransform);

          cTransform m2;
          m2.setScale (1.0f, -1.0f);
          m2.multiply (m1);
          m1.setTranslate (0.0f, -extent[1] * 0.5f);
          m1.multiply (m2);
          m1.getInverse (inverse);
          }
          //}}}
        else
          paint->mTransform.getInverse (inverse);

      #ifdef NANOVG_UNIFORMBUFFER
        if (tex->type == TEXTURE_RGBA)
          texType = (tex->flags & cVg::IMAGE_PREMULTIPLIED) ? 0 : 1;
        else
          texType = 2;
      #else
        if (tex->type == TEXTURE_RGBA)
          texType = (tex->flags & cVg::IMAGE_PREMULTIPLIED) ? 0.f : 1.f;
        else
          texType = 2.f;
      #endif
        }
      else {
        type = SHADER_FILL_GRADIENT;
        radius = paint->radius;
        feather = paint->feather;
        paint->mTransform.getInverse (inverse);
        }
      inverse.getMatrix3x4 (paintMatrix);
      }
    //}}}
    //{{{
    void setImage (cPaint* paint, cScissor* scissor, cTexture* tex) {

      innerColor = nvgPremulColor (paint->innerColor);
      outerColor = nvgPremulColor (paint->outerColor);

      if ((scissor->extent[0] < -0.5f) || (scissor->extent[1] < -0.5f)) {
        memset (scissorMatrix, 0, sizeof(scissorMatrix));
        scissorExt[0] = 1.0f;
        scissorExt[1] = 1.0f;
        scissorScale[0] = 1.0f;
        scissorScale[1] = 1.0f;
        }
      else {
        cTransform inverse;
        scissor->mTransform.getInverse (inverse);
        inverse.getMatrix3x4 (scissorMatrix);
        scissorExt[0] = scissor->extent[0];
        scissorExt[1] = scissor->extent[1];
        scissorScale[0] = scissor->mTransform.getAverageScaleX();
        scissorScale[1] = scissor->mTransform.getAverageScaleY();
        }

      memcpy (extent, paint->extent, sizeof(extent));
      strokeMult = 1.0f;
      strokeThreshold = -1.0f;

      type = SHADER_IMAGE;
    #ifdef NANOVG_UNIFORMBUFFER
      if (tex->type == TEXTURE_RGBA)
        texType = (tex->flags & cVg::IMAGE_PREMULTIPLIED) ? 0 : 1;
      else
        texType = 2;
    #else
      if (tex->type == TEXTURE_RGBA)
        texType = (tex->flags & cVg::IMAGE_PREMULTIPLIED) ? 0.f : 1.f;
      else
        texType = 2.f;
    #endif

      cTransform inverse;
      paint->mTransform.getInverse (inverse);
      inverse.getMatrix3x4 (paintMatrix);
      }
    //}}}
    //{{{
    void setSimple() {
      type = SHADER_SIMPLE;
      strokeThreshold = -1.0f;
      }
    //}}}

  private:
  #ifdef NANOVG_UNIFORMBUFFER
    float scissorMatrix[12]; // 3vec4's
    float paintMatrix[12];   // 3vec4's
    struct sVgColour innerColor;
    struct sVgColour outerColor;
    float scissorExt[2];
    float scissorScale[2];
    float extent[2];
    float radius;
    float feather;
    float strokeMult;
    float strokeThreshold;
    int texType;
    int type;
  #else
    union {
      struct {
        float scissorMatrix[12]; // 3vec4's
        float paintMatrix[12];   // 3vec4's
        struct sVgColour innerColor;
        struct sVgColour outerColor;
        float scissorExt[2];
        float scissorScale[2];
        float extent[2];
        float radius;
        float feather;
        float strokeMult;
        float strokeThreshold;
        float texType;
        float type;
        };
      #define NANOVG_GL_UNIFORMARRAY_SIZE 11
      float uniformArray[NANOVG_GL_UNIFORMARRAY_SIZE][4];
      };
  #endif
    };
  //}}}
  //{{{
  class cShader {
  public:
    //{{{
    ~cShader() {
      if (prog)
        glDeleteProgram (prog);
      if (vert)
        glDeleteShader (vert);
      if (frag)
        glDeleteShader (frag);
      }
    //}}}

    //{{{
    bool create (const char* opts) {

      const char* str[3];
      str[0] = kShaderHeader;
      str[1] = opts != nullptr ? opts : "";

      prog = glCreateProgram();
      vert = glCreateShader (GL_VERTEX_SHADER);
      str[2] = kVertShader;
      glShaderSource (vert, 3, str, 0);

      frag = glCreateShader (GL_FRAGMENT_SHADER);
      str[2] = kFragShader;
      glShaderSource (frag, 3, str, 0);

      glCompileShader (vert);
      GLint status;
      glGetShaderiv (vert, GL_COMPILE_STATUS, &status);
      if (status != GL_TRUE) {
        //{{{  error return
        dumpShaderError (vert, "shader", "vert");
        return false;
        }
        //}}}

      glCompileShader (frag);
      glGetShaderiv (frag, GL_COMPILE_STATUS, &status);
      if (status != GL_TRUE) {
        //{{{  error return
        dumpShaderError (frag, "shader", "frag");
        return false;
        }
        //}}}

      glAttachShader (prog, vert);
      glAttachShader (prog, frag);

      glBindAttribLocation (prog, 0, "vertex");
      glBindAttribLocation (prog, 1, "tcoord");

      glLinkProgram (prog);
      glGetProgramiv (prog, GL_LINK_STATUS, &status);
      if (status != GL_TRUE) {
        //{{{  error return
        dumpProgramError (prog, "shader");
        return false;
        }
        //}}}

      glUseProgram (prog);

      return true;
      }
    //}}}

    //{{{
    void getUniforms() {

      location[LOCATION_VIEWSIZE] = glGetUniformLocation (prog, "viewSize");
      location[LOCATION_TEX] = glGetUniformLocation (prog, "tex");

    #ifdef NANOVG_UNIFORMBUFFER
      location[LOCATION_FRAG] = glGetUniformBlockIndex (prog, "frag");
    #else
      location[LOCATION_FRAG] = glGetUniformLocation (prog, "frag");
    #endif
      }
    //}}}

    //{{{
    void setTex (int tex) {
      glUniform1i (location[LOCATION_TEX], tex);
      }
    //}}}
    //{{{
    void setViewport (float* viewport) {
      glUniform2fv (location[LOCATION_VIEWSIZE], 1, viewport);
      }
    //}}}

  #ifdef NANOVG_UNIFORMBUFFER
    //{{{
    void bindUniformBlock() {
      glUniformBlockBinding (prog, location[LOCATION_FRAG], FRAG_BINDING);
      }
    //}}}
  #else
    //{{{
    void setFrags (float* frags) {
      glUniform4fv (location[LOCATION_FRAG], NANOVG_GL_UNIFORMARRAY_SIZE, frags);
      }
    //}}}
  #endif

  private:
    //{{{
    const char* kShaderHeader =

    #ifdef NANOVG_GLES2
      "#version 100\n"
      "#define NANOVG_GL2 1\n"
    #elif NANOVG_GLES3
      "#version 300 es\n"
      "#define NANOVG_GL3 1\n"
    #elif NANOVG_GL2
      "#define NANOVG_GL2 1\n"
    #else
      "#version 150 core\n"
      "#define NANOVG_GL3 1\n"
    #endif

    #ifdef NANOVG_UNIFORMBUFFER
      "#define USE_UNIFORMBUFFER 1\n"
    #else
      "#define UNIFORMARRAY_SIZE 11\n"
    #endif
      "\n";
    //}}}
    //{{{
    const char* kVertShader =

      "uniform vec2 viewSize;"

    #ifdef NANOVG_GL3
      "in vec2 vertex;"
      "in vec2 tcoord;"
      "out vec2 ftcoord;"
      "out vec2 fpos;"
    #else
      "attribute vec2 vertex;"
      "attribute vec2 tcoord;"
      "varying vec2 ftcoord;"
      "varying vec2 fpos;"
    #endif

      "void main() {"
        "ftcoord = tcoord;"
        "fpos = vertex;"
        "gl_Position = vec4(2.0*vertex.x/viewSize.x - 1.0, 1.0 - 2.0*vertex.y/viewSize.y, 0, 1);"
        "}";
    //}}}
    //{{{
    const char* kFragShader =
      //{{{  precision
      "#ifdef GL_ES\n"
        "#if defined(GL_FRAGMENT_PRECISION_HIGH) || defined(NANOVG_GL3)\n"
          "precision highp float;\n"
        "#else\n"
          "precision mediump float;\n"
        "#endif\n"
      "#endif\n"
      //}}}
      //{{{  vars
      "#ifdef NANOVG_GL3\n"
        "#ifdef USE_UNIFORMBUFFER\n"
          "layout(std140) uniform frag {\n"
            "mat3 scissorMatrix;\n"
            "mat3 paintMatrix;\n"
            "vec4 innerColor;\n"
            "vec4 outerColor;\n"
            "vec2 scissorExt;\n"
            "vec2 scissorScale;\n"
            "vec2 extent;\n"
            "float radius;\n"
            "float feather;\n"
            "float strokeMult;\n"
            "float strokeThreshold;\n"
            "int texType;\n"
            "int type;\n"
            "};\n"
        "#else\n"
          "uniform vec4 frag[UNIFORMARRAY_SIZE];\n"
        "#endif\n"

        "uniform sampler2D tex;\n"
        "in vec2 ftcoord;\n"
        "in vec2 fpos;\n"
        "out vec4 outColor;\n"
      "#else\n"
        "uniform vec4 frag[UNIFORMARRAY_SIZE];\n"
        "uniform sampler2D tex;\n"
        "varying vec2 ftcoord;\n"
        "varying vec2 fpos;\n"
      "#endif\n"

      "#ifndef USE_UNIFORMBUFFER\n"
        "#define scissorMatrix mat3(frag[0].xyz, frag[1].xyz, frag[2].xyz)\n"
        "#define paintMatrix mat3(frag[3].xyz, frag[4].xyz, frag[5].xyz)\n"
        "#define innerColor frag[6]\n"
        "#define outerColor frag[7]\n"
        "#define scissorExt frag[8].xy\n"
        "#define scissorScale frag[8].zw\n"
        "#define extent frag[9].xy\n"
        "#define radius frag[9].z\n"
        "#define feather frag[9].w\n"
        "#define strokeMult frag[10].x\n"
        "#define strokeThreshold frag[10].y\n"
        "#define texType int(frag[10].z)\n"
        "#define type int(frag[10].w)\n"
      "#endif\n"
      //}}}

      "float sdroundrect(vec2 pt, vec2 ext, float rad) {"
        "vec2 ext2 = ext - vec2(rad,rad);"
        "vec2 d = abs(pt) - ext2;"
        "return min(max(d.x,d.y),0.0) + length(max(d,0.0)) - rad;"
        "}"

      // Scissoring
      "float scissorMask(vec2 p) {"
        "vec2 sc = (abs((scissorMatrix * vec3(p,1.0)).xy) - scissorExt);"
        "sc = vec2(0.5,0.5) - sc * scissorScale;"
        "return clamp(sc.x,0.0,1.0) * clamp(sc.y,0.0,1.0);"
        "}\n"

      // EDGE_AA Stroke - from [0..1] to clipped pyramid, where the slope is 1px
      "float strokeMask() { return min(1.0, (1.0-abs(ftcoord.x*2.0-1.0))*strokeMult) * min(1.0, ftcoord.y); }\n"

      "void main() {\n"
        "vec4 result;\n"
        "float scissor = scissorMask(fpos);\n"
        "float strokeAlpha = strokeMask();\n"

        "if (type == 0) {\n"
          //{{{  SHADER_FILL_GRADIENT - calc grad color using box grad
          "vec2 pt = (paintMatrix * vec3(fpos,1.0)).xy;\n"
          "float d = clamp((sdroundrect(pt, extent, radius) + feather*0.5) / feather, 0.0, 1.0);\n"
          "vec4 color = mix(innerColor,outerColor,d);\n"

          // Combine alpha
          "color *= strokeAlpha * scissor;\n"
          "result = color;\n"
          //}}}
        "} else if (type == 1) {\n"
          //{{{  SHADER_FILL_IMAGE - image calc color fron texture
          "vec2 pt = (paintMatrix * vec3(fpos,1.0)).xy / extent;\n"

          "#ifdef NANOVG_GL3\n"
            "vec4 color = texture(tex, pt);\n"
          "#else\n"
            "vec4 color = texture2D(tex, pt);\n"
          "#endif\n"

          "if (texType == 1) color = vec4(color.xyz*color.w,color.w);"
          "if (texType == 2) color = vec4(color.x);"

          "color *= innerColor;\n"            // apply color tint and alpha
          "color *= strokeAlpha * scissor;\n" // combine alpha
          "result = color;\n"
          //}}}
        "} else if (type == 2) {\n"
          //{{{  SHADER_SIMPLE - stencil fill
          "result = vec4(1,1,1,1);\n"
          //}}}
        "} else if (type == 3) {\n"
          //{{{  SHADER_IMAGE - textured tris
          "#ifdef NANOVG_GL3\n"
            "vec4 color = texture(tex, ftcoord);\n"
          "#else\n"
            "vec4 color = texture2D(tex, ftcoord);\n"
          "#endif\n"

          "if (texType == 1) color = vec4(color.xyz*color.w,color.w);"
          "if (texType == 2) color = vec4(color.x);"

          "color *= scissor;\n"
          "result = color * innerColor;\n"
          //}}}
        "}\n"

      "if (strokeAlpha < strokeThreshold) discard;\n"

      "#ifdef NANOVG_GL3\n"
        "outColor = result;\n"
      "#else\n"
        "gl_FragColor = result;\n"
      "#endif\n"

      "}\n";
    //}}}

    //{{{
    void dumpShaderError (GLuint shader, const char* name, const char* type) {

      GLchar str[512+1];
      GLsizei len = 0;
      glGetShaderInfoLog (shader, 512, &len, str);
      if (len > 512)
        len = 512;
      str[len] = '\0';

      printf ("Shader %s/%s error:%s\n", name, type, str);
      }
    //}}}
    //{{{
    void dumpProgramError (GLuint prog, const char* name) {

      GLchar str[512+1];
      GLsizei len = 0;
      glGetProgramInfoLog (prog, 512, &len, str);
      if (len > 512)
        len = 512;
      str[len] = '\0';

      printf ("Program %s error:%s\n", name, str);
      }
    //}}}

    // vars
    GLuint prog = 0;
    GLuint frag = 0;
    GLuint vert = 0;

    GLint location[MAX_LOCATIONS];
    };
  //}}}

  //{{{
  cDraw* allocDraw() {
  // allocate a draw, return pointer to it

    if (mNumDraws + 1 > mNumAllocatedDraws) {
      mNumAllocatedDraws = maxi (mNumDraws + 1, 128) + mNumAllocatedDraws / 2; // 1.5x Overallocate
      mDraws = (cDraw*)realloc (mDraws, sizeof(cDraw) * mNumAllocatedDraws);
      }
    return &mDraws[mNumDraws++];
    }
  //}}}
  //{{{
  int allocFrags (int numFrags) {
  // allocate numFrags, return index of first

    if (mNumFrags + numFrags > mNumAllocatedFrags) {
      mNumAllocatedFrags = maxi (mNumFrags + numFrags, 128) + mNumAllocatedFrags / 2; // 1.5x Overallocate
      mFrags = (cFrag*)realloc (mFrags, mNumAllocatedFrags * sizeof(cFrag));
      }

    int firstFragIndex = mNumFrags;
    mNumFrags += numFrags;
    return firstFragIndex;
    }
  //}}}
  //{{{
  int allocPathVertices (int numPaths) {
  // allocate numPaths pathVertices, return index of first

    if (mNumPathVertices + numPaths > mNumAllocatedPathVertices) {
      mNumAllocatedPathVertices = maxi (mNumPathVertices + numPaths, 128) + mNumAllocatedPathVertices / 2; // 1.5x Overallocate
      mPathVertices = (cPathVertices*)realloc (mPathVertices, mNumAllocatedPathVertices * sizeof(cPathVertices));
      }

    int firstPathVerticeIndex = mNumPathVertices;
    mNumPathVertices += numPaths;
    return firstPathVerticeIndex;
    }
  //}}}
  //{{{
  cTexture* allocTexture() {

    cTexture* texture = nullptr;
    for (int i = 0; i < mNumTextures; i++) {
      if (mTextures[i].id == 0) {
        texture = &mTextures[i];
        break;
        }
      }

    if (texture == nullptr) {
      if (mNumTextures + 1 > mNumAllocatedTextures) {
        mNumAllocatedTextures = maxi (mNumTextures + 1, 4) +  mNumAllocatedTextures / 2; // 1.5x Overallocate
        mTextures = (cTexture*)realloc (mTextures, mNumAllocatedTextures * sizeof(cTexture));
        if (mTextures == nullptr)
          return nullptr;
        }
      texture = &mTextures[mNumTextures++];
      }

    memset (texture, 0, sizeof(*texture));
    texture->id = ++mTextureId;
    return texture;
    }
  //}}}
  //{{{
  cTexture* findTexture (int textureId) {

    for (int i = 0; i < mNumTextures; i++)
      if (mTextures[i].id == textureId)
        return &mTextures[i];

    return nullptr;
    }
  //}}}

  //{{{
  void setStencilMask (GLuint mask) {

    if (mStencilMask != mask) {
      mStencilMask = mask;
      glStencilMask (mask);
      }
    }
  //}}}
  //{{{
  void setStencilFunc (GLenum func, GLint ref, GLuint mask) {

    if ((mStencilFunc != func) || (mStencilFuncRef != ref) || (mStencilFuncMask != mask)) {
      mStencilFunc = func;
      mStencilFuncRef = ref;
      mStencilFuncMask = mask;
      glStencilFunc (func, ref, mask);
      }
    }
  //}}}
  //{{{
  void setBindTexture (GLuint texture) {

    if (mBindTexture != texture) {
      mBindTexture = texture;
      glBindTexture (GL_TEXTURE_2D, texture);
      }
    }
  //}}}
  //{{{
  void setUniforms (int firstFragIndex, int image) {

  #ifdef NANOVG_UNIFORMBUFFER
    glBindBufferRange (GL_UNIFORM_BUFFER, FRAG_BINDING, mFragBuffer, firstFragIndex * sizeof(cFrag), sizeof(cFrag));
  #else
    mShader.setFrags ((float*)(&mFrags[firstFragIndex]));
  #endif

    if (image) {
      auto tex = findTexture (image);
      setBindTexture (tex ? tex->tex : 0);
      }
    else
      setBindTexture (0);
    }
  //}}}

  //{{{
  GLenum convertBlendFuncFactor (eBlendFactor factor) {

    switch (factor) {
      case NVG_ZERO:
        return GL_ZERO;
      case NVG_ONE:
        return GL_ONE;
      case NVG_SRC_COLOR:
        return GL_SRC_COLOR;
      case NVG_ONE_MINUS_SRC_COLOR:
        return GL_ONE_MINUS_SRC_COLOR;
      case NVG_DST_COLOR:
        return GL_DST_COLOR;
      case NVG_ONE_MINUS_DST_COLOR:
        return GL_ONE_MINUS_DST_COLOR;
      case NVG_SRC_ALPHA:
        return GL_SRC_ALPHA;
      case  NVG_ONE_MINUS_SRC_ALPHA:
        return GL_ONE_MINUS_SRC_ALPHA;
      case NVG_DST_ALPHA:
        return GL_DST_ALPHA;
      case NVG_ONE_MINUS_DST_ALPHA:
        return GL_ONE_MINUS_DST_ALPHA;
      case NVG_SRC_ALPHA_SATURATE:
        return GL_SRC_ALPHA_SATURATE;
      default:
        return GL_INVALID_ENUM;
        }
    }
  //}}}

  //{{{  vars
  float mViewport[2];
  cShader mShader;

  GLuint mStencilMask = 0;
  GLenum mStencilFunc = 0;
  GLint mStencilFuncRef = 0;
  GLuint mStencilFuncMask = 0;
  GLuint mBindTexture = 0;

  int mTextureId = 0;
  int mNumTextures = 0;
  int mNumAllocatedTextures = 0;
  cTexture* mTextures = nullptr;

  GLuint mVertexBuffer = 0;
  GLuint mVertexArray = 0;
  GLuint mFragBuffer = 0;

  // per frame buffers
  int mNumDraws = 0;
  int mNumAllocatedDraws = 0;
  cDraw* mDraws = nullptr;

  int mNumFrags = 0;
  int mNumAllocatedFrags = 0;
  cFrag* mFrags = nullptr;

  int mNumPathVertices = 0;
  int mNumAllocatedPathVertices = 0;
  cPathVertices* mPathVertices = nullptr;
  //}}}
  };
