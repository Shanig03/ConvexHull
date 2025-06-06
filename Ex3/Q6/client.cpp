#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <vector>
#include <sstream>
#include <iomanip>

#define PORT 9034
#define BUFSIZE 1024

class ConvexHullClient {
private:
    int clientSocket;
    struct sockaddr_in serverAddr;
    std::string inputBuffer;
    bool connected;

public:
    ConvexHullClient() : clientSocket(-1), connected(false) {}
    
    ~ConvexHullClient() {
        disconnect();
    }
    
    bool connect() {
        // Create socket
        if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            std::cerr << "Socket creation error" << std::endl;
            return false;
        }
        
        // Configure server address
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);
        
        // Convert IPv4 address from text to binary
        if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
            std::cerr << "Invalid address / Address not supported" << std::endl;
            close(clientSocket);
            return false;
        }
        
        // Connect to server
        if (::connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Connection failed" << std::endl;
            close(clientSocket);
            return false;
        }
        
        connected = true;
        std::cout << "Connected to Convex Hull Server!" << std::endl;
        return true;
    }
    
    void disconnect() {
        if (connected && clientSocket != -1) {
            close(clientSocket);
            connected = false;
            clientSocket = -1;
            std::cout << "Disconnected from server." << std::endl;
        }
    }
    
    bool sendCommand(const std::string& command) {
        if (!connected) {
            std::cerr << "Not connected to server" << std::endl;
            return false;
        }
        
        std::string message = command + "\n";
        ssize_t sent = send(clientSocket, message.c_str(), message.length(), 0);
        if (sent < 0) {
            std::cerr << "Failed to send command" << std::endl;
            return false;
        }
        
        return true;
    }
    
    std::string receiveResponse() {
        if (!connected) {
            return "";
        }
        
        char buffer[BUFSIZE];
        ssize_t received = recv(clientSocket, buffer, BUFSIZE - 1, 0);
        if (received <= 0) {
            std::cerr << "Connection lost or error receiving data" << std::endl;
            connected = false;
            return "";
        }
        
        buffer[received] = '\0';
        std::string response(buffer);
        
        // Remove trailing newline if present
        if (!response.empty() && response.back() == '\n') {
            response.pop_back();
        }
        if (!response.empty() && response.back() == '\r') {
            response.pop_back();
        }
        
        return response;
    }
    
    bool isConnected() const {
        return connected;
    }
    
    void interactiveMode() {
        if (!connected) {
            std::cerr << "Not connected to server" << std::endl;
            return;
        }
        
        // Set stdin to non-blocking mode
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
        
        fd_set readfds;
        int maxfd = (clientSocket > STDIN_FILENO) ? clientSocket : STDIN_FILENO;
        char buffer[BUFSIZE];
        
        std::cout << "\n=== Interactive Mode ===" << std::endl;
        std::cout << "Commands: Newgraph <n>, <x,y>, CH, Newpoint <x,y>, Removepoint <x,y>, Status" << std::endl;
        std::cout << "Type 'help' for help, 'exit' to quit" << std::endl;
        std::cout << "> " << std::flush;
        
        while (connected) {
            // Clear and set file descriptor set
            FD_ZERO(&readfds);
            FD_SET(STDIN_FILENO, &readfds);
            FD_SET(clientSocket, &readfds);
            
            // Wait for activity
            int activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);
            
            if (activity < 0) {
                perror("select error");
                break;
            }
            
            // Check for server response
            if (FD_ISSET(clientSocket, &readfds)) {
                int valread = read(clientSocket, buffer, BUFSIZE - 1);
                if (valread <= 0) {
                    std::cout << "\nConnection to server lost." << std::endl;
                    connected = false;
                    break;
                }
                
                buffer[valread] = '\0';
                std::cout << "\rServer: " << buffer;
                
                if (buffer[valread - 1] != '\n') {
                    std::cout << std::endl;
                }
                
                std::cout << "> " << inputBuffer << std::flush;
            }
            
            // Check for user input
            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                char ch;
                int n = read(STDIN_FILENO, &ch, 1);
                
                if (n > 0) {
                    if (ch == '\n') {
                        if (inputBuffer == "exit") {
                            break;
                        } else if (inputBuffer == "help") {
                            showHelp();
                        } else if (!inputBuffer.empty()) {
                            sendCommand(inputBuffer);
                        }
                        
                        inputBuffer.clear();
                        std::cout << std::endl << "> " << std::flush;
                        
                    } else if (ch == '\b' || ch == 127) {
                        if (!inputBuffer.empty()) {
                            inputBuffer.pop_back();
                            std::cout << "\b \b" << std::flush;
                        }
                        
                    } else if (ch >= 32 && ch <= 126) {
                        inputBuffer += ch;
                        std::cout << ch << std::flush;
                    }
                }
            }
        }
        
        // Restore stdin to blocking mode
        fcntl(STDIN_FILENO, F_SETFL, flags);
    }
    
    void showHelp() {
        std::cout << "\n=== Convex Hull Server Commands ===" << std::endl;
        std::cout << "  Newgraph <n>     - Create new graph with n points" << std::endl;
        std::cout << "  <x,y>            - Add point (x,y) to graph" << std::endl;
        std::cout << "  CH               - Calculate convex hull area" << std::endl;
        std::cout << "  Newpoint <x,y>   - Add new point to existing graph" << std::endl;
        std::cout << "  Removepoint <x,y>- Remove point from graph" << std::endl;
        std::cout << "  Status           - Show current graph status" << std::endl;
        std::cout << "  help             - Show this help message" << std::endl;
        std::cout << "  exit             - Quit client" << std::endl;
        std::cout << "\n=== Examples ===" << std::endl;
        std::cout << "  Newgraph 4" << std::endl;
        std::cout << "  0,0" << std::endl;
        std::cout << "  1,0" << std::endl;
        std::cout << "  1,1" << std::endl;
        std::cout << "  0,1" << std::endl;
        std::cout << "  CH" << std::endl;
        std::cout << std::endl;
    }
    
    void runDemo() {
        if (!connected) {
            std::cerr << "Not connected to server" << std::endl;
            return;
        }
        
        std::cout << "\n=== Running Demo ===" << std::endl;
        
        // Demo: Create a square
        std::vector<std::string> commands = {
            "Status",
            "Newgraph 4",
            "0,0",
            "4,0", 
            "4,3",
            "0,3",
            "CH",
            "Status",
            "Newpoint 2,1.5",
            "CH",
            "Removepoint 2,1.5",
            "CH",
            "Status"
        };
        
        std::cout << "Creating a square and testing operations..." << std::endl;
        
        for (const auto& cmd : commands) {
            std::cout << "\nSending: " << cmd << std::endl;
            if (sendCommand(cmd)) {
                std::string response = receiveResponse();
                if (!response.empty()) {
                    std::cout << "Response: " << response << std::endl;
                }
                usleep(500000); // 0.5 second delay for readability
            }
        }
        
        std::cout << "\nDemo completed!" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    std::cout << "=== Convex Hull Client ===" << std::endl;
    
    ConvexHullClient client;
    
    if (!client.connect()) {
        std::cerr << "Failed to connect to server" << std::endl;
        return -1;
    }
    
    // Wait for welcome message
    std::string welcome = client.receiveResponse();
    if (!welcome.empty()) {
        std::cout << "Server: " << welcome << std::endl;
    }
    
    // Check command line arguments
    if (argc > 1) {
        std::string mode(argv[1]);
        if (mode == "demo") {
            client.runDemo();
            return 0;
        } else if (mode == "help") {
            client.showHelp();
            return 0;
        }
    }
    
    // Default: interactive mode
    client.interactiveMode();
    
    return 0;
}