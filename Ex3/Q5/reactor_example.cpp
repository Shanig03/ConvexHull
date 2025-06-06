#include "reactor.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <signal.h>

#define PORT 9035
#define BUFSIZE 1024

// Forward declarations
void* handleServerSocket(int serverFd);
void* handleClientSocket(int clientFd);
void* handleServerInput(int stdinFd);
void signalHandler(int signum);

// Global reactor pointer for signal handler
void* globalReactor = nullptr;

// Signal handler for graceful shutdown
void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down reactor..." << std::endl;
    if (globalReactor) {
        stopReactor(globalReactor);
        globalReactor = nullptr;
    }
    exit(0);
}

// Callback function for handling server socket (new connections)
void* handleServerSocket(int serverFd) {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    int clientSocket = accept(serverFd, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientSocket < 0) {
        perror("Accept failed");
        return nullptr;
    }
    
    std::cout << "New connection from " << inet_ntoa(clientAddr.sin_addr) 
              << ":" << ntohs(clientAddr.sin_port) 
              << " (fd: " << clientSocket << ")" << std::endl;
    
    // Add the new client socket to the reactor
    if (addFdToReactor(globalReactor, clientSocket, handleClientSocket) != 0) {
        std::cout << "Failed to add client socket to reactor" << std::endl;
        close(clientSocket);
    }
    
    // Send welcome message
    const char* welcome = "Welcome to Reactor Server! Type 'quit' to disconnect.\n";
    send(clientSocket, welcome, strlen(welcome), 0);
    
    return nullptr;
}

// Callback function for handling client sockets (data from clients)
void* handleClientSocket(int clientFd) {
    char buffer[BUFSIZE];
    int valread = read(clientFd, buffer, BUFSIZE - 1);
    
    if (valread <= 0) {
        // Client disconnected
        std::cout << "Client disconnected (fd: " << clientFd << ")" << std::endl;
        removeFdFromReactor(globalReactor, clientFd);
        close(clientFd);
        return nullptr;
    }
    
    buffer[valread] = '\0';
    
    // Remove newline characters
    std::string message(buffer);
    while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
        message.pop_back();
    }
    
    std::cout << "Received from client (fd: " << clientFd << "): " << message << std::endl;
    
    // Check for quit command
    if (message == "quit") {
        const char* goodbye = "Goodbye!\n";
        send(clientFd, goodbye, strlen(goodbye), 0);
        
        std::cout << "Client requested disconnect (fd: " << clientFd << ")" << std::endl;
        removeFdFromReactor(globalReactor, clientFd);
        close(clientFd);
        return nullptr;
    }
    
    // Echo the message back to client
    std::string response = "Echo: " + message + "\n";
    send(clientFd, response.c_str(), response.length(), 0);
    
    return nullptr;
}

// Callback function for handling stdin (server console input)
void* handleServerInput([[maybe_unused]]int stdinFd) {
    std::string input;
    std::getline(std::cin, input);
    
    if (input == "quit" || input == "exit") {
        std::cout << "Server shutdown requested..." << std::endl;
        stopReactor(globalReactor);
        globalReactor = nullptr;
        exit(0);
    } else if (input == "status") {
        reactor* r = static_cast<reactor*>(globalReactor);
        std::cout << "Reactor status: " << (r->isRunning() ? "Running" : "Stopped") << std::endl;
        std::cout << "Active file descriptors: " << r->getActiveFdCount() << std::endl;
    } else if (input == "help") {
        std::cout << "Available commands:" << std::endl;
        std::cout << "  status - Show reactor status" << std::endl;
        std::cout << "  help   - Show this help message" << std::endl;
        std::cout << "  quit   - Shutdown server" << std::endl;
    } else if (!input.empty()) {
        std::cout << "Unknown command: " << input << " (type 'help' for available commands)" << std::endl;
    }
    
    return nullptr;
}

int main() {
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Create server socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(serverSocket);
        return -1;
    }
    
    // Configure server address
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);
    
    // Bind socket
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(serverSocket);
        return -1;
    }
    
    // Start listening
    if (listen(serverSocket, 5) < 0) {
        perror("Listen failed");
        close(serverSocket);
        return -1;
    }
    
    std::cout << "Reactor Server starting on port " << PORT << std::endl;
    
    // Start the reactor
    globalReactor = startReactor();
    if (!globalReactor) {
        std::cout << "Failed to start reactor" << std::endl;
        close(serverSocket);
        return -1;
    }
    
    // Add server socket to reactor
    if (addFdToReactor(globalReactor, serverSocket, handleServerSocket) != 0) {
        std::cout << "Failed to add server socket to reactor" << std::endl;
        stopReactor(globalReactor);
        close(serverSocket);
        return -1;
    }
    
    // Add stdin to reactor for server commands
    if (addFdToReactor(globalReactor, STDIN_FILENO, handleServerInput) != 0) {
        std::cout << "Failed to add stdin to reactor" << std::endl;
        stopReactor(globalReactor);
        close(serverSocket);
        return -1;
    }
    
    std::cout << "Reactor started successfully!" << std::endl;
    std::cout << "Server listening on port " << PORT << std::endl;
    std::cout << "Type 'help' for available commands, 'quit' to exit" << std::endl;
    std::cout << "> " << std::flush;
    
    // Keep main thread alive while reactor runs
    reactor* r = static_cast<reactor*>(globalReactor);
    while (r->isRunning()) {
        sleep(1);
    }
    
    // Cleanup
    close(serverSocket);
    std::cout << "Server shut down complete." << std::endl;
    
    return 0;
}