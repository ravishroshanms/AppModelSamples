#pragma once

#include <windows.h>

// Window procedures
LRESULT CALLBACK ModernButtonProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ModernListBoxProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Original window procedures storage
extern WNDPROC originalButtonProc;
extern WNDPROC originalListBoxProc;

// Setup functions
void SetupCustomWindowProcs();