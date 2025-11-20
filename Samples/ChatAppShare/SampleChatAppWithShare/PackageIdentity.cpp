#include "framework.h"
#include "PackageIdentity.h"
#include <VersionHelpers.h>
#include <appmodel.h>
#include <shellapi.h>
#include <vector>
#include <algorithm>

// Package Identity Variables
bool g_isSparsePackageSupported = false;
bool g_isRunningWithIdentity = false;
bool g_packageIdentityInitialized = false;

// Initialize package identity management
bool InitializePackageIdentity()
{
    if (g_packageIdentityInitialized)
        return true;

    OutputDebugStringW(L"ChatApp: Initializing package identity management...\n");

    // Check OS support for sparse packages
    g_isSparsePackageSupported = IsSparsePackageSupported();
    
    // Check if already running with identity
    g_isRunningWithIdentity = IsRunningWithIdentity();
    
    g_packageIdentityInitialized = true;
    
    wchar_t statusLog[256];
    swprintf_s(statusLog, L"ChatApp: Package Identity Status - Sparse supported: %s, Has identity: %s\n",
        g_isSparsePackageSupported ? L"Yes" : L"No",
        g_isRunningWithIdentity ? L"Yes" : L"No");
    OutputDebugStringW(statusLog);
    
    return true;
}

// Register package with external location (improved implementation)
HRESULT RegisterPackageWithExternalLocation(const std::wstring& externalLocation, const std::wstring& packagePath)
{
    try
    {
        OutputDebugStringW(L"ChatApp: Attempting to register package with external location...\n");
        
        wchar_t logBuffer[512];
        swprintf_s(logBuffer, L"ChatApp: External location: %s\n", externalLocation.c_str());
        OutputDebugStringW(logBuffer);
        swprintf_s(logBuffer, L"ChatApp: Package path: %s\n", packagePath.c_str());
        OutputDebugStringW(logBuffer);
        
        // Check if the package file exists
        DWORD fileAttributes = GetFileAttributesW(packagePath.c_str());
        if (fileAttributes == INVALID_FILE_ATTRIBUTES)
        {
            OutputDebugStringW(L"ChatApp: Package file not found, trying PowerShell registration method.\n");
            return RegisterPackageWithExternalLocationPowerShell(externalLocation, packagePath);
        }
        
        // Try PowerShell registration first as it's more reliable
        HRESULT powershellResult = RegisterPackageWithExternalLocationPowerShell(externalLocation, packagePath);
        if (SUCCEEDED(powershellResult))
        {
            OutputDebugStringW(L"ChatApp: Package registration via PowerShell succeeded.\n");
            return powershellResult;
        }
        
        // If PowerShell failed, log the error
        wchar_t errorLog[256];
        swprintf_s(errorLog, L"ChatApp: PowerShell registration failed with HRESULT: 0x%08X\n", powershellResult);
        OutputDebugStringW(errorLog);
        
        // For now, return the PowerShell result since we don't have other registration methods implemented
        return powershellResult;
    }
    catch (...)
    {
        OutputDebugStringW(L"ChatApp: Exception occurred during package registration\n");
        return E_FAIL;
    }
}

