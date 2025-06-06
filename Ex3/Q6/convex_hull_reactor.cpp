#include "reactor.hpp"
#include "convex_hull.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <signal.h>
#include <map>

#define PORT 9034
#define MAXCLIENTS 10
#define BUFSIZE 1024

// Global variables
std::vector<Point> globalGraph;
int counter = 0;
void* globalReactor = nullptr;
int globalServerSocket = -1;
std::map<int, std::string> clientBuffers; // Store partial messages per client

// Signal handler for graceful shutdown
void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down server..." << std::endl;
    
    if (globalReactor) {
        stopReactor(globalReactor);
        globalReactor = nullptr;
    }
    
    if (globalServerSocket != -1) {
        close(globalServerSocket);
        globalServerSocket = -1;
    }
    
    exit(0);
}

// Forward declarations
void* handleServerSocket(int serverFd);
void* handleClientSocket(int clientFd);
void* handleServerInput(int stdinFd);

// Send message to a specific client
void sendToClient(int clientSocket, const std::string& message) {
    std::string msg = message + "\n";
    send(clientSocket, msg.c_str(), msg.length(), 0);
}

// Process command from a client and return response
std::string processCommand(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;
    
    if (cmd == "Newgraph") {
        int n;
        iss >> n;
        
        // Clear the global graph and prepare for new points
        globalGraph.clear();
        globalGraph.reserve(n);
        
        counter = n; // Set counter for expected points
        if (n <= 0) {
            return "Invalid number of points. Please specify a positive integer.";
        }
        return "Ready for " + std::to_string(n) + " points. Send points one by one.";
        
    } else if (cmd == "CH") {
        // Calculate and return convex hull area
        std::vector<Point> hull = convexHull(globalGraph);
        double area = polygonArea(hull);
        
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << area;
        return "Convex Hull Area: " + oss.str();
        
    } else if (cmd == "Newpoint") {
        // Add a new point to existing graph
        std::string pointStr;
        iss >> pointStr;
        Point newPoint = parsePoint(pointStr);
        globalGraph.push_back(newPoint);
        
        return "New point added: (" + std::to_string(newPoint.x) + "," + std::to_string(newPoint.y) + ")";
        
    } else if (cmd == "Removepoint") {
        // Remove a point from the graph
        std::string pointStr;
        iss >> pointStr;
        Point pointToRemove = parsePoint(pointStr);
        
        // Find and remove the point
        auto it = std::find(globalGraph.begin(), globalGraph.end(), pointToRemove);
        if (it != globalGraph.end()) {
            globalGraph.erase(it);
            return "Point removed: (" + std::to_string(pointToRemove.x) + "," + std::to_string(pointToRemove.y) + ")";
        } else {
            return "Point not found: (" + std::to_string(pointToRemove.x) + "," + std::to_string(pointToRemove.y) + ")";
        }
        
    } else if (cmd == "Status") {
        // Return current graph status
        return "Current graph has " + std::to_string(globalGraph.size()) + " points";
        
    } else {
        if (counter > 0) {        
            try {
                Point newPoint = parsePoint(command);
                globalGraph.push_back(newPoint);
                counter--;
                return "Point added: (" + std::to_string(newPoint.x) + "," + std::to_string(newPoint.y) + ")";
            } catch (...) {
                return "Unknown command or invalid point format. Please use one of the following commands:\n"
                    "Newgraph <n>, <x,y>, CH, Newpoint <x,y>, Removepoint <x,y>, Status";
            }
        } else {
            return "The graph is full. Please start a new graph with 'Newgraph <n>' command or add new points with 'Newpoint <x,y>'.";
        }
    }
}

// Callback function for handling server socket (new connections)
void* handleServerSocket(int serverFd) {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    int newSocket = accept(serverFd, (struct sockaddr*)&clientAddr, &clientLen);
    if (newSocket < 0) {
        perror("Accept failed");
        return nullptr;
    }
    
    std::cout << "New connection from " << inet_ntoa(clientAddr.sin_addr) 
              << ":" << ntohs(clientAddr.sin_port) 
              << " (fd: " << newSocket << ")" << std::endl;
    
    // Add the new client socket to the reactor
    if (addFdToReactor(globalReactor, newSocket, handleClientSocket) != 0) {
        std::cout << "Failed to add client socket to reactor" << std::endl;
        close(newSocket);
        return nullptr;
    }
    
    // Initialize client buffer
    clientBuffers[newSocket] = "";
    
    // Send welcome message
    sendToClient(newSocket, "Commands: Newgraph <n>, <x,y>, CH, Newpoint <x,y>, Removepoint <x,y>, Status");
    
    std::cout << "Added client to reactor (fd: " << newSocket << ")" << std::endl;
    
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
        clientBuffers.erase(clientFd);
        close(clientFd);
        return nullptr;
    }
    
    buffer[valread] = '\0';
    
    // Add received data to client's buffer
    clientBuffers[clientFd] += std::string(buffer);
    
    // Process complete lines
    std::string& clientBuffer = clientBuffers[clientFd];
    size_t pos;
    
    while ((pos = clientBuffer.find('\n')) != std::string::npos) {
        std::string command = clientBuffer.substr(0, pos);
        clientBuffer.erase(0, pos + 1);
        
        // Remove carriage return if present
        if (!command.empty() && command.back() == '\r') {
            command.pop_back();
        }
        
        if (!command.empty()) {
            std::cout << "Received command from client (fd: " << clientFd << "): " << command << std::endl;
            
            // Process command and get response
            std::string response = processCommand(command);
            
            // Send response back to client
            sendToClient(clientFd, response);
            
            std::cout << "Sent response to client (fd: " << clientFd << "): " << response << std::endl;
        }
    }
    
    return nullptr;
}

