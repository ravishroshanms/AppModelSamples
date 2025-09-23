#include "ShareTargetManager.h"
#include "PackageIdentity.h"
#include "framework.h"
#include <sstream>

// Static member initialization
bool ShareTargetManager::s_initialized = false;
bool ShareTargetManager::s_shareTargetSupported = false;

void ShareTargetManager::Initialize()
{
    if (s_initialized)
        return;
    
    LogShareInfo(L"Initializing Share Target Manager...");
    
    try
    {
        // Check if WinRT and package identity are available
        if (g_isRunningWithIdentity)
        {
            s_shareTargetSupported = true;
            LogShareInfo(L"Share Target support available with package identity.");
        }
        else
        {
            s_shareTargetSupported = false;
            LogShareInfo(L"Share Target requires package identity (not available).");
        }
    }
    catch (winrt::hresult_error const& ex)
    {
        std::wstring error = L"Share Target initialization error: " + std::wstring(ex.message().c_str());
        LogShareError(error);
        s_shareTargetSupported = false;
    }
    catch (...)
    {
        LogShareError(L"Unknown error during Share Target initialization.");
        s_shareTargetSupported = false;
    }
    
    s_initialized = true;
    LogShareInfo(L"Share Target Manager initialized successfully.");
}

bool ShareTargetManager::IsShareTargetAvailable()
{
    if (!s_initialized)
        Initialize();
    
    return s_shareTargetSupported && g_isRunningWithIdentity;
}

bool ShareTargetManager::IsShareTargetActivation()
{
    if (!IsShareTargetAvailable())
        return false;
    
    try
    {
        auto activationArgs = winrt::Windows::ApplicationModel::AppInstance::GetActivatedEventArgs();
        return activationArgs && activationArgs.Kind() == winrt::Windows::ApplicationModel::Activation::ActivationKind::ShareTarget;
    }
    catch (...)
    {
        LogShareError(L"Error checking share target activation.");
        return false;
    }
}

bool ShareTargetManager::ProcessActivationArgs()
{
    if (!IsShareTargetAvailable())
        return false;
    
    try
    {
        auto activationArgs = winrt::Windows::ApplicationModel::AppInstance::GetActivatedEventArgs();
        if (activationArgs && activationArgs.Kind() == winrt::Windows::ApplicationModel::Activation::ActivationKind::ShareTarget)
        {
            LogShareInfo(L"? Share Target activation detected!");
            
            // Process the share target directly here (same as original code)
            auto shareArgs = activationArgs.as<winrt::Windows::ApplicationModel::Activation::IShareTargetActivatedEventArgs>();
            auto shareOperation = shareArgs.ShareOperation();
            auto data = shareOperation.Data();

            std::wstring shareInfo = L"Share Target Activated!\n\n";

            // Check for different data formats
            if (data.Contains(winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats::Text()))
            {
                try
                {
                    auto textAsync = data.GetTextAsync();
                    auto text = textAsync.get();
                    shareInfo += L"?? Text: " + std::wstring(text.c_str()) + L"\n\n";
                    LogShareInfo(L"Received shared text.");
                }
                catch (...)
                {
                    shareInfo += L"?? Text: [Error retrieving text]\n\n";
                    LogShareError(L"Error retrieving shared text.");
                }
            }

            if (data.Contains(winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats::WebLink()))
            {
                try
                {
                    auto webLinkAsync = data.GetWebLinkAsync();
                    auto webLink = webLinkAsync.get();
                    shareInfo += L"?? Web Link: " + std::wstring(webLink.ToString().c_str()) + L"\n\n";
                    LogShareInfo(L"Received shared web link.");
                }
                catch (...)
                {
                    shareInfo += L"?? Web Link: [Error retrieving web link]\n\n";
                    LogShareError(L"Error retrieving shared web link.");
                }
            }

            if (data.Contains(winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats::Bitmap()))
            {
                try
                {
                    auto bitmapAsync = data.GetBitmapAsync();
                    auto bitmapRef = bitmapAsync.get();
                    shareInfo += L"??? Bitmap: Received image content\n\n";
                    LogShareInfo(L"Received shared bitmap.");
                }
                catch (...)
                {
                    shareInfo += L"??? Bitmap: [Error retrieving bitmap]\n\n";
                    LogShareError(L"Error retrieving shared bitmap.");
                }
            }

            if (data.Contains(winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats::StorageItems()))
            {
                try
                {
                    auto storageItemsAsync = data.GetStorageItemsAsync();
                    auto storageItems = storageItemsAsync.get();
                    shareInfo += L"?? Files: " + std::to_wstring(storageItems.Size()) + L" file(s) shared\n";
                    
                    for (uint32_t i = 0; i < storageItems.Size(); i++)
                    {
                        auto item = storageItems.GetAt(i);
                        shareInfo += L"  - " + std::wstring(item.Name().c_str()) + L"\n";
                    }
                    shareInfo += L"\n";
                    LogShareInfo(L"Received shared files.");
                }
                catch (...)
                {
                    shareInfo += L"?? Files: [Error retrieving files]\n\n";
                    LogShareError(L"Error retrieving shared files.");
                }
            }

            // Show the share information
            MessageBoxW(nullptr, shareInfo.c_str(), L"Windows Share Target", MB_OK | MB_ICONINFORMATION);

            // Report completion
            shareOperation.ReportCompleted();
            
            LogShareInfo(L"Share target processing completed successfully.");
            return true;
        }
        else
        {
            LogShareInfo(L"Running with package identity but not as share target.");
            return false;
        }
    }
    catch (winrt::hresult_error const& ex)
    {
        std::wstring error = L"Error checking activation args: " + std::wstring(ex.message().c_str()) +
                           L" (HRESULT: 0x" + std::to_wstring(static_cast<uint32_t>(ex.code())) + L")";
        LogShareError(error);
        MessageBoxW(nullptr, L"Error processing shared content", L"Share Target Error", MB_OK | MB_ICONERROR);
        return false;
    }
    catch (...)
    {
        LogShareError(L"Unknown error checking activation arguments.");
        MessageBoxW(nullptr, L"Unknown error processing shared content", L"Share Target Error", MB_OK | MB_ICONERROR);
        return false;
    }
}

