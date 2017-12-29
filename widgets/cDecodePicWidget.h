// cDecodePicWidget.h - glue cMultiPic and cPicWidget together
//{{{  includes
#pragma once

#include "cPicWidget.h"
#include "../decoders/cDecodePic.h"

extern "C" {
  uint8_t* stbi_load_from_memory (uint8_t* const* buffer, int len, int* x, int* y, int* channels_in_file, int desired_channels);
  }
//}}}

class cDecodePicWidget : public cPicWidget {
public:
  cDecodePicWidget() : cPicWidget (-1, mMyValue, mMyChanged) {}
  cDecodePicWidget (float width, float height) : cPicWidget (width, height, -1, mMyValue, mMyChanged) {}
  //{{{
  cDecodePicWidget (float width, float height, int selectValue, int& value, bool& changed)
    : cPicWidget (width, height, selectValue, value, changed) {}
  //}}}
  //{{{
  cDecodePicWidget (const uint8_t* buffer, int size, float width, float height, int selectValue, int& value, bool& changed)
      : cPicWidget (width, height, selectValue, value, changed) {
    setPic (buffer, size);
    }
  //}}}
  virtual ~cDecodePicWidget() {}

  //{{{
  void setPic (const uint8_t* buffer, int size) {
    #ifdef NANOVG
      int width, height, components;
      uint8_t* pic = stbi_load_from_memory ((uint8_t* const*)buffer, size, &width, &height, &components, 4);
      cPicWidget::setPic (pic, width, height, components);
    #else
      cDecodePic decodePic;
      decodePic.setPic (buffer, size, 3);
      cPicWidget::setPic (decodePic.getPic(), decodePic.getWidth(), decodePic.getHeight(), decodePic.getComponents());
    #endif
    }
  //}}}
  //{{{
  void setFileName (std::string fileName) {
    mFileName = fileName;
    cDecodePic decodePic;
    if (decodePic.setFileName (fileName))
      cPicWidget::setPic (decodePic.getPic(), decodePic.getWidth(), decodePic.getHeight(), decodePic.getComponents());
    }
  //}}}

  //{{{
  void onDraw (iDraw* draw) {
    cPicWidget::onDraw (draw);
    if (mFileName.size())
      draw->drawText (COL_WHITE, getFontHeight(), mFileName, mX+2, mY+1, 800, mHeight-1);
    }
  //}}}

private:
  bool mMyChanged;
  int mMyValue;

  std::string mFileName;
  };
