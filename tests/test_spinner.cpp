// Unit tests for the spinner's behaviour.
#include "ui/SpinnerModel.h"
#include <cmath>
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
static bool Near(double a, double b) { return std::fabs(a - b) < 1e-9; }

using ui::SpinnerModel;

int main() {
    // Limits and stepping.
    SpinnerModel h(25, -1000, 1000, 0.5);
    h.StepBy(2);
    CHECK(Near(h.Value(), 26));
    h.Set(5000);
    CHECK(Near(h.Value(), 1000));                 // clamped
    // Dragging: one change from start to end; Ctrl 10x, Alt 1/10.
    SpinnerModel d(10, 0, 100, 0.1);
    d.BeginDrag();
    d.DragTo(30, false, false);
    CHECK(d.Dragging() && Near(d.Value(), 13));
    d.DragTo(30, true, false);
    CHECK(Near(d.Value(), 40));                   // total movement, not accumulated steps
    d.DragTo(30, false, true);
    CHECK(Near(d.Value(), 10.3));
    d.EndDrag();
    CHECK(!d.Dragging() && Near(d.Value(), 10.3));
    // Right-click during a drag cancels it.
    d.BeginDrag();
    d.DragTo(-50, false, false);
    d.CancelDrag();
    CHECK(Near(d.Value(), 10.3));
    // Right-click resets to 0, or the nearest limit when 0 is not allowed.
    d.Reset();
    CHECK(Near(d.Value(), 0));
    SpinnerModel segs(5, 1, 200, 1, true);
    segs.Reset();
    CHECK(Near(segs.Value(), 1));
    // Integers round; text shows no decimals.
    segs.Set(7.6);
    CHECK(Near(segs.Value(), 8) && segs.Text() == L"8");
    // Text: one decimal by default; "-0.0" shows as "0.0".
    SpinnerModel r(-0.04, -10, 10, 0.1);
    CHECK(r.Text() == L"0.0");
    r.Set(12.345);
    CHECK(r.Text() == L"10.0");
    // Typed entry: numbers accepted (with spaces), anything else ignored.
    CHECK(r.SetText(L"  3.25 ") && Near(r.Value(), 3.25) && r.Text() == L"3.3");
    CHECK(!r.SetText(L"abc") && !r.SetText(L"4x") && !r.SetText(L"") && Near(r.Value(), 3.25));
    CHECK(r.SetText(L"-99") && Near(r.Value(), -10));   // clamped
    if (failures == 0) std::printf("spinner: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
