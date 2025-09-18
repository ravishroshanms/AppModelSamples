// SampleChatAppWithShare.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "SampleChatAppWithShare.h"
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <gdiplus.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "uxtheme.lib")

#define MAX_LOADSTRING 100

// Modern UI Color Scheme
#define COLOR_PRIMARY        RGB(64, 128, 255)      // Modern blue
#define COLOR_PRIMARY_DARK   RGB(45, 100, 220)     // Darker blue for hover
#define COLOR_APP_BACKGROUND RGB(248, 249, 250)    // Light gray background
#define COLOR_SURFACE        RGB(255, 255, 255)    // White surface
#define COLOR_TEXT_PRIMARY   RGB(33, 37, 41)       // Dark text
#define COLOR_TEXT_SECONDARY RGB(108, 117, 125)    // Gray text
#define COLOR_BORDER         RGB(222, 226, 230)    // Light border
#define COLOR_HOVER          RGB(248, 249, 250)    // Hover background
#define COLOR_CHAT_BUBBLE_OUT RGB(0, 123, 255)     // Outgoing message
#define COLOR_CHAT_BUBBLE_IN  RGB(233, 236, 239)   // Incoming message

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Chat Application Variables
HWND hContactsList;
HWND hChatDisplay;
HWND hMessageInput;
HWND hSendButton;
HWND hContactName;
HWND hShareFileButton;
HWND hSharedFilesList;

// Modern UI Variables
HBRUSH hBrushBackground;
HBRUSH hBrushSurface;
HBRUSH hBrushPrimary;
HBRUSH hBrushHover;
HFONT hFontRegular;
HFONT hFontBold;
HFONT hFontTitle;
HPEN hPenBorder;

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

std::vector<Contact> contacts;
std::map<std::wstring, std::vector<std::wstring>> chatHistory;
int selectedContactIndex = -1;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    ModernButtonProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK    ModernListBoxProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void                InitializeContacts();
void                CreateChatUI(HWND hWnd);
void                LoadContactChat(int contactIndex);
void                SendChatMessage();
void                AddMessageToChat(const std::wstring& message, bool isOutgoing);
void                ShareFile();
void                AddSharedFileToChat(const SharedFile& file, bool isOutgoing);
void                UpdateSharedFilesList();
void                OpenSharedFile(int fileIndex);
std::wstring        GetFileExtensionIcon(const std::wstring& filePath);
std::wstring        FormatFileSize(DWORD fileSize);
void                InitializeModernUI();
void                CleanupModernUI();
void                DrawModernButton(HDC hdc, RECT rect, const std::wstring& text, bool isHovered, bool isPressed);
void                DrawContactItem(HDC hdc, RECT rect, const Contact& contact, bool isSelected);

// Window procedure storage for subclassing
WNDPROC originalButtonProc = nullptr;
WNDPROC originalListBoxProc = nullptr;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize COM for shell operations
    CoInitialize(NULL);

    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SAMPLECHATAPPWITHSHARE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Initialize modern UI
    InitializeModernUI();

    // Initialize dummy contacts
    InitializeContacts();

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SAMPLECHATAPPWITHSHARE));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    CoUninitialize();
    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SAMPLECHATAPPWITHSHARE));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_SAMPLECHATAPPWITHSHARE);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 1200, 700, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

