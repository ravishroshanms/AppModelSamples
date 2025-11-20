#include "ContactSelectionDialog.h"
#include "Resource.h"
#include "UIConstants.h"
#include <commctrl.h>

// Static member initialization
ContactSelectionDialog::SelectionResult ContactSelectionDialog::s_dialogResult;
std::wstring ContactSelectionDialog::s_currentFilePath;
std::wstring ContactSelectionDialog::s_currentFileName;

ContactSelectionDialog::SelectionResult ContactSelectionDialog::ShowContactSelectionDialog(HWND hParent, const std::wstring& filePath, const std::wstring& fileName)
{
    // Store the file information for the dialog
    s_currentFilePath = filePath;
    s_currentFileName = fileName;
    
    // Reset the result
    s_dialogResult = SelectionResult();
    
    // Show the modal dialog
    INT_PTR result = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CONTACT_SELECTION), hParent, ContactSelectionDlgProc);
    
    if (result == IDOK)
    {
        s_dialogResult.wasSelected = true;
        s_dialogResult.filePath = s_currentFilePath;
        s_dialogResult.fileName = s_currentFileName;
    }
    else
    {
        s_dialogResult.wasSelected = false;
    }
    
    return s_dialogResult;
}

INT_PTR CALLBACK ContactSelectionDialog::ContactSelectionDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            // Set the dialog title with file name
            std::wstring title = L"Share \"" + s_currentFileName + L"\" - Select Contact";
            SetWindowText(hDlg, title.c_str());
            
            // Set up the dialog layout and styling
            SetupDialogSizing(hDlg);
            
            // Initialize and populate the contact list
            HWND hListBox = GetDlgItem(hDlg, IDC_CONTACT_SELECTION_LIST);
            if (hListBox)
            {
                InitializeContactList(hListBox);
                PopulateContactList(hListBox);
            }
            
            // Set up the share message edit control
            HWND hMessageEdit = GetDlgItem(hDlg, IDC_SHARE_MESSAGE_EDIT);
            if (hMessageEdit)
            {
                std::wstring defaultMessage = L"I'm sharing \"" + s_currentFileName + L"\" with you!";
                SetWindowText(hMessageEdit, defaultMessage.c_str());
            }
            
            // Initially disable the Select button until a contact is chosen
            EnableWindow(GetDlgItem(hDlg, IDC_SELECT_CONTACT_BUTTON), FALSE);
            
            // Center the dialog on the parent
            RECT rcParent, rcDlg;
            HWND hParent = GetParent(hDlg);
            if (hParent)
            {
                GetWindowRect(hParent, &rcParent);
                GetWindowRect(hDlg, &rcDlg);
                
                int x = rcParent.left + (rcParent.right - rcParent.left - (rcDlg.right - rcDlg.left)) / 2;
                int y = rcParent.top + (rcParent.bottom - rcParent.top - (rcDlg.bottom - rcDlg.top)) / 2;
                
                SetWindowPos(hDlg, HWND_TOP, x, y, 0, 0, SWP_NOSIZE);
            }
            
            return TRUE;
        }
        
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_CONTACT_SELECTION_LIST:
            if (HIWORD(wParam) == LBN_SELCHANGE)
            {
                int selectedIndex = (int)SendDlgItemMessage(hDlg, IDC_CONTACT_SELECTION_LIST, LB_GETCURSEL, 0, 0);
                HandleContactSelection(hDlg, selectedIndex);
            }
            else if (HIWORD(wParam) == LBN_DBLCLK)
            {
                // Double-click selects and closes dialog
                OnSelectContact(hDlg);
            }
            break;
            
        case IDC_SELECT_CONTACT_BUTTON:
            OnSelectContact(hDlg);
            break;
            
        case IDC_CANCEL_SELECTION_BUTTON:
        case IDCANCEL:
            OnCancel(hDlg);
            break;
            
        case IDOK:
            OnSelectContact(hDlg);
            break;
        }
        break;
        
    case WM_CLOSE:
        OnCancel(hDlg);
        break;
    }
    
    return FALSE;
}

void ContactSelectionDialog::InitializeContactList(HWND hListBox)
{
    // Set up the list box for contact display
    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
}

