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
    
    // Add a static flag to prevent multiple processing
    static bool s_alreadyProcessed = false;
    if (s_alreadyProcessed) {
        LogShareInfo(L"Share target already processed - skipping duplicate call");
        return false;
    }
    
    try
    {
        auto activationArgs = winrt::Windows::ApplicationModel::AppInstance::GetActivatedEventArgs();
        if (activationArgs && activationArgs.Kind() == winrt::Windows::ApplicationModel::Activation::ActivationKind::ShareTarget)
        {
            LogShareInfo(L"? Share Target activation detected!");
            s_alreadyProcessed = true; // Mark as processed
            
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
                    
                    LogShareInfo(L"Setting selectedContactIndex to: " + std::to_wstring(result.contactIndex));
                    LogShareInfo(L"Selected contact name: " + contacts[result.contactIndex].name);
                    
                    // Add messages directly to the selected contact instead of relying on selectedContactIndex
                    Contact& selectedContact = contacts[result.contactIndex];
                    
                    // Add the custom share message if provided
                    if (!result.shareMessage.empty())
                    {
                        std::wstring formattedShareMessage = L"You: " + result.shareMessage + L" ??";
                        selectedContact.messages.push_back(formattedShareMessage);
                        LogShareInfo(L"Added share message: " + result.shareMessage);
                    }
                    
                    // Add shared content messages to the chat
                    for (const auto& item : sharedItems)
                    {
                        std::wstring shareMsg = L"?? Received via Share: " + item;
                        std::wstring formattedMessage = selectedContact.name + L": " + shareMsg;
                        selectedContact.messages.push_back(formattedMessage);
                        LogShareInfo(L"Added shared content message: " + shareMsg);
                    }
                    
                    // Update the last message preview for this contact
                    if (!sharedItems.empty())
                    {
                        std::wstring lastMsg = L"?? Received via Share: " + sharedItems[0];
                        selectedContact.lastMessage = lastMsg.length() > 50 ? lastMsg.substr(0, 47) + L"..." : lastMsg;
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
                                
                                selectedContact.sharedFiles.push_back(newFile);
                                LogShareInfo(L"Added shared file: " + newFile.fileName);
                            }
                        }
                        catch (...)
                        {
                            LogShareError(L"Error adding shared files to contact.");
                        }
                    }
                    
                    // Show success message and exit the application
                    std::wstring successMsg = L"Content has been shared successfully with " + contacts[result.contactIndex].name + L"!\n\nThe application will now close.";
                    MessageBoxW(hMainWindow, successMsg.c_str(), L"Sharing Complete", MB_OK | MB_ICONINFORMATION);
                    
                    LogShareInfo(L"Share target content added to contact: " + contacts[result.contactIndex].name);
                    LogShareInfo(L"Selected contact index set to: " + std::to_wstring(result.contactIndex));
                    LogShareInfo(L"Contact now has " + std::to_wstring(selectedContact.messages.size()) + L" messages");
                    
                    // Report completion to Windows
                    shareOperation.ReportCompleted();
                    
                    LogShareInfo(L"Share target processing completed successfully. Exiting application.");
                    
                    // Exit the application after successful sharing
                    PostQuitMessage(0);
                    return true;
                }
            }
            else
            {
                // User cancelled - show a brief message and exit
                MessageBoxW(hMainWindow, L"Share operation was cancelled.\n\nThe application will now close.", L"Share Cancelled", MB_OK | MB_ICONINFORMATION);
                LogShareInfo(L"Share target operation cancelled by user.");
                
                // Report completion to Windows
                shareOperation.ReportCompleted();
                
                LogShareInfo(L"Share target processing cancelled. Exiting application.");
                
                // Exit the application after cancellation
                PostQuitMessage(0);
                return true;
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
        s_alreadyProcessed = true; // Mark as processed even on error to prevent retries
        std::wstring error = L"Error checking activation args: " + std::wstring(ex.message().c_str()) +
                           L" (HRESULT: 0x" + std::to_wstring(static_cast<uint32_t>(ex.code())) + L")";
        LogShareError(error);
        MessageBoxW(nullptr, L"Error processing shared content", L"Share Target Error", MB_OK | MB_ICONERROR);
        return false;
    }
    catch (...)
    {
        s_alreadyProcessed = true; // Mark as processed even on error to prevent retries
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