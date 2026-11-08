#pragma once
#include "math/Mat4.h"
#include <vector>

// The home grid of the orthographic views (Top, Front, Left), computed directly in
// pixels so every line lands on exactly the pixel R3 used. No Windows or OpenGL
// dependency (unit-tested in tests/test_ortho_grid.cpp).
namespace view {

enum class LineKind { Minor, Major, Axis };

struct GridLine {
    bool vertical;   // true: a column at `pixel`; false: a row at `pixel` (top-down)
    int pixel;
    LineKind kind;
};

// The 2D view of an orthographic viewport: which world units it shows.
struct OrthoView {
    double width = 400.0;   // world units across the viewport's width (R3's default)
    double centerU = 0.0;   // world coordinates at the viewport centre: U to the right,
    double centerV = 0.0;   // V up (Top: X/Y, Front: X/Z, Left: -Y/Z)
};

inline constexpr double kGridSpacing = 10.0;   // world units between lines
inline constexpr int    kMajorEvery  = 10;     // every 10th line is major

// Zoomed in, R3 subdivides the grid by tens (measured, r3.1-open-instances.png: its Left
// view, 10 units = 185 px, draws 1-unit lines 18.5 px apart; parent_child_link's views,
// where 1 unit would be 4.9 px, keep 10). The world spacing drawn: 10 units, divided by
// 10 while the finer lines would be at least kSubdivideGap px apart. NOTE: the
// threshold is only known to lie in (4.9, 18.5].
inline constexpr double kSubdivideGap = 10.0;
double OrthoGridSpacing(int w, const OrthoView& v);

// The minor lines' grey level: darker as they spread apart, from #A0A0A0 towards the
// major lines' #747474 (measured at six spacings, 18.5 to 48.6 px; floor(160 - 0.8674 g)
// reproduces all six, and at the default 21.6 px gives Phase 2's #8D8D8D).
int OrthoMinorShade(int w, const OrthoView& v);

// Pixel column of world coordinate `u` / pixel row (top-down) of world coordinate `v`.
int ColumnOf(double u, int width, int height, const OrthoView& view);
int RowOf(double v, int width, int height, const OrthoView& view);

// The same before rounding down: the pixel is floor() of these. Objects drawn in an
// ortho view go through this too, so their edges land on the grid's pixels.
double ColumnF(double u, int width, int height, const OrthoView& view);
double RowF(double v, int width, int height, const OrthoView& view);

// The inverse: the world coordinates (along the view's right and up) at the centre of
// pixel column `x` / row `y`. Used to turn mouse positions into points on the
// construction plane.
double UOfColumn(int x, int width, int height, const OrthoView& view);
double VOfRow(int y, int width, int height, const OrthoView& view);

// World -> clip space for an ortho view, consistent with ColumnF/RowF: a world point
// lands at window x = ColumnF(u), y = RowF(v) (top-down), where u = point . right and
// v = point . up. Depth: points nearer the viewer (larger point . (right x up)) get
// smaller clip z, so the depth test keeps the nearest surface.
math::Mat4 OrthoWorldToClip(const math::Vec3& right, const math::Vec3& up, int width, int height,
                            const OrthoView& view);

// Every grid line visible in a `width` x `height` viewport.
std::vector<GridLine> OrthoGridLines(int width, int height, const OrthoView& view);

} // namespace view
