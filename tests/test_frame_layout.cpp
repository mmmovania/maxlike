// Unit tests for ui::ComputeFrameLayout. Plain asserts, no framework.
// Expected rectangles at 100% were measured from the R3.1 reference screenshot:
// a maximised 1920x1040 window has a 1920x998 client area below the menu bar.
#include "ui/FrameLayout.h"
#include <cstdio>
#include <initializer_list>

static int failures = 0;

static void Check(const char* what, const ui::Rect& got, const ui::Rect& want) {
    if (got == want) return;
    ++failures;
    std::printf("FAIL %s: got {%d,%d,%d,%d} want {%d,%d,%d,%d}\n", what,
                got.left, got.top, got.right, got.bottom,
                want.left, want.top, want.right, want.bottom);
}

static void CheckNonNegative(const char* what, const ui::Rect& r) {
    if (r.Width() >= 0 && r.Height() >= 0) return;
    ++failures;
    std::printf("FAIL %s: negative size %dx%d\n", what, r.Width(), r.Height());
}

static void ReferenceLayoutAt100Percent() {
    const auto L = ui::ComputeFrameLayout(1920, 998, 96);
    Check("tabPanel",     L.tabPanel,     {0,    4,   1920, 19});
    Check("mainToolbar",  L.mainToolbar,  {0,    19,  1920, 57});
    Check("viewports",    L.viewports,    {0,    57,  1736, 915});
    Check("timeSlider",   L.timeSlider,   {0,    915, 1736, 932});
    Check("trackBar",     L.trackBar,     {0,    932, 1736, 948});
    Check("commandPanel", L.commandPanel, {1736, 57,  1920, 948});
    Check("statusArea",   L.statusArea,   {0,    948, 1920, 998});
}

static void ScalesAt150Percent() {
    const auto L = ui::ComputeFrameLayout(2880, 1497, 144);
    // 4*1.5 = 6; 15*1.5 = 22.5 rounds to 23 -> toolbar starts at 29; 38*1.5 = 57 -> body at 86.
    Check("150% toolbar", L.mainToolbar,  {0, 29, 2880, 86});
    // 184*1.5 = 276 -> panel starts at 2604; 50*1.5 = 75 -> status at 1422.
    Check("150% command", L.commandPanel, {2604, 86, 2880, 1422});
}

static void TinyWindowNeverNegative() {
    for (int w : {0, 1, 50, 200}) {
        for (int h : {0, 1, 30, 100}) {
            const auto L = ui::ComputeFrameLayout(w, h, 96);
            CheckNonNegative("tabPanel", L.tabPanel);
            CheckNonNegative("mainToolbar", L.mainToolbar);
            CheckNonNegative("viewports", L.viewports);
            CheckNonNegative("timeSlider", L.timeSlider);
            CheckNonNegative("trackBar", L.trackBar);
            CheckNonNegative("commandPanel", L.commandPanel);
            CheckNonNegative("statusArea", L.statusArea);
        }
    }
}

int main() {
    ReferenceLayoutAt100Percent();
    ScalesAt150Percent();
    TinyWindowNeverNegative();
    if (failures == 0) std::printf("frame layout: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
