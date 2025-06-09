#include "pch.h"

#include <winrt/Windows.Management.Deployment.h>
#include <winrt/Windows.ApplicationModel.Activation.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.ShareTarget.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.h>
#include <windows.h>
#include <VersionHelpers.h>
#include <appmodel.h> // Add this include for GetCurrentPackageFullName
#include <shellapi.h>

using namespace winrt;
using namespace winrt::Windows::Management::Deployment;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::ApplicationModel::DataTransfer::ShareTarget;
using namespace winrt::Windows::ApplicationModel::DataTransfer;
using namespace winrt::Windows::Storage;

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "evr.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "Shell32.lib")

using namespace Microsoft::WRL;

IMFMediaSession* pSession = nullptr;
IMFMediaSource* pSource = nullptr;
IMFActivate* pActivate = nullptr;
IMFVideoDisplayControl* pVideoDisplay = nullptr;
HWND hwndVideo = nullptr;

HRESULT InitializeWebcam(HWND hwnd) 
{
    IMFAttributes* pAttributes = nullptr;
    IMFActivate** ppDevices = nullptr;
    UINT32 count = 0;

    // Create an attribute store to specify the video capture device
    HRESULT hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr)) return hr;

    hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED(hr)) return hr;

    // Enumerate video capture devices
    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    if (FAILED(hr)) return hr;

    if (count == 0) 
    {
        MessageBox(hwnd, L"No webcam found.", L"Error", MB_ICONERROR);
        return E_FAIL;
    }

    // Use the first device
    pActivate = ppDevices[0];
    for (UINT32 i = 1; i < count; i++) 
    {
        ppDevices[i]->Release();
    }
    CoTaskMemFree(ppDevices);
    pAttributes->Release();

    // Create the media source for the webcam
    hr = pActivate->ActivateObject(__uuidof(IMFMediaSource), (void**)&pSource);
    if (FAILED(hr)) return hr;

    // Create a media session
    hr = MFCreateMediaSession(nullptr, &pSession);
    if (FAILED(hr)) return hr;

    // Create a topology for the webcam
    IMFTopology* pTopology = nullptr;
    hr = MFCreateTopology(&pTopology);
    if (FAILED(hr)) return hr;

    IMFPresentationDescriptor* pPD = nullptr;
    hr = pSource->CreatePresentationDescriptor(&pPD);
    if (FAILED(hr)) return hr;

    DWORD streamCount = 0;
    pPD->GetStreamDescriptorCount(&streamCount);

    for (DWORD i = 0; i < streamCount; i++) 
    {
        BOOL selected = FALSE;
        IMFStreamDescriptor* pSD = nullptr;
        hr = pPD->GetStreamDescriptorByIndex(i, &selected, &pSD);
        if (FAILED(hr)) continue;

        if (selected) 
        {
            IMFTopologyNode* pSourceNode = nullptr;
            IMFTopologyNode* pOutputNode = nullptr;

            // Create a source node
            hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &pSourceNode);
            if (FAILED(hr)) continue;

            hr = pSourceNode->SetUnknown(MF_TOPONODE_SOURCE, pSource);
            if (FAILED(hr)) continue;

            hr = pSourceNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, pPD);
            if (FAILED(hr)) continue;

            hr = pSourceNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, pSD);
            if (FAILED(hr)) continue;

            // Create an output node
            hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pOutputNode);
            if (FAILED(hr)) continue;

            IMFActivate* pRendererActivate = nullptr;
            hr = MFCreateVideoRendererActivate(hwnd, &pRendererActivate);
            if (FAILED(hr)) continue;

            hr = pOutputNode->SetObject(pRendererActivate);
            if (FAILED(hr)) continue;

            // Add nodes to the topology
            hr = pTopology->AddNode(pSourceNode);
            if (FAILED(hr)) continue;

            hr = pTopology->AddNode(pOutputNode);
            if (FAILED(hr)) continue;

            hr = pSourceNode->ConnectOutput(0, pOutputNode, 0);
            if (FAILED(hr)) continue;

            pSourceNode->Release();
            pOutputNode->Release();
        }
        pSD->Release();
    }

    hr = pSession->SetTopology(0, pTopology);
    if (FAILED(hr)) return hr;

    pTopology->Release();
    pPD->Release();

    // Start the session
    PROPVARIANT varStart;
    PropVariantInit(&varStart);
    hr = pSession->Start(&GUID_NULL, &varStart);
    PropVariantClear(&varStart);

    return hr;
}

