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
    r = PickRegion(objects, 20, 20, 30, 30);              // inside object 0's square, no edge
    CHECK(r.empty());
    r = PickRegion(objects, 0, 0, 200, 200);              // everything inside
    CHECK(r.size() == 2);

    if (failures == 0) std::printf("picking: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