// Alternative implementation using PowerShell for package registration
HRESULT RegisterPackageWithExternalLocationPowerShell(const std::wstring& externalLocation, const std::wstring& packagePath)
{
    try
    {
        OutputDebugStringW(L"ChatApp: Attempting PowerShell package registration...\n");
        
        // Check if the package file exists
        DWORD fileAttributes = GetFileAttributesW(packagePath.c_str());
        if (fileAttributes == INVALID_FILE_ATTRIBUTES)
        {
            wchar_t errorLog[512];
            swprintf_s(errorLog, L"ChatApp: Package file not found at: %s\n", packagePath.c_str());
            OutputDebugStringW(errorLog);
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }
        
        // Build PowerShell command for MSIX package registration with external location
        // Use Add-AppxPackage with -ExternalLocation parameter
        std::wstring powershellCmd = L"powershell.exe -ExecutionPolicy Bypass -Command \"";
        powershellCmd += L"try { ";
        powershellCmd += L"Add-AppxPackage -Path '";
        powershellCmd += packagePath;
        powershellCmd += L"' -ExternalLocation '";
        powershellCmd += externalLocation;
        powershellCmd += L"' -ForceTargetApplicationShutdown; ";
        powershellCmd += L"Write-Host 'Package registration successful'; ";
        powershellCmd += L"exit 0; ";
        powershellCmd += L"} catch { ";
        powershellCmd += L"Write-Host ('Package registration failed: ' + $_.Exception.Message); ";
        powershellCmd += L"exit 1; ";
        powershellCmd += L"}\"";
        
        wchar_t logBuffer[1024];
        swprintf_s(logBuffer, L"ChatApp: PowerShell command: %s\n", powershellCmd.c_str());
        OutputDebugStringW(logBuffer);
        
        // Execute PowerShell command
        STARTUPINFOW si = {};
        PROCESS_INFORMATION pi = {};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        
        BOOL success = CreateProcessW(
            nullptr,
            const_cast<LPWSTR>(powershellCmd.c_str()),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &si,
            &pi
        );
        
        if (success)
        {
            OutputDebugStringW(L"ChatApp: PowerShell process started, waiting for completion...\n");
            
            // Wait for PowerShell to complete (with timeout)
            DWORD waitResult = WaitForSingleObject(pi.hProcess, 60000); // Increased timeout for package registration
            
            DWORD exitCode = 0;
            if (waitResult == WAIT_OBJECT_0)
            {
                GetExitCodeProcess(pi.hProcess, &exitCode);
                wchar_t exitLog[256];
                swprintf_s(exitLog, L"ChatApp: PowerShell process completed with exit code: %d\n", exitCode);
                OutputDebugStringW(exitLog);
            }
            else if (waitResult == WAIT_TIMEOUT)
            {
                OutputDebugStringW(L"ChatApp: PowerShell process timed out. Package registration may still be in progress.\n");
                TerminateProcess(pi.hProcess, 1);
                exitCode = 1;
            }
            else
            {
                DWORD waitError = GetLastError();
                wchar_t waitLog[256];
                swprintf_s(waitLog, L"ChatApp: Wait failed with error: %d\n", waitError);
                OutputDebugStringW(waitLog);
                exitCode = 1;
            }
            
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            
            return (exitCode == 0) ? S_OK : E_FAIL;
        }
        else
        {
            DWORD error = GetLastError();
            wchar_t errorLog[256];
            swprintf_s(errorLog, L"ChatApp: Failed to start PowerShell process. Error: %d\n", error);
            OutputDebugStringW(errorLog);
            return HRESULT_FROM_WIN32(error);
        }
    }
    catch (...)
    {
        OutputDebugStringW(L"ChatApp: Exception occurred during PowerShell package registration\n");
        return E_FAIL;
    }
}

