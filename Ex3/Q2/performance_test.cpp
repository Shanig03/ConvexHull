#include "performance_test.hpp"
#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <chrono>
#include <string>

// Implement Point methods
Point::Point(double x, double y) : x(x), y(y) {}

bool Point::operator<(const Point& other) const {
    if (x != other.x) return x < other.x;
    return y < other.y;
}

// Cross product of vectors OA and OB
double crossProduct(const Point& O, const Point& A, const Point& B) {
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

// Vector-based convex hull implementation
std::vector<Point> convexHullVector(std::vector<Point> points) {
    int n = points.size();
    if (n <= 1) return points;
    
    std::sort(points.begin(), points.end());
    
    // Build lower hull using vector
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
    
    hull.pop_back();
    return hull;
}

// Deque-based convex hull implementation
std::deque<Point> convexHullDeque(std::vector<Point> points) {
    int n = points.size();
    if (n <= 1) {
        std::deque<Point> result;
        for (const auto& p : points) result.push_back(p);
        return result;
    }
    
    std::sort(points.begin(), points.end());
    
    // Build lower hull using deque
    std::deque<Point> hull;
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
    
    hull.pop_back();
    return hull;
}

// Calculate area using vector
double polygonAreaVector(const std::vector<Point>& vertices) {
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

// Calculate area using deque
double polygonAreaDeque(const std::deque<Point>& vertices) {
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
    for (int i = 0; i < n; i++) {
        std::string line;
        std::cin >> line;
        
        size_t commaPos = line.find(',');
        if (commaPos != std::string::npos) {
            double x = std::stod(line.substr(0, commaPos));
            double y = std::stod(line.substr(commaPos + 1));
            points[i] = Point(x, y);
        }
    }
    
    std::cout << "\n=== PERFORMANCE COMPARISON ===" << std::endl;
    
    // Test Vector Implementation
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<Point> hullVector = convexHullVector(points);
    double areaVector = polygonAreaVector(hullVector);
    auto end = std::chrono::high_resolution_clock::now();
    auto durationVector = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "\nVECTOR IMPLEMENTATION:" << std::endl;
    std::cout << "Hull points: " << hullVector.size() << std::endl;
    std::cout << "Convex hull points: ";
    for (const auto& p : hullVector) {
        std::cout << "(" << p.x << "," << p.y << ") ";
    }
    std::cout << std::endl;
    std::cout << "Area: " << std::fixed << std::setprecision(1) << areaVector << std::endl;
    std::cout << "Execution time: " << durationVector.count() << " microseconds" << std::endl;
    
    // Test Deque Implementation
    start = std::chrono::high_resolution_clock::now();
    std::deque<Point> hullDeque = convexHullDeque(points);
    double areaDeque = polygonAreaDeque(hullDeque);
    end = std::chrono::high_resolution_clock::now();
    auto durationDeque = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "\nDEQUE IMPLEMENTATION:" << std::endl;
    std::cout << "Hull points: " << hullDeque.size() << std::endl;
    std::cout << "Convex hull points: ";
    for (const auto& p : hullDeque) {
        std::cout << "(" << p.x << "," << p.y << ") ";
    }
    std::cout << std::endl;
    std::cout << "Area: " << std::fixed << std::setprecision(1) << areaDeque << std::endl;
    std::cout << "Execution time: " << durationDeque.count() << " microseconds" << std::endl;
    
    // Performance Analysis
    std::cout << "\n=== PERFORMANCE ANALYSIS ===" << std::endl;
    std::cout << "Vector time: " << durationVector.count() << " microseconds" << std::endl;
    std::cout << "Deque time:  " << durationDeque.count() << " microseconds" << std::endl;
    
    if (durationVector.count() < durationDeque.count()) {
        double improvement = (double)(durationDeque.count() - durationVector.count()) / durationDeque.count() * 100;
        std::cout << "\nVector is FASTER by " << std::fixed << std::setprecision(2) 
                  << improvement << "%" << std::endl;
    } else if (durationDeque.count() < durationVector.count()) {
        double improvement = (double)(durationVector.count() - durationDeque.count()) / durationVector.count() * 100;
        std::cout << "\nDeque is FASTER by " << std::fixed << std::setprecision(2) 
                  << improvement << "%" << std::endl;
    } else {
        std::cout << "\nBoth implementations have similar performance." << std::endl;
    }
    
    return 0;
}