#include "view/Tripod.h"
#include <cmath>
#include <cstdlib>

namespace view {

// Measured from the reference (all four views):
//   origin 25 px from the left and from the bottom;
//   each axis ends at origin + round(24.5 * screen direction) (rounded in a y-up frame);
//   a line from the origin to the end, inclusive;
//   the axis letter's top-left corner is 9 px above the end point.
static constexpr int kOriginInset = 25;
static constexpr double kLength = 24.5;
static constexpr int kLetterRaise = 9;

// The letters, as measured ('#' = set). 'y' has a descender; 'z' has an empty first column.
static const char* const kGlyphX[] = {"##....##", ".##..##.", "..####..", "...##...",
                                      "..####..", ".##..##.", "##....##"};
static const char* const kGlyphY[] = {"##....##", "##....##", ".##..##.", ".##..##.", "..####..",
                                      "..####..", "...##...", "...##...", "..##....", "###....."};
static const char* const kGlyphZ[] = {".######", ".....##", "....##.", "...##..",
                                      "..##...", ".##....", ".######"};

struct Glyph { const char* const* rows; int count; };
static const Glyph kGlyphs[3] = {{kGlyphX, 7}, {kGlyphY, 10}, {kGlyphZ, 7}};

static int RoundHalfUp(double v) { return static_cast<int>(std::floor(v + 0.5)); }

// Bresenham's line: every pixel from (x0,y0) to (x1,y1), inclusive.
static void Line(int x0, int y0, int x1, int y1, int axis, std::vector<TripodPixel>& out) {
    const int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        out.push_back({x0, y0, axis});
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

std::vector<TripodPixel> TripodPixels(const double screenAxes[3][2], int viewHeight) {
    std::vector<TripodPixel> out;
    const int ox = kOriginInset, oy = viewHeight - kOriginInset;
    for (int a = 0; a < 3; ++a) {
        const int ex = ox + RoundHalfUp(kLength * screenAxes[a][0]);
        const int ey = oy - RoundHalfUp(kLength * screenAxes[a][1]); // up = fewer rows
        Line(ox, oy, ex, ey, a, out);
        const Glyph& g = kGlyphs[a];
        for (int r = 0; r < g.count; ++r)
            for (int c = 0; g.rows[r][c]; ++c)
                if (g.rows[r][c] == '#') out.push_back({ex + c, ey - kLetterRaise + r, a});
    }
    return out;
}

} // namespace view
