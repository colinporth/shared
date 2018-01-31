// cFileList.h
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
  cFileItem (const std::string& path) : mPath(path) {}
  virtual ~cFileItem() {}

  std::string getPath() { return mPath; }

private:
  std::string mPath;
  };
//}}}

class cFileList {
public:
  //{{{
  cFileList(const string& fileName, const std::string& matchString) :
      mFileName(fileName), mMatchString(matchString) {

    getFiles();
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

    CoInitializeEx (NULL, COINIT_MULTITHREADED);
    cLog::setThreadName ("wtch");

    // Watch the directory for file creation and deletion.
    auto handle = FindFirstChangeNotification (mFileName.c_str(), TRUE,
                                               FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME);
    if (handle == INVALID_HANDLE_VALUE)
     cLog::log (LOGERROR, "FindFirstChangeNotification function failed");

    //while (!getExit()) {
    while (true) {
      cLog::log (LOGINFO, "Waiting for notification");
      if (WaitForSingleObject (handle, INFINITE) == WAIT_OBJECT_0) {
        // A file was created, renamed, or deleted in the directory.
        getFiles();
        if (FindNextChangeNotification (handle) == FALSE)
          cLog::log (LOGERROR, "FindNextChangeNotification function failed");
        }
      else
        cLog::log (LOGERROR, "No changes in the timeout period");
      }

    cLog::log (LOGINFO, "exit");
    CoUninitialize();
    }
  //}}}

private:
  //{{{
  bool resolveShortcut (const std::string& shortcut, std::string& fullName) {

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
              lstrcpynA ((char*)fullName.c_str(), szPath, MAX_PATH);
              return true;
              }
            }
          }
        }
      }

    fullName[0] = '\0';
    return false;
    }
  //}}}
  //{{{
  void scanDirectory (const std::string& parentName, const string& directoryName) {

    string pathFileName = parentName.empty() ? directoryName : parentName + "/" + directoryName;
    string searchStr (pathFileName +  "/*");

    WIN32_FIND_DATAA findFileData;
    auto file = FindFirstFileExA (searchStr.c_str(), FindExInfoBasic, &findFileData,
                                  FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
    if (file != INVALID_HANDLE_VALUE) {
      do {
        if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (findFileData.cFileName[0] != '.'))
          scanDirectory (pathFileName, findFileData.cFileName);
        else if (PathMatchSpecA (findFileData.cFileName, mMatchString.c_str()))
          if ((findFileData.cFileName[0] != '.') && (findFileData.cFileName[0] != '..'))
            mFileItemList.push_back (cFileItem (pathFileName + "/" + findFileData.cFileName));
        } while (FindNextFileA (file, &findFileData));

      FindClose (file);
      }
    }
  //}}}
  //{{{
  void getFiles() {

    mFileItemList.clear();

    if (mFileName.empty())
      return;

    string useFileName = mFileName;
    if (mFileName.find (".lnk") <= mFileName.size()) {
      string fullName;
      if (resolveShortcut (mFileName.c_str(), fullName))
        useFileName = fullName;
      }

    if (GetFileAttributesA (useFileName.c_str()) & FILE_ATTRIBUTE_DIRECTORY)
      scanDirectory ("", useFileName);
    else if (!useFileName.empty())
      mFileItemList.push_back (cFileItem (useFileName));
    }
  //}}}

  concurrency::concurrent_vector <cFileItem> mFileItemList;

  std::string mFileName;
  std::string mMatchString;
  int mItemIndex = 0;
  bool mChanged = false;
  };
