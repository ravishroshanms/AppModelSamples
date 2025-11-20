#include "UIManager.h"
#include "UIConstants.h"
#include "ModernUI.h"
#include "ChatModels.h"
#include "FileManager.h"
#include "WindowProcs.h"
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

// External declaration - hInst is defined in the main cpp file
extern HINSTANCE hInst;

// UI Window handles definitions
HWND hContactsList = nullptr;
HWND hChatDisplay = nullptr;
HWND hMessageInput = nullptr;
HWND hSendButton = nullptr;
HWND hContactName = nullptr;
HWND hShareFileButton = nullptr;
HWND hSharedFilesList = nullptr;

void CreateChatUI(HWND hWnd)
{
    RECT rect;
    GetClientRect(hWnd, &rect);
    
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    // Set window background to modern color
    SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, (LONG_PTR)hBrushBackground);
    
    // Create contacts list (left panel) with modern styling
    hContactsList = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"LISTBOX", NULL,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_OWNERDRAWFIXED,
        20, 20, 280, height - 40,
        hWnd, (HMENU)IDC_CONTACTS_LIST, hInst, NULL);
    
    // Set custom item height for contact list
    ::SendMessage(hContactsList, LB_SETITEMHEIGHT, 0, CONTACT_ITEM_HEIGHT);
    
    // Create contact name label with modern styling
    hContactName = CreateWindow(L"STATIC", L"Select a contact to start chatting",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        320, 20, 300, 30,
        hWnd, (HMENU)IDC_CONTACT_NAME, hInst, NULL);
    
    // Create chat display area with modern styling
    hChatDisplay = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT", NULL,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        320, 60, width - 600, height - 280,
        hWnd, (HMENU)IDC_CHAT_DISPLAY, hInst, NULL);
    
    // Create shared files section header
    CreateWindow(L"STATIC", L"Shared Files",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        width - 260, 20, 120, 30,
        hWnd, NULL, hInst, NULL);
        
    // Create shared files list with modern styling
    hSharedFilesList = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"LISTBOX", NULL,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
        width - 260, 60, 240, height - 280,
        hWnd, (HMENU)IDC_SHARED_FILES_LIST, hInst, NULL);
    
    // Create message input with placeholder styling
    hMessageInput = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT", NULL,
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL,
        320, height - 200, width - 600, 60,
        hWnd, (HMENU)IDC_MESSAGE_INPUT, hInst, NULL);
    
    // Create modern styled buttons
    hSendButton = CreateWindow(L"BUTTON", L"Send",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
        320, height - 130, 100, 45,
        hWnd, (HMENU)IDC_SEND_BUTTON, hInst, NULL);
    
    hShareFileButton = CreateWindow(L"BUTTON", L"Share File",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
        440, height - 130, 120, 45,
        hWnd, (HMENU)IDC_SHARE_FILE_BUTTON, hInst, NULL);
    
    // Populate contacts list
    for (const auto& contact : contacts) {
        ::SendMessage(hContactsList, LB_ADDSTRING, 0, (LPARAM)contact.name.c_str());
    }
    
    // Apply modern fonts to controls
    ::SendMessage(hContactName, WM_SETFONT, (WPARAM)hFontTitle, TRUE);
    ::SendMessage(hChatDisplay, WM_SETFONT, (WPARAM)hFontRegular, TRUE);
    ::SendMessage(hMessageInput, WM_SETFONT, (WPARAM)hFontRegular, TRUE);
    ::SendMessage(hSharedFilesList, WM_SETFONT, (WPARAM)hFontRegular, TRUE);
    
    // Subclass buttons for custom drawing
    SetupCustomWindowProcs();
    
    // Set modern background colors
    SetupWindowColors();
}

void ResizeChatUI(HWND hWnd)
{
    RECT rect;
    GetClientRect(hWnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    if (hContactsList) {
        SetWindowPos(hContactsList, NULL, 20, 20, 280, height - 40, SWP_NOZORDER);
    }
    if (hChatDisplay) {
        SetWindowPos(hChatDisplay, NULL, 320, 60, width - 600, height - 280, SWP_NOZORDER);
    }
    if (hSharedFilesList) {
        SetWindowPos(hSharedFilesList, NULL, width - 260, 60, 240, height - 280, SWP_NOZORDER);
    }
    if (hMessageInput) {
        SetWindowPos(hMessageInput, NULL, 320, height - 200, width - 600, 60, SWP_NOZORDER);
    }
    if (hSendButton) {
        SetWindowPos(hSendButton, NULL, 320, height - 130, 100, 45, SWP_NOZORDER);
    }
    if (hShareFileButton) {
        SetWindowPos(hShareFileButton, NULL, 440, height - 130, 120, 45, SWP_NOZORDER);
    }
}

void SetupWindowColors()
{
    SetClassLongPtr(hChatDisplay, GCLP_HBRBACKGROUND, (LONG_PTR)hBrushSurface);
    SetClassLongPtr(hMessageInput, GCLP_HBRBACKGROUND, (LONG_PTR)hBrushSurface);
    SetClassLongPtr(hSharedFilesList, GCLP_HBRBACKGROUND, (LONG_PTR)hBrushSurface);
}