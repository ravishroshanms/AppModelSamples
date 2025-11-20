#pragma once

#include <windows.h>
#include <string>
#include "ChatModels.h"

// Modern UI Variables
extern HBRUSH hBrushBackground;
extern HBRUSH hBrushSurface;
extern HBRUSH hBrushPrimary;
extern HBRUSH hBrushHover;
extern HFONT hFontRegular;
extern HFONT hFontBold;
extern HFONT hFontTitle;
extern HPEN hPenBorder;

// Modern UI Functions
void InitializeModernUI();
void CleanupModernUI();
void DrawModernButton(HDC hdc, RECT rect, const std::wstring& text, bool isHovered, bool isPressed);
void DrawContactItem(HDC hdc, RECT rect, const Contact& contact, bool isSelected);