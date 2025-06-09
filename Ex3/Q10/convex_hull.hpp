#ifndef CONVEX_HULL_HPP
#define CONVEX_HULL_HPP

#include <vector>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <mutex>
#include "reactor_proactor.hpp"
#include <condition_variable>
#include <atomic>

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

void sendToClient(int clientSocket, const std::string& message);

std::string processCommand(const std::string& command);

void* handleClient(int clientSocket);

// Global variables
extern std::vector<Point> globalGraph;

extern int counter;

extern std::mutex graphMutex;


// For the CH area watcher thread
extern std::mutex chAreaMutex;

extern std::condition_variable chAreaCond;

extern double lastCHArea;

extern std::atomic<bool> chAreaAtLeast100;

extern std::atomic<bool> watcherRunning;


void* chAreaWatcherThread(void*);

extern pthread_t watcherThread;

void notifyCHAreaChanged(double area);

#endif // CONVEX_HULL_HPP