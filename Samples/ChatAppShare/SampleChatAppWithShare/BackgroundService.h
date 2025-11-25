#pragma once

#include <windows.h>
#include <string>
#include <thread>

// Forward declarations and constants
extern const std::string PIPE_NAME;
extern const std::string HELLO_WORLD_RESPONSE;

class HelloWorldService {
private:
    bool m_running;
    std::thread m_serviceThread;

public:
    HelloWorldService();
    ~HelloWorldService();
    
    bool Start();
    void Stop();
    bool IsRunning() const;

private:
    void ServiceLoop();
    void HandleClient(HANDLE hPipe);
};
