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

double ColumnF(double u, int w, int /*h*/, const OrthoView& v) {
    return (w - 1) / 2.0 + kOffsetX + (u - v.centerU) * ScaleX(w, v);
}

double RowF(double vv, int w, int h, const OrthoView& v) {
    return (h - 1) / 2.0 + kOffsetY - (vv - v.centerV) * ScaleY(w, h, v);
}

int ColumnOf(double u, int w, int h, const OrthoView& v) { return static_cast<int>(std::floor(ColumnF(u, w, h, v))); }
int RowOf(double vv, int w, int h, const OrthoView& v) { return static_cast<int>(std::floor(RowF(vv, w, h, v))); }

// Lines closer than this (in pixels) are skipped when zoomed far out; the axes are
// always drawn. NOTE: R3's exact thinning rule is not measured yet.
static constexpr double kMinLineGap = 4.0;

math::Mat4 OrthoWorldToClip(const math::Vec3& right, const math::Vec3& up, int w, int h, const OrthoView& v) {
    // ColumnF and RowF are linear: x = ax + bx * u, y = ay + by * v.
    const double ax = ColumnF(0, w, h, v), bx = ColumnF(1, w, h, v) - ax;
    const double ay = RowF(0, w, h, v), by = RowF(1, w, h, v) - ay;
    const math::Vec3 toward = math::Cross(right, up);          // out of the screen
    constexpr double kDepth = 1e5;                             // world units of depth range
    // clip x = 2 x / W - 1, clip y = 1 - 2 y / H (top-down rows), clip z = -(p . toward) / kDepth.
    const double sx = 2.0 / w, sy = -2.0 / h;
    math::Mat4 m;
    m.m[0][0] = sx * bx * right.x; m.m[1][0] = sx * bx * right.y; m.m[2][0] = sx * bx * right.z; m.m[3][0] = sx * ax - 1.0;
    m.m[0][1] = sy * by * up.x;    m.m[1][1] = sy * by * up.y;    m.m[2][1] = sy * by * up.z;    m.m[3][1] = sy * ay + 1.0;
    m.m[0][2] = -toward.x / kDepth; m.m[1][2] = -toward.y / kDepth; m.m[2][2] = -toward.z / kDepth; m.m[3][2] = 0.0;
    m.m[0][3] = 0; m.m[1][3] = 0; m.m[2][3] = 0; m.m[3][3] = 1;
    return m;
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

    // Which lines are far enough apart to draw: minor lines need kMinLineGap pixels
    // per spacing, major lines that much per 10 spacings.
    const double gap = kGridSpacing * ScaleX(w, v);
    const bool minors = gap >= kMinLineGap;
    const bool majors = gap * kMajorEvery >= kMinLineGap;
    auto wanted = [&](long long i) {
        const LineKind k = KindOf(i);
        return k == LineKind::Axis || (k == LineKind::Major && majors) || (k == LineKind::Minor && minors);
    };
    // Far out, step over whole groups instead of visiting millions of indices.
    const long long step = minors ? 1 : kMajorEvery;
    auto first = [&](long long i0) { return minors ? i0 : (i0 / kMajorEvery - 1) * kMajorEvery; };

    if (majors || minors) {
        for (long long i = first(u0); i <= u1; i += step) {
            if (!wanted(i)) continue;
            const int c = ColumnOf(i * kGridSpacing, w, h, v);
            if (c >= 0 && c < w) lines.push_back({true, c, KindOf(i)});
        }
        for (long long i = first(v0); i <= v1; i += step) {
            if (!wanted(i)) continue;
            const int r = RowOf(i * kGridSpacing, w, h, v);
            if (r >= 0 && r < h) lines.push_back({false, r, KindOf(i)});
        }
    }
    // The axes, if visible, even when everything else is too dense.
    if (!majors) {
        const int c = ColumnOf(0, w, h, v), r = RowOf(0, w, h, v);
        if (c >= 0 && c < w) lines.push_back({true, c, LineKind::Axis});
        if (r >= 0 && r < h) lines.push_back({false, r, LineKind::Axis});
    }
    return lines;
}

} // namespace view
