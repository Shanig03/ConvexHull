#include "convex_hull.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <string>
#include <sstream>

// Point implementation
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
    int t = hull.size() + 1;
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

int main() {
    std::vector<Point> currentGraph;
    std::string line;
    std::cout << "Enter commands (Newgraph <number of points>, CH, Newpoint <i,j>, Removepoint <i,j>) or type 'exit' to quit. " << std::endl;

    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "Newgraph") {
            int n;
            iss >> n;
            
            currentGraph.clear();
            currentGraph.reserve(n);
            
            // Read n points
            for (int i = 0; i < n; i++) {
                std::string pointLine;
                std::getline(std::cin, pointLine);
                Point p = parsePoint(pointLine);
                currentGraph.push_back(p);
            }
            
        } else if (command == "CH") {
            // Calculate and output convex hull area
            std::vector<Point> hull = convexHull(currentGraph);
            double area = polygonArea(hull);
            std::cout << std::fixed << std::setprecision(1) << area << std::endl;
            
        } else if (command == "Newpoint") {
            std::string pointStr;
            iss >> pointStr;
            Point newPoint = parsePoint(pointStr);
            currentGraph.push_back(newPoint);
            
        } else if (command == "Removepoint") {
            std::string pointStr;
            iss >> pointStr;
            Point pointToRemove = parsePoint(pointStr);
            
            // Find and remove the point
            auto it = std::find(currentGraph.begin(), currentGraph.end(), pointToRemove);
            if (it != currentGraph.end()) {
                currentGraph.erase(it);
            }
        } else if (command == "exit") {
            break;
        } else {
            std::cout << "Unknown command: " << command << std::endl;
        }
    }
    
    return 0;
}