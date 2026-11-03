#include "view/OrthoGrid.h"
#include <cmath>

namespace view {

// Fitted to every grid line of the three ortho views in the reference screenshot
// (180 lines, all exact). R3 drew lines with its own software rasterizer; these
// constants reproduce its arithmetic:
//   - `width` world units span (W - 0.5) pixels across;
//   - the visible height is width * H / W world units over (H - 0.5) pixels;
//   - a line lands on floor(centre + offset + coordinate * scale), with the small
//     sub-pixel offsets below (they differ per axis because rows count downwards).
static constexpr double kOffsetX = 0.745;
static constexpr double kOffsetY = 0.22;

static double ScaleX(int w, const OrthoView& v) { return (w - 0.5) / v.width; }
static double ScaleY(int w, int h, const OrthoView& v) {
    return (h - 0.5) / (v.width * h / static_cast<double>(w));
}

int ColumnOf(double u, int w, int /*h*/, const OrthoView& v) {
    return static_cast<int>(std::floor((w - 1) / 2.0 + kOffsetX + (u - v.centerU) * ScaleX(w, v)));
}

int RowOf(double vv, int w, int h, const OrthoView& v) {
    return static_cast<int>(std::floor((h - 1) / 2.0 + kOffsetY - (vv - v.centerV) * ScaleY(w, h, v)));
}

static LineKind KindOf(long long index) {
    if (index == 0) return LineKind::Axis;
    return (index % kMajorEvery == 0) ? LineKind::Major : LineKind::Minor;
}

std::vector<GridLine> OrthoGridLines(int w, int h, const OrthoView& v) {
    std::vector<GridLine> lines;
    if (w <= 0 || h <= 0 || v.width <= 0) return lines;

    // World range visible in each direction, then every multiple of the spacing in it.
    const double halfU = v.width / 2.0 + kGridSpacing;
    const double halfV = v.width * h / static_cast<double>(w) / 2.0 + kGridSpacing;
    const long long u0 = static_cast<long long>(std::floor((v.centerU - halfU) / kGridSpacing));
    const long long u1 = static_cast<long long>(std::ceil((v.centerU + halfU) / kGridSpacing));
    const long long v0 = static_cast<long long>(std::floor((v.centerV - halfV) / kGridSpacing));
    const long long v1 = static_cast<long long>(std::ceil((v.centerV + halfV) / kGridSpacing));

    for (long long i = u0; i <= u1; ++i) {
        const int c = ColumnOf(i * kGridSpacing, w, h, v);
        if (c >= 0 && c < w) lines.push_back({true, c, KindOf(i)});
    }
    for (long long i = v0; i <= v1; ++i) {
        const int r = RowOf(i * kGridSpacing, w, h, v);
        if (r >= 0 && r < h) lines.push_back({false, r, KindOf(i)});
    }
    return lines;
}

} // namespace view