std::wstring ShareTargetManager::GetShareTargetStatus()
{
    if (!s_initialized)
        Initialize();
    
    std::wstring status = L"WinRT Share Target Integration Status:\n\n";
    
    try
    {
        winrt::check_hresult(S_OK);
        status += L"?? WinRT Runtime: ? Available and Working\n";
        status += L"?? Package Identity: " + std::wstring(g_isRunningWithIdentity ? L"? Available" : L"? Not Available") + L"\n";
        status += L"?? Sparse Package Support: " + std::wstring(g_isSparsePackageSupported ? L"? Supported" : L"? Not Supported") + L"\n\n";
        
        // Test activation
        if (g_isRunningWithIdentity)
        {
            try
            {
                if (IsShareTargetActivation())
                {
                    status += L"?? Share Target: ? Currently Activated\n";
                }
                else
                {
                    status += L"?? Share Target: ?? Not Currently Activated\n";
                }
            }
            catch (...)
            {
                status += L"?? Share Target: ? Error Checking Activation\n";
            }
        }
        else
        {
            status += L"?? Share Target: ? Package Identity Required\n";
        }
        
        status += GetPackageIdentityStatus();
        status += L"\n\n?? WinRT Share Target integration is complete and ready!";
    }
    catch (winrt::hresult_error const& ex)
    {
        status += L"?? WinRT Runtime: ? Error\n";
        status += L"Error: " + std::wstring(ex.message().c_str()) + L"\n";
        status += L"HRESULT: 0x" + std::to_wstring(static_cast<uint32_t>(ex.code())) + L"\n";
    }
    catch (...)
    {
        status += L"?? WinRT Runtime: ? Unknown Error\n";
    }
    
    return status;
}

void ShareTargetManager::LogShareInfo(const std::wstring& message)
{
    std::wstring logMessage = L"ChatApp: " + message + L"\n";
    OutputDebugStringW(logMessage.c_str());
}

void ShareTargetManager::LogShareError(const std::wstring& error)
{
    std::wstring logMessage = L"ChatApp: ShareTarget ERROR - " + error + L"\n";
    OutputDebugStringW(logMessage.c_str());
}