void Cleanup() 
{
    if (pSession) 
    {
        pSession->Close();
        pSession->Release();
    }
    if (pSource) pSource->Release();
    if (pActivate) pActivate->Release();
    MFShutdown();
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
    switch (msg) 
    {
    case WM_DESTROY:
        Cleanup();
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
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
            wchar_t log[256];
            swprintf_s(log, L"Current OS Version: %u.%u.%u\n", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
            OutputDebugStringW(log);
        }
    }

    // Log the required version
    OutputDebugStringW(L"Required minimum version: 10.0.19041\n");

    // Compare with required version
    if (osvi.dwMajorVersion > 10 ||
        (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion > 0) ||
        (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0 && osvi.dwBuildNumber >= 19041))
    {
        OutputDebugStringW(L"Sparse package is supported on this OS.\n");
        return true;
    }
    OutputDebugStringW(L"Sparse package is NOT supported on this OS.\n");
    return false;
}

HRESULT RegisterPackageWithExternalLocation(const std::wstring& externalLocation, const std::wstring& packagePath)
{ 
    winrt::Windows::Management::Deployment::PackageManager packageManager;  
    winrt::Windows::Management::Deployment::AddPackageOptions addOptions;  
    addOptions.ExternalLocationUri(winrt::Windows::Foundation::Uri(winrt::hstring(externalLocation)));
    auto result = packageManager.AddPackageByUriAsync(winrt::Windows::Foundation::Uri(winrt::hstring(packagePath)), addOptions).get();

    return static_cast<HRESULT>(result.ExtendedErrorCode().value);
}

// Returns true if the app is running with package identity
bool IsRunningWithIdentity()
{
    UINT32 length = 0;
    LONG rc = GetCurrentPackageFullName(&length, nullptr);

    // Existing code remains unchanged
    if (rc == ERROR_INSUFFICIENT_BUFFER)
    {
        std::wstring packageFullName(length, L'\0');
        rc = GetCurrentPackageFullName(&length, packageFullName.data());
        return rc == ERROR_SUCCESS;
    }
    return false;
}

// Relaunches the current executable
void RelaunchApplication()
{
    wchar_t exePath[MAX_PATH] = {0};
    // Ensure the buffer is zero-initialized and check for errors
    DWORD len = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    if (len == 0 || len == MAX_PATH)
        return; // Failed to get path or path too long

    // Use ShellExecuteW to relaunch the current executable
    HINSTANCE result = ShellExecuteW(nullptr, L"open", exePath, nullptr, nullptr, SW_SHOWNORMAL);
    if ((INT_PTR)result <= 32)
    {
        // Optionally log or handle the error here
        OutputDebugStringW(L"Failed to relaunch application.\n");
    }
}

// Add this function to handle share activation
winrt::Windows::Foundation::IAsyncAction HandleShareTarget(winrt::Windows::Foundation::IInspectable const& args)
{
    auto shareArgs = args.as<IShareTargetActivatedEventArgs>();
    ShareOperation shareOperation = shareArgs.ShareOperation();

    if (shareOperation.Data().Contains(StandardDataFormats::Bitmap()))
    {
        auto bitmapRef = co_await shareOperation.Data().GetBitmapAsync();
        // TODO: Convert bitmapRef to a format you can display in your Win32 window
        MessageBox(nullptr, L"Received a bitmap via Share Target.", L"Share Target", MB_OK);
        shareOperation.ReportCompleted();
    }
    else if (shareOperation.Data().Contains(StandardDataFormats::StorageItems()))
    {
        winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Storage::IStorageItem> items = co_await shareOperation.Data().GetStorageItemsAsync();
        for (auto const& item : items)
        {
            if (auto file = item.try_as<winrt::Windows::Storage::StorageFile>())
            {
                std::wstring msg = L"Received file: " + std::wstring(file.Name().c_str());
                MessageBox(nullptr, msg.c_str(), L"Share Target", MB_OK);
            }
        }
        shareOperation.ReportCompleted();
    }
    else
    {
        MessageBox(nullptr, L"No supported image format received.", L"Share Target", MB_OK);
        shareOperation.ReportCompleted();
    }
}

