// SampleChatAppWithShare.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "ShareApp.h"
#include "UIConstants.h"
#include "ChatModels.h"
#include "ModernUI.h"
#include "ChatManager.h"
#include "FileManager.h"
#include "UIManager.h"
#include "PackageIdentity.h"
#include "ShareTargetManager.h"
#include "WindowProcs.h"
#include <commctrl.h>
#include <shellapi.h>
#include <objbase.h>
#include <winrt/base.h>
#include <sstream>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "ole32.lib")
// Add WinRT libraries to fix linking error
#pragma comment(lib, "windowsapp.lib")
#pragma comment(lib, "runtimeobject.lib")

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::ApplicationModel::DataTransfer;
using namespace Windows::ApplicationModel::DataTransfer::ShareTarget;

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Global flag to track if we're in share-only mode
bool g_isShareOnlyMode = false;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Init package identity checks
    InitializePackageIdentity();

    // Initialize WinRT with proper error handling
    try
    {
        winrt::init_apartment();
        OutputDebugStringW(L"ChatApp: WinRT initialized successfully.\n");
    }
    catch (hresult_error const& ex)
    {
        std::wstring errorMsg = L"ChatApp: WinRT initialization failed: " + std::wstring(ex.message().c_str()) + 
                               L" (HRESULT: 0x" + std::to_wstring(static_cast<uint32_t>(ex.code())) + L")\n";
        OutputDebugStringW(errorMsg.c_str());
    }
    catch (...)
    {
        OutputDebugStringW(L"ChatApp: WinRT initialization failed with unknown error.\n");
    }

    // Initialize Share Target Manager
    ShareTargetManager::Initialize();

    // Note: Don't process share target activation here - UI isn't created yet
    // This will be handled after UI creation in WndProc WM_CREATE

    // Check if this is a share target activation to determine if we need the main window
    if (g_isRunningWithIdentity && ShareTargetManager::IsShareTargetActivation())
    {
        g_isShareOnlyMode = true;
        OutputDebugStringW(L"ChatApp: Running in share-only mode - main window will not be created\n");
    }

    if (g_isSparsePackageSupported && !g_isRunningWithIdentity)
    {
        std::wstring executableDir = GetExecutableDirectory();
        std::wstring packagePath = executableDir + L"\\Weixin_1.0.2.0_x86__v4k3sbdawh17a.msix";
        
        // Validate the MSIX package before attempting registration
        if (ValidateMsixPackage(packagePath))
        {
            HRESULT result = RegisterPackageWithExternalLocation(executableDir, packagePath);
            if (SUCCEEDED(result))
            {
                OutputDebugStringW(L"ChatApp: Package registration succeeded. Relaunching...\n");
                RelaunchApplication();
                return 0; // Exit after relaunch
            }
            else
            {
                // Log the error but continue without package identity
                wchar_t errorLog[256];
                swprintf_s(errorLog, L"ChatApp: Failed to register package. HRESULT: 0x%08X. Continuing without package identity.\n", result);
                OutputDebugStringW(errorLog);
            }
        }
        else
        {
            OutputDebugStringW(L"ChatApp: MSIX package validation failed. Continuing without package identity.\n");
        }
    }
    else if (g_isRunningWithIdentity)
    {
        // Process share target activation using the ShareTargetManager
        ShareTargetManager::ProcessActivationArgs();
    }
    else
    {
        // Log the current status
        std::wstring status = GetPackageIdentityStatus();
        OutputDebugStringW(status.c_str());
    }

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
    
    if (!g_isShareOnlyMode)
    {
        // Only register window class if we're not in share-only mode
        MyRegisterClass(hInstance);
        
        // Initialize modern UI
        InitializeModernUI();
    }

    // Initialize dummy contacts (needed for share target)
    InitializeContacts();

    if (g_isShareOnlyMode)
    {
        // In share-only mode, process the share target directly without creating a window
        OutputDebugStringW(L"ChatApp: Processing share target in share-only mode\n");
        ShareTargetManager::ProcessActivationArgs();
    }
    else
    {
        // Normal mode: create and show the main window
        if (!InitInstance(hInstance, nCmdShow))
        {
            return FALSE;
        }
    }

    HACCEL hAccelTable = nullptr;
    if (!g_isShareOnlyMode)
    {
        hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SAMPLECHATAPPWITHSHARE));
    }

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!g_isShareOnlyMode && !TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else if (g_isShareOnlyMode)
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

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        CreateChatUI(hWnd);
        
        // Process share target activation after UI is created
        if (g_isRunningWithIdentity)
        {
            ShareTargetManager::ProcessActivationArgs();
        }
        break;
        
    case WM_SIZE:
        ResizeChatUI(hWnd);
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
                    UpdateSharedFilesList();
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
            case 2000: // Test WinRT Share Target Status
                {
                    // Use ShareTargetManager to get status
                    std::wstring statusInfo = ShareTargetManager::GetShareTargetStatus();
                    MessageBoxW(hWnd, statusInfo.c_str(), L"WinRT Share Target Status", MB_OK | MB_ICONINFORMATION);
                }
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
        
    case WM_TIMER:
        ProcessAutoReply(hWnd, (int)wParam);
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
        {
            // Add WinRT and package identity information to the about dialog
            std::wstring aboutText = L"Chat Application with WinRT Share Target Support\n\n";
            
            // Test WinRT status
            try
            {
                winrt::check_hresult(S_OK);
                aboutText += L"? WinRT Status: ? Initialized and Working\n";
                
                if (g_isRunningWithIdentity)
                {
                    if (ShareTargetManager::IsShareTargetActivation())
                    {
                        aboutText += L"?? Share Target: ? Currently Activated via Windows Share Sheet\n";
                    }
                    else
                    {
                        aboutText += L"?? Share Target: ? Ready for Activation\n";
                    }
                }
                else
                {
                    aboutText += L"?? Share Target: ? Package Identity Required\n";
                }
            }
            catch (...)
            {
                aboutText += L"?? WinRT Status: ? Error or Not Available\n";
            }
            
            aboutText += L"?? Package Identity: " + std::wstring(g_isRunningWithIdentity ? L"? Available" : L"? Not Available") + L"\n";
            aboutText += L"?? Sparse Package Support: " + std::wstring(g_isSparsePackageSupported ? L"? Supported" : L"? Not Supported") + L"\n\n";
            aboutText += L"Current Features:\n";
            aboutText += L"? WinRT Runtime Integration\n";
            aboutText += L"? Windows Share Target Support\n";
            aboutText += L"? Package Identity Management\n";
            aboutText += L"? MSIX Package Registration\n";
            aboutText += L"? Modern Chat UI\n\n";
            aboutText += GetPackageIdentityStatus();
            
            SetDlgItemTextW(hDlg, IDC_STATIC, aboutText.c_str());
            return (INT_PTR)TRUE;
        }

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