void ContactSelectionDialog::PopulateContactList(HWND hListBox)
{
    // Clear existing items
    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
    
    // Ensure contacts are initialized
    if (contacts.empty())
    {
        InitializeContacts();
    }
    
    // Debug logging
    OutputDebugStringW((L"ContactSelectionDialog: Populating list with " + std::to_wstring(contacts.size()) + L" contacts\n").c_str());
    
    // Add all contacts to the list
    for (size_t i = 0; i < contacts.size(); ++i)
    {
        const Contact& contact = contacts[i];
        
        // Create display text with contact name and status
        std::wstring displayText = contact.name + L" - " + contact.status;
        if (!contact.isOnline)
        {
            displayText += L" (Offline)";
        }
        
        int itemIndex = (int)SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)displayText.c_str());
        
        // Store the contact index as item data
        SendMessage(hListBox, LB_SETITEMDATA, itemIndex, (LPARAM)i);
        
        // Debug log each contact being added
        OutputDebugStringW((L"ContactSelectionDialog: Added contact " + std::to_wstring(i) + L": " + contact.name + L"\n").c_str());
    }
    
    // If no contacts were added, add a placeholder
    if (contacts.empty())
    {
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"No contacts available");
        OutputDebugStringW(L"ContactSelectionDialog: No contacts available - added placeholder\n");
    }
    else
    {
        OutputDebugStringW((L"ContactSelectionDialog: Successfully populated " + std::to_wstring(contacts.size()) + L" contacts\n").c_str());
    }
}

void ContactSelectionDialog::HandleContactSelection(HWND hDlg, int selectedIndex)
{
    if (selectedIndex != LB_ERR)
    {
        // Enable the Select button when a contact is selected
        EnableWindow(GetDlgItem(hDlg, IDC_SELECT_CONTACT_BUTTON), TRUE);
        
        // Get the contact index from item data
        HWND hListBox = GetDlgItem(hDlg, IDC_CONTACT_SELECTION_LIST);
        int contactIndex = (int)SendMessage(hListBox, LB_GETITEMDATA, selectedIndex, 0);
        
        if (contactIndex >= 0 && contactIndex < (int)contacts.size())
        {
            // Update the share message with the selected contact's name
            const Contact& contact = contacts[contactIndex];
            std::wstring personalizedMessage = L"Hey " + contact.name + L"! I'm sharing \"" + s_currentFileName + L"\" with you.";
            
            HWND hMessageEdit = GetDlgItem(hDlg, IDC_SHARE_MESSAGE_EDIT);
            if (hMessageEdit)
            {
                SetWindowText(hMessageEdit, personalizedMessage.c_str());
            }
        }
    }
    else
    {
        // Disable the Select button when no contact is selected
        EnableWindow(GetDlgItem(hDlg, IDC_SELECT_CONTACT_BUTTON), FALSE);
    }
}

void ContactSelectionDialog::OnSelectContact(HWND hDlg)
{
    HWND hListBox = GetDlgItem(hDlg, IDC_CONTACT_SELECTION_LIST);
    int selectedIndex = (int)SendMessage(hListBox, LB_GETCURSEL, 0, 0);
    
    if (selectedIndex == LB_ERR)
    {
        MessageBox(hDlg, L"Please select a contact to share with.", L"No Contact Selected", MB_OK | MB_ICONWARNING);
        return;
    }
    
    // Check if contacts are available
    if (contacts.empty())
    {
        MessageBox(hDlg, L"No contacts are available. Please ensure the application is properly initialized.", L"No Contacts Available", MB_OK | MB_ICONWARNING);
        return;
    }
    
    // Get the contact index from item data
    int contactIndex = (int)SendMessage(hListBox, LB_GETITEMDATA, selectedIndex, 0);
    
    // Debug logging
    OutputDebugStringW((L"ContactSelectionDialog: Selected contact index: " + std::to_wstring(contactIndex) + L", total contacts: " + std::to_wstring(contacts.size()) + L"\n").c_str());
    
    if (contactIndex >= 0 && contactIndex < (int)contacts.size())
    {
        // Get the share message
        HWND hMessageEdit = GetDlgItem(hDlg, IDC_SHARE_MESSAGE_EDIT);
        WCHAR messageBuffer[512] = {0};
        if (hMessageEdit)
        {
            GetWindowText(hMessageEdit, messageBuffer, 512);
        }
        
        // Store the result
        s_dialogResult.contactIndex = contactIndex;
        s_dialogResult.shareMessage = messageBuffer;
        
        // Debug logging
        OutputDebugStringW((L"ContactSelectionDialog: Contact selected - " + contacts[contactIndex].name + L"\n").c_str());
        
        // Close dialog with success
        EndDialog(hDlg, IDOK);
    }
    else
    {
        MessageBox(hDlg, L"Invalid contact selection. Please try again.", L"Selection Error", MB_OK | MB_ICONERROR);
        OutputDebugStringW((L"ContactSelectionDialog: Invalid contact index: " + std::to_wstring(contactIndex) + L"\n").c_str());
    }
}

void ContactSelectionDialog::OnCancel(HWND hDlg)
{
    // Close dialog with cancel
    EndDialog(hDlg, IDCANCEL);
}

void ContactSelectionDialog::SetupDialogSizing(HWND hDlg)
{
    // Set dialog size (approximately 400x500 pixels)
    SetWindowPos(hDlg, NULL, 0, 0, 420, 520, SWP_NOMOVE | SWP_NOZORDER);
}

void ContactSelectionDialog::ApplyModernStyling(HWND hDlg)
{
    // Modern styling is optional for now - skip to avoid dependencies
}