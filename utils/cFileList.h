// cFileList.h - windows only for now
//{{{  includes
#pragma once

#include <shobjidl.h>
#include <shlguid.h>
#include <shellapi.h>
#include <shlwapi.h>

#include <vector>
#include <chrono>
#include "date.h"

#pragma comment(lib,"shlwapi.lib")

using namespace std;
using namespace chrono;
//}}}

//{{{
class cFileItem {
public:
  //{{{
  cFileItem (const string& pathName, const string& fileName) :
      mPathName(pathName), mFileName(fileName) {

    if (pathName.empty()) {
      // extract any path name from filename
      string::size_type lastSlashPos = mFileName.rfind ('\\');
      if (lastSlashPos != string::npos) {
        mPathName = string (mFileName, 0, lastSlashPos);
        mFileName = mFileName.substr (lastSlashPos+1);
        }
      }

    // extract any extension from filename
    string::size_type lastDotPos = mFileName.rfind ('.');
    if (lastDotPos != string::npos) {
      mExtension = mFileName.substr (lastDotPos+1);
      mFileName = string (mFileName, 0, lastDotPos);
      }


    auto fileHandle = CreateFile (getFullName().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    mFileSize = GetFileSize (fileHandle, NULL);

    FILETIME creationTime;
    FILETIME lastAccessTime;
    FILETIME lastWriteTime;
    GetFileTime (fileHandle, &creationTime, &lastAccessTime, &lastWriteTime);

    CloseHandle (fileHandle);

    mCreationTimePoint = getFileTimePoint (creationTime);
    mLastAccessTimePoint = getFileTimePoint (lastAccessTime);
    mLastWriteTimePoint = getFileTimePoint (lastWriteTime);
    }
  //}}}
  virtual ~cFileItem() {}

  string getPathName() const { return mPathName; }
  string getFileName() const { return mFileName; }
  string getExtension() const { return mExtension; }
  string getFullName() const {
    return (mPathName.empty() ? mFileName : mPathName + "/" + mFileName) +
           (mExtension.empty() ? "" : "." + mExtension); }

  //{{{
  string getFileSizeString() const {

    if (mFileSize < 1000)
      return dec(mFileSize) + "b";

    if (mFileSize < 1000000)
      return frac((float)mFileSize/1000.f,4,1,' ') + "k";

    if (mFileSize < 1000000000)
      return frac((float)mFileSize/1000000.f,4,1,' ') + "m";

    return frac((float)mFileSize/1000000000.f,4,1,' ') + "g";
    }
  //}}}

  //{{{
  string getCreationTimeString() const {

    if (mCreationTimePoint.time_since_epoch() == seconds::zero())
      return "";
    else
      return date::format (kTimeFormatStr, floor<seconds>(mCreationTimePoint));
    }
  //}}}
  //{{{
  string getLastAccessTimeString() const {

    if (mLastAccessTimePoint.time_since_epoch() == seconds::zero())
      return "";
    else
      return date::format (kTimeFormatStr, floor<seconds>(mLastAccessTimePoint));
    }
  //}}}
  //{{{
  string getLastWriteTimeString() const {

    if (mLastWriteTimePoint.time_since_epoch() == seconds::zero())
      return "";
    else
      return date::format (kTimeFormatStr, floor<seconds>(mLastWriteTimePoint));
    }
  //}}}

private:
  const string kTimeFormatStr = "%D %T";

  //{{{
  system_clock::time_point getFileTimePoint (FILETIME fileTime) {

    // filetime_duration has the same layout as FILETIME; 100ns intervals
    using filetime_duration = duration<int64_t, ratio<1, 10'000'000>>;

    // January 1, 1601 (NT epoch) - January 1, 1970 (Unix epoch):
    constexpr duration<int64_t> nt_to_unix_epoch{INT64_C(-11644473600)};

    const filetime_duration asDuration{static_cast<int64_t>(
        (static_cast<uint64_t>((fileTime).dwHighDateTime) << 32)
            | (fileTime).dwLowDateTime)};
    const auto withUnixEpoch = asDuration + nt_to_unix_epoch;
    return system_clock::time_point{ duration_cast<system_clock::duration>(withUnixEpoch)};
    }
  //}}}

  string mPathName;
  string mFileName;
  string mExtension;

  uint64_t mFileSize = 0;
  time_point<system_clock> mCreationTimePoint;
  time_point<system_clock> mLastAccessTimePoint;
  time_point<system_clock> mLastWriteTimePoint;
  };
//}}}

class cFileList {
public:
  //{{{
  cFileList (const string& fileName, const string& matchString) {

    if (!fileName.empty()) {
      mMatchString = matchString;

      auto resolvedFileName = fileName;
      if (mFileName.find (".lnk") <= fileName.size()) {
        resolvedFileName = resolveShortcut (fileName);
        if (resolvedFileName.empty()) {
          cLog::log (LOGERROR, "cFileList - link " + fileName + " unresolved");
          return;
          }
        }

      if (GetFileAttributesA (resolvedFileName.c_str()) & FILE_ATTRIBUTE_DIRECTORY) {
        mWatchRootName = resolvedFileName;
        scanDirectory ("", resolvedFileName);
        }
      else if (!resolvedFileName.empty())
        mFileItemList.push_back (cFileItem ("", resolvedFileName));
      }
    }
  //}}}
  virtual ~cFileList() {}

