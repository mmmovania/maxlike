#pragma once
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

// Pixel column of world coordinate `u` / pixel row (top-down) of world coordinate `v`.
int ColumnOf(double u, int width, int height, const OrthoView& view);
int RowOf(double v, int width, int height, const OrthoView& view);

// Every grid line visible in a `width` x `height` viewport.
std::vector<GridLine> OrthoGridLines(int width, int height, const OrthoView& view);

} // namespace view
