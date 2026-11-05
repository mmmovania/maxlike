#pragma once
#include "view/OrthoGrid.h"
#include "view/PerspectiveView.h"

// Viewport navigation as pure functions: each takes a view and a mouse movement in
// pixels and returns the changed view. No Windows or OpenGL dependency
// (unit-tested in tests/test_navigation.cpp).
namespace view {

// ---- orthographic views ----
// World units per pixel horizontally (the same scale the grid uses).
double OrthoUnitsPerPixel(const OrthoView& v, int width);

// Drag by (dx, dy) pixels: the world moves with the mouse.
OrthoView PanOrtho(OrthoView v, int dx, int dy, int width, int height);

// Zoom by `factor` (> 1 zooms in) about the view centre.
OrthoView ZoomOrtho(OrthoView v, double factor);

// Zoom so the pixel rectangle [x0,x1] x [y0,y1] fills the view.
OrthoView ZoomOrthoRegion(OrthoView v, int x0, int y0, int x1, int y1, int width, int height);

// ---- perspective view ----
// Drag by (dx, dy) pixels: move the camera and its target sideways/up, so the point
// at the target's depth follows the mouse.
PerspectiveView PanPerspective(PerspectiveView v, int dx, int dy, int width);

// Move towards the target by `factor` (> 1 = closer).
PerspectiveView ZoomPerspective(PerspectiveView v, double factor);

// Orbit around the target: (dx, dy) pixels turn azimuth and elevation.
PerspectiveView OrbitPerspective(PerspectiveView v, int dx, int dy);

// ---- shared ----
// Zoom factor for a vertical drag of `dy` pixels (up = in), and for wheel clicks.
double ZoomFactorForDrag(int dy);
double ZoomFactorForWheel(int clicks);

inline constexpr double kOrbitDegreesPerPixel = 0.5;
inline constexpr double kMaxElevation = 89.0;

} // namespace view
