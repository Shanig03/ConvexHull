#include "convex_hull.hpp"
#include "reactor_proactor.hpp"
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
#include <thread>
#include <mutex>
#include <atomic>
#include <csignal>

#define PORT 9034
#define BUFSIZE 1024

// Global graph data structure shared by all clients
std::vector<Point> globalGraph;
int counter = 0;
std::mutex graphMutex; // Mutex to protect shared graph resource
std::atomic<bool> serverRunning{true};
int globalServerSocket = -1;

Point::Point(double x, double y) : x(x), y(y) {}

bool Point::operator<(const Point& other) const {
    if (x != other.x) return x < other.x;
    return y < other.y;
}

bool Point::operator==(const Point& other) const {
    return std::abs(x - other.x) < 1e-9 && std::abs(y - other.y) < 1e-9;
}

// Cross product of vectors OA and OB
double crossProduct(const Point& O, const Point& A, const Point& B) {
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

// Signal handler for graceful shutdown
void signalHandler(int signum) {
    if (signum == SIGINT) {
        std::cout << "\nShutdown requested..." << std::endl;
        serverRunning = false;
        if (globalServerSocket != -1) {
            close(globalServerSocket);
        }
        exit(0);
    }
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
        
        // Lock mutex to protect shared graph
        std::lock_guard<std::mutex> lock(graphMutex);

        if (iss.fail()) {
            return "Please specify a valid number of points.";
        }
        
        // Clear the global graph and prepare for new points
        globalGraph.clear();
        globalGraph.reserve(n);
        
        counter = n; // Set counter for expected points
        if (n <= 0) {
            return "Invalid number of points. Please specify a positive integer.";
        }
        return "Ready for " + std::to_string(n) + " points. Send points one by one.";
        
    } else if (cmd == "CH") {
        // Lock mutex to protect shared graph during algorithm execution
        std::lock_guard<std::mutex> lock(graphMutex);
        
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
        
        // Lock mutex to protect shared graph
        std::lock_guard<std::mutex> lock(graphMutex);
        globalGraph.push_back(newPoint);
        
        return "New point added: (" + std::to_string(newPoint.x) + "," + std::to_string(newPoint.y) + ")";
        
    } else if (cmd == "Removepoint") {
        // Remove a point from the graph
        std::string pointStr;
        iss >> pointStr;
        Point pointToRemove = parsePoint(pointStr);
        
        // Lock mutex to protect shared graph
        std::lock_guard<std::mutex> lock(graphMutex);
        
        // Find and remove the point
        auto it = std::find(globalGraph.begin(), globalGraph.end(), pointToRemove);
        if (it != globalGraph.end()) {
            globalGraph.erase(it);
            return "Point removed: (" + std::to_string(pointToRemove.x) + "," + std::to_string(pointToRemove.y) + ")";
        } else {
            return "Point not found: (" + std::to_string(pointToRemove.x) + "," + std::to_string(pointToRemove.y) + ")";
        }
        
    } else if (cmd == "Status") {
        // Lock mutex to protect shared graph
        std::lock_guard<std::mutex> lock(graphMutex);
        
        // Return current graph status
        return "Current graph has " + std::to_string(globalGraph.size()) + " points";
        
    } else {
        // Lock mutex to protect counter and graph access
        std::lock_guard<std::mutex> lock(graphMutex);
        
        if (counter > 0){        
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

// Handle client connection in a separate thread
void* handleClient(int clientSocket) {
    char buffer[BUFSIZE];
    int valread;
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    getpeername(clientSocket, (struct sockaddr*)&clientAddr, &addrLen);

    std::cout << "Client handler thread started for " << inet_ntoa(clientAddr.sin_addr) 
              << ":" << ntohs(clientAddr.sin_port) << std::endl;

    sendToClient(clientSocket, "Commands: Newgraph <n>, <x,y>, CH, Newpoint <x,y>, Removepoint <x,y>, Status");
    
    while (true) {
        valread = read(clientSocket, buffer, BUFSIZE - 1);
        if (valread <= 0) {
            std::cout << "Client disconnected: " << inet_ntoa(clientAddr.sin_addr) 
                      << ":" << ntohs(clientAddr.sin_port) << std::endl;
            break;
        }
        buffer[valread] = '\0';
        std::string command(buffer);
        if (!command.empty() && command.back() == '\n') command.pop_back();
        if (!command.empty() && command.back() == '\r') command.pop_back();

        std::cout << "Received command from " << inet_ntoa(clientAddr.sin_addr) 
                  << ": " << command << std::endl;

        std::string response = processCommand(command);
        sendToClient(clientSocket, response);

        std::cout << "Sent response to " << inet_ntoa(clientAddr.sin_addr) 
                  << ": " << response << std::endl;
    }
    close(clientSocket);
    std::cout << "Client handler thread ending for " << inet_ntoa(clientAddr.sin_addr) 
              << ":" << ntohs(clientAddr.sin_port) << std::endl;
    return nullptr;
}


int main() {
    signal(SIGINT, signalHandler); // Handle Ctrl+C for graceful shutdown
    
    int serverSocket;
    struct sockaddr_in serverAddr;

    // Create server socket
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to allow reuse of address
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    globalServerSocket = serverSocket; // Store global server socket for cleanup

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Start listening for connections
    if (listen(serverSocket, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Convex Hull Server listening on port " << PORT << std::endl;
    std::cout << "Available commands: Newgraph <n>, <x,y>, CH, Newpoint <x,y>, Removepoint <x,y>, Status" << std::endl;
    std::cout << "Server will create a new thread for each client connection (proactor)." << std::endl;

    // PROACTOR: Start proactor instead of manual accept/thread loop 
    pthread_t proactor_tid = startProactor(serverSocket, handleClient);

    // Wait for signal or server termination
    pause();

    // Cleanup 
    close(serverSocket);
    stopProactor(proactor_tid);

    return 0;
}