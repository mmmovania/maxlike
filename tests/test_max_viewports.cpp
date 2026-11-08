// The viewports saved in the R3.1 samples, and their conversion to maxlike's views.
// Calibrated against R3.1's screenshot of instances.max: the Top view shows 2.725 px per
// unit over its 865-px width, i.e. 317 units across = 400 x zoom (0.79068).
#include "io/MaxViewports.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
static bool Near(double a, double b, double eps) { return std::fabs(a - b) < eps; }

static std::vector<io::MaxViewport> Views(const char* name) {
    std::ifstream f(std::string(MAXLIKE_SAMPLES) + "/" + name + ".max", std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::vector<io::MaxViewport> v;
    std::string error;
    if (!io::ReadViewports(b, v, error)) std::printf("  %s: %s\n", name, error.c_str());
    return v;
}

int main() {
    const auto v = Views("instances");
    CHECK(v.size() == 4);
    if (v.size() == 4) {
        // Top, Front, Left, Perspective, in layout order.
        CHECK(v[0].type == 2 && v[1].type == 4 && v[2].type == 0 && v[3].type == 7);
        const view::OrthoView top = io::ToOrthoView(v[0]);
        CHECK(Near(top.width, 316.27, 0.01));
        CHECK(Near(865 / top.width, 2.725, 0.015));     // R3.1's ~113 px per 41.47-unit spacing, read by hand (+-0.01)
        CHECK(Near(top.centerU, -37.312, 1e-3) && Near(top.centerV, -2.3175, 1e-3));
        CHECK(Near(io::ToOrthoView(v[2]).width, 400 * v[2].zoom, 1e-9) && Near(v[2].zoom, 0.11681, 1e-5));   // the Left view, zoomed far in
        const view::PerspectiveView p = io::ToPerspectiveView(v[3]);
        CHECK(Near(p.fovDeg, 45, 1e-3) && Near(p.distance, 645.82, 0.01));
        CHECK(Near(p.elevationDeg, 15, 1e-3) && Near(p.azimuthDeg, 0, 1e-3));
        // The eye recomputed by maxlike's own camera equals Max's: -T * R^T.
        const math::Vec3 eye = p.Eye();
        CHECK(Near(eye.x, -36.948, 0.01) && Near(eye.y, 8.742 * 0.25882 - 650.81 * 0.96593, 0.02) &&
              Near(eye.z, 8.742 * 0.96593 + 650.81 * 0.25882, 0.02));
    }
    // Every sample saves four viewports: Top, Front, Left, Perspective.
    for (const char* f : {"box", "boxes", "cone", "parent_child_link", "pivot", "plane", "teapot", "torus"}) {
        const auto w = Views(f);
        CHECK(w.size() == 4 && w[0].type == 2 && w[1].type == 4 && w[2].type == 0 && w[3].type == 7);
    }
    if (failures == 0) std::printf("max viewports: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
