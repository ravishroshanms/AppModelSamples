#include "BackgroundService.h"
#include <iostream>

// Global constants definition
const std::string PIPE_NAME = "\\\\.\\pipe\\HelloWorldService";
const std::string HELLO_WORLD_RESPONSE = "Hello world";

HelloWorldService::HelloWorldService() : m_running(false) {}

HelloWorldService::~HelloWorldService() {
    Stop();
}

bool HelloWorldService::Start() {
    if (m_running) {
        return false;
    }

    m_running = true;
    m_serviceThread = std::thread(&HelloWorldService::ServiceLoop, this);

    std::cout << "Hello World Service started. Listening on pipe: " << PIPE_NAME << std::endl;
    return true;
}

void HelloWorldService::Stop() {
    if (m_running) {
        m_running = false;
        if (m_serviceThread.joinable()) {
            m_serviceThread.join();
        }
        std::cout << "Hello World Service stopped." << std::endl;
    }
}

bool HelloWorldService::IsRunning() const {
    return m_running;
}

void HelloWorldService::ServiceLoop() {
    while (m_running) {
        // Create named pipe
        HANDLE hPipe = CreateNamedPipeA(
            PIPE_NAME.c_str(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1, // Max instances
            1024, // Output buffer size
            1024, // Input buffer size
            0, // Default timeout
            nullptr // Security attributes
        );

        if (hPipe == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to create named pipe. Error: " << GetLastError() << std::endl;
            continue;
        }

        // Wait for client connection
        std::cout << "Waiting for client connection..." << std::endl;

        if (ConnectNamedPipe(hPipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED) {
            std::cout << "Client connected!" << std::endl;
            HandleClient(hPipe);
        }
        else {
            std::cerr << "Failed to connect to client. Error: " << GetLastError() << std::endl;
        }

        CloseHandle(hPipe);
    }
}

void HelloWorldService::HandleClient(HANDLE hPipe) {
    char buffer[1024];
    DWORD bytesRead;
    DWORD bytesWritten;

    // Read request from client
    if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr)) {
        buffer[bytesRead] = '\0';
        std::cout << "Received request: " << buffer << std::endl;

        // Send "Hello world" response
        const char* response = HELLO_WORLD_RESPONSE.c_str();
        if (WriteFile(hPipe, response, static_cast<DWORD>(HELLO_WORLD_RESPONSE.length()), &bytesWritten, nullptr)) {
            std::cout << "Sent response: " << response << std::endl;
        }
        else {
            std::cerr << "Failed to send response. Error: " << GetLastError() << std::endl;
        }
    }
    else {
        std::cerr << "Failed to read from client. Error: " << GetLastError() << std::endl;
    }

    // Flush the pipe to allow the client to read the pipe's contents before disconnecting
    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
}