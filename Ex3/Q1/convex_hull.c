#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    double x, y;
} Point;

// Function to find the cross product of vectors OA and OB
double cross_product(Point O, Point A, Point B) {
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

// Function to calculate distance between two points
double distance(Point a, Point b) {
    return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

// Comparison function for qsort - sort by polar angle with respect to bottom-most point
Point pivot;
int compare(const void *a, const void *b) {
    Point *p1 = (Point *)a;
    Point *p2 = (Point *)b;
    
    double cross = cross_product(pivot, *p1, *p2);
    if (cross == 0) {
        // If collinear, sort by distance from pivot
        return distance(pivot, *p1) < distance(pivot, *p2) ? -1 : 1;
    }
    return cross > 0 ? -1 : 1;
}

// Function to find convex hull using Graham scan
int convex_hull(Point points[], int n, Point hull[]) {
    if (n < 3) return 0;
    
    // Find the bottom-most point (and left-most in case of tie)
    int min_idx = 0;
    for (int i = 1; i < n; i++) {
        if (points[i].y < points[min_idx].y || 
            (points[i].y == points[min_idx].y && points[i].x < points[min_idx].x)) {
            min_idx = i;
        }
    }
    
    // Swap the bottom-most point to the first position
    Point temp = points[0];
    points[0] = points[min_idx];
    points[min_idx] = temp;
    
    // Set pivot for sorting
    pivot = points[0];
    
    // Sort points by polar angle with respect to pivot
    qsort(points + 1, n - 1, sizeof(Point), compare);
    
    // Remove points with same angle (keep the farthest)
    int m = 1;
    for (int i = 1; i < n; i++) {
        while (i < n - 1 && cross_product(pivot, points[i], points[i + 1]) == 0) {
            i++;
        }
        points[m++] = points[i];
    }
    
    if (m < 3) return 0;
    
    // Create convex hull
    hull[0] = points[0];
    hull[1] = points[1];
    hull[2] = points[2];
    int hull_size = 3;
    
    for (int i = 3; i < m; i++) {
        // Remove points that make clockwise turn
        while (hull_size > 1 && cross_product(hull[hull_size-2], hull[hull_size-1], points[i]) <= 0) {
            hull_size--;
        }
        hull[hull_size++] = points[i];
    }
    
    return hull_size;
}

// Function to calculate area of polygon using shoelace formula
double polygon_area(Point hull[], int n) {
    if (n < 3) return 0.0;
    
    double area = 0.0;
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        area += hull[i].x * hull[j].y - hull[j].x * hull[i].y;
    }
    return fabs(area) / 2.0;
}

int main() {
    int n;
    scanf("%d", &n);
    
    Point *points = (Point *)malloc(n * sizeof(Point));
    Point *hull = (Point *)malloc(n * sizeof(Point));
    
    // Read points
    for (int i = 0; i < n; i++) {
        scanf("%lf,%lf", &points[i].x, &points[i].y);
    }
    
    // Find convex hull
    int hull_size = convex_hull(points, n, hull);
    
    // Calculate and print area
    double area = polygon_area(hull, hull_size);
    printf("%.1f\n", area);
    
    free(points);
    free(hull);
    
    return 0;
}