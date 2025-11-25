#pragma once
#include <windows.h>
#include <string>

/**
 * HelloWorldClient - A simple client class to communicate with the HelloWorld background service
 * 
 * Usage:
 *   std::string response = HelloWorldClient::GetHelloWorld();
 *   if (!response.empty()) {
 *       // Use the response
 *   }
 */
class HelloWorldClient {
public:
    /**
     * Connects to the HelloWorld background service and requests "Hello world"
     * @return The response from the service, or empty string if failed
     */
    static std::string GetHelloWorld();

private:
    static const std::string PIPE_NAME;
    static const int TIMEOUT_MS = 20000; // 20 seconds
};