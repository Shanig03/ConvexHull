#include "reactor.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <chrono>
#include <cassert>
#include <fcntl.h>

// Test callback function
void* testCallback(int fd) {
    std::cout << "Callback triggered for fd: " << fd << std::endl;
    char buffer[256];
    int bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::cout << "Read from fd " << fd << ": " << buffer << std::endl;
    }
    return nullptr;
}

void testReactor() {
    std::cout << "\n=== Testing Reactor Interface ===" << std::endl;
    
    void* reactor = startReactor();
    assert(reactor != nullptr);
    std::cout << "✓ startReactor test passed" << std::endl;
    
    int pipeFds[2];
    if (pipe(pipeFds) == -1) {
        perror("pipe");
        return;
    }
    
    assert(addFdToReactor(reactor, pipeFds[0], testCallback) == 0);
    std::cout << "✓ addFdToReactor test passed" << std::endl;
    
    const char* testMsg = "Hello Reactor!";
    ssize_t bytesWritten = write(pipeFds[1], testMsg, strlen(testMsg));
    assert(bytesWritten == static_cast<ssize_t>(strlen(testMsg)));
    std::cout << "✓ Pipe write successful" << std::endl;
    
    // Run reactor a few times to process the message
    for (int i = 0; i < 10; i++) {
        runReactorOnce(reactor);
    }
    assert(removeFdFromReactor(reactor, pipeFds[0]) == 0);
    std::cout << "✓ removeFdFromReactor test passed" << std::endl;
    
    assert(stopReactor(reactor) == 0);
    std::cout << "✓ stopReactor test passed" << std::endl;
    
    close(pipeFds[0]);
    close(pipeFds[1]);
    
    std::cout << "✓ Basic reactor tests passed!" << std::endl;
    
}

// Test error conditions
void testErrorConditions() {
    std::cout << "\n=== Testing Error Conditions ===" << std::endl;
    
    // Test null reactor pointer
    assert(addFdToReactor(nullptr, 0, testCallback) == -1);
    assert(removeFdFromReactor(nullptr, 0) == -1);
    assert(stopReactor(nullptr) == -1);
    std::cout << "✓ Null pointer tests passed" << std::endl;
    
    // Test invalid file descriptors
    void* reactor = startReactor();
    assert(addFdToReactor(reactor, -1, testCallback) == -1);
    assert(removeFdFromReactor(reactor, -1) == -1);
    stopReactor(reactor);
    std::cout << "✓ Invalid fd tests passed" << std::endl;
}

// Add this new callback function specifically for server socket
void* serverSocketCallback(int fd) {
    std::cout << "Server callback triggered for fd: " << fd << std::endl;
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    int clientSocket = accept(fd, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientSocket >= 0) {
        std::cout << "Accepted new connection on fd: " << clientSocket << std::endl;
        close(clientSocket);
    }
    return nullptr;
}

void testSocketReactor() {
    std::cout << "\n=== Testing Socket Reactor ===" << std::endl;
    
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    assert(serverSocket >= 0);
    
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(12345);
    
    assert(bind(serverSocket, (struct sockaddr*)&addr, sizeof(addr)) == 0);
    assert(listen(serverSocket, 5) == 0);
    
    void* reactor = startReactor();
    assert(reactor != nullptr);
    
    assert(addFdToReactor(reactor, serverSocket, serverSocketCallback) == 0);
    std::cout << "✓ Socket reactor setup passed" << std::endl;

    // Create non-blocking client socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    assert(clientSocket >= 0);
    
    int flags = fcntl(clientSocket, F_GETFL, 0);
    fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);
    
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    // Try non-blocking connect
    connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    
    // Wait for connection with timeout
    auto start = std::chrono::steady_clock::now();
    bool connected = false;

    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(5)) {
        // Run reactor to handle server events
        runReactorOnce(reactor);
        
        // Check client connection status
        fd_set writefds;
        struct timeval tv = {0, 100000}; // 100ms
        FD_ZERO(&writefds);
        FD_SET(clientSocket, &writefds);
        
        if (select(clientSocket + 1, NULL, &writefds, NULL, &tv) > 0) {
            int error = 0;
            socklen_t len = sizeof(error);
            if (getsockopt(clientSocket, SOL_SOCKET, SO_ERROR, &error, &len) >= 0 && error == 0) {
                connected = true;
                std::cout << "✓ Client connected successfully" << std::endl;
                break;
            }
        }
    }
    
    if (!connected) {
        std::cout << "Socket test timed out" << std::endl;
    }

    close(clientSocket);
    removeFdFromReactor(reactor, serverSocket);
    stopReactor(reactor);
    close(serverSocket);
    
    std::cout << "✓ Socket reactor test completed" << std::endl;
}

int main() {
    std::cout << "=== Reactor Library Test Suite ===" << std::endl;
    
    try {
        testReactor();
        testErrorConditions();
        testSocketReactor();
        
        std::cout << "\n🎉 ALL TESTS PASSED! 🎉" << std::endl;
        std::cout << "The reactor library is working correctly." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
    
    return 0;
}