// cFileList.h - windows only for now
//{{{  includes
#pragma once

#include <shobjidl.h>
#include <shlguid.h>
#include <shellapi.h>
#include <shlwapi.h>

#include <vector>

#pragma comment(lib,"shlwapi.lib")
//}}}

//{{{
class cFileItem {
public:
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
    }
  //}}}
  virtual ~cFileItem() {}

  std::string getPathName() { return mPathName; }
  std::string getFileName() { return mFileName; }
  std::string getExtension() { return mExtension; }
  std::string getFullName() {
    return (mPathName.empty() ? mFileName : mPathName + "/" + mFileName) +
           (mExtension.empty() ? "" : "." + mExtension); }

private:
  std::string mPathName;
  std::string mFileName;
  std::string mExtension;
  // size
  // creation data
  };
//}}}

class cFileList {
public:
  //{{{
  cFileList (const string& fileName, const std::string& matchString) {

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
        mWatchName = resolvedFileName;
        scanDirectory ("", resolvedFileName);
        }
      else if (!resolvedFileName.empty())
        mFileItemList.push_back (cFileItem ("", resolvedFileName));
      }
    }
  //}}}
  virtual ~cFileList() {}

  //{{{  gets
  int size() { return (int)mFileItemList.size(); }
  bool empty() { return mFileItemList.empty(); }

  bool isCurIndex (int index) { return mItemIndex == index; }
  bool isChanged() { return mChanged; }

  cFileItem getCurFileItem() { return getFileItem (mItemIndex); }
  cFileItem getFileItem (int index) { return mFileItemList[index]; }
  //}}}

  // actions
  void setChanged() { mChanged = true; }
  void resetChanged() { mChanged = false; }
  void setIndex (int index) { mItemIndex = index; }
  //{{{
  bool prev() {
    if (!empty() && (mItemIndex > 0)) {
      mItemIndex--;
      return true;
      }
    return false;
    }
  //}}}
  //{{{
  bool next() {
    if (!empty() && (mItemIndex < size()-1)) {
      mItemIndex++;
      return true;
      }
    return false;
    }
  //}}}

  //{{{
  void watchThread() {

    if (!mWatchName.empty()) {
      CoInitializeEx (NULL, COINIT_MULTITHREADED);
      cLog::setThreadName ("wtch");

      // Watch the directory for file creation and deletion.
      auto handle = FindFirstChangeNotification (mWatchName.c_str(), TRUE,
                                                 FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME);
      if (handle == INVALID_HANDLE_VALUE)
       cLog::log (LOGERROR, "FindFirstChangeNotification function failed");

      //while (!getExit()) {
      while (true) {
        cLog::log (LOGINFO, "Waiting for notification");
        if (WaitForSingleObject (handle, INFINITE) == WAIT_OBJECT_0) {
          // A file was created, renamed, or deleted in the directory.
          scanDirectory ("", mWatchName);
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

    std::string pathFileName = parentName.empty() ? directoryName : parentName + "/" + directoryName;
    std::string searchStr (pathFileName +  "/*");

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
  concurrency::concurrent_vector <cFileItem> mFileItemList;

  std::string mFileName;
  std::string mMatchString;

  std::string mWatchName;

  int mItemIndex = 0;
  bool mChanged = false;
  };
