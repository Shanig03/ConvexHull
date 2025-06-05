#include "convex_hull.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iomanip>

// Point implementation
Point::Point(double x, double y) : x(x), y(y) {}

bool Point::operator<(const Point& other) const {
    if (x != other.x) return x < other.x;
    return y < other.y;
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

int main() {
    int n;
    std::cout << "Enter number of points: ";
    std::cin >> n;
    
    std::vector<Point> points(n);
    
    std::cout << "Enter points (format: x,y):" << std::endl;
    // Read points
    for (int i = 0; i < n; i++) {
        std::string line;
        std::cin >> line;
        
        // Find comma position
        size_t commaPos = line.find(',');
        if (commaPos != std::string::npos) {
            double x = std::stod(line.substr(0, commaPos));
            double y = std::stod(line.substr(commaPos + 1));
            points[i] = Point(x, y);
        }
    }
    
    // Find convex hull
    std::vector<Point> hull = convexHull(points);
    
    // Calculate and output area
    double area = polygonArea(hull);
    std::cout << "Area of convex hull: ";
    std::cout << std::fixed << std::setprecision(1) << area << std::endl;
    
    return 0;
}