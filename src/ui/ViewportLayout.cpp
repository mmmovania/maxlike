#include "ui/ViewportLayout.h"
#include <algorithm>

namespace ui::viewports {

Layout ComputeViewportLayout(int w, int h, unsigned /*dpi*/) {
    w = std::max(w, 0);
    h = std::max(h, 0);
    const int top = std::min(kTopLine, h);
    const int frameH = h - top;

    // The shared column/row (measured: column 868 of 1736, row 429 of the area).
    const int splitX = w / 2;
    const int splitY = top + frameH / 2;

    // Outer rectangles; neighbours overlap on the split line.
    const Rect outer[kCount] = {
        {0,      top,    splitX + 1, splitY + 1},
        {splitX, top,    w,          splitY + 1},
        {0,      splitY, splitX + 1, h},
        {splitX, splitY, w,          h},
    };

    Layout L;
    const int inset = kBorder + kRing;
    for (int i = 0; i < kCount; ++i) {
        L.view[i].outer = outer[i];
        Rect c{outer[i].left + inset, outer[i].top + inset, outer[i].right - inset, outer[i].bottom - inset};
        c.right = std::max(c.right, c.left);
        c.bottom = std::max(c.bottom, c.top);
        L.view[i].content = c;
    }
    return L;
}

} // namespace ui::viewports
