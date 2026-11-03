// Unit tests for the status-area layout. Expected rectangles measured from the
// R3.1 reference screenshot (status area 1920 x 50, 100%).
#include "ui/StatusLayout.h"
#include <cstdio>
#include <initializer_list>

static int failures = 0;

static void Check(const char* what, const ui::Rect& got, const ui::Rect& want) {
    if (got == want) return;
    ++failures;
    std::printf("FAIL %s: got {%d,%d,%d,%d} want {%d,%d,%d,%d}\n", what,
                got.left, got.top, got.right, got.bottom, want.left, want.top, want.right, want.bottom);
}

using namespace ui::statusbar;

static void ReferenceLayout() {
    const Layout L = ComputeStatusLayout(1920, 50, kDefaultListenerWidth, 96);
    Check("listenerTop",    L.listenerTop,    {1, 2, 149, 25});
    Check("listenerBottom", L.listenerBottom, {1, 25, 149, 48});
    Check("splitter",       L.splitter,       {151, 0, 155, 50});
    Check("promptTop",      L.promptTop,      {157, 4, 1300, 25});
    Check("promptBottom",   L.promptBottom,   {157, 29, 1354, 50});
    Check("lock",           L.lock,           {1300, 4, 1329, 25});
    Check("x",              L.x,              {1329, 4, 1399, 25});
    Check("y",              L.y,              {1401, 4, 1471, 25});
    Check("z",              L.z,              {1473, 4, 1543, 25});
    Check("grid",           L.grid,           {1545, 4, 1645, 25});
    Check("spare",          L.spare,          {1356, 29, 1456, 50});
}

static void ListenerResizes() {
    const Layout L = ComputeStatusLayout(1920, 50, 300, 96);
    Check("wider listener", L.listenerTop, {1, 2, 301, 25});
    if (L.promptTop.left != 309) { ++failures; std::printf("FAIL prompt follows splitter\n"); }
    // Clamped: the prompt line keeps at least 40 px.
    const Layout C = ComputeStatusLayout(1920, 50, 5000, 96);
    if (C.promptTop.right - C.promptTop.left < 40) { ++failures; std::printf("FAIL clamp\n"); }
}

static void NarrowWindowNeverNegative() {
    for (int w : {0, 50, 300, 700}) {
        const Layout L = ComputeStatusLayout(w, 50, kDefaultListenerWidth, 96);
        for (const ui::Rect* r : {&L.listenerTop, &L.listenerBottom, &L.splitter, &L.promptTop,
                                  &L.promptBottom, &L.lock, &L.x, &L.y, &L.z, &L.grid, &L.spare})
            if (r->Width() < 0 || r->Height() < 0) { ++failures; std::printf("FAIL negative at w=%d\n", w); }
    }
}

int main() {
    ReferenceLayout();
    ListenerResizes();
    NarrowWindowNeverNegative();
    if (failures == 0) std::printf("status layout: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
