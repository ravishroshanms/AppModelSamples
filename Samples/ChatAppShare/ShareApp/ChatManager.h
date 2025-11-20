#pragma once

#include <windows.h>
#include <string>

// Chat management functions
void LoadContactChat(int contactIndex);
void SendChatMessage();
void AddMessageToChat(const std::wstring& message, bool isOutgoing);
void ProcessAutoReply(HWND hWnd, int timerType);