  // gets
  size_t size() { return mFileItemList.size(); }
  bool empty() { return mFileItemList.empty(); }

  bool isCurIndex (int index) { return mItemIndex == index; }
  unsigned getIndex() { return mItemIndex; }
  cFileItem getCurFileItem() { return getFileItem (mItemIndex); }
  cFileItem getFileItem (int index) { return mFileItemList[index]; }

  // actions
  void setIndex (int index) { mItemIndex = index; }
  //{{{
  bool prevIndex() {
    if (!empty() && (mItemIndex > 0)) {
      mItemIndex--;
      return true;
      }
    return false;
    }
  //}}}
  //{{{
  bool nextIndex() {
    if (!empty() && (mItemIndex < size()-1)) {
      mItemIndex++;
      return true;
      }
    return false;
    }
  //}}}

  //{{{
  void watchThread() {

    if (!mWatchRootName.empty()) {
      CoInitializeEx (NULL, COINIT_MULTITHREADED);
      cLog::setThreadName ("wtch");

      // Watch the directory for file creation and deletion.
      auto handle = FindFirstChangeNotification (mWatchRootName.c_str(), TRUE,
                                                 FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME);
      if (handle == INVALID_HANDLE_VALUE)
       cLog::log (LOGERROR, "FindFirstChangeNotification function failed");

      //while (!getExit()) {
      while (true) {
        cLog::log (LOGINFO, "Waiting for notification");
        if (WaitForSingleObject (handle, INFINITE) == WAIT_OBJECT_0) {
          // A file was created, renamed, or deleted in the directory.
          mFileItemList.clear();
          scanDirectory ("", mWatchRootName);
          if (FindNextChangeNotification (handle) == FALSE)
            cLog::log (LOGERROR, "FindNextChangeNotification function failed");
          }
        else
          cLog::log (LOGERROR, "No changes in the timeout period");
        }

      cLog::log (LOGINFO, "exit");
      CoUninitialize();
      }
    }
  //}}}

private:
  //{{{
  string resolveShortcut (const string& shortcut) {

    // get IShellLink interface
    IShellLinkA* iShellLink;
    if (CoCreateInstance (CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&iShellLink) == S_OK) {
      // get IPersistFile interface
      IPersistFile* iPersistFile;
      iShellLink->QueryInterface (IID_IPersistFile,(void**)&iPersistFile);

      // IPersistFile uses LPCOLESTR, ensure string is Unicode
      WCHAR wideShortcutFileName[MAX_PATH];
      MultiByteToWideChar (CP_ACP, 0, shortcut.c_str(), -1, wideShortcutFileName, MAX_PATH);

      // open shortcut file and init it from its contents
      if (iPersistFile->Load (wideShortcutFileName, STGM_READ) == S_OK) {
        // find target of shortcut, even if it has been moved or renamed
        if (iShellLink->Resolve (NULL, SLR_UPDATE) == S_OK) {
          // get the path to shortcut
          char szPath[MAX_PATH];
          WIN32_FIND_DATAA wfd;
          if (iShellLink->GetPath (szPath, MAX_PATH, &wfd, SLGP_RAWPATH) == S_OK) {
            // Get the description of the target
            char szDesc[MAX_PATH];
            if (iShellLink->GetDescription (szDesc, MAX_PATH) == S_OK) {
              string fullName;
              lstrcpynA ((char*)fullName.c_str(), szPath, MAX_PATH);
              return fullName;
              }
            }
          }
        }
      }

    return "";
    }
  //}}}
  //{{{
  void scanDirectory (const string& parentName, const string& directoryName) {

    auto pathFileName = parentName.empty() ? directoryName : parentName + "/" + directoryName;
    auto searchStr = pathFileName +  "/*";

    WIN32_FIND_DATAA findFileData;
    auto file = FindFirstFileExA (searchStr.c_str(), FindExInfoBasic, &findFileData,
                                  FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
    if (file != INVALID_HANDLE_VALUE) {
      do {
        if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (findFileData.cFileName[0] != '.'))
          scanDirectory (pathFileName, findFileData.cFileName);
        else if (PathMatchSpecA (findFileData.cFileName, mMatchString.c_str()))
          if ((findFileData.cFileName[0] != '.') && (findFileData.cFileName[0] != '..'))
            mFileItemList.push_back (cFileItem (pathFileName, findFileData.cFileName));
        } while (FindNextFileA (file, &findFileData));

      FindClose (file);
      }
    }
  //}}}

  // vars
  string mFileName;
  string mMatchString;
  string mWatchRootName;

  concurrency::concurrent_vector <cFileItem> mFileItemList;
  unsigned mItemIndex = 0;
  };
