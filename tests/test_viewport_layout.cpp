// Unit tests for the viewport-area geometry. Expected rectangles measured from
// the R3.1 reference: the area is 1736 x 858 (client y 57..914).
#include "ui/ViewportLayout.h"
#include <cstdio>
#include <initializer_list>

static int failures = 0;

static void Check(const char* what, const ui::Rect& got, const ui::Rect& want) {
    if (got == want) return;
    ++failures;
    std::printf("FAIL %s: got {%d,%d,%d,%d} want {%d,%d,%d,%d}\n", what,
                got.left, got.top, got.right, got.bottom, want.left, want.top, want.right, want.bottom);
}

using namespace ui::viewports;

static void ReferenceLayout() {
    const Layout L = ComputeViewportLayout(1736, 858, 96);
    // Outer frames: Top x 0..868, rows 1..429 (screen y 100..528), and so on.
    Check("TL outer", L.view[kTopLeft].outer,     {0,   1,   869,  430});
    Check("TR outer", L.view[kTopRight].outer,    {868, 1,   1736, 430});
    Check("BL outer", L.view[kBottomLeft].outer,  {0,   429, 869,  858});
    Check("BR outer", L.view[kBottomRight].outer, {868, 429, 1736, 858});
    // Content: Top's starts at x 3 and screen y 103, ends at x 865 and screen y 525.
    Check("TL content", L.view[kTopLeft].content,     {3,   4,   866,  427});
    Check("TR content", L.view[kTopRight].content,    {871, 4,   1733, 427});
    Check("BL content", L.view[kBottomLeft].content,  {3,   432, 866,  855});
    Check("BR content", L.view[kBottomRight].content, {871, 432, 1733, 855});
}

static void TinyAreaNeverNegative() {
    for (int w : {0, 1, 5, 40}) {
        for (int h : {0, 1, 5, 40}) {
            const Layout L = ComputeViewportLayout(w, h, 96);
            for (const Frame& f : L.view) {
                if (f.content.Width() < 0 || f.content.Height() < 0 ||
                    f.outer.Width() < 0 || f.outer.Height() < 0) {
                    ++failures;
                    std::printf("FAIL negative size at %dx%d\n", w, h);
                }
            }
        }
    }
}

static void DividersMove() {
    // Dragging the vertical divider to column 600 and back.
    Arrangement a;
    a.splitX = FractionForSplitX(600, 1736);
    int sx = 0, sy = 0;
    SplitLines(1736, 858, a, sx, sy);
    if (sx != 600) { ++failures; std::printf("FAIL split x: %d\n", sx); }
    a.splitY = FractionForSplitY(300, 858);
    SplitLines(1736, 858, a, sx, sy);
    if (sy != 300) { ++failures; std::printf("FAIL split y: %d\n", sy); }
    const Layout L = ComputeViewportLayout(1736, 858, 96, a);
    Check("TL after drag", L.view[kTopLeft].outer, {0, 1, 601, 301});
    // Clamped: every viewport keeps at least kMinViewport pixels of content.
    a.splitX = FractionForSplitX(2, 1736);
    const Layout C = ComputeViewportLayout(1736, 858, 96, a);
    if (C.view[kTopLeft].content.Width() < kMinViewport) { ++failures; std::printf("FAIL clamp\n"); }
}

static void Maximize() {
    Arrangement a;
    a.maximized = kBottomRight;
    const Layout L = ComputeViewportLayout(1736, 858, 96, a);
    Check("maximised outer", L.view[kBottomRight].outer, {0, 1, 1736, 858});
    Check("maximised content", L.view[kBottomRight].content, {3, 4, 1733, 855});
    Check("hidden", L.view[kTopLeft].content, {0, 0, 0, 0});
}

int main() {
    ReferenceLayout();
    DividersMove();
    Maximize();
    TinyAreaNeverNegative();
    if (failures == 0) std::printf("viewport layout: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
