#ifndef CONVEX_HULL_HPP
#define CONVEX_HULL_HPP

#include <vector>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 9034
#define MAXCLIENTS 10
#define BUFSIZE 1024

// Point structure
struct Point {
    double x, y;
    
    Point(double x = 0, double y = 0);

    bool operator<(const Point& other) const;

    bool operator==(const Point& other) const;
};

// Function declarations
double crossProduct(const Point& O, const Point& A, const Point& B);

std::vector<Point> convexHull(std::vector<Point> points);

double polygonArea(const std::vector<Point>& vertices);

Point parsePoint(const std::string& pointStr);

// Sends a message to a specific client
void sendToClient(int clientSocket, const std::string& message);

// Processes a command from a client and returns the response
std::string processCommand(const std::string& command);

// Global variable declaration
extern std::vector<Point> globalGraph;

#endif // CONVEX_HULL_HPP