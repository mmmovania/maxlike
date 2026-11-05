// Unit tests for viewport navigation.
#include "view/Navigation.h"
#include <cmath>
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
#define NEAR(a, b, eps) (std::fabs((a) - (b)) <= (eps))

using namespace view;

static void PanFollowsTheMouse() {
    // After panning 50 px right and 30 px down, the grid's axes move by those pixels.
    const int w = 863, h = 423;
    const OrthoView v0;
    const OrthoView v1 = PanOrtho(v0, 50, 30, w, h);
    CHECK(ColumnOf(0, w, h, v1) - ColumnOf(0, w, h, v0) == 50);
    CHECK(std::abs((RowOf(0, w, h, v1) - RowOf(0, w, h, v0)) - 30) <= 1); // vertical scale differs slightly
}

static void ZoomKeepsTheCentre() {
    OrthoView v;
    v.centerU = 30; v.centerV = -20;
    const OrthoView z = ZoomOrtho(v, 2.0);
    CHECK(NEAR(z.width, 200.0, 1e-9) && z.centerU == 30 && z.centerV == -20);
    CHECK(NEAR(ZoomOrtho(z, 0.5).width, 400.0, 1e-9));
}

static void RegionZoomFitsTheRectangle() {
    const int w = 800, h = 400;
    const OrthoView v0;
    // The right half of the view becomes the whole view: width halves, centre moves right.
    const OrthoView v1 = ZoomOrthoRegion(v0, 400, 100, 799, 300, w, h);
    CHECK(v1.width < v0.width * 0.55 && v1.width > v0.width * 0.45);
    CHECK(v1.centerU > 90 && v1.centerU < 110);
    // A tiny rectangle (a click) changes nothing.
    const OrthoView v2 = ZoomOrthoRegion(v0, 10, 10, 11, 11, w, h);
    CHECK(v2.width == v0.width && v2.centerU == v0.centerU);
}

static void OrbitKeepsDistanceAndClamps() {
    PerspectiveView p;
    const double d0 = math::Length(p.Eye() - p.target);
    const PerspectiveView q = OrbitPerspective(p, 120, 40);   // +60 deg azimuth, +20 elevation
    CHECK(NEAR(math::Length(q.Eye() - q.target), d0, 1e-9));
    CHECK(NEAR(q.azimuthDeg, 60.0, 1e-9) && NEAR(q.elevationDeg, 35.0, 1e-9));
    CHECK(OrbitPerspective(p, 0, 10000).elevationDeg == kMaxElevation);
}

static void PerspectivePanMovesTarget() {
    PerspectiveView p;
    const PerspectiveView q = PanPerspective(p, 100, 0, 862);  // drag right: target moves -X
    CHECK(q.target.x < 0 && NEAR(q.target.y, 0, 1e-9) && NEAR(q.target.z, 0, 1e-9));
    // Distance and angles are unchanged; only the target moved.
    CHECK(q.distance == p.distance && q.elevationDeg == p.elevationDeg);
}

static void ZoomFactors() {
    CHECK(ZoomFactorForDrag(-100) > 1.0 && ZoomFactorForDrag(100) < 1.0);   // up = in
    CHECK(NEAR(ZoomFactorForDrag(50) * ZoomFactorForDrag(-50), 1.0, 1e-12));  // reversible
    CHECK(NEAR(ZoomFactorForWheel(1) * ZoomFactorForWheel(-1), 1.0, 1e-12));
}

int main() {
    PanFollowsTheMouse();
    ZoomKeepsTheCentre();
    RegionZoomFitsTheRectangle();
    OrbitKeepsDistanceAndClamps();
    PerspectivePanMovesTarget();
    ZoomFactors();
    if (failures == 0) std::printf("navigation: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