void InitializeContacts()
{
    contacts = {
        {L"Alice Johnson", L"Hey, how are you?", {L"Alice: Hey, how are you?", L"You: I'm doing great, thanks!", L"Alice: That's wonderful to hear!"}, {}, L"Available", true},
        {L"Bob Smith", L"See you tomorrow!", {L"Bob: Are we still meeting tomorrow?", L"You: Yes, see you at 3 PM", L"Bob: See you tomorrow!"}, {}, L"In a meeting", true},
        {L"Carol Williams", L"Thanks for the help", {L"Carol: Could you help me with the project?", L"You: Of course! What do you need?", L"Carol: Thanks for the help"}, {}, L"Available", true},
        {L"David Brown", L"Great presentation!", {L"David: Great presentation today!", L"You: Thank you! I'm glad you liked it"}, {}, L"Away", false},
        {L"Emma Davis", L"Coffee later?", {L"Emma: Want to grab coffee later?", L"You: Sure! What time works for you?", L"Emma: Coffee later?"}, {}, L"Available", true},
        {L"Frank Miller", L"Happy Birthday!", {L"Frank: Happy Birthday!", L"You: Thank you so much!"}, {}, L"Busy", true},
        {L"Grace Wilson", L"Meeting rescheduled", {L"Grace: Meeting has been rescheduled to 4 PM", L"You: Got it, thanks for letting me know"}, {}, L"Available", true},
        {L"Henry Taylor", L"Weekend plans?", {L"Henry: Any plans for the weekend?", L"You: Nothing concrete yet", L"Henry: Weekend plans?"}, {}, L"Offline", false},
        {L"Ivy Anderson", L"Project update", {L"Ivy: Here's the project update you requested", L"You: Perfect, reviewing it now"}, {}, L"Available", true},
        {L"Jack Thompson", L"Game night Friday", {L"Jack: Game night this Friday?", L"You: Count me in!", L"Jack: Game night Friday"}, {}, L"Gaming", true},
        {L"Kate Garcia", L"Recipe sharing", {L"Kate: Loved that recipe you shared!", L"You: I'm so glad you enjoyed it!"}, {}, L"Cooking", true},
        {L"Leo Martinez", L"Workout buddy", {L"Leo: Gym session tomorrow morning?", L"You: Absolutely! 7 AM as usual?"}, {}, L"At the gym", true},
        {L"Mia Rodriguez", L"Book recommendation", {L"Mia: Any good book recommendations?", L"You: I just finished a great mystery novel"}, {}, L"Reading", true},
        {L"Noah Lee", L"Tech discussion", {L"Noah: Thoughts on the new framework?", L"You: It looks promising! Want to discuss over lunch?"}, {}, L"Coding", true},
        {L"Olivia Clark", L"Travel planning", {L"Olivia: Planning the vacation itinerary", L"You: Excited to see what you've planned!"}, {}, L"Traveling", false}
    };

    // Add some sample shared files to demonstrate the feature
    SYSTEMTIME st;
    GetSystemTime(&st);
    
    contacts[0].sharedFiles.push_back({L"Project_Proposal.docx", L"C:\\Documents\\Project_Proposal.docx", L"Alice", st});
    contacts[1].sharedFiles.push_back({L"Meeting_Notes.pdf", L"C:\\Documents\\Meeting_Notes.pdf", L"Bob", st});
    contacts[2].sharedFiles.push_back({L"Budget_Spreadsheet.xlsx", L"C:\\Documents\\Budget_Spreadsheet.xlsx", L"Carol", st});
}

