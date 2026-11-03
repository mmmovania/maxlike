#pragma once
#include <vector>

// The axis tripod in the bottom-left corner of each viewport, as exact pixels.
// No Windows or OpenGL dependency (unit-tested in tests/test_tripod.cpp against the
// four tripods of the reference screenshot).
namespace view {

struct TripodPixel {
    int x, y;   // viewport pixels, top-down
    int axis;   // 0 = X (red), 1 = Y (green), 2 = Z (blue)
};

// `screenAxes[a]` = how world axis a appears on screen: its components along the
// screen's right and up directions (unit length when the axis lies in the screen
// plane, 0 when it points at the viewer). Pixels come out in drawing order (X, Y, Z),
// so later pixels cover earlier ones.
std::vector<TripodPixel> TripodPixels(const double screenAxes[3][2], int viewHeight);

} // namespace view
