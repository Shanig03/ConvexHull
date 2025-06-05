#ifndef PERFORMANCE_TEST_HPP
#define PERFORMANCE_TEST_HPP

#include <vector>
#include <deque>

struct Point {
    double x, y;
    
    Point(double x = 0, double y = 0);
    bool operator<(const Point& other) const;
};

// Function declarations for cross product calculation
double crossProduct(const Point& O, const Point& A, const Point& B);

// Vector-based implementations
std::vector<Point> convexHullVector(std::vector<Point> points);

double polygonAreaVector(const std::vector<Point>& vertices);

// Deque-based implementations
std::deque<Point> convexHullDeque(std::vector<Point> points);

double polygonAreaDeque(const std::deque<Point>& vertices);

#endif // PERFORMANCE_TEST_HPP