#pragma once
#include "ui/FrameLayout.h" // ui::Rect

// Geometry of the 2x2 viewport area, with no Windows dependency (unit-tested in
// tests/test_viewport_layout.cpp). Measured from the reference at 100%:
//
//   row 0 of the area      : a 1-px line (highlight at x 0, shadow elsewhere)
//   each viewport (outer)  : 2-px border (white if active, else black),
//                            then a 1-px #C0C0C0 ring, then the content
//   neighbours             : share one border column/row; the active viewport is
//                            drawn last, so its colour wins on the shared line
namespace ui::viewports {

inline constexpr int kTopLine = 1;     // the line above the viewports
inline constexpr int kBorder  = 2;     // white / black border width
inline constexpr int kRing    = 1;     // #C0C0C0 ring inside the border

enum Index { kTopLeft = 0, kTopRight = 1, kBottomLeft = 2, kBottomRight = 3, kCount = 4 };

struct Frame {
    Rect outer;    // border included
    Rect content;  // where the view is drawn (an OpenGL window)
};

struct Layout {
    Frame view[kCount];
};

// `w` x `h` is the whole viewport area. Lines stay 1 px thick at every DPI, like
// the rest of the classic frame, so `dpi` only matters for future gaps.
Layout ComputeViewportLayout(int w, int h, unsigned dpi);

} // namespace ui::viewports
