#pragma once

#include <windows.h>

// UI Window handles
extern HWND hContactsList;
extern HWND hChatDisplay;
extern HWND hMessageInput;
extern HWND hSendButton;
extern HWND hContactName;
extern HWND hShareFileButton;
extern HWND hSharedFilesList;

// UI management functions
void CreateChatUI(HWND hWnd);
void ResizeChatUI(HWND hWnd);
void SetupWindowColors();