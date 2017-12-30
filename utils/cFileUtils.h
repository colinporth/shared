// cFileUtils.h
#pragma once
#include <string>
#include <vector>

bool resolveShortcut (const char* shortcutFileName, char* fullName);
std::vector<std::string> listDirectory (const std::string& parentName, const std::string& directoryName, char* pathMatchName);
std::vector<std::string> getFiles (const std::string& fileName, const std::string& match);
