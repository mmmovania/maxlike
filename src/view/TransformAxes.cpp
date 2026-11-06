#include "view/TransformAxes.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace view {

namespace {

// Same letters as the viewport tripod (view/Tripod.cpp).
const char* const kGlyphX[] = {"##....##", ".##..##.", "..####..", "...##...", "..####..", ".##..##.", "##....##"};
const char* const kGlyphY[] = {"##....##", "##....##", ".##..##.", ".##..##.", "..####..",
                               "..####..", "...##...", "...##...", "..##....", "###....."};
const char* const kGlyphZ[] = {".######", ".....##", "....##.", "...##..", "..##...", ".##....", ".######"};
struct Glyph { const char* const* rows; int count; };
const Glyph kGlyphs[3] = {{kGlyphX, 7}, {kGlyphY, 10}, {kGlyphZ, 7}};

int Round(double v) { return static_cast<int>(std::floor(v + 0.5)); }

void Line(int x0, int y0, int x1, int y1, uint32_t c, std::vector<ColoredPixel>& out) {
    const int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        out.push_back({x0, y0, c});
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void Letter(int axis, int tipX, int tipY, uint32_t c, std::vector<ColoredPixel>& out) {
    const Glyph& g = kGlyphs[axis];
    for (int r = 0; r < g.count; ++r)
        for (int col = 0; g.rows[r][col]; ++col)
            if (g.rows[r][col] == '#') out.push_back({tipX + col, tipY - 9 + r, c});
}

} // namespace

std::vector<ColoredPixel> TransformAxesPixels(AxesStyle style, const double s[3][2], int ox, int oy,
                                              const bool active[3]) {
    std::vector<ColoredPixel> out;
    // Screen direction of each axis in pixels (x right, y down) and its length on screen.
    double dx[3], dy[3], len[3];
    for (int a = 0; a < 3; ++a) { dx[a] = s[a][0]; dy[a] = -s[a][1]; len[a] = std::hypot(dx[a], dy[a]); }
    // Inactive axes first, so the active ones are drawn over them.
    const int order[6] = {0, 1, 2, 0, 1, 2};
    for (int pass = 0; pass < 2; ++pass)
        for (int i = 0; i < 3; ++i) {
            const int a = order[i];
            if (active[a] != (pass == 1)) continue;
            if (style == AxesStyle::Tripod) {
                const uint32_t c = active[a] ? kTripodActive : kTripodInactive;
                const int ex = ox + Round(84 * dx[a]), ey = oy + Round(84 * dy[a]);
                if (len[a] > 0.1) {
                    Line(ox, oy, ex, ey, c, out);
                    // Open arrowhead: two strokes back from the tip (measured on X: to (-8, -8)
                    // and (-8, +9) of the tip; ±8 used for every direction).
                    const double ux = dx[a] / len[a], uy = dy[a] / len[a];
                    Line(ex, ey, Round(ex - 8 * ux + 8 * uy), Round(ey - 8 * uy - 8 * ux), c, out);
                    Line(ex, ey, Round(ex - 8 * ux - 8 * uy), Round(ey - 8 * uy + 8 * ux), c, out);
                }
                Letter(a, ex, ey, c, out);
                continue;
            }
            // Move gizmo: axes pointing at the viewer are not drawn. Positions in screen
            // pixels (y down), rounded half up (measured, r3.1-move-selected.png).
            if (len[a] < 0.1) continue;
            const double ux = dx[a], uy = dy[a];   // keeps perspective foreshortening
            Line(ox + Round(17 * ux), oy + Round(17 * uy), ox + Round(60 * ux), oy + Round(60 * uy),
                 active[a] ? kGizmoActive : kAxisColor[a], out);
            // Filled arrowhead: a triangle (tip 84.5 px, base 62 px, half-width 3.75 px,
            // from a pivot half a pixel down), filling pixels whose centres are inside.
            // Fitted to six measured heads; NOTE: about 10 % of each head's edge pixels
            // still differ from R3's own rasteriser.
            {
                const double nx = -uy / len[a], ny = ux / len[a];
                const double px0 = ox, py0 = oy + 0.5;
                const double T[2] = {px0 + 84.5 * ux, py0 + 84.5 * uy};
                const double B1[2] = {px0 + 62 * ux + 3.75 * nx, py0 + 62 * uy + 3.75 * ny};
                const double B2[2] = {px0 + 62 * ux - 3.75 * nx, py0 + 62 * uy - 3.75 * ny};
                auto edge = [](const double* P, const double* Q, double x, double y) {
                    return (Q[0] - P[0]) * (y - P[1]) - (Q[1] - P[1]) * (x - P[0]);
                };
                const int x0 = static_cast<int>(std::floor(std::min({T[0], B1[0], B2[0]}))) - 1;
                const int x1 = static_cast<int>(std::ceil(std::max({T[0], B1[0], B2[0]}))) + 1;
                const int y0 = static_cast<int>(std::floor(std::min({T[1], B1[1], B2[1]}))) - 1;
                const int y1 = static_cast<int>(std::ceil(std::max({T[1], B1[1], B2[1]}))) + 1;
                for (int y = y0; y <= y1; ++y)
                    for (int x = x0; x <= x1; ++x) {
                        const double cx = x + 0.5, cy = y + 0.5;
                        const double e1 = edge(T, B1, cx, cy), e2 = edge(B1, B2, cx, cy), e3 = edge(B2, T, cx, cy);
                        if ((e1 >= 0 && e2 >= 0 && e3 >= 0) || (e1 <= 0 && e2 <= 0 && e3 <= 0))
                            out.push_back({x, y, kAxisColor[a]});
                    }
            }
            Letter(a, ox + Round(84.5 * ux), oy + Round(84.5 * uy), active[a] ? kGizmoActive : kAxisColor[a], out);
        }
    if (style == AxesStyle::MoveGizmo) {
        // Plane corners for each pair of visible axes.
        for (int a = 0; a < 3; ++a)
            for (int b = a + 1; b < 3; ++b) {
                if (len[a] < 0.1 || len[b] < 0.1) continue;
                const bool plane = active[a] && active[b];
                // Measured (Top, XY): 84 px along both axes from the pivot.
                const int cx = ox + Round(84 * (dx[a] + dx[b])), cy = oy + Round(84 * (dy[a] + dy[b]));
                const uint32_t armA = plane ? kGizmoActive : kAxisColor[b];   // the arm parallel to axis a
                const uint32_t armB = plane ? kGizmoActive : kAxisColor[a];
                Line(cx, cy, cx - Round(17 * dx[a] / len[a]), cy - Round(17 * dy[a] / len[a]), armA, out);
                Line(cx, cy, cx - Round(17 * dx[b] / len[b]), cy - Round(17 * dy[b] / len[b]), armB, out);
            }
    }
    return out;
}

} // namespace view
