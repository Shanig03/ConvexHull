#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>

#define PORT 9034
#define BUFSIZE 1024

int main() {
    int clientSocket;
    struct sockaddr_in serverAddr;
    fd_set readfds;
    char buffer[BUFSIZE];
    std::string inputBuffer = "";
    int valread, maxfd;
    
    // Create socket
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }
    
    // Configure server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    
    // Convert IPv4 address from text to binary
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address / Address not supported" << std::endl;
        return -1;
    }
    
    // Connect to server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        return -1;
    }
    
    // Set stdin to non-blocking mode
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    
    std::cout << "Connected to Convex Hull Server!" << std::endl;
    std::cout << "Type your commands or 'exit' to quit:" << std::endl;
    std::cout << "> " << std::flush;
    
    maxfd = (clientSocket > STDIN_FILENO) ? clientSocket : STDIN_FILENO;
    
    while (true) {
        // Clear and set file descriptor set
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);     // Monitor stdin for user input
        FD_SET(clientSocket, &readfds);     // Monitor socket for server messages
        
        // Wait for activity on either stdin or socket
        int activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        
        if (activity < 0) {
            perror("select error");
            break;
        }
        
        // Check if there's data from server
        if (FD_ISSET(clientSocket, &readfds)) {
            valread = read(clientSocket, buffer, BUFSIZE - 1);
            if (valread <= 0) {
                std::cout << "\nConnection to server lost." << std::endl;
                break;
            }
            
            buffer[valread] = '\0';
            std::cout << "\rServer: " << buffer;
            
            // Check if buffer doesn't end with newline
            if (buffer[valread - 1] != '\n') {
                std::cout << std::endl;
            }
            
            // Reprint the prompt and current input
            std::cout << "> " << inputBuffer << std::flush;
        }
        
        // Check if there's user input from stdin
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char ch;
            int n = read(STDIN_FILENO, &ch, 1);
            
            if (n > 0) {
                if (ch == '\n') {
                    // User pressed enter - process the command
                    if (inputBuffer == "exit") {
                        break;
                    }
                    
                    if (!inputBuffer.empty()) {
                        // Send command to server
                        std::string message = inputBuffer + "\n";
                        send(clientSocket, message.c_str(), message.length(), 0);
                    }
                    
                    // Clear input buffer and show new prompt
                    inputBuffer.clear();
                    std::cout << std::endl << "> " << std::flush;
                    
                } else if (ch == '\b' || ch == 127) {
                    // Backspace - remove last character
                    if (!inputBuffer.empty()) {
                        inputBuffer.pop_back();
                        std::cout << "\b \b" << std::flush;  // Erase character on screen
                    }
                    
                } else if (ch >= 32 && ch <= 126) {
                    // Printable character - add to input buffer
                    inputBuffer += ch;
                    std::cout << ch << std::flush;  // Echo character
                }
            }
        }
    }
    
    // Restore stdin to blocking mode
    fcntl(STDIN_FILENO, F_SETFL, flags);
    
    // Close connection
    close(clientSocket);
    std::cout << "\nDisconnected from server." << std::endl;
    
    return 0;
}