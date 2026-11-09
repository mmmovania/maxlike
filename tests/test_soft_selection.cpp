// Edit Mesh's Soft Selection (Phase 10.6): the falloff curve against hand-computed values,
// and the weights on a mesh.
#include "scene/SoftSelection.h"
#include "scene/Primitives.h"
#include <cmath>
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
static bool Near(double a, double b) { return std::fabs(a - b) < 1e-12; }
using scene::soft::AffectRegion;

int main() {
    // The curve: ends, and the middle by hand. Pinch 0, Bubble 0 is 3u^2 - 2u^3.
    CHECK(Near(AffectRegion(0, 20, 0, 0), 1) && Near(AffectRegion(20, 20, 0, 0), 0) && Near(AffectRegion(30, 20, 0, 0), 0));
    CHECK(Near(AffectRegion(10, 20, 0, 0), 0.5));                 // u = 0.5: 3/4 - 2/8
    CHECK(Near(AffectRegion(5, 20, 0, 0), 3 * 0.5625 - 2 * 0.421875));   // u = 0.75
    // Pinch 1 removes the u^2 term: u^3 (0.125 at the middle); Bubble 1 adds 3u(1-u)^2.
    CHECK(Near(AffectRegion(10, 20, 1, 0), 0.125));
    CHECK(Near(AffectRegion(10, 20, 0, 1), 0.5 + 3 * 0.5 * 0.25));
    // Falling off: closer is heavier, never above 1 or below 0 (pinch and bubble 0).
    double last = 2;
    for (int d = 0; d <= 20; ++d) {
        const double w = AffectRegion(d, 20, 0, 0);
        CHECK(w <= last + 1e-15 && w >= 0 && w <= 1);
        last = w;
    }
    // Weights on a plane: the selected corner 1, a neighbour by its distance, far vertices 0.
    scene::PlaneObject plane(100, 100);
    plane.Params().Set(scene::PlaneObject::kLengthSegs, 10);
    plane.Params().Set(scene::PlaneObject::kWidthSegs, 10);
    const scene::Mesh m = plane.Evaluate();
    CHECK(m.verts.size() == 121);
    const auto w = scene::soft::Weights(m, {0}, 25, 0, 0);
    CHECK(Near(w[0], 1));
    CHECK(Near(w[1], AffectRegion(math::Length(m.verts[1] - m.verts[0]), 25, 0, 0)) && w[1] > 0 && w[1] < 1);
    CHECK(Near(w[120], 0));
    size_t affected = 0;
    for (double x : w) affected += x > 0;
    // Grid spacing 10: within 25 of the corner lie the points (i, j) with 10 sqrt(i^2+j^2) < 25.
    size_t expected = 0;
    for (int i = 0; i <= 10; ++i) for (int j = 0; j <= 10; ++j) expected += 10 * std::sqrt(i * i + j * j) < 25;
    CHECK(affected == expected);
    // Edge Distance 1: only the corner's edge neighbours (and itself).
    const auto w1 = scene::soft::Weights(m, {0}, 25, 0, 0, 1);
    size_t near1 = 0;
    for (double x : w1) near1 += x > 0;
    CHECK(near1 >= 3 && near1 <= 4 && near1 < affected);
    if (failures == 0) std::printf("soft selection: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
