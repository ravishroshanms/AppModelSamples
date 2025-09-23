#include "FileManager.h"
#include "ChatModels.h"
#include "ChatManager.h"
#include "UIManager.h"
#include <commdlg.h>
#include <algorithm>

#pragma comment(lib, "comdlg32.lib")

// External declarations for UI window handles
extern HWND hSharedFilesList;

void ShareFile()
{
    if (selectedContactIndex < 0) {
        MessageBox(NULL, L"Please select a contact to share files with. ??", L"No Contact Selected", MB_OK | MB_ICONWARNING);
        return;
    }
    
    OPENFILENAME ofn;
    WCHAR szFile[260] = {0};
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"All Files\0*.*\0?? Text Files\0*.TXT\0?? Document Files\0*.DOC;*.DOCX\0??? Image Files\0*.BMP;*.JPG;*.PNG;*.GIF\0?? PDF Files\0*.PDF\0?? Excel Files\0*.XLS;*.XLSX\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = L"Select File to Share";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    
    if (GetOpenFileName(&ofn)) {
        // Extract file name from full path
        std::wstring fullPath(szFile);
        size_t lastSlash = fullPath.find_last_of(L"\\");
        std::wstring fileName;
        if (lastSlash != std::wstring::npos) {
            fileName = fullPath.substr(lastSlash + 1);
        } else {
            fileName = fullPath;
        }
        
        // Create shared file entry
        SharedFile newFile;
        newFile.fileName = fileName;
        newFile.filePath = fullPath;
        newFile.sharedBy = L"You";
        GetSystemTime(&newFile.timeShared);
        
        // Add to chat and shared files
        AddSharedFileToChat(newFile, true);
        
        // Simulate auto-reply from contact acknowledging the file
        SetTimer(GetParent(hSharedFilesList), 2, 1500, NULL);
    }
}

void AddSharedFileToChat(const SharedFile& file, bool isOutgoing)
{
    Contact* contact = GetSelectedContact();
    if (!contact) return;
    
    std::wstring sharer = isOutgoing ? L"You" : contact->name;
    std::wstring formattedMessage = sharer + L" shared: " + file.fileName;
    
    contact->messages.push_back(formattedMessage);
    
    // Add to shared files list
    if (isOutgoing) {
        SharedFile newFile = file;
        newFile.sharedBy = L"You";
        contact->sharedFiles.push_back(newFile);
    }
    
    // Update both chat display and shared files list
    LoadContactChat(selectedContactIndex);
}

void UpdateSharedFilesList()
{
    Contact* contact = GetSelectedContact();
    if (!contact) return;
    
    // Clear the list
    ::SendMessage(hSharedFilesList, LB_RESETCONTENT, 0, 0);
    
    // Add shared files with file type icons
    for (const auto& file : contact->sharedFiles) {
        std::wstring icon = GetFileExtensionIcon(file.fileName);
        std::wstring displayText = icon + L" " + file.fileName;
        displayText += L"\n   ?? " + file.sharedBy;
        ::SendMessage(hSharedFilesList, LB_ADDSTRING, 0, (LPARAM)displayText.c_str());
    }
}

void OpenSharedFile(int fileIndex)
{
    Contact* contact = GetSelectedContact();
    if (!contact || fileIndex < 0 || fileIndex >= (int)contact->sharedFiles.size()) return;
    
    const SharedFile& file = contact->sharedFiles[fileIndex];
    
    // Enhanced file info display with emojis and better formatting
    std::wstring message = L"File Information\n\n";
    message += L"?? Name: " + file.fileName + L"\n";
    message += L"?? Path: " + file.filePath + L"\n";
    message += L"?? Shared by: " + file.sharedBy + L"\n";
    
    WCHAR timeStr[100];
    swprintf_s(timeStr, 100, L"?? Shared on: %02d/%02d/%04d at %02d:%02d", 
        file.timeShared.wMonth, file.timeShared.wDay, file.timeShared.wYear,
        file.timeShared.wHour, file.timeShared.wMinute);
    message += timeStr;
    message += L"\n\n?? Tip: In a real application, this file would open automatically!";
    
    MessageBox(NULL, message.c_str(), L"Shared File Information", MB_OK | MB_ICONINFORMATION);
    
    // In a real application, you would use ShellExecute to open the file:
    // ShellExecute(NULL, L"open", file.filePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

std::wstring GetFileExtensionIcon(const std::wstring& filePath)
{
    size_t dotPos = filePath.find_last_of(L".");
    if (dotPos != std::wstring::npos) {
        std::wstring ext = filePath.substr(dotPos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
        
        if (ext == L"txt") return L"??";
        if (ext == L"doc" || ext == L"docx") return L"??";
        if (ext == L"pdf") return L"??";
        if (ext == L"xls" || ext == L"xlsx") return L"??";
        if (ext == L"jpg" || ext == L"png" || ext == L"gif" || ext == L"bmp") return L"???";
        if (ext == L"mp3" || ext == L"wav") return L"??";
        if (ext == L"mp4" || ext == L"avi") return L"??";
        if (ext == L"zip" || ext == L"rar") return L"???";
        if (ext == L"exe" || ext == L"msi") return L"??";
    }
    return L"??";
}

std::wstring FormatFileSize(DWORD fileSize)
{
    const DWORD KB = 1024;
    const DWORD MB = KB * 1024;
    const DWORD GB = MB * 1024;
    
    WCHAR buffer[50];
    if (fileSize >= GB) {
        swprintf_s(buffer, 50, L"%.1f GB", (double)fileSize / GB);
    } else if (fileSize >= MB) {
        swprintf_s(buffer, 50, L"%.1f MB", (double)fileSize / MB);
    } else if (fileSize >= KB) {
        swprintf_s(buffer, 50, L"%.1f KB", (double)fileSize / KB);
    } else {
        swprintf_s(buffer, 50, L"%d bytes", fileSize);
    }
    return std::wstring(buffer);
}