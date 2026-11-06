#pragma once
#include <vector>

// Picking objects in a viewport from their projected outlines (window pixels).
// No Windows or OpenGL dependency (unit-tested in tests/test_picking.cpp).
namespace view {

struct Segment2 { double x0, y0, x1, y1; };
struct Triangle2 { double x[3], y[3]; double depth; };   // depth: smaller = nearer

// Distance from a point to a segment (pixels).
double DistanceToSegment(double px, double py, const Segment2& s);

// Wireframe picking: the object (index into `outlines`) with an edge nearest to (x, y)
// within `tolerance` pixels, or -1. NOTE: R3's pick tolerance is unverified (4 px used).
int PickNearest(const std::vector<std::vector<Segment2>>& outlines, double x, double y, double tolerance);

// Shaded picking: the object with the nearest triangle containing (x, y), or -1.
int PickSurface(const std::vector<std::vector<Triangle2>>& surfaces, double x, double y);

// Region selection, "crossing": every object with an edge inside or crossing the
// rectangle. NOTE: R3's default (window or crossing) is unverified.
std::vector<int> PickRegion(const std::vector<std::vector<Segment2>>& outlines,
                            double x0, double y0, double x1, double y1);

} // namespace view
