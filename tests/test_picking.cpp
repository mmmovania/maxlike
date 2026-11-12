// Unit tests for picking objects from their projected outlines.
#include "view/Picking.h"
#include <cmath>
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

using namespace view;

// A square outline from (x, y) of size s.
static std::vector<Segment2> Square(double x, double y, double s) {
    return {{x, y, x + s, y}, {x + s, y, x + s, y + s}, {x + s, y + s, x, y + s}, {x, y + s, x, y}};
}

int main() {
    CHECK(std::fabs(DistanceToSegment(5, 3, {0, 0, 10, 0}) - 3) < 1e-12);
    CHECK(std::fabs(DistanceToSegment(-4, 3, {0, 0, 10, 0}) - 5) < 1e-12);   // past the end
    CHECK(std::fabs(DistanceToSegment(1, 1, {1, 1, 1, 1})) < 1e-12);          // degenerate

    const std::vector<std::vector<Segment2>> objects = {Square(10, 10, 40), Square(45, 20, 30)};
    // Wireframe: the edge must be near; inside a wireframe square is not a hit.
    CHECK(PickNearest(objects, 30, 12, 4) == 0);
    CHECK(PickNearest(objects, 30, 30, 4) == -1);
    CHECK(PickNearest(objects, 47, 30, 4) == 1);       // nearer to object 1's edge (x 45) than 0's (x 50)
    CHECK(PickNearest(objects, 200, 200, 4) == -1);

    // Shaded: the nearest triangle containing the point wins.
    std::vector<std::vector<Triangle2>> surfaces(2);
    surfaces[0].push_back({{0, 100, 0}, {0, 0, 100}, 5.0});
    surfaces[1].push_back({{0, 0, 100}, {0, 100, 0}, 2.0});   // other winding, nearer
    CHECK(PickSurface(surfaces, 10, 10) == 1);
    surfaces[1][0].depth = 9.0;
    CHECK(PickSurface(surfaces, 10, 10) == 0);
    CHECK(PickSurface(surfaces, 90, 90) == -1);

    // Region (crossing): touching or inside counts, in any drag direction.
    auto r = PickRegion(objects, 0, 0, 20, 20);
    CHECK(r.size() == 1 && r[0] == 0);
    r = PickRegion(objects, 80, 60, 40, 15);              // dragged up-left, crosses both
    CHECK(r.size() == 2);
    // Window (11.3): only objects entirely inside; ones the rectangle merely crosses are not.
    {
        const auto w = PickRegionWindow(objects, -1000, -1000, 1000, 1000);
        CHECK(w.size() == objects.size());
        const auto c = PickRegion(objects, 80, 60, 40, 15), cw = PickRegionWindow(objects, 80, 60, 40, 15);
        CHECK(c.size() == 2 && cw.size() == 1 && cw[0] == 1);   // object 1 lies inside; object 0 is only crossed
    }
    r = PickRegion(objects, 20, 20, 30, 30);              // inside object 0's square, no edge
    CHECK(r.empty());
    r = PickRegion(objects, 0, 0, 200, 200);              // everything inside
    CHECK(r.size() == 2);

    // Points: the nearest within the tolerance wins; rectangles in any direction.
    const std::vector<Point2> pts = {{10, 10}, {13, 10}, {50, 50}};
    CHECK(PickNearestPoint(pts, 12, 10, 4) == 1 && PickNearestPoint(pts, 9, 11, 4) == 0);
    CHECK(PickNearestPoint(pts, 30, 30, 4) == -1);
    auto in = PickPointsInRect(pts, 60, 60, 5, 5);
    CHECK(in.size() == 3);
    in = PickPointsInRect(pts, 0, 0, 20, 20);
    CHECK(in.size() == 2 && in[0] == 0 && in[1] == 1);
    if (failures == 0) std::printf("picking: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
