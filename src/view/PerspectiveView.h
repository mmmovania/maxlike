#pragma once
#include "math/Mat4.h"
#include <vector>

// The perspective viewport's camera and home grid. No Windows or OpenGL
// dependency (unit-tested in tests/test_perspective.cpp).
namespace view {

// R3's default perspective view, fitted to the reference screenshot (step 2.2b):
// the camera orbits a target point; 250 units away, 15 degrees above the ground,
// straight behind (looking along +Y), with a 45-degree horizontal field of view.
struct PerspectiveView {
    math::Vec3 target{0, 0, 0};
    double distance = 250.0;
    double elevationDeg = 15.0;   // angle above the XY plane
    double azimuthDeg = 0.0;      // 0 = camera on the -Y side, looking along +Y
    double fovDeg = 45.0;         // horizontal

    math::Vec3 Eye() const;
    math::Mat4 ViewMatrix() const;
    math::Mat4 ProjectionMatrix(int width, int height) const;
};

struct WorldLine {
    math::Vec3 a, b;
    int kind; // 0 minor, 1 major, 2 axis (same order as LineKind)
};

// The home grid on the XY plane, +-extent units, lines every 10 (measured: the far
// edge of the reference's grid is the line at Y = 200).
inline constexpr double kHomeGridExtent = 200.0;
std::vector<WorldLine> HomeGridLines(double extent = kHomeGridExtent);

} // namespace view