// Callback function for handling stdin (server console input)
void* handleServerInput(int stdinFd) {
    std::string input;
    std::getline(std::cin, input);
    
    if (input == "exit" || input == "quit") {
        std::cout << "Server shutdown requested..." << std::endl;
        signalHandler(SIGTERM);
    } else if (input == "status") {
        reactor* r = static_cast<reactor*>(globalReactor);
        std::cout << "=== Server Status ===" << std::endl;
        std::cout << "Reactor running: " << (r->isRunning() ? "Yes" : "No") << std::endl;
        std::cout << "Active connections: " << (r->getActiveFdCount() - 2) << std::endl; // -2 for server socket and stdin
        std::cout << "Graph points: " << globalGraph.size() << std::endl;
        std::cout << "Expected points: " << counter << std::endl;
    } else if (input == "graph") {
        std::cout << "=== Current Graph ===" << std::endl;
        if (globalGraph.empty()) {
            std::cout << "Graph is empty" << std::endl;
        } else {
            for (size_t i = 0; i < globalGraph.size(); ++i) {
                std::cout << "Point " << i << ": (" << globalGraph[i].x << ", " << globalGraph[i].y << ")" << std::endl;
            }
            
            // Calculate and show convex hull
            std::vector<Point> hull = convexHull(globalGraph);
            double area = polygonArea(hull);
            std::cout << "Convex Hull Area: " << std::fixed << std::setprecision(1) << area << std::endl;
        }
    } else if (input == "help") {
        std::cout << "=== Server Commands ===" << std::endl;
        std::cout << "  status - Show server status" << std::endl;
        std::cout << "  graph  - Show current graph and convex hull" << std::endl;
        std::cout << "  help   - Show this help message" << std::endl;
        std::cout << "  exit   - Shutdown server" << std::endl;
    } else if (!input.empty()) {
        std::cout << "Unknown command: '" << input << "' (type 'help' for available commands)" << std::endl;
    }
    
    std::cout << "> " << std::flush;
    
    return nullptr;
}

int main() {
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    std::cout << "=== Convex Hull Server with Reactor Pattern ===" << std::endl;
    std::cout << "Starting server on port " << PORT << "..." << std::endl;
    
    // Create server socket
    globalServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (globalServerSocket < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(globalServerSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(globalServerSocket);
        return -1;
    }
    
    // Configure server address
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);
    
    // Bind socket to address
    if (bind(globalServerSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(globalServerSocket);
        return -1;
    }
    
    // Start listening for connections
    if (listen(globalServerSocket, MAXCLIENTS) < 0) {
        perror("Listen failed");
        close(globalServerSocket);
        return -1;
    }
    
    std::cout << "Socket created and bound successfully" << std::endl;
    
    // Start the reactor
    globalReactor = startReactor();
    if (!globalReactor) {
        std::cout << "Failed to start reactor" << std::endl;
        close(globalServerSocket);
        return -1;
    }
    
    std::cout << "Reactor started successfully" << std::endl;
    
    // Add server socket to reactor
    if (addFdToReactor(globalReactor, globalServerSocket, handleServerSocket) != 0) {
        std::cout << "Failed to add server socket to reactor" << std::endl;
        stopReactor(globalReactor);
        close(globalServerSocket);
        return -1;
    }
    
    // Add stdin to reactor for server commands
    if (addFdToReactor(globalReactor, STDIN_FILENO, handleServerInput) != 0) {
        std::cout << "Failed to add stdin to reactor" << std::endl;
        stopReactor(globalReactor);
        close(globalServerSocket);
        return -1;
    }
    
    std::cout << "=== Server Started Successfully ===" << std::endl;
    std::cout << "Listening on port " << PORT << std::endl;
    std::cout << "Available client commands: Newgraph <n>, <x,y>, CH, Newpoint <x,y>, Removepoint <x,y>, Status" << std::endl;
    std::cout << "Server commands: status, graph, help, exit" << std::endl;
    std::cout << "> " << std::flush;
    
    // Keep main thread alive while reactor runs
    reactor* r = static_cast<reactor*>(globalReactor);
    while (r && r->isRunning()) {
        sleep(1);
    }
    
    // Cleanup (this will be reached after signalHandler is called)
    std::cout << "Cleaning up resources..." << std::endl;
    
    if (globalServerSocket != -1) {
        close(globalServerSocket);
    }
    
    std::cout << "Server shutdown complete." << std::endl;
    
    return 0;
}