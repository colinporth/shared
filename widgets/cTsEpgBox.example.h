// cTsEpgBox.h
//{{{  includes
#pragma once

#include "../common/cD2dWindow.h"
#include "../../shared/dvb/cTransportStream.h"
//}}}

class cTsEpgBox : public cD2dWindow::cBox {
public:
  //{{{
  cTsEpgBox (cD2dWindow* window, float width, float height, cTransportStream* ts)
      : cBox("tsEpg", window, width, height), mTs(ts) {}
  //}}}
  virtual ~cTsEpgBox() {}

  //{{{
  bool onDown (bool right, cPoint pos)  {

    if (!getTimedOn() || mWindow->getTimedMenuOn()) {
      pos += getTL();

      for (auto boxItem : mBoxItemVec)
        if (boxItem->inside (pos)) {
          boxItem->onDown();
          getWindow()->changed();
          return true;
          }

      togglePin();
      }

    return true;
    }
  //}}}
  //{{{
  void onDraw (ID2D1DeviceContext* dc) {

    if (!getTimedOn() || mWindow->getTimedMenuOn()) {
      std::lock_guard<std::mutex> lockGuard (mTs->mMutex);
      clear();
      if (mTs->mServiceMap.size() > 1) {
        draw (dc, mRect);
        }
      }
    }
  //}}}

protected:
  //{{{
  class cBoxItem {
  public:
    cBoxItem (cTsEpgBox* box, cService* service, float textHeight, cRect r) :
      mBox(box), mService(service), mRect(r), mTextHeight(textHeight) {}
    ~cBoxItem() {}

    bool inside (const cPoint& pos) { return mRect.inside (pos); }
    void setRect (const cRect& r) { mRect = r; }

    virtual void onDown() {}

    virtual void onDraw (ID2D1DeviceContext* dc) {
      mRect.right = mRect.left + mBox->drawText (
        dc, mStr, mBox->getWindow()->getTextFormat(), mRect, mBrush, mTextHeight);
      }

  protected:
    cTsEpgBox* mBox;
    cService* mService;
    cRect mRect;
    const float mTextHeight;

    std::string mStr;
    ID2D1SolidColorBrush* mBgndBrush = nullptr;
    ID2D1SolidColorBrush* mBrush = nullptr;
    };
  //}}}
  //{{{
  void draw (ID2D1DeviceContext* dc, cRect r) {

    auto todayTime = mTs->getTime();
    auto todayDatePoint = date::floor<date::days>(todayTime);
    auto todayYearMonthDay = date::year_month_day{todayDatePoint};
    auto today = todayYearMonthDay.day();

    const float kLineHeight = 16.f;
    const float kSmallLineHeight = 13.f;

    auto lineHeight = (getHeight() / mTs->mServiceMap.size() > kLineHeight) ? kLineHeight : kSmallLineHeight;
    auto bigLineHeight = lineHeight*4.f/3.f;
    r.left += lineHeight / 5.f;

    for (auto& service : mTs->mServiceMap) {
      r.bottom = r.top + bigLineHeight;
      mBoxItemVec.push_back (new cServiceName (this, &service.second, bigLineHeight, r));
      r.top = r.bottom;

      if (service.second.getNowEpgItem()) {
        r.bottom = r.top + lineHeight;
        mBoxItemVec.push_back (new cServiceNow (this, &service.second, mTs, lineHeight, r));
        r.top = r.bottom;
        }

      if (service.second.getShowEpg()) {
        for (auto epgItem : service.second.getEpgItemMap()) {
          auto time = epgItem.second->getTime();
          auto datePoint = date::floor<date::days>(time);
          auto yearMonthDay = date::year_month_day{datePoint};
          auto day = yearMonthDay.day();
          if ((day == today) && (time > todayTime)) { // later today
            r.bottom = r.top + lineHeight;
            mBoxItemVec.push_back (new cServiceEpg (this, &service.second, epgItem.second, lineHeight, r));
            r.top = r.bottom;
            }
          }
        }
      r.top += lineHeight/4.f;
      r.bottom = r.top + lineHeight;
      }

    // draw services boxItems
    auto bgndRect = mRect;
    bgndRect.bottom = r.bottom;
    dc->FillRectangle (bgndRect, mWindow->getTransparentBgndBrush());
    for (auto boxItem : mBoxItemVec)
      boxItem->onDraw (dc);
    }
  //}}}
  //{{{
  void clear() {

    while (!mBoxItemVec.empty()) {
      auto boxItem = mBoxItemVec.back();
      delete boxItem;
      mBoxItemVec.pop_back();
      }
    }
  //}}}

  // vars
  cTransportStream* mTs;
  std::vector<cBoxItem*> mBoxItemVec;

private:
  //{{{
  class cServiceName : public cBoxItem {
  public:
    cServiceName (cTsEpgBox* box, cService* service, float textHeight, cRect r) :
        cBoxItem(box, service, textHeight, r) {
      mStr = service->getNameString();
      mBrush = mService->getShowEpg() ? mBox->getWindow()->getWhiteBrush() : mBox->getWindow()->getBlueBrush();
      }
    virtual ~cServiceName() {}

    virtual void onDown() { mService->toggleShowEpg(); }
    };
  //}}}
  //{{{
  class cServiceNow : public cBoxItem {
  public:
    cServiceNow (cTsEpgBox* box, cService* service, cTransportStream* ts, float textHeight, cRect r) :
        cBoxItem(box, service, textHeight,  r), mTs(ts) {

      mStr = "- " + date::format ("%H:%M", floor<std::chrono::seconds>(mService->getNowEpgItem()->getTime())) +
             " " + dec (std::chrono::duration_cast<std::chrono::minutes>(mService->getNowEpgItem()->getDuration()).count()) + "m" +
             " " + mService->getNowEpgItem()->getTitleString();
      mBrush = mService->getNowEpgItem()->getRecord() ? mBox->getWindow()->getWhiteBrush() : mBox->getWindow()->getBlueBrush();
      }
    virtual ~cServiceNow() {}

    //{{{
    virtual void onDown() {

      if (mService->getNowEpgItem()->toggleRecord())
        mTs->start (mService, mService->getNowEpgItem()->getTitleString(), mTs->getTime(), true);
      else
        mTs->stop (mService);
      }
    //}}}

  private:
    cTransportStream* mTs;
    };
  //}}}
  //{{{
  class cServiceEpg : public cBoxItem {
  public:
    cServiceEpg (cTsEpgBox* box, cService* service, cEpgItem* epgItem, float textHeight, cRect r) :
        cBoxItem(box, service, textHeight, r), mEpgItem(epgItem) {
      mStr = "  " + date::format ("%H:%M", floor<std::chrono::seconds>(mEpgItem->getTime())) +
             " " + mEpgItem->getTitleString();
      mBrush = mEpgItem->getRecord() ? mBox->getWindow()->getWhiteBrush() : mBox->getWindow()->getBlueBrush();
      }
    virtual ~cServiceEpg() {}

    virtual void onDown() { mEpgItem->toggleRecord(); }

  private:
    cEpgItem* mEpgItem;
    };
  //}}}
  };
