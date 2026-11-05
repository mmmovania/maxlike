// Unit tests for the 3D maths and the default perspective view. The reference's
// perspective viewport is 862 x 423; its horizontal grid lines (constant Y) lie on
// these rows, measured from the screenshot.
#include "math/Mat4.h"
#include "view/PerspectiveView.h"
#include <cmath>
#include <cstdio>
#include <iterator>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

static void LookAtBasics() {
    // Looking from (0,-10,0) at the origin with Z up: the origin is straight ahead
    // (eye space -Z), +X is to the right, +Z is up.
    const math::Mat4 v = math::LookAt({0, -10, 0}, {0, 0, 0}, {0, 0, 1});
    const math::Vec4 o = v * math::Vec4{0, 0, 0, 1};
    CHECK(std::fabs(o.x) < 1e-9 && std::fabs(o.y) < 1e-9 && std::fabs(o.z + 10) < 1e-9);
    const math::Vec4 x = v * math::Vec4{1, 0, 0, 1};
    CHECK(x.x > 0.99);
    const math::Vec4 z = v * math::Vec4{0, 0, 1, 1};
    CHECK(z.y > 0.99);
}

static void PixelToClipCorners() {
    const math::Mat4 m = math::PixelToClip(800, 600);
    const math::Vec4 tl = m * math::Vec4{0, 0, 0, 1};
    const math::Vec4 br = m * math::Vec4{800, 600, 0, 1};
    CHECK(std::fabs(tl.x + 1) < 1e-9 && std::fabs(tl.y - 1) < 1e-9);
    CHECK(std::fabs(br.x - 1) < 1e-9 && std::fabs(br.y + 1) < 1e-9);
}

static void DefaultViewMatchesReference() {
    const int W = 862, H = 423;
    const view::PerspectiveView v;
    const math::Mat4 vp = v.ProjectionMatrix(W, H) * v.ViewMatrix();
    // Rows of the lines Y = 200, 190, ..., -110 (Y = 0 is the X axis, at row 211).
    const int rows[] = {89, 93, 97, 100, 104, 109, 113, 118, 123, 128, 133, 139, 145, 151,
                        158, 166, 173, 182, 191, 200, 211, 222, 234, 247, 262, 277, 295,
                        314, 335, 359, 386, 417};
    for (size_t i = 0; i < std::size(rows); ++i) {
        const double Y = 200.0 - 10.0 * static_cast<double>(i);
        double px = 0, py = 0;
        CHECK(math::ProjectToPixel(vp, {0, Y, 0}, W, H, px, py));
        const int row = static_cast<int>(std::floor(py));
        if (std::abs(row - rows[i]) > 1) {
            ++failures;
            std::printf("FAIL line Y=%g: row %d, measured %d\n", Y, row, rows[i]);
        }
    }
    // The Y axis is the vertical centre column (431).
    double px = 0, py = 0;
    math::ProjectToPixel(vp, {0, 100, 0}, W, H, px, py);
    CHECK(static_cast<int>(std::floor(px)) == 431);
}

static void HomeGridShape() {
    const auto lines = view::HomeGridLines();
    CHECK(lines.size() == 2 * 41);                  // -200..200 every 10, both directions
    int axes = 0, majors = 0;
    for (const auto& l : lines) { axes += l.kind == 2; majors += l.kind == 1; }
    CHECK(axes == 2 && majors == 2 * 4);            // +-100, +-200 in both directions
}

int main() {
    LookAtBasics();
    PixelToClipCorners();
    DefaultViewMatchesReference();
    HomeGridShape();
    if (failures == 0) std::printf("perspective: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
