#pragma once

#include <string>
#include <vector>
#include <map>
#include <windows.h>

struct SharedFile {
    std::wstring fileName;
    std::wstring filePath;
    std::wstring sharedBy;
    SYSTEMTIME timeShared;
};

struct Contact {
    std::wstring name;
    std::wstring lastMessage;
    std::vector<std::wstring> messages;
    std::vector<SharedFile> sharedFiles;
    std::wstring status;
    bool isOnline;
};

// Global data
extern std::vector<Contact> contacts;
extern std::map<std::wstring, std::vector<std::wstring>> chatHistory;
extern int selectedContactIndex;

// Contact management functions
void InitializeContacts();
Contact* GetSelectedContact();
bool IsValidContactIndex(int index);