#include "view/PerspectiveView.h"
#include "view/OrthoGrid.h"
#include <cmath>

namespace view {

static constexpr double kPi = 3.14159265358979323846;

math::Vec3 PerspectiveView::Eye() const {
    const double el = elevationDeg * kPi / 180.0, az = azimuthDeg * kPi / 180.0;
    // Start behind the target (-Y), raise by the elevation, turn by the azimuth.
    const double horizontal = distance * std::cos(el);
    return {target.x + horizontal * std::sin(az),
            target.y - horizontal * std::cos(az),
            target.z + distance * std::sin(el)};
}

math::Mat4 PerspectiveView::ViewMatrix() const {
    return math::LookAt(Eye(), target, {0, 0, 1}); // Max's world is Z-up
}

math::Mat4 PerspectiveView::ProjectionMatrix(int w, int h) const {
    const double aspect = h > 0 ? static_cast<double>(w) / h : 1.0;
    return math::PerspectiveHorizontal(fovDeg, aspect, 1.0, 100000.0);
}

std::vector<WorldLine> HomeGridLines(double extent) {
    std::vector<WorldLine> lines;
    const int n = static_cast<int>(extent / kGridSpacing);
    for (int i = -n; i <= n; ++i) {
        const double c = i * kGridSpacing;
        const int kind = (i == 0) ? 2 : (i % kMajorEvery == 0 ? 1 : 0);
        lines.push_back({{c, -extent, 0}, {c, extent, 0}, kind});   // parallel to Y
        lines.push_back({{-extent, c, 0}, {extent, c, 0}, kind});   // parallel to X
    }
    return lines;
}

} // namespace view
