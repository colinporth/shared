// cGlWindow.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <shlwapi.h> // for shell path functions
#include <shobjidl.h>
#include <shlguid.h>

#include "utils.h"
#include "cLog.h"

#include "cFileUtils.h"

#pragma comment(lib,"shlwapi.lib")

using namespace std;
//}}}

//{{{
bool resolveShortcut (const char* shortcutFileName, char* fullName) {

  // get IShellLink interface
  IShellLinkA* iShellLink;
  if (CoCreateInstance (CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&iShellLink) == S_OK) {
    // get IPersistFile interface
    IPersistFile* iPersistFile;
    iShellLink->QueryInterface (IID_IPersistFile,(void**)&iPersistFile);

    // IPersistFile uses LPCOLESTR, ensure string is Unicode
    WCHAR wideShortcutFileName[MAX_PATH];
    MultiByteToWideChar (CP_ACP, 0, shortcutFileName, -1, wideShortcutFileName, MAX_PATH);

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
            lstrcpynA (fullName, szPath, MAX_PATH);
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
vector<string> listDirectory (const string& parentName, const string& directoryName, char* pathMatchName) {

  vector<string> fileNames;

  string mFullDirName = parentName.empty() ? directoryName : parentName + "/" + directoryName;
  string searchStr (mFullDirName +  "/*");

  WIN32_FIND_DATAA findFileData;
  auto file = FindFirstFileExA (searchStr.c_str(), FindExInfoBasic, &findFileData,
                                FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
  if (file != INVALID_HANDLE_VALUE) {
    do {
      if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (findFileData.cFileName[0] != '.'))
        listDirectory (mFullDirName, findFileData.cFileName, pathMatchName);
      else if (PathMatchSpecA (findFileData.cFileName, pathMatchName))
        if ((findFileData.cFileName[0] != '.') && (findFileData.cFileName[0] != '..'))
          fileNames.push_back (mFullDirName + "/" + findFileData.cFileName);
      } while (FindNextFileA (file, &findFileData));

    FindClose (file);
    }

  return fileNames;
  }
//}}}

//{{{
vector<string> getFiles (const string& fileName, const string& match) {

  vector<string> fileNames;

  if (fileName.empty())
    return fileNames;

  string name = fileName;
  if (name.find (".lnk") <= fileName.size()) {
    char fullName[MAX_PATH];
    if (resolveShortcut (fileName.c_str(), fullName))
      name = fullName;
    }
  if (GetFileAttributesA (fileName.c_str()) & FILE_ATTRIBUTE_DIRECTORY)
    fileNames = listDirectory (string(), name, (char*)(match.c_str()));
  else if (!fileName.empty())
    fileNames.push_back (name);

  return fileNames;
  }
//}}}