// Relaunch the current application
void RelaunchApplication()
{
    OutputDebugStringW(L"ChatApp: Attempting to relaunch application...\n");
    
    wchar_t exePath[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    if (len == 0 || len == MAX_PATH)
    {
        OutputDebugStringW(L"ChatApp: Failed to get executable path for relaunch.\n");
        return;
    }
    
    // Log the executable path
    wchar_t logBuffer[512];
    swprintf_s(logBuffer, L"ChatApp: Relaunching: %s\n", exePath);
    OutputDebugStringW(logBuffer);
    
    // Add a small delay to allow package registration to complete
    Sleep(2000);
    
    // Use ShellExecuteW to relaunch the current executable
    HINSTANCE result = ShellExecuteW(nullptr, L"open", exePath, nullptr, nullptr, SW_SHOWNORMAL);
    
    // Fix: Use proper casting for x64 compatibility
    INT_PTR resultValue = reinterpret_cast<INT_PTR>(result);
    if (resultValue <= 32)
    {
        // Log the error
        wchar_t errorLog[256];
        swprintf_s(errorLog, L"ChatApp: Failed to relaunch application. ShellExecute error code: %lld\n", 
                   static_cast<long long>(resultValue));
        OutputDebugStringW(errorLog);
        
        // Try alternative relaunch method using CreateProcess
        OutputDebugStringW(L"ChatApp: Trying alternative relaunch method...\n");
        
        STARTUPINFOW si = {};
        PROCESS_INFORMATION pi = {};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOWNORMAL;
        
        BOOL createResult = CreateProcessW(
            exePath,
            nullptr,
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &si,
            &pi
        );
        
        if (createResult)
        {
            OutputDebugStringW(L"ChatApp: Alternative relaunch method succeeded.\n");
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        else
        {
            DWORD createError = GetLastError();
            wchar_t createErrorLog[256];
            swprintf_s(createErrorLog, L"ChatApp: Alternative relaunch also failed. Error: %d\n", createError);
            OutputDebugStringW(createErrorLog);
        }
    }
    else
    {
        OutputDebugStringW(L"ChatApp: Application relaunch initiated successfully.\n");
    }
}

// Checks if the OS version is Windows 10 2004 (build 19041) or later
bool IsSparsePackageSupported()
{
    // Windows 10 2004 is version 10.0.19041
    OSVERSIONINFOEXW osvi = {};
    osvi.dwOSVersionInfoSize = sizeof(osvi);

    // Get the actual version using RtlGetVersion (undocumented but reliable)
    typedef LONG (WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOEXW);
    HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
    if (hMod) {
        RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");
        if (fxPtr != nullptr) {
            fxPtr((PRTL_OSVERSIONINFOEXW)&osvi);
            
            // Log version information for debugging
            wchar_t log[256];
            swprintf_s(log, L"ChatApp: Current OS Version: %u.%u.%u\n", 
                osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
            OutputDebugStringW(log);
        }
    }

    // Compare with required version (Windows 10 2004 build 19041)
    if (osvi.dwMajorVersion > 10 ||
        (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion > 0) ||
        (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0 && osvi.dwBuildNumber >= 19041))
    {
        OutputDebugStringW(L"ChatApp: Sparse package is supported on this OS.\n");
        return true;
    }
    
    OutputDebugStringW(L"ChatApp: Sparse package is NOT supported on this OS.\n");
    return false;
}

// Returns true if the app is running with package identity
bool IsRunningWithIdentity()
{
    UINT32 length = 0;
    LONG rc = GetCurrentPackageFullName(&length, nullptr);

    if (rc == ERROR_INSUFFICIENT_BUFFER)
    {
        std::vector<wchar_t> packageFullName(length);
        rc = GetCurrentPackageFullName(&length, packageFullName.data());
        if (rc == ERROR_SUCCESS)
        {
            OutputDebugStringW(L"ChatApp: Running with package identity.\n");
            return true;
        }
    }
    
    OutputDebugStringW(L"ChatApp: Not running with package identity.\n");
    return false;
}

// Helper to get the directory of the current executable
std::wstring GetExecutableDirectory()
{
    wchar_t exePath[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    if (len == 0 || len == MAX_PATH)
    {
        OutputDebugStringW(L"ChatApp: Failed to get executable path.\n");
        return L"";
    }
    
    std::wstring path(exePath);
    size_t pos = path.find_last_of(L"\\/");
    if (pos != std::wstring::npos)
        path = path.substr(0, pos);
        
    return path;
}

// Initialize package identity-specific flows and features
void InitializePackageIdentityFlow()
{
    OutputDebugStringW(L"ChatApp: Initializing package identity-specific flows...\n");
    
    if (!g_isRunningWithIdentity) {
        OutputDebugStringW(L"ChatApp: Not running with package identity - skipping identity-specific flows.\n");
        return;
    }
    
    // TODO: Add package identity-specific initialization here:
    
    // 1. Single Instance Management
    // - Check for existing app instances
    // - Register current instance
    // - Handle instance redirection
    
    // 2. Share Target Registration
    // - Check for share target activation
    // - Initialize share target handlers
    // - Set up cross-process communication
    
    // 3. Enhanced Security Features
    // - Initialize secure file handling
    // - Set up identity-based permissions
    // - Enable enhanced data protection
    
    // 4. App Model Integration
    // - Register activation handlers
    // - Set up background task support
    // - Initialize notification system
    
    OutputDebugStringW(L"ChatApp: Package identity flows initialized (placeholder - features to be implemented).\n");
}

// Helper to validate MSIX package existence and basic properties
bool ValidateMsixPackage(const std::wstring& packagePath)
{
    OutputDebugStringW(L"ChatApp: Validating MSIX package...\n");
    
    // Check if file exists
    DWORD fileAttributes = GetFileAttributesW(packagePath.c_str());
    if (fileAttributes == INVALID_FILE_ATTRIBUTES)
    {
        wchar_t errorLog[512];
        swprintf_s(errorLog, L"ChatApp: MSIX package not found at: %s\n", packagePath.c_str());
        OutputDebugStringW(errorLog);
        return false;
    }
    
    // Check if it's a file (not a directory)
    if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        OutputDebugStringW(L"ChatApp: Package path points to a directory, not a file.\n");
        return false;
    }
    
    // Check file extension
    size_t dotPos = packagePath.find_last_of(L'.');
    if (dotPos == std::wstring::npos)
    {
        OutputDebugStringW(L"ChatApp: Package file has no extension.\n");
        return false;
    }
    
    std::wstring extension = packagePath.substr(dotPos);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
    
    if (extension != L".msix" && extension != L".appx")
    {
        wchar_t extLog[256];
        swprintf_s(extLog, L"ChatApp: Package has unexpected extension: %s\n", extension.c_str());
        OutputDebugStringW(extLog);
        return false;
    }
    
    // Get file size
    HANDLE hFile = CreateFileW(packagePath.c_str(), GENERIC_READ, FILE_SHARE_READ, 
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER fileSize;
        if (GetFileSizeEx(hFile, &fileSize))
        {
            wchar_t sizeLog[256];
            swprintf_s(sizeLog, L"ChatApp: Package size: %lld bytes\n", fileSize.QuadPart);
            OutputDebugStringW(sizeLog);
        }
        CloseHandle(hFile);
    }
    
    OutputDebugStringW(L"ChatApp: MSIX package validation passed.\n");
    return true;
}

// Get current package identity status as a formatted string
std::wstring GetPackageIdentityStatus()
{
    std::wstring status = L"ChatApp Package Identity Status:\n";
    status += L"- Sparse package supported: " + std::wstring(g_isSparsePackageSupported ? L"Yes" : L"No") + L"\n";
    status += L"- Running with identity: " + std::wstring(g_isRunningWithIdentity ? L"Yes" : L"No") + L"\n";
    status += L"- Initialized: " + std::wstring(g_packageIdentityInitialized ? L"Yes" : L"No") + L"\n";
    
    if (g_isRunningWithIdentity)
    {
        // Try to get package full name
        UINT32 length = 0;
        LONG rc = GetCurrentPackageFullName(&length, nullptr);
        if (rc == ERROR_INSUFFICIENT_BUFFER && length > 0)
        {
            std::vector<wchar_t> packageFullName(length);
            rc = GetCurrentPackageFullName(&length, packageFullName.data());
            if (rc == ERROR_SUCCESS)
            {
                status += L"- Package full name: " + std::wstring(packageFullName.data()) + L"\n";
            }
        }
    }
    
    return status;
}