// Unit tests for the ortho grid: every line of the three ortho views in the R3.1
// reference screenshot (measured along a column and a row that cross no other line).
#include "view/OrthoGrid.h"
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <iterator>
#include <map>

static int failures = 0;
enum K { m, M, A };
struct Expect { int pixel; K kind; };

static view::LineKind ToKind(K k) {
    return k == m ? view::LineKind::Minor : k == M ? view::LineKind::Major : view::LineKind::Axis;
}

static void Check(const char* name, int w, int h, const Expect* cols, size_t nc, const Expect* rows, size_t nr) {
    std::map<int, view::LineKind> gotCols, gotRows;
    for (const view::GridLine& l : view::OrthoGridLines(w, h, view::OrthoView{}))
        (l.vertical ? gotCols : gotRows)[l.pixel] = l.kind;
    auto compare = [&](const char* what, const std::map<int, view::LineKind>& got, const Expect* e, size_t n) {
        if (got.size() != n) { ++failures; std::printf("FAIL %s %s: %zu lines, want %zu\n", name, what, got.size(), n); }
        for (size_t i = 0; i < n; ++i) {
            auto it = got.find(e[i].pixel);
            if (it == got.end() || it->second != ToKind(e[i].kind)) {
                ++failures;
                std::printf("FAIL %s %s: line at %d missing or wrong kind\n", name, what, e[i].pixel);
            }
        }
    };
    compare("columns", gotCols, cols, nc);
    compare("rows", gotRows, rows, nr);
}

static void PanAndZoom() {
    // Panning by one grid spacing moves the axis line by about one grid cell.
    view::OrthoView v;
    const int axis = view::ColumnOf(0, 863, 423, v);
    v.centerU = 10;
    const int moved = view::ColumnOf(0, 863, 423, v);
    if (!(axis - moved >= 21 && axis - moved <= 22)) { ++failures; std::printf("FAIL pan: %d -> %d\n", axis, moved); }
    // Zooming far out thins the grid: never more lines than pixels, axes still there.
    v = {}; v.width = 1e7;
    {
        const auto far = view::OrthoGridLines(863, 423, v);
        int axes = 0;
        for (const view::GridLine& l : far) axes += l.kind == view::LineKind::Axis;
        if (axes != 2 || far.size() > 863 + 423) { ++failures; std::printf("FAIL thinning: %zu lines, %d axes\n", far.size(), axes); }
    }
    // Zooming out (wider view) never produces lines outside the viewport.
    v = {}; v.width = 100000;
    for (const view::GridLine& l : view::OrthoGridLines(500, 300, v))
        if (l.pixel < 0 || l.pixel >= (l.vertical ? 500 : 300)) { ++failures; std::printf("FAIL out of range\n"); break; }
}

static void WorldToClipMatchesPixels() {
    // Front view: right = +X, up = +Z. A world point must land where ColumnF/RowF say.
    const math::Vec3 right{1, 0, 0}, up{0, 0, 1};
    view::OrthoView v;
    v.centerU = 12; v.centerV = -7; v.width = 333;
    const int w = 862, h = 423;
    const math::Mat4 m = view::OrthoWorldToClip(right, up, w, h, v);
    for (const math::Vec3& p : {math::Vec3{0, 0, 0}, math::Vec3{-90, 40, 55}, math::Vec3{130, -5, -60}}) {
        const math::Vec4 c = m * math::Vec4{p.x, p.y, p.z, 1};
        const double x = (c.x + 1) / 2 * w, y = (1 - c.y) / 2 * h;
        if (std::fabs(x - view::ColumnF(p.x, w, h, v)) > 1e-9 || std::fabs(y - view::RowF(p.z, w, h, v)) > 1e-9) {
            ++failures; std::printf("FAIL world to clip at (%g %g %g)\n", p.x, p.y, p.z);
        }
    }
    // Nearer the viewer (Front looks along +Y, so smaller y is nearer) = smaller depth.
    const math::Vec4 nearP = m * math::Vec4{0, -50, 0, 1}, farP = m * math::Vec4{0, 50, 0, 1};
    if (!(nearP.z < farP.z)) { ++failures; std::printf("FAIL depth order\n"); }
}

int main() {
    WorldToClipMatchesPixels();
    { // Top
        const Expect cols[] = {{0,M},{22,m},{43,m},{65,m},{86,m},{108,m},{129,m},{151,m},{172,m},{194,m},{216,M},{237,m},{259,m},{280,m},{302,m},{323,m},{345,m},{367,m},{388,m},{410,m},{431,A},{453,m},{474,m},{496,m},{517,m},{539,m},{561,m},{582,m},{604,m},{625,m},{647,M},{668,m},{690,m},{712,m},{733,m},{755,m},{776,m},{798,m},{819,m},{841,m},{862,M}};
        const Expect rows[] = {{17,m},{38,m},{60,m},{81,m},{103,m},{125,m},{146,m},{168,m},{189,m},{211,A},{232,m},{254,m},{275,m},{297,m},{318,m},{340,m},{362,m},{383,m},{405,m}};
        Check("Top", 863, 423, cols, std::size(cols), rows, std::size(rows));
    }
    { // Front
        const Expect cols[] = {{0,M},{22,m},{43,m},{65,m},{86,m},{108,m},{129,m},{151,m},{172,m},{194,m},{215,M},{237,m},{258,m},{280,m},{302,m},{323,m},{345,m},{366,m},{388,m},{409,m},{431,A},{452,m},{474,m},{495,m},{517,m},{538,m},{560,m},{582,m},{603,m},{625,m},{646,M},{668,m},{689,m},{711,m},{732,m},{754,m},{775,m},{797,m},{818,m},{840,m},{861,M}};
        const Expect rows[] = {{17,m},{39,m},{60,m},{82,m},{103,m},{125,m},{146,m},{168,m},{189,m},{211,A},{232,m},{254,m},{275,m},{297,m},{318,m},{340,m},{361,m},{383,m},{404,m}};
        Check("Front", 862, 423, cols, std::size(cols), rows, std::size(rows));
    }
    { // Left
        const Expect cols[] = {{0,M},{22,m},{43,m},{65,m},{86,m},{108,m},{129,m},{151,m},{172,m},{194,m},{216,M},{237,m},{259,m},{280,m},{302,m},{323,m},{345,m},{367,m},{388,m},{410,m},{431,A},{453,m},{474,m},{496,m},{517,m},{539,m},{561,m},{582,m},{604,m},{625,m},{647,M},{668,m},{690,m},{712,m},{733,m},{755,m},{776,m},{798,m},{819,m},{841,m},{862,M}};
        const Expect rows[] = {{17,m},{38,m},{60,m},{81,m},{103,m},{125,m},{146,m},{168,m},{189,m},{211,A},{232,m},{254,m},{275,m},{297,m},{318,m},{340,m},{362,m},{383,m},{405,m}};
        Check("Left", 863, 423, cols, std::size(cols), rows, std::size(rows));
    }
    PanAndZoom();
    if (failures == 0) std::printf("ortho grid: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
