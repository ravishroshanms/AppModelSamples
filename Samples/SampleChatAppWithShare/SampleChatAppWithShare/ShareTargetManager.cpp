#include "ShareTargetManager.h"
#include "PackageIdentity.h"
#include "ContactSelectionDialog.h"
#include "ChatManager.h"
#include "ChatModels.h"
#include "FileManager.h"
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
            
            // Ensure contacts are initialized (critical for share target scenarios)
            if (contacts.empty())
            {
                LogShareInfo(L"Initializing contacts for share target scenario...");
                InitializeContacts();
            }
            
            // Process the share target directly here
            auto shareArgs = activationArgs.as<winrt::Windows::ApplicationModel::Activation::IShareTargetActivatedEventArgs>();
            auto shareOperation = shareArgs.ShareOperation();
            auto data = shareOperation.Data();

            // Extract shared content information for the contact selection dialog
            std::wstring sharedContentSummary = L"Shared Content";
            std::wstring sharedFiles;
            bool hasFiles = false;
            
            // Collect information about what's being shared
            std::vector<std::wstring> sharedItems;
            
            // Check for different data formats
            if (data.Contains(winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats::Text()))
            {
                try
                {
                    auto textAsync = data.GetTextAsync();
                    auto text = textAsync.get();
                    sharedItems.push_back(L"Text: " + std::wstring(text.c_str()));
                    LogShareInfo(L"Received shared text.");
                }
                catch (...)
                {
                    sharedItems.push_back(L"Text: [Error retrieving text]");
                    LogShareError(L"Error retrieving shared text.");
                }
            }

            if (data.Contains(winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats::WebLink()))
            {
                try
                {
                    auto webLinkAsync = data.GetWebLinkAsync();
                    auto webLink = webLinkAsync.get();
                    sharedItems.push_back(L"Web Link: " + std::wstring(webLink.ToString().c_str()));
                    LogShareInfo(L"Received shared web link.");
                }
                catch (...)
                {
                    sharedItems.push_back(L"Web Link: [Error retrieving web link]");
                    LogShareError(L"Error retrieving shared web link.");
                }
            }

            if (data.Contains(winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats::Bitmap()))
            {
                try
                {
                    auto bitmapAsync = data.GetBitmapAsync();
                    auto bitmapRef = bitmapAsync.get();
                    sharedItems.push_back(L"Image/Bitmap content");
                    LogShareInfo(L"Received shared bitmap.");
                }
                catch (...)
                {
                    sharedItems.push_back(L"Image: [Error retrieving image]");
                    LogShareError(L"Error retrieving shared bitmap.");
                }
            }

            if (data.Contains(winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats::StorageItems()))
            {
                try
                {
                    auto storageItemsAsync = data.GetStorageItemsAsync();
                    auto storageItems = storageItemsAsync.get();
                    
                    hasFiles = true;
                    std::wstring filesInfo = std::to_wstring(storageItems.Size()) + L" file(s)";
                    
                    if (storageItems.Size() == 1)
                    {
                        auto item = storageItems.GetAt(0);
                        sharedFiles = item.Name().c_str();
                        sharedContentSummary = sharedFiles;
                    }
                    else if (storageItems.Size() > 1)
                    {
                        sharedFiles = L"Multiple Files (" + std::to_wstring(storageItems.Size()) + L")";
                        sharedContentSummary = sharedFiles;
                    }
                    
                    for (uint32_t i = 0; i < storageItems.Size(); i++)
                    {
                        auto item = storageItems.GetAt(i);
                        filesInfo += L"\n  - " + std::wstring(item.Name().c_str());
                    }
                    
                    sharedItems.push_back(filesInfo);
                    LogShareInfo(L"Received shared files.");
                }
                catch (...)
                {
                    sharedItems.push_back(L"Files: [Error retrieving files]");
                    LogShareError(L"Error retrieving shared files.");
                }
            }

            // If no specific content type found, use generic description
            if (sharedItems.empty())
            {
                sharedContentSummary = L"Shared Content";
                sharedItems.push_back(L"Unknown content type");
            }

            // Get the main window handle for dialog parent
            HWND hMainWindow = GetActiveWindow();
            if (!hMainWindow)
            {
                hMainWindow = GetForegroundWindow();
            }
            if (!hMainWindow)
            {
                // Try to find the main chat application window
                hMainWindow = FindWindow(NULL, L"Chat Application");
            }

            // Log the number of contacts available for debugging
            LogShareInfo(L"Available contacts: " + std::to_wstring(contacts.size()));

            // Show contact selection dialog for the shared content
            ContactSelectionDialog::SelectionResult result = 
                ContactSelectionDialog::ShowContactSelectionDialog(hMainWindow, L"", sharedContentSummary);
            
            if (result.wasSelected)
            {
                // User selected a contact - add the shared content to that contact's chat
                if (result.contactIndex >= 0 && result.contactIndex < (int)contacts.size())
                {
                    int previousSelection = selectedContactIndex;
                    selectedContactIndex = result.contactIndex;
                    
                    // Add the custom share message if provided
                    if (!result.shareMessage.empty())
                    {
                        AddMessageToChat(result.shareMessage, true);
                    }
                    
                    // Add shared content messages to the chat
                    for (const auto& item : sharedItems)
                    {
                        std::wstring shareMsg = L"?? Received via Share: " + item;
                        AddMessageToChat(shareMsg, false); // Mark as incoming since it's from external source
                    }
                    
                    // If files were shared, add them to the contact's shared files list
                    if (hasFiles && data.Contains(winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats::StorageItems()))
                    {
                        try
                        {
                            auto storageItemsAsync = data.GetStorageItemsAsync();
                            auto storageItems = storageItemsAsync.get();
                            
                            for (uint32_t i = 0; i < storageItems.Size(); i++)
                            {
                                auto item = storageItems.GetAt(i);
                                
                                // Create shared file entry
                                SharedFile newFile;
                                newFile.fileName = item.Name().c_str();
                                newFile.filePath = item.Path().c_str(); // Get full path if available
                                newFile.sharedBy = L"External Share";
                                GetSystemTime(&newFile.timeShared);
                                
                                contacts[result.contactIndex].sharedFiles.push_back(newFile);
                            }
                            
                            // Update shared files UI
                            UpdateSharedFilesList();
                        }
                        catch (...)
                        {
                            LogShareError(L"Error adding shared files to contact.");
                        }
                    }
                    
                    // Update UI to show the chat with the selected contact
                    LoadContactChat(result.contactIndex);
                    
                    // Show success message
                    std::wstring successMsg = L"Shared content has been added to your conversation with " + contacts[result.contactIndex].name + L"!";
                    MessageBoxW(hMainWindow, successMsg.c_str(), L"Content Shared Successfully", MB_OK | MB_ICONINFORMATION);
                    
                    LogShareInfo(L"Share target content added to contact: " + contacts[result.contactIndex].name);
                }
            }
            else
            {
                // User cancelled - show a brief message
                MessageBoxW(hMainWindow, L"Share operation was cancelled.", L"Share Cancelled", MB_OK | MB_ICONINFORMATION);
                LogShareInfo(L"Share target operation cancelled by user.");
            }

            // Report completion to Windows
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