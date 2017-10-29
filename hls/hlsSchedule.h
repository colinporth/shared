// hlsSchedule.h
#pragma once
//{{{  includes
#include "../rapidjson/document.h"
#include "../widgets/cDecodePicWidget.h"

#include "../utils/utils.h"
#include "../utils/cLog.h"
//}}}

//{{{
static string getTimeStrFromSecs (time_t secsSinceMidnight) {

  return dec(secsSinceMidnight / (60*60)) + ':' +
         dec((secsSinceMidnight / 60) % 60, 2, '0') + ':' +
         dec(secsSinceMidnight % 60, 2, '0');
  }
//}}}

//{{{
class cScheduleItem {
public:
  void* operator new (size_t size) { return smallMalloc (size, "cScheduleItem"); }
  void operator delete (void *ptr) { smallFree (ptr); }

  time_t mStart;
  uint32_t mDuration;
  std::string mTitle;
  std::string mSynopsis;
  std::string mImagePid;
  };
//}}}
//{{{
class cSchedule {
public:
  void* operator new (size_t size) { return smallMalloc (size, "cSchedule"); }
  void operator delete (void *ptr) { smallFree (ptr); }

  //{{{
  void load (cHttp& http, uint16_t chan) {

    cLog::log (LOGINFO1, "cSchedule::load");

    // get schedule json
    switch (chan) {
      case 1: http.get ("www.bbc.co.uk", "radio1/programmes/schedules/england/today.json"); break;
      case 2: http.get ("www.bbc.co.uk", "radio2/programmes/schedules/today.json"); break;
      case 3: http.get ("www.bbc.co.uk", "radio3/programmes/schedules/today.json"); break;
      case 4: http.get ("www.bbc.co.uk", "radio4/programmes/schedules/fm/today.json"); break;
      case 5: http.get ("www.bbc.co.uk", "5live/programmes/schedules/today.json"); break;
      case 6: http.get ("www.bbc.co.uk", "6music/programmes/schedules/today.json"); break;
      default:;
      }

    // parse it
    rapidjson::Document schedule;
    auto parseError = schedule.Parse ((const char*)http.getContent(), http.getContentSize()).HasParseError();
    http.freeContent();
    if (parseError) {
      cLog::log (LOGERROR, "cSchedule::load error");
      return;
      }

    for (auto& element : schedule["schedule"]["day"]["broadcasts"].GetArray()) {
      auto item = new cScheduleItem();
      auto broadcast = element.GetObject();
      //item->mStart    = getTime (broadcast["start"].GetString());
      item->mDuration = broadcast["duration"].GetInt();
      if (broadcast["programme"]["display_titles"]["title"].IsString())
        item->mTitle    = broadcast["programme"]["display_titles"]["title"].GetString();
      item->mSynopsis = broadcast["programme"]["short_synopsis"].GetString();
      item->mImagePid = broadcast["programme"]["image"]["pid"].GetString();
      mSchedule.push_back (item);
      }

    cLog::log (LOGINFO1, "cSchedule::load done size:%d %s %s",
                         http.getContentSize(), http.getLastHost().c_str(), http.getLastPath().c_str());
    list();
    }
  //}}}

  //{{{
  cScheduleItem* findItem (uint32_t secs) {

    for (auto item : mSchedule)
      if ((secs >= item->mStart) && (secs < item->mStart + item->mDuration))
        return item;
    return nullptr;
    }
  //}}}
  //{{{
  void clear() {
    for (auto item : mSchedule) {
      delete (item);
      item = nullptr;
      }
    mSchedule.clear();
    }
  //}}}

private:
  //{{{
  void list() {

    for (auto item : mSchedule)
      cLog::log (LOGINFO2, getTimeStrFromSecs (item->mStart) + " " +
                           getTimeStrFromSecs (item->mDuration) + " " +
                           item->mTitle);
    }
  //}}}

  std::vector<cScheduleItem*> mSchedule;
  };
//}}}

//string mImagePid;
//uint8_t* mContent = nullptr;
//int mContentSize = 0;
//bool mPicChanged = false;
//int mPicValue = 0;
//cSchedule mSchedule;
//mSchedule.clear();
//mSchedule.load (http, chan);
//{{{
//cScheduleItem* findItem (uint32_t sec) {
  //return mSchedule.findItem (sec);
  //}
//}}}
//{{{
//void loadPicAtPlayFrame (cHttp& http) {

  //auto item = findItem (getPlayTzSec());
  //auto imagePid = item ? item->mImagePid : "";
  //if (imagePid != mImagePid) {
    //if (imagePid.empty())
      //mContent = nullptr;
    //else {
      //http.get ("ichef.bbci.co.uk", "images/ic/160x90/" + imagePid + ".jpg");
      //cLog::log (LOGINFO1, "loadPicAtPlayFrame imagePid:%s size:%d", imagePid.c_str(), http.getContentSize());
      //if (http.getContent()) {
        //new cJpegPic (3, http.getContent());
        //auto temp = (uint8_t*)bigMalloc (http.getContentSize(), "cHls::jpegPic");
        //memcpy (temp, http.getContent(), http.getContentSize());
        //mContentSize = http.getContentSize();
        //mContent = temp;
        //http.freeContent();
        //cLog::log (LOGINFO1, "- loaded imagePid:%s size:%d", imagePid.c_str(), http.getContentSize());
        //}
      //else
        //mContent = nullptr;
      //}
    //mImagePid = imagePid;
    //}
  //}
//}}}
