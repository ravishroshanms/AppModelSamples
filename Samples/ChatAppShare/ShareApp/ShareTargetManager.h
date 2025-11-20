#pragma once

#include <windows.h>
#include <string>

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
    
private:
    static bool s_initialized;
    static bool s_shareTargetSupported;
};