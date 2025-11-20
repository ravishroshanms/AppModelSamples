#include "WindowProcs.h"
#include "UIConstants.h"
#include "ModernUI.h"
#include "ChatModels.h"
#include "UIManager.h"

// External declarations for UI window handles
extern HWND hSendButton;
extern HWND hShareFileButton;
extern HWND hContactsList;

// Window procedure storage for subclassing
WNDPROC originalButtonProc = nullptr;
WNDPROC originalListBoxProc = nullptr;

void SetupCustomWindowProcs()
{
    // Subclass buttons for custom drawing
    originalButtonProc = (WNDPROC)SetWindowLongPtr(hSendButton, GWLP_WNDPROC, (LONG_PTR)ModernButtonProc);
    SetWindowLongPtr(hShareFileButton, GWLP_WNDPROC, (LONG_PTR)ModernButtonProc);
    
    // Subclass contact list for custom drawing
    originalListBoxProc = (WNDPROC)SetWindowLongPtr(hContactsList, GWLP_WNDPROC, (LONG_PTR)ModernListBoxProc);
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
    case WM_LBUTTONDOWN:
        if (hWnd == hContactsList)
        {
            // Handle mouse click to ensure proper contact selection with scrolling
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            
            // Calculate which contact was clicked based on the scroll position
            int topIndex = (int)::SendMessage(hWnd, LB_GETTOPINDEX, 0, 0);
            int clickedVisiblePos = pt.y / CONTACT_ITEM_HEIGHT;
            int actualContactIndex = topIndex + clickedVisiblePos;
            
            // Ensure the clicked contact is valid
            if (actualContactIndex >= 0 && actualContactIndex < (int)contacts.size())
            {
                // Set the correct selection in the listbox
                ::SendMessage(hWnd, LB_SETCURSEL, actualContactIndex, 0);
                
                // Trigger the selection change event to update the UI
                HWND hParent = GetParent(hWnd);
                int controlId = GetDlgCtrlID(hWnd);
                ::SendMessage(hParent, WM_COMMAND, MAKEWPARAM(controlId, LBN_SELCHANGE), (LPARAM)hWnd);
                
                return 0; // We handled the click
            }
        }
        break;
        
    case WM_PAINT:
        if (hWnd == hContactsList)
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            RECT clientRect;
            GetClientRect(hWnd, &clientRect);
            
            // Fill background
            FillRect(hdc, &clientRect, hBrushSurface);
            
            int itemCount = (int)contacts.size();
            int selectedIndex = (int)::SendMessage(hWnd, LB_GETCURSEL, 0, 0);
            
            // Get the first visible item index to handle scrolling correctly
            int topIndex = (int)::SendMessage(hWnd, LB_GETTOPINDEX, 0, 0);
            
            // Calculate how many items can be visible
            int visibleItemCount = (clientRect.bottom / CONTACT_ITEM_HEIGHT) + 1;
            
            // Draw only the visible items
            for (int visiblePos = 0; visiblePos < visibleItemCount; visiblePos++)
            {
                int actualIndex = topIndex + visiblePos;
                
                // Stop if we've reached the end of the contact list
                if (actualIndex >= itemCount) break;
                
                RECT itemRect = {0, visiblePos * CONTACT_ITEM_HEIGHT, clientRect.right, (visiblePos + 1) * CONTACT_ITEM_HEIGHT};
                
                // Draw the contact that should be visible at this position
                DrawContactItem(hdc, itemRect, contacts[actualIndex], actualIndex == selectedIndex);
            }
            
            EndPaint(hWnd, &ps);
            return 0;
        }
        break;
    }
    
    return CallWindowProc(originalListBoxProc, hWnd, msg, wParam, lParam);
}