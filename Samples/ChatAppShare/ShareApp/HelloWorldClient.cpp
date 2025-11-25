#include "HelloWorldClient.h"
#include <iostream>
#include <cstring>

const std::string HelloWorldClient::PIPE_NAME = "\\\\.\\pipe\\HelloWorldService";

std::string HelloWorldClient::GetHelloWorld() {
    HANDLE hPipe;
    char buffer[1024];
    DWORD bytesRead, bytesWritten;
    
    // Try to open the named pipe
    while (true) {
        hPipe = CreateFileA(
            PIPE_NAME.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if (hPipe != INVALID_HANDLE_VALUE) {
            break;
        }

        if (GetLastError() != ERROR_PIPE_BUSY) {
            std::cerr << "Could not open pipe. Error: " << GetLastError() << std::endl;
            return "";
        }

        // All pipe instances are busy, wait a bit
        if (!WaitNamedPipeA(PIPE_NAME.c_str(), TIMEOUT_MS)) {
            std::cerr << "Could not open pipe: timeout after " << TIMEOUT_MS << "ms." << std::endl;
            return "";
        }
    }

    // Set pipe to message-read mode
    DWORD dwMode = PIPE_READMODE_MESSAGE;
    if (!SetNamedPipeHandleState(hPipe, &dwMode, nullptr, nullptr)) {
        std::cerr << "SetNamedPipeHandleState failed. Error: " << GetLastError() << std::endl;
        CloseHandle(hPipe);
        return "";
    }

    // Send a request message to the pipe server
    const char* message = "GET_HELLO_WORLD";
    if (!WriteFile(hPipe, message, static_cast<DWORD>(strlen(message)), &bytesWritten, nullptr)) {
        std::cerr << "WriteFile to pipe failed. Error: " << GetLastError() << std::endl;
        CloseHandle(hPipe);
        return "";
    }

    // Read the response from the server
    if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr)) {
        buffer[bytesRead] = '\0';
        CloseHandle(hPipe);
        return std::string(buffer);
    }
    else {
        std::cerr << "ReadFile from pipe failed. Error: " << GetLastError() << std::endl;
        CloseHandle(hPipe);
        return "";
    }
}