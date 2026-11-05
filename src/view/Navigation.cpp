#include "view/Navigation.h"
#include <algorithm>
#include <cmath>

namespace view {

static constexpr double kPi = 3.14159265358979323846;

double OrthoUnitsPerPixel(const OrthoView& v, int w) {
    return w > 0 ? v.width / (w - 0.5) : 1.0; // matches the grid's horizontal scale
}

OrthoView PanOrtho(OrthoView v, int dx, int dy, int w, int /*h*/) {
    // Moving the mouse right moves the world right, i.e. the view centre left.
    const double u = OrthoUnitsPerPixel(v, w);
    v.centerU -= dx * u;
    v.centerV += dy * u; // screen y grows downwards, world V upwards
    return v;
}

OrthoView ZoomOrtho(OrthoView v, double factor) {
    if (factor > 0) v.width = std::clamp(v.width / factor, 1e-3, 1e7);
    return v;
}

OrthoView ZoomOrthoRegion(OrthoView v, int x0, int y0, int x1, int y1, int w, int h) {
    if (x1 < x0) std::swap(x0, x1);
    if (y1 < y0) std::swap(y0, y1);
    const int rw = x1 - x0, rh = y1 - y0;
    if (rw < 2 || rh < 2 || w <= 0 || h <= 0) return v;
    const double u = OrthoUnitsPerPixel(v, w);
    // New centre: the rectangle's centre, measured from the view centre.
    v.centerU += ((x0 + x1) / 2.0 - (w - 1) / 2.0) * u;
    v.centerV -= ((y0 + y1) / 2.0 - (h - 1) / 2.0) * u;
    // New width: the rectangle must fit both horizontally and vertically.
    const double fitW = rw * u;
    const double fitH = rh * u * w / static_cast<double>(h);
    v.width = std::clamp(std::max(fitW, fitH), 1e-3, 1e7);
    return v;
}

PerspectiveView PanPerspective(PerspectiveView v, int dx, int dy, int w) {
    if (w <= 0) return v;
    // World units per pixel at the target's distance.
    const double perPixel = 2.0 * v.distance * std::tan(v.fovDeg * kPi / 360.0) / w;
    const math::Mat4 view = v.ViewMatrix();
    const math::Vec3 right{view.m[0][0], view.m[1][0], view.m[2][0]};
    const math::Vec3 up{view.m[0][1], view.m[1][1], view.m[2][1]};
    v.target = v.target - right * (dx * perPixel) + up * (dy * perPixel);
    return v;
}

PerspectiveView ZoomPerspective(PerspectiveView v, double factor) {
    if (factor > 0) v.distance = std::clamp(v.distance / factor, 1e-2, 1e7);
    return v;
}

PerspectiveView OrbitPerspective(PerspectiveView v, int dx, int dy) {
    v.azimuthDeg = std::fmod(v.azimuthDeg + dx * kOrbitDegreesPerPixel, 360.0);
    v.elevationDeg = std::clamp(v.elevationDeg + dy * kOrbitDegreesPerPixel, -kMaxElevation, kMaxElevation);
    return v;
}

double ZoomFactorForDrag(int dy) { return std::exp(-dy * 0.01); }      // 100 px up = x2.7
double ZoomFactorForWheel(int clicks) { return std::pow(1.25, clicks); } // one notch = x1.25

} // namespace view