void InitializeModernUI()
{
    // Create brushes for modern color scheme
    hBrushBackground = CreateSolidBrush(COLOR_APP_BACKGROUND);
    hBrushSurface = CreateSolidBrush(COLOR_SURFACE);
    hBrushPrimary = CreateSolidBrush(COLOR_PRIMARY);
    hBrushHover = CreateSolidBrush(COLOR_HOVER);
    
    // Create modern fonts
    hFontRegular = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        
    hFontBold = CreateFont(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        
    hFontTitle = CreateFont(20, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    
    // Create pen for borders
    hPenBorder = CreatePen(PS_SOLID, 1, COLOR_BORDER);
    
    // Initialize GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

void CleanupModernUI()
{
    // Cleanup GDI objects
    if (hBrushBackground) DeleteObject(hBrushBackground);
    if (hBrushSurface) DeleteObject(hBrushSurface);
    if (hBrushPrimary) DeleteObject(hBrushPrimary);
    if (hBrushHover) DeleteObject(hBrushHover);
    if (hFontRegular) DeleteObject(hFontRegular);
    if (hFontBold) DeleteObject(hFontBold);
    if (hFontTitle) DeleteObject(hFontTitle);
    if (hPenBorder) DeleteObject(hPenBorder);
    
    // Shutdown GDI+
    Gdiplus::GdiplusShutdown(NULL);
}

void DrawModernButton(HDC hdc, RECT rect, const std::wstring& text, bool isHovered, bool isPressed)
{
    // Create rounded rectangle region
    HRGN hRgn = CreateRoundRectRgn(rect.left, rect.top, rect.right, rect.bottom, 8, 8);
    
    // Fill background
    HBRUSH hBrush = CreateSolidBrush(isPressed ? COLOR_PRIMARY_DARK : 
                                    isHovered ? COLOR_PRIMARY : COLOR_PRIMARY);
    FillRgn(hdc, hRgn, hBrush);
    DeleteObject(hBrush);
    
    // Draw border with a brush instead of pen
    HBRUSH borderBrush = CreateSolidBrush(COLOR_BORDER);
    FrameRgn(hdc, hRgn, borderBrush, 1, 1);
    DeleteObject(borderBrush);
    DeleteObject(hRgn);
    
    // Draw text
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    SelectObject(hdc, hFontBold);
    
    DrawText(hdc, text.c_str(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void DrawContactItem(HDC hdc, RECT rect, const Contact& contact, bool isSelected)
{
    // Fill background
    HBRUSH bgBrush = CreateSolidBrush(isSelected ? COLOR_HOVER : COLOR_SURFACE);
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);
    
    // Draw avatar circle
    int avatarSize = 40;
    int avatarX = rect.left + 12;
    int avatarY = rect.top + (rect.bottom - rect.top - avatarSize) / 2;
    
    HBRUSH avatarBrush = CreateSolidBrush(COLOR_PRIMARY);
    HPEN avatarPen = CreatePen(PS_SOLID, 2, contact.isOnline ? RGB(34, 197, 94) : RGB(156, 163, 175));
    
    SelectObject(hdc, avatarBrush);
    SelectObject(hdc, avatarPen);
    
    Ellipse(hdc, avatarX, avatarY, avatarX + avatarSize, avatarY + avatarSize);
    
    DeleteObject(avatarBrush);
    DeleteObject(avatarPen);
    
    // Draw contact name
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, COLOR_TEXT_PRIMARY);
    SelectObject(hdc, hFontBold);
    
    RECT nameRect = {avatarX + avatarSize + 12, rect.top + 8, rect.right - 8, rect.top + 28};
    DrawText(hdc, contact.name.c_str(), -1, &nameRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    
    // Draw status
    SetTextColor(hdc, COLOR_TEXT_SECONDARY);
    SelectObject(hdc, hFontRegular);
    
    RECT statusRect = {avatarX + avatarSize + 12, rect.top + 30, rect.right - 8, rect.top + 48};
    DrawText(hdc, contact.status.c_str(), -1, &statusRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    
    // Draw last message preview
    RECT msgRect = {avatarX + avatarSize + 12, rect.top + 50, rect.right - 8, rect.bottom - 8};
    DrawText(hdc, contact.lastMessage.c_str(), -1, &msgRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

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
    ::SendMessage(hContactsList, LB_SETITEMHEIGHT, 0, 72);
    
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
    originalButtonProc = (WNDPROC)SetWindowLongPtr(hSendButton, GWLP_WNDPROC, (LONG_PTR)ModernButtonProc);
    SetWindowLongPtr(hShareFileButton, GWLP_WNDPROC, (LONG_PTR)ModernButtonProc);
    
    // Subclass contact list for custom drawing
    originalListBoxProc = (WNDPROC)SetWindowLongPtr(hContactsList, GWLP_WNDPROC, (LONG_PTR)ModernListBoxProc);
    
    // Set modern background colors
    SetClassLongPtr(hChatDisplay, GCLP_HBRBACKGROUND, (LONG_PTR)hBrushSurface);
    SetClassLongPtr(hMessageInput, GCLP_HBRBACKGROUND, (LONG_PTR)hBrushSurface);
    SetClassLongPtr(hSharedFilesList, GCLP_HBRBACKGROUND, (LONG_PTR)hBrushSurface);
}

void LoadContactChat(int contactIndex)
{
    if (contactIndex < 0 || contactIndex >= (int)contacts.size()) return;
    
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

void UpdateSharedFilesList()
{
    if (selectedContactIndex < 0 || selectedContactIndex >= (int)contacts.size()) return;
    
    const Contact& contact = contacts[selectedContactIndex];
    
    // Clear the list
    ::SendMessage(hSharedFilesList, LB_RESETCONTENT, 0, 0);
    
    // Add shared files with file type icons
    for (const auto& file : contact.sharedFiles) {
        std::wstring icon = GetFileExtensionIcon(file.fileName);
        std::wstring displayText = icon + L" " + file.fileName;
        displayText += L"\n   ?? " + file.sharedBy;
        ::SendMessage(hSharedFilesList, LB_ADDSTRING, 0, (LPARAM)displayText.c_str());
    }
}

void AddMessageToChat(const std::wstring& message, bool isOutgoing)
{
    if (selectedContactIndex < 0 || selectedContactIndex >= (int)contacts.size()) return;
    
    Contact& contact = contacts[selectedContactIndex];
    
    // Add timestamp to message
    SYSTEMTIME st;
    GetLocalTime(&st);
    WCHAR timeStr[50];
    swprintf_s(timeStr, 50, L"[%02d:%02d] ", st.wHour, st.wMinute);
    
    std::wstring formattedMessage;
    if (isOutgoing) {
        formattedMessage = L"You: " + message + L" ?";
    } else {
        formattedMessage = contact.name + L": " + message;
    }
    
    contact.messages.push_back(formattedMessage);
    contact.lastMessage = message.length() > 50 ? message.substr(0, 47) + L"..." : message;
    
    // Update chat display
    LoadContactChat(selectedContactIndex);
    
    // Refresh contacts list to update last message preview
    InvalidateRect(hContactsList, NULL, TRUE);
}

void AddSharedFileToChat(const SharedFile& file, bool isOutgoing)
{
    if (selectedContactIndex < 0 || selectedContactIndex >= (int)contacts.size()) return;
    
    Contact& contact = contacts[selectedContactIndex];
    std::wstring sharer = isOutgoing ? L"You" : contact.name;
    std::wstring formattedMessage = sharer + L" shared: " + file.fileName;
    
    contact.messages.push_back(formattedMessage);
    
    // Add to shared files list
    if (isOutgoing) {
        SharedFile newFile = file;
        newFile.sharedBy = L"You";
        contact.sharedFiles.push_back(newFile);
    }
    
    // Update both chat display and shared files list
    LoadContactChat(selectedContactIndex);
}

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
        size_t lastSlash = fullPath.find_last_of(L"\\");// Create shared file entry
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
        SetTimer(GetParent(hMessageInput), 2, 1500, NULL);
    }
}

void OpenSharedFile(int fileIndex)
{
    if (selectedContactIndex < 0 || selectedContactIndex >= (int)contacts.size()) return;
    
    const Contact& contact = contacts[selectedContactIndex];
    if (fileIndex < 0 || fileIndex >= (int)contact.sharedFiles.size()) return;
    
    const SharedFile& file = contact.sharedFiles[fileIndex];
    
    // Enhanced file info display with emojis and better formatting
    std::wstring message = L"File Information\n\n";
    message += L"Name: " + file.fileName + L"\n";
    message += L"Path: " + file.filePath + L"\n";
    message += L"Shared by: " + file.sharedBy + L"\n";
    
    WCHAR timeStr[100];
    swprintf_s(timeStr, 100, L"Shared on: %02d/%02d/%04d at %02d:%02d", 
        file.timeShared.wMonth, file.timeShared.wDay, file.timeShared.wYear,
        file.timeShared.wHour, file.timeShared.wMinute);
    message += timeStr;
    message += L"\n\nTip: In a real application, this file would open automatically!";
    
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

// Modern button subclass procedure
LRESULT CALLBACK ModernButtonProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static bool isHovered = false;
    static bool isPressed = false;
    
    switch (msg)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            RECT rect;
            GetClientRect(hWnd, &rect);
            
            WCHAR text[256];
            GetWindowText(hWnd, text, 256);
            
            DrawModernButton(hdc, rect, std::wstring(text), isHovered, isPressed);
            
            EndPaint(hWnd, &ps);
            return 0;
        }
        
    case WM_MOUSEMOVE:
        if (!isHovered)
        {
            isHovered = true;
            InvalidateRect(hWnd, NULL, FALSE);
            
            TRACKMOUSEEVENT tme = {};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hWnd;
            TrackMouseEvent(&tme);
        }
        break;
        
    case WM_MOUSELEAVE:
        isHovered = false;
        InvalidateRect(hWnd, NULL, FALSE);
        break;
        
    case WM_LBUTTONDOWN:
        isPressed = true;
        InvalidateRect(hWnd, NULL, FALSE);
        SetCapture(hWnd);
        break;
        
    case WM_LBUTTONUP:
        if (isPressed)
        {
            isPressed = false;
            InvalidateRect(hWnd, NULL, FALSE);
            ReleaseCapture();
            
            // Send click notification to parent
            HWND hParent = GetParent(hWnd);
            int controlId = GetDlgCtrlID(hWnd);
            ::SendMessage(hParent, WM_COMMAND, MAKEWPARAM(controlId, BN_CLICKED), (LPARAM)hWnd);
        }
        break;
    }
    
    return CallWindowProc(originalButtonProc, hWnd, msg, wParam, lParam);
}

// Modern listbox subclass procedure  
LRESULT CALLBACK ModernListBoxProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
        if (hWnd == hContactsList)
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            RECT clientRect;
            GetClientRect(hWnd, &clientRect);
            
            // Fill background
            FillRect(hdc, &clientRect, hBrushSurface);
            
            int itemCount = contacts.size();
            int itemHeight = 72;
            int selectedIndex = ::SendMessage(hWnd, LB_GETCURSEL, 0, 0);
            
            for (int i = 0; i < itemCount; i++)
            {
                RECT itemRect = {0, i * itemHeight, clientRect.right, (i + 1) * itemHeight};
                DrawContactItem(hdc, itemRect, contacts[i], i == selectedIndex);
            }
            
            EndPaint(hWnd, &ps);
            return 0;
        }
        break;
    }
    
    return CallWindowProc(originalListBoxProc, hWnd, msg, wParam, lParam);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        CreateChatUI(hWnd);
        break;
        
    case WM_SIZE:
        {
            // Resize chat UI elements when window is resized with modern spacing
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
        break;
        
    case WM_CTLCOLORSTATIC:
        {
            HDC hdcStatic = (HDC)wParam;
            SetTextColor(hdcStatic, COLOR_TEXT_PRIMARY);
            SetBkColor(hdcStatic, COLOR_APP_BACKGROUND);
            return (INT_PTR)hBrushBackground;
        }
        
    case WM_CTLCOLOREDIT:
        {
            HDC hdcEdit = (HDC)wParam;
            SetTextColor(hdcEdit, COLOR_TEXT_PRIMARY);
            SetBkColor(hdcEdit, COLOR_SURFACE);
            return (INT_PTR)hBrushSurface;
        }
        
    case WM_CTLCOLORLISTBOX:
        {
            HDC hdcList = (HDC)wParam;
            SetTextColor(hdcList, COLOR_TEXT_PRIMARY);
            SetBkColor(hdcList, COLOR_SURFACE);
            return (INT_PTR)hBrushSurface;
        }
        
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            // Fill the main window background with modern color
            RECT rect;
            GetClientRect(hWnd, &rect);
            FillRect(hdc, &rect, hBrushBackground);
            
            EndPaint(hWnd, &ps);
        }
        break;
        
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);
            
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDC_CONTACTS_LIST:
                if (wmEvent == LBN_SELCHANGE) {
                    int selectedIndex = (int)::SendMessage(hContactsList, LB_GETCURSEL, 0, 0);
                    LoadContactChat(selectedIndex);
                }
                break;
            case IDC_SEND_BUTTON:
                SendChatMessage();
                break;
            case IDC_SHARE_FILE_BUTTON:
                ShareFile();
                break;
            case IDC_SHARED_FILES_LIST:
                if (wmEvent == LBN_DBLCLK) {
                    int selectedFile = (int)::SendMessage(hSharedFilesList, LB_GETCURSEL, 0, 0);
                    OpenSharedFile(selectedFile);
                }
                break;
            case IDC_MESSAGE_INPUT:
                if (wmEvent == EN_CHANGE) {
                    // Enable/disable send button based on input with visual feedback
                    WCHAR buffer[1024];
                    GetWindowText(hMessageInput, buffer, 1024);
                    bool hasText = wcslen(buffer) > 0;
                    EnableWindow(hSendButton, hasText);
                    InvalidateRect(hSendButton, NULL, FALSE);
                }
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
        
    case WM_TIMER:
        {
            if (wParam == 1 && selectedContactIndex >= 0) {
                KillTimer(hWnd, 1);
                
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
            else if (wParam == 2 && selectedContactIndex >= 0) {
                KillTimer(hWnd, 2);
                
                // Auto-reply acknowledging the shared file with emojis
                std::vector<std::wstring> fileReplies = {
                    L"Thanks for sharing the file!",
                    L"Got the file, will check it out.",
                    L"File received, thanks! ?",
                    L"Perfect timing, I needed this file.",
                    L"Awesome, downloading now!"
                };
                
                int replyIndex = rand() % fileReplies.size();
                AddMessageToChat(fileReplies[replyIndex], false);
            }
        }
        break;
        
    case WM_KEYDOWN:
        {
            if (GetFocus() == hMessageInput && wParam == VK_RETURN) {
                if (!(GetKeyState(VK_SHIFT) & 0x8000)) {
                    // Enter without Shift sends the message
                    SendChatMessage();
                    return 0;
                }
            }
        }
        break;
        
    case WM_DESTROY:
        CleanupModernUI();
        PostQuitMessage(0);
        break;
        
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
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