// Helper to get the directory of the current executable (build output path)
std::wstring GetExecutableDirectory()
{
    wchar_t exePath[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    if (len == 0 || len == MAX_PATH)
        return L"";
    std::wstring path(exePath);
    size_t pos = path.find_last_of(L"\\/");
    if (pos != std::wstring::npos)
        path = path.substr(0, pos);
    return path;
}

int RunWebcamAppMainLoop(HINSTANCE hInstance, int nCmdShow)
{
    MFStartup(MF_VERSION);

    // Register window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"WebcamApp";

    RegisterClass(&wc);

    // Create window
    hwndVideo = CreateWindowEx(0, wc.lpszClassName, L"Webcam Viewer", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, hInstance, nullptr);

    if (!hwndVideo) return -1;

    ShowWindow(hwndVideo, nCmdShow);

    // Initialize webcam
    if (FAILED(InitializeWebcam(hwndVideo)))
    {
        MessageBox(nullptr, L"Failed to initialize webcam. Please check if app is running with identity. If not, turn on camera usage for desktop app in Settings and relaunch.", L"Error", MB_ICONERROR);
        return -1;
    }

    // Main message loop
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) 
{
    if (!IsSparsePackageSupported())
    {
        OutputDebugStringW(L"Sparse package is not supported. Application is not running with package identity.\n");
        return RunWebcamAppMainLoop(hInstance, nCmdShow);
    }

    winrt::init_apartment();

    // Check if running with identity
    if (!IsRunningWithIdentity())
    {
        // Given that for this installer we are not creating any installer, externalLocation is derived from the build output directory
        // Replace this with Existing unpackaged App Install root location here.
        std::wstring externalLocation = GetExecutableDirectory();

        // Since your Sparse package(Package with external location) .msix file will be located in the existing unpackaged app install root location, 
        // use the same externalLocation to get .msix.
        // For this sample to work, you would need to create .msix((Package with external location)) at same location as the build executable.
        std::wstring packagePath = externalLocation + L"\\PackageWithExternalLocationCppSample_1.0.0.0_x86__8h66172c634n0.msix";

        HRESULT hr = RegisterPackageWithExternalLocation(externalLocation, packagePath);
        if (SUCCEEDED(hr))
        {
            RelaunchApplication();
            return 0;
        }
        else
        {
            OutputDebugStringW(L"Application is not running with package identity.\n");
            return RunWebcamAppMainLoop(hInstance, nCmdShow);
        }
    }

    auto activationArgs = winrt::Windows::ApplicationModel::AppInstance::GetActivatedEventArgs();
    if (activationArgs)
    {
        if (activationArgs.Kind() == winrt::Windows::ApplicationModel::Activation::ActivationKind::ShareTarget)
        {
            HandleShareTarget(activationArgs).get(); // Wait for coroutine to finish
            return 0;
        }
    }
    return RunWebcamAppMainLoop(hInstance, nCmdShow);
}

void RemovePackageWithExternalLocation() // example of how to uninstall a package with external location
{
    winrt::Windows::Management::Deployment::PackageManager packageManager;
    auto deploymentOperation{ packageManager.RemovePackageAsync(L"PackageWithExternalLocationCppSample_1.0.0.0_neutral__h91ms92gdsmmt") };
    deploymentOperation.get();
}
