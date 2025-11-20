#include "ChatManager.h"
#include "ChatModels.h"
#include "FileManager.h"
#include "UIManager.h"
#include <vector>
#include <algorithm>
#include <random>

void LoadContactChat(int contactIndex)
{
    if (!IsValidContactIndex(contactIndex)) return;
    
    selectedContactIndex = contactIndex;
    const Contact& contact = contacts[contactIndex];
    
    // Update contact name with status indicator
    std::wstring headerText = contact.name + L" " + L" (" + contact.status + L")";
    SetWindowText(hContactName, headerText.c_str());
    
    // Clear and populate chat display with better formatting
    SetWindowText(hChatDisplay, L"");
    
    std::wstring chatText;
    for (const auto& message : contact.messages) {
        // Add timestamps and better message formatting
        SYSTEMTIME st;
        GetLocalTime(&st);
        WCHAR timeStr[50];
        swprintf_s(timeStr, 50, L"[%02d:%02d] ", st.wHour, st.wMinute);
        
        if (message.find(L"You:") == 0) {
            chatText += L"                                           ";  // Right align for your messages
            chatText += timeStr + message + L"\r\n\r\n";
        } else {
            chatText += timeStr + message + L"\r\n\r\n";
        }
    }
    
    SetWindowText(hChatDisplay, chatText.c_str());
    
    // Scroll to bottom
    ::SendMessage(hChatDisplay, EM_SETSEL, -1, -1);
    ::SendMessage(hChatDisplay, EM_SCROLLCARET, 0, 0);
    
    // Update shared files list
    UpdateSharedFilesList();
    
    // Refresh contact list to show selection
    InvalidateRect(hContactsList, NULL, TRUE);
}

void AddMessageToChat(const std::wstring& message, bool isOutgoing)
{
    Contact* contact = GetSelectedContact();
    if (!contact) return;
    
    // Add timestamp to message
    SYSTEMTIME st;
    GetLocalTime(&st);
    WCHAR timeStr[50];
    swprintf_s(timeStr, 50, L"[%02d:%02d] ", st.wHour, st.wMinute);
    
    std::wstring formattedMessage;
    if (isOutgoing) {
        formattedMessage = L"You: " + message + L" ??";
    } else {
        formattedMessage = contact->name + L": " + message;
    }
    
    contact->messages.push_back(formattedMessage);
    contact->lastMessage = message.length() > 50 ? message.substr(0, 47) + L"..." : message;
    
    // Update chat display
    LoadContactChat(selectedContactIndex);
    
    // Refresh contacts list to update last message preview
    InvalidateRect(hContactsList, NULL, TRUE);
}

void SendChatMessage()
{
    if (selectedContactIndex < 0) return;
    
    WCHAR buffer[1024];
    GetWindowText(hMessageInput, buffer, 1024);
    
    std::wstring message(buffer);
    if (!message.empty()) {
        AddMessageToChat(message, true);
        SetWindowText(hMessageInput, L"");
        
        // Simulate auto-reply after a short delay
        SetTimer(GetParent(hMessageInput), 1, 2000, NULL);
    }
}

void ProcessAutoReply(HWND hWnd, int timerType)
{
    if (selectedContactIndex < 0) return;
    
    KillTimer(hWnd, timerType);
    
    if (timerType == 1) {
        // Auto-reply from the selected contact with more variety
        std::vector<std::wstring> autoReplies = {
            L"Got it!",
            L"Thanks for letting me know!",
            L"Sounds good!",
            L"I'll get back to you soon.",
            L"Perfect!",
            L"Absolutely!",
            L"Let me think about it.",
            L"Great idea!"
        };
        
        int replyIndex = rand() % autoReplies.size();
        AddMessageToChat(autoReplies[replyIndex], false);
    }
    else if (timerType == 2) {
        // Auto-reply acknowledging the shared file
        std::vector<std::wstring> fileReplies = {
            L"Thanks for sharing the file!",
            L"Got the file, will check it out.",
            L"File received, thanks! ??",
            L"Perfect timing, I needed this file.",
            L"Awesome, downloading now!"
        };
        
        int replyIndex = rand() % fileReplies.size();
        AddMessageToChat(fileReplies[replyIndex], false);
    }
}