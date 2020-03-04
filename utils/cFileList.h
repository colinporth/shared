// cFileList.h - windows only for now
//{{{  includes
#pragma once

#include <shobjidl.h>
#include <shlguid.h>
#include <shellapi.h>
#include <shlwapi.h>

#include <vector>
#include <string>
#include <chrono>
#include "../date/date.h"

#include "concurrent_vector.h"

#pragma comment(lib,"shlwapi.lib")
//}}}

class cFileList {
public:
  //{{{
  class cFileItem {
  public:
    static const int kFields = 3;

    //{{{
    cFileItem (const std::string& pathName, const std::string& fileName) :
        mPathName(pathName), mFileName(fileName) {

      if (pathName.empty()) {
        // extract any path name from filename
        std::string::size_type lastSlashPos = mFileName.rfind ('\\');
        if (lastSlashPos != std::string::npos) {
          mPathName = std::string (mFileName, 0, lastSlashPos);
          mFileName = mFileName.substr (lastSlashPos+1);
          }
        }

      // extract any extension from filename
      std::string::size_type lastDotPos = mFileName.rfind ('.');
      if (lastDotPos != std::string::npos) {
        mExtension = mFileName.substr (lastDotPos+1);
        mFileName = std::string (mFileName, 0, lastDotPos);
        }


      auto fileHandle = CreateFile (getFullName().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
      LARGE_INTEGER largeInteger;
      if (!GetFileSizeEx (fileHandle, &largeInteger))
        cLog::log (LOGERROR, "fileSizeEx problem");
      else
        mFileSize = largeInteger.QuadPart;

      FILETIME creation;
      FILETIME lastAccess;
      FILETIME lastWrite;
      GetFileTime (fileHandle, &creation, &lastAccess, &lastWrite);

      CloseHandle (fileHandle);

      mCreationTimePoint = getFileTimePoint (creation);
      mLastAccessTimePoint = getFileTimePoint (lastAccess);
      mLastWriteTimePoint = getFileTimePoint (lastWrite);
      }
    //}}}
    virtual ~cFileItem() {}

    std::string getPathName() const { return mPathName; }
    std::string getFileName() const { return mFileName; }
    std::string getExtension() const { return mExtension; }
    std::string getFullName() const { return (mPathName.empty() ? mFileName : mPathName + "/" + mFileName) +
                                             (mExtension.empty() ? "" : "." + mExtension); }

    //{{{
    std::string getFileSizeString() const {

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
    std::string getCreationString() const {

      if (mCreationTimePoint.time_since_epoch() == std::chrono::seconds::zero())
        return "";
      else
        return date::format ("%H.%M %a %d %b %Y", floor<std::chrono::seconds>(mCreationTimePoint));
      }
    //}}}
    //{{{
    std::string getLastWriteString() const {

      if (mLastWriteTimePoint.time_since_epoch() == std::chrono::seconds::zero())
        return "";
      else
        return date::format ("%H.%M %a %d %b %Y", floor<std::chrono::seconds>(mLastWriteTimePoint));
      }
    //}}}
    //{{{
    std::string getLastAccessString() const {

      if (mLastAccessTimePoint.time_since_epoch() == std::chrono::seconds::zero())
        return "";
      else
        return date::format ("%H.%M %a %d %b %Y", floor<std::chrono::seconds>(mLastAccessTimePoint));
      }
    //}}}
    //{{{
    std::string getFieldString (int field) const {

      switch (field) {
        case 0: return getFileName();
        case 1: return getFileSizeString();
        case 2: return getCreationString();
        case 3: return getLastWriteString();
        case 4: return getLastAccessString();
        case 5: return getPathName();
        case 6: return getExtension();
        case 7: return getFullName();
        }

      return "empty";
      }
    //}}}
    //{{{
    std::chrono::system_clock::time_point getFileTimePoint (FILETIME fileTime) {

      // filetime_duration has the same layout as FILETIME; 100ns intervals
      using filetime_duration = std::chrono::duration<int64_t, std::ratio<1, 10'000'000>>;

      // January 1, 1601 (NT epoch) - January 1, 1970 (Unix epoch):
      constexpr std::chrono::duration<int64_t> nt_to_unix_epoch{INT64_C(-11644473600)};

      const filetime_duration asDuration{static_cast<int64_t>(
          (static_cast<uint64_t>((fileTime).dwHighDateTime) << 32)
              | (fileTime).dwLowDateTime)};
      const auto withUnixEpoch = asDuration + nt_to_unix_epoch;
      return std::chrono::system_clock::time_point{ std::chrono::duration_cast<std::chrono::system_clock::duration>(withUnixEpoch)};
      }
    //}}}

    std::string mPathName;
    std::string mFileName;
    std::string mExtension;

    uint64_t mFileSize = 0;
    std::chrono::time_point<std::chrono::system_clock> mCreationTimePoint;
    std::chrono::time_point<std::chrono::system_clock> mLastWriteTimePoint;
    std::chrono::time_point<std::chrono::system_clock> mLastAccessTimePoint;
    };
  //}}}

  //{{{
  cFileList (const std::string& fileName, const std::string& matchString) {

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

      sort();
      }
    }
  //}}}
  //{{{
  cFileList (const std::vector<std::string>& names, const std::string& matchString) {
  // parse vector of names

    if (!names.empty()) {
      mMatchString = matchString;

      for (auto name : names) {
        auto resolvedFileName = name;
        if (name.find (".lnk") <= name.size()) {
          resolvedFileName = resolveShortcut (name);
          if (resolvedFileName.empty()) {
            cLog::log (LOGERROR, "cFileList vec - link " + name + " unresolved");
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

      sort();
      }
    }
  //}}}
  virtual ~cFileList() {}

  // gets
  bool empty() { return mFileItemList.empty(); }
  size_t size() { return mFileItemList.size(); }

  bool isCurIndex (unsigned index) { return mItemIndex == index; }
  unsigned getIndex() { return mItemIndex; }
  //{{{
  bool ensureItemVisible() {
    // one shot use
    bool result = mEnsureItemVisible;
    mEnsureItemVisible = false;
    return result;
    }
  //}}}

  cFileItem getCurFileItem() { return getFileItem (mItemIndex); }
  cFileItem getFileItem (unsigned index) { return mFileItemList[index]; }

  // actions
  void setIndex (unsigned index) { mItemIndex = index; }
  //{{{
  bool prevIndex() {
    if (!empty() && (mItemIndex > 0)) {
      mItemIndex--;
      mEnsureItemVisible = true;
      return true;
      }
    return false;
    }
  //}}}
  //{{{
  bool nextIndex() {
    if (!empty() && (mItemIndex < size()-1)) {
      mItemIndex++;
      mEnsureItemVisible = true;
      return true;
      }
    return false;
    }
  //}}}
  //{{{
  void nextSort() {
    mCompareField = (mCompareField + 1) % cFileItem::kFields;
    sort();
    }
  //}}}
  //{{{
  void toggleSortUp() {
    mCompareFieldDescending = !mCompareFieldDescending;
    sort();
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
          sort();
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
  std::string resolveShortcut (const std::string& shortcut) {

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
              std::string fullName;
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
  void scanDirectory (const std::string& parentName, const std::string& directoryName) {

    auto pathFileName = parentName.empty() ? directoryName : parentName + "/" + directoryName;
    auto searchStr = pathFileName +  "/*";

    WIN32_FIND_DATAA findFileData;
    auto file = FindFirstFileExA (searchStr.c_str(), FindExInfoBasic, &findFileData,
                                  FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
    if (file != INVALID_HANDLE_VALUE) {
      do {
        if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (findFileData.cFileName[0] != '.'))
          scanDirectory (pathFileName, findFileData.cFileName);
        else if (PathMatchSpecExA (findFileData.cFileName, mMatchString.c_str(), PMSF_MULTIPLE) == S_OK)
          if ((findFileData.cFileName[0] != '.') && (findFileData.cFileName[0] != '..'))
            mFileItemList.push_back (cFileItem (pathFileName, findFileData.cFileName));
        } while (FindNextFileA (file, &findFileData));

      FindClose (file);
      }
    }
  //}}}
  //{{{
  void sort() {
    std::sort (mFileItemList.begin(), mFileItemList.end(),
      // compare lambda
      [=](const cFileItem& a, const cFileItem& b) {
        switch (mCompareField) {
          case 0:  return mCompareFieldDescending ? (a.mFileName > b.mFileName) : (a.mFileName < b.mFileName);
          case 1:  return mCompareFieldDescending ? (a.mFileSize > b.mFileSize) : (a.mFileSize < b.mFileSize);
          case 2:  return mCompareFieldDescending ? (a.mCreationTimePoint > b.mCreationTimePoint) : (a.mCreationTimePoint < b.mCreationTimePoint);
          }
        return (a.mFileName > b.mFileName);
        } );
    }
  //}}}

  // vars
  std::string mFileName;
  std::string mMatchString;
  std::string mWatchRootName;

  int mCompareField = 0;
  bool mCompareFieldDescending = false;

  concurrency::concurrent_vector <cFileItem> mFileItemList;

  unsigned mItemIndex = 0;
  bool mEnsureItemVisible = false;
  };
