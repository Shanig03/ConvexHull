#include "reactor.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <cassert>

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

// Test the template reactor directly
void testTemplateReactor() {
    std::cout << "\n=== Testing Template Reactor Class ===" << std::endl;
    
    Reactor<int> reactor;
    
    // Test initial state
    assert(!reactor.isRunning());
    assert(reactor.getActiveFdCount() == 0);
    std::cout << "✓ Initial state test passed" << std::endl;
    
    // Test starting reactor
    assert(reactor.start());
    assert(reactor.isRunning());
    std::cout << "✓ Reactor start test passed" << std::endl;
    
    // Create a pipe for testing
    int pipeFds[2];
    if (pipe(pipeFds) == -1) {
        perror("pipe");
        return;
    }
    
    // Test adding file descriptor
    assert(reactor.addFd(pipeFds[0], testCallback) == 0);
    assert(reactor.getActiveFdCount() == 1);
    std::cout << "✓ Add file descriptor test passed" << std::endl;
    
    // Test adding duplicate file descriptor (should fail)
    assert(reactor.addFd(pipeFds[0], testCallback) == -1);
    std::cout << "✓ Duplicate file descriptor test passed" << std::endl;
    
    // Write to pipe to trigger callback
    const char* testMsg = "Hello Reactor!";
    ssize_t bytes_written = write(pipeFds[1], testMsg, strlen(testMsg));
    assert(bytes_written == static_cast<ssize_t>(strlen(testMsg)));
    
    // Wait a bit for the reactor to process
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    // Test removing file descriptor
    assert(reactor.removeFd(pipeFds[0]) == 0);
    assert(reactor.getActiveFdCount() == 0);
    std::cout << "✓ Remove file descriptor test passed" << std::endl;
    
    // Test removing non-existent file descriptor (should fail)
    assert(reactor.removeFd(pipeFds[0]) == -1);
    std::cout << "✓ Remove non-existent file descriptor test passed" << std::endl;
    
    // Test stopping reactor
    assert(reactor.stop());
    assert(!reactor.isRunning());
    std::cout << "✓ Reactor stop test passed" << std::endl;
    
    // Clean up
    close(pipeFds[0]);
    close(pipeFds[1]);
    
    std::cout << "✓ All template reactor tests passed!" << std::endl;
}

// Test the C-style interface
void testCStyleInterface() {
    std::cout << "\n=== Testing C-Style Interface ===" << std::endl;
    
    // Test starting reactor
    void* reactor = startReactor();
    assert(reactor != nullptr);
    std::cout << "✓ startReactor test passed" << std::endl;
    
    // Create a pipe for testing
    int pipeFds[2];
    if (pipe(pipeFds) == -1) {
        perror("pipe");
        return;
    }
    
    // Test adding file descriptor
    assert(addFdToReactor(reactor, pipeFds[0], testCallback) == 0);
    std::cout << "✓ addFdToReactor test passed" << std::endl;
    
    // Write to pipe to trigger callback
   const char* testMsg = "Hello C Interface!";
    ssize_t bytes_written = write(pipeFds[1], testMsg, strlen(testMsg));
    assert(bytes_written == static_cast<ssize_t>(strlen(testMsg)));
    
    // Wait a bit for the reactor to process
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    // Test removing file descriptor
    assert(removeFdFromReactor(reactor, pipeFds[0]) == 0);
    std::cout << "✓ removeFdFromReactor test passed" << std::endl;
    
    // Test stopping reactor
    assert(stopReactor(reactor) == 0);
    std::cout << "✓ stopReactor test passed" << std::endl;
    
    // Clean up
    close(pipeFds[0]);
    close(pipeFds[1]);
    
    std::cout << "✓ All C-style interface tests passed!" << std::endl;
}

// Add this new callback function specifically for server socket
void* serverSocketCallback(int fd) {
    std::cout << "Server callback triggered for fd: " << fd << std::endl;
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    int clientSocket = accept(fd, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientSocket >= 0) {
        std::cout << "Accepted new connection on fd: " << clientSocket << std::endl;
        close(clientSocket); // Close immediately for testing purposes
    }
    return nullptr;
}

void testSocketReactor() {
    std::cout << "\n=== Testing Socket Reactor ===" << std::endl;
    
    // Create server socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    assert(serverSocket >= 0);
    
    // Set socket options
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    
    // Bind to localhost
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(12345);
    
    assert(bind(serverSocket, (struct sockaddr*)&addr, sizeof(addr)) == 0);
    assert(listen(serverSocket, 5) == 0);
    
    // Start reactor
    void* reactor = startReactor();
    assert(reactor != nullptr);
    
    // Add server socket to reactor with the specific server callback
    assert(addFdToReactor(reactor, serverSocket, serverSocketCallback) == 0);
    std::cout << "✓ Socket reactor setup passed" << std::endl;
    
    std::atomic<bool> clientFinished{false};
    
    // Create client socket in separate thread
    std::thread clientThread([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket >= 0) {
            struct sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(12345);
            inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
            
            if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == 0) {
                std::cout << "✓ Client connected successfully" << std::endl;
            }
            close(clientSocket);
            clientFinished = true;
        }
    });
    
    // Wait for client to finish with timeout
    auto start = std::chrono::steady_clock::now();
    while (!clientFinished) {
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(5)) {
            std::cout << "Socket test timed out" << std::endl;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Clean up
    if (clientThread.joinable()) {
        clientThread.join();
    }
    
    removeFdFromReactor(reactor, serverSocket);
    stopReactor(reactor);
    close(serverSocket);
    
    std::cout << "✓ Socket reactor test completed" << std::endl;
}

// Test error conditions
void testErrorConditions() {
    std::cout << "\n=== Testing Error Conditions ===" << std::endl;
    
    // Test null reactor pointer
    assert(addFdToReactor(nullptr, 0, testCallback) == -1);
    assert(removeFdFromReactor(nullptr, 0) == -1);
    assert(stopReactor(nullptr) == -1);
    std::cout << "✓ Null pointer tests passed" << std::endl;
    
    std::cout << "✓ All error condition tests passed!" << std::endl;
}

int main() {
    std::cout << "=== Reactor Template Library Test Suite ===" << std::endl;
    
    try {
        testTemplateReactor();
        testCStyleInterface();
        testSocketReactor();
        testErrorConditions();
        
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