#pragma once

#include <windows.h>
#include <string>
#include "ChatModels.h"

// File management functions
void ShareFile();
void AddSharedFileToChat(const SharedFile& file, bool isOutgoing);
void UpdateSharedFilesList();
void OpenSharedFile(int fileIndex);
std::wstring GetFileExtensionIcon(const std::wstring& filePath);
std::wstring FormatFileSize(DWORD fileSize);