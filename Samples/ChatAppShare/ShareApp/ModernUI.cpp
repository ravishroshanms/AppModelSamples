#include "ModernUI.h"
#include "UIConstants.h"
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

// Modern UI Variables definitions
HBRUSH hBrushBackground = nullptr;
HBRUSH hBrushSurface = nullptr;
HBRUSH hBrushPrimary = nullptr;
HBRUSH hBrushHover = nullptr;
HFONT hFontRegular = nullptr;
HFONT hFontBold = nullptr;
HFONT hFontTitle = nullptr;
HPEN hPenBorder = nullptr;

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
    int avatarX = rect.left + 12;
    int avatarY = rect.top + (rect.bottom - rect.top - AVATAR_SIZE) / 2;
    
    HBRUSH avatarBrush = CreateSolidBrush(COLOR_PRIMARY);
    HPEN avatarPen = CreatePen(PS_SOLID, 2, contact.isOnline ? RGB(34, 197, 94) : RGB(156, 163, 175));
    
    SelectObject(hdc, avatarBrush);
    SelectObject(hdc, avatarPen);
    
    Ellipse(hdc, avatarX, avatarY, avatarX + AVATAR_SIZE, avatarY + AVATAR_SIZE);
    
    DeleteObject(avatarBrush);
    DeleteObject(avatarPen);
    
    // Draw contact name
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, COLOR_TEXT_PRIMARY);
    SelectObject(hdc, hFontBold);
    
    RECT nameRect = {avatarX + AVATAR_SIZE + 12, rect.top + 8, rect.right - 8, rect.top + 28};
    DrawText(hdc, contact.name.c_str(), -1, &nameRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    
    // Draw status
    SetTextColor(hdc, COLOR_TEXT_SECONDARY);
    SelectObject(hdc, hFontRegular);
    
    RECT statusRect = {avatarX + AVATAR_SIZE + 12, rect.top + 30, rect.right - 8, rect.top + 48};
    DrawText(hdc, contact.status.c_str(), -1, &statusRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    
    // Draw last message preview
    RECT msgRect = {avatarX + AVATAR_SIZE + 12, rect.top + 50, rect.right - 8, rect.bottom - 8};
    DrawText(hdc, contact.lastMessage.c_str(), -1, &msgRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}