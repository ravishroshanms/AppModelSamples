#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include "ChatModels.h"

// Contact Selection Dialog Manager
class ContactSelectionDialog
{
public:
    // Structure to hold the result of contact selection
    struct SelectionResult
    {
        bool wasSelected;
        int contactIndex;
        std::wstring shareMessage;
        std::wstring filePath;
        std::wstring fileName;
        
        SelectionResult() : wasSelected(false), contactIndex(-1) {}
    };

    // Show the contact selection dialog
    static SelectionResult ShowContactSelectionDialog(HWND hParent, const std::wstring& filePath, const std::wstring& fileName);

private:
    // Dialog procedure for contact selection
    static INT_PTR CALLBACK ContactSelectionDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    
    // Helper functions
    static void InitializeContactList(HWND hListBox);
    static void PopulateContactList(HWND hListBox);
    static void UpdateContactListDisplay(HWND hListBox);
    static void HandleContactSelection(HWND hDlg, int selectedIndex);
    static void OnSelectContact(HWND hDlg);
    static void OnCancel(HWND hDlg);
    static void SetupDialogSizing(HWND hDlg);
    static void ApplyModernStyling(HWND hDlg);
    
    // Static data for dialog communication
    static SelectionResult s_dialogResult;
    static std::wstring s_currentFilePath;
    static std::wstring s_currentFileName;
};