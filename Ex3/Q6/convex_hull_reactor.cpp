#include "reactor.hpp" 
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
#include <fcntl.h>
#include <map>
#include <sys/select.h>

#define PORT 9034
#define BUFSIZE 1024

// Point class
class Point {
public:
    double x, y;
    
    Point(double x = 0, double y = 0) : x(x), y(y) {}
    
    bool operator<(const Point& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
    
    bool operator==(const Point& other) const {
        return std::abs(x - other.x) < 1e-9 && std::abs(y - other.y) < 1e-9;
    }
};

// Function pointer typedef
typedef void* (*reactorFunc)(int fd);


// Global variables
std::vector<Point> globalGraph;
int counter = 0;
std::map<int, bool> clientSockets;
int serverSocket;
reactor* globalReactor = nullptr; 

// Function prototypes
void* handleNewConnection(int fd);
void* handleClientData(int fd);
void* handleServerInput(int fd);
std::string processCommand(const std::string& command);
void sendToClient(int clientSocket, const std::string& message);
Point parsePoint(const std::string& pointStr);
double polygonArea(const std::vector<Point>& vertices);
std::vector<Point> convexHull(std::vector<Point> points);
double crossProduct(const Point& O, const Point& A, const Point& B);

// Send message to a specific client
void sendToClient(int clientSocket, const std::string& message) {
    std::string msg = message + "\n";
    send(clientSocket, msg.c_str(), msg.length(), 0);
}

// Parse point from string "x,y"
Point parsePoint(const std::string& pointStr) {
    size_t commaPos = pointStr.find(',');
    if (commaPos != std::string::npos) {
        double x = std::stod(pointStr.substr(0, commaPos));
        double y = std::stod(pointStr.substr(commaPos + 1));
        return Point(x, y);
    }
    return Point(0, 0);
}

// Cross product of vectors OA and OB
double crossProduct(const Point& O, const Point& A, const Point& B) {
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

// Calculate convex hull using Andrew's monotone chain algorithm
std::vector<Point> convexHull(std::vector<Point> points) {
    int n = points.size();
    if (n <= 1) return points;
    
    // Sort points lexicographically
    std::sort(points.begin(), points.end());
    
    // Build lower hull
    std::vector<Point> hull;
    for (int i = 0; i < n; i++) {
        while (hull.size() >= 2 && 
               crossProduct(hull[hull.size()-2], hull[hull.size()-1], points[i]) <= 0) {
            hull.pop_back();
        }
        hull.push_back(points[i]);
    }
    
    // Build upper hull
    size_t t = hull.size() + 1;
    for (int i = n - 2; i >= 0; i--) {
        while (hull.size() >= t && 
               crossProduct(hull[hull.size()-2], hull[hull.size()-1], points[i]) <= 0) {
            hull.pop_back();
        }
        hull.push_back(points[i]);
    }
    
    // Remove the last point because it's the same as the first
    hull.pop_back();
    
    return hull;
}

// Calculate area of polygon using shoelace formula
double polygonArea(const std::vector<Point>& vertices) {
    int n = vertices.size();
    if (n < 3) return 0.0;
    
    double area = 0.0;
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        area += vertices[i].x * vertices[j].y;
        area -= vertices[j].x * vertices[i].y;
    }
    
    return std::abs(area) / 2.0;
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

// Handle new client connections
void* handleNewConnection(int fd) {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    int newSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
    if (newSocket < 0) {
        perror("Accept failed");
        return nullptr;
    }
    
    std::cout << "New connection from " << inet_ntoa(clientAddr.sin_addr) 
              << ":" << ntohs(clientAddr.sin_port) << " (fd: " << newSocket << ")" << std::endl;
    
    // Send welcome message
    sendToClient(newSocket, "Commands: Newgraph <n>, <x,y>, CH, Newpoint <x,y>, Removepoint <x,y>, Status");
    
    // Add new client socket to reactor
     if (globalReactor->addFd(newSocket, handleClientData) == 0) {
        clientSockets[newSocket] = true;
        std::cout << "Client added to reactor (fd: " << newSocket << ")" << std::endl;
    } else {
        std::cout << "Failed to add client to reactor" << std::endl;
        close(newSocket);
    }
    
    return nullptr;
}

// Handle client data
void* handleClientData(int fd) {
    char buffer[BUFSIZE];
    int valread = read(fd, buffer, BUFSIZE - 1);
    
    if (valread <= 0) {
        // Client disconnected
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        getpeername(fd, (struct sockaddr*)&clientAddr, &clientLen);
        std::cout << "Client disconnected (fd: " << fd << ")" << std::endl;
        
        // Remove from reactor and close socket
        globalReactor->removeFd(fd);        
        clientSockets.erase(fd);
        close(fd);
        
        return nullptr;
    }
    
    // Process received command
    buffer[valread] = '\0';
    
    // Remove newline if present
    std::string command(buffer);
    if (!command.empty() && command.back() == '\n') {
        command.pop_back();
    }
    if (!command.empty() && command.back() == '\r') {
        command.pop_back();
    }
    
    std::cout << "Received command from fd " << fd << ": " << command << std::endl;
    
    // Process command and get response
    std::string response = processCommand(command);
    
    // Send response back to client
    sendToClient(fd, response);
    
    std::cout << "Sent response to fd " << fd << ": " << response << std::endl;
    
    return nullptr;
}

// Handle server input (stdin)
void* handleServerInput(int fd) {
    char buffer[BUFSIZE];
    int valread = read(STDIN_FILENO, buffer, BUFSIZE - 1);
    
    if (valread > 0) {
        buffer[valread] = '\0';
        std::string input(buffer);
        
        // Remove newline if present
        if (!input.empty() && input.back() == '\n') {
            input.pop_back();
        }
        if (!input.empty() && input.back() == '\r') {
            input.pop_back();
        }
        
        if (input == "exit") {
            std::cout << "Shutting down server..." << std::endl;
            
            // Close all client sockets
            for (auto& pair : clientSockets) {
                globalReactor->removeFd(pair.first);
                close(pair.first);
            }
            clientSockets.clear();
            
            // Stop reactor
            globalReactor->stop();
            
            // Close server socket
            close(serverSocket);
            
            exit(0);
        } else if (input == "status") {
            std::cout << "Server status:" << std::endl;
            std::cout << "  Connected clients: " << clientSockets.size() << std::endl;
            std::cout << "  Graph points: " << globalGraph.size() << std::endl;
            std::cout << "  Pending points: " << counter << std::endl;
        }
    }
    
    return nullptr;
}

int main() {
    struct sockaddr_in serverAddr;
    globalReactor = new reactor();

    // Create server socket
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    
    // Configure server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);
    
    // Bind socket to address
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Start listening for connections
    if (listen(serverSocket, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    std::cout << "Reactor-based Convex Hull Server listening on port " << PORT << std::endl;
    std::cout << "Available commands: Newgraph <n>, <x,y>, CH, Newpoint <x,y>, Removepoint <x,y>, Status" << std::endl;
    std::cout << "Server commands: 'exit' to shutdown, 'status' for server info" << std::endl;
    
    if (!globalReactor->start()) {
        std::cerr << "Failed to start reactor" << std::endl;
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    // Add server socket to reactor
    if (globalReactor->addFd(serverSocket, handleNewConnection) != 0) {
        std::cerr << "Failed to add server socket to reactor" << std::endl;
        globalReactor->stop();
        delete globalReactor;
        close(serverSocket);
        exit(EXIT_FAILURE);
    }
    
    // Add stdin to reactor
    if (globalReactor->addFd(STDIN_FILENO, handleServerInput) != 0) {
        std::cerr << "Failed to add stdin to reactor" << std::endl;
        globalReactor->stop();
        delete globalReactor;
        close(serverSocket);
        exit(EXIT_FAILURE);
    }
    
    std::cout << "Reactor started successfully" << std::endl;
    
    // Main event loop - single-threaded
    while (globalReactor->isRunning()) {
        globalReactor->runOnce();
    }
    
    delete globalReactor;
    return 0;
}