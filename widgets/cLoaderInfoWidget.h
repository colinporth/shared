// cLoaderInfoWidget.h
#pragma once
//{{{  includes
#include "cWidget.h"

#include "../utils/cLoader.h"
#include "../utils/cSong.h"
#include "../utils/iVideoPool.h"
//}}}
class cLoaderInfoWidget : public cWidget {
public:
  //{{{
  cLoaderInfoWidget (cLoader* loader, const std::string& id = "cLoaderInfoWidget")
    : cWidget (kBlackF, 0.f, kBox, id), mLoader(loader) {}
  //}}}
  virtual ~cLoaderInfoWidget() {}

  void toggleGraphics() { mShowGraphics = ! mShowGraphics; }

  virtual void onDraw (iDraw* draw) {

    cVg* vg = draw->getVg();
    cSong* song = mLoader->getSong();
    iVideoPool* videoPool = mLoader->getVideoPool();

    if (mShowGraphics) {
      // info text
      std::string str;
      if (videoPool)
        str = videoPool->getInfoString() + " ";
      str += mLoader->getInfoString();

      vg->setFontSize (kFontHeight);
      vg->setTextAlign (cVg::eAlignLeft | cVg::eAlignTop);
      vg->setFillColour (kBlackF);
      vg->text (mOrg + cPointF(2.f, 2.f), str);
      vg->setFillColour (kWhiteF);
      vg->text (mOrg, str);
      }
    }

private:
  cLoader* mLoader;
  bool mShowGraphics = true;
  };
