#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <tlhelp32.h>
#include <shellapi.h>
#include <appmodel.h>

#pragma comment(lib, "shell32.lib")

// Simple Share Target Manager with minimal dependencies
class ShareTargetManager
{
public:
    // Initialize the share target manager
    static void Initialize();
    
    // Check if the current activation is a share target
    static bool IsShareTargetActivation();
    
    // Check activation arguments and process if it's a share target
    static bool ProcessActivationArgs();
    
    // Get share target status information
    static std::wstring GetShareTargetStatus();
    
    // Check if share target is available (requires package identity)
    static bool IsShareTargetAvailable();

private:
    // Logging helpers
    static void LogShareInfo(const std::wstring& message);
    static void LogShareError(const std::wstring& error);
    
    // Package application discovery and launch helpers using proper Package Manager APIs
    static std::vector<DWORD> GetPackageProcesses();
    static bool IsProcessInSamePackage(DWORD processId);
    static bool IsProcessInSamePackage(DWORD processId, const std::wstring& targetPackageFamilyName);
    static HWND FindPackageApplicationWindow(const std::wstring& windowTitle);
    static bool LaunchPackageApplication(const std::wstring& appExecutableName);
    static HWND WaitForApplicationWindow(const std::wstring& windowTitle, DWORD timeoutMs = 10000);
    static HWND FindOrLaunchPackageApplication(const std::wstring& appExecutableName);
    
private:
    static bool s_initialized;
    static bool s_shareTargetSupported;
};