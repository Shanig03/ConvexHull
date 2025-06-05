#ifndef CONVEX_HULL_HPP
#define CONVEX_HULL_HPP

#include <vector>
#include <string>

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

#endif // CONVEX_HULL_HPP