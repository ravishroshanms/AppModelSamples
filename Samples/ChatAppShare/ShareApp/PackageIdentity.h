#pragma once

#include <windows.h>
#include <string>

// Package Identity Variables
extern bool g_isSparsePackageSupported;
extern bool g_isRunningWithIdentity;
extern bool g_packageIdentityInitialized;

// Package Identity Management functions
bool IsSparsePackageSupported();
bool IsRunningWithIdentity();
bool InitializePackageIdentity();
void InitializePackageIdentityFlow();
std::wstring GetExecutableDirectory();
HRESULT RegisterPackageWithExternalLocation(const std::wstring& externalLocation, const std::wstring& packagePath);
HRESULT RegisterPackageWithExternalLocationPowerShell(const std::wstring& externalLocation, const std::wstring& packagePath);
void RelaunchApplication();

// Helper functions
bool ValidateMsixPackage(const std::wstring& packagePath);
std::wstring GetPackageIdentityStatus();