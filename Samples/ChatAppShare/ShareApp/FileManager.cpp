#include "FileManager.h"
#include "ChatModels.h"
#include "ChatManager.h"
#include "UIManager.h"
#include <commdlg.h>
#include <algorithm>

#pragma comment(lib, "comdlg32.lib")

// External declarations for UI window handles
extern HWND hSharedFilesList;
extern HWND hContactsList;

void ShareFile()
{
    // Check if a contact is selected first
    if (selectedContactIndex < 0) {
        MessageBox(NULL, L"Please select a contact to share files with.", L"No Contact Selected", MB_OK | MB_ICONWARNING);
        return;
    }
    
    // Get the main window handle (parent of the share file button)
    HWND hMainWindow = GetParent(hSharedFilesList);
    while (GetParent(hMainWindow))
    {
        hMainWindow = GetParent(hMainWindow);
    }
    
    OPENFILENAME ofn;
    WCHAR szFile[260] = {0};
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hMainWindow;  // Set the main window as owner
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"All Files\0*.*\0?? Text Files\0*.TXT\0?? Document Files\0*.DOC;*.DOCX\0??? Image Files\0*.BMP;*.JPG;*.PNG;*.GIF\0?? PDF Files\0*.PDF\0?? Excel Files\0*.XLS;*.XLSX\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = L"Select File to Share";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    
    if (GetOpenFileName(&ofn))
    {
        // Extract file name from full path
        std::wstring fullPath(szFile);
        size_t lastSlash = fullPath.find_last_of(L"\\");
        std::wstring fileName;
        if (lastSlash != std::wstring::npos)
        {
            fileName = fullPath.substr(lastSlash + 1);
        }
        else
        {
            fileName = fullPath;
        }
        
        // Create shared file entry
        SharedFile newFile;
        newFile.fileName = fileName;
        newFile.filePath = fullPath;
        newFile.sharedBy = L"You";
        GetSystemTime(&newFile.timeShared);
        
        // Add file to the currently selected contact's shared files
        if (selectedContactIndex >= 0 && selectedContactIndex < (int)contacts.size())
        {
            contacts[selectedContactIndex].sharedFiles.push_back(newFile);
            
            // Add file sharing notification to chat
            std::wstring fileShareMsg = L"?? Shared file: " + fileName;
            AddMessageToChat(fileShareMsg, true);
            
            // Update UI
            AddSharedFileToChat(newFile, true);
            
            // Update contacts list display to show the shared activity
            InvalidateRect(hContactsList, NULL, TRUE);
            
            // Show success message with current contact
            std::wstring successMsg = L"File \"" + fileName + L"\" has been shared with " + contacts[selectedContactIndex].name + L"!";
            MessageBox(hMainWindow, successMsg.c_str(), L"File Shared Successfully", MB_OK | MB_ICONINFORMATION);
            
            // Simulate auto-reply from contact acknowledging the file
            SetTimer(hMainWindow, 2, 2000, NULL);
        }
    }
}

void AddSharedFileToChat(const SharedFile& file, bool isOutgoing)
{
    Contact* contact = GetSelectedContact();
    if (!contact) return;
    
    std::wstring sharer = isOutgoing ? L"You" : contact->name;
    
    // Format file sharing message with timestamp
    SYSTEMTIME st;
    GetLocalTime(&st);
    WCHAR timeStr[50];
    swprintf_s(timeStr, 50, L"[%02d:%02d] ", st.wHour, st.wMinute);
    
    std::wstring shareMessage = sharer + L" shared: " + file.fileName + L" ??";
    contact->messages.push_back(shareMessage);
    
    // Update last message preview
    contact->lastMessage = L"?? " + file.fileName;
    
    // Add to shared files list
    contact->sharedFiles.push_back(file);
    
    // Refresh UI
    LoadContactChat(selectedContactIndex);
    UpdateSharedFilesList();
}

void OpenSharedFile(int fileIndex)
{
    Contact* contact = GetSelectedContact();
    if (!contact || fileIndex < 0 || fileIndex >= (int)contact->sharedFiles.size()) {
        return;
    }
    
    const SharedFile& file = contact->sharedFiles[fileIndex];
    
    // Try to open the file with the default application
    HINSTANCE result = ShellExecute(NULL, L"open", file.filePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    
    if ((intptr_t)result <= 32) {
        // If opening failed, show file location in explorer
        std::wstring explorerCmd = L"/select,\"" + file.filePath + L"\"";
        ShellExecute(NULL, L"open", L"explorer.exe", explorerCmd.c_str(), NULL, SW_SHOWNORMAL);
    }
}

void UpdateSharedFilesList()
{
    if (!hSharedFilesList) return;
    
    Contact* contact = GetSelectedContact();
    
    // Clear the list
    SendMessage(hSharedFilesList, LB_RESETCONTENT, 0, 0);
    
    if (!contact) return;
    
    // Add shared files to the list
    for (const auto& file : contact->sharedFiles) {
        std::wstring displayText = file.fileName + L" (shared by " + file.sharedBy + L")";
        SendMessage(hSharedFilesList, LB_ADDSTRING, 0, (LPARAM)displayText.c_str());
    }
}