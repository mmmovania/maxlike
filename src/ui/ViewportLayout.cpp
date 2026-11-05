#include "ui/ViewportLayout.h"
#include <algorithm>

namespace ui::viewports {

// Range a divider may take so both sides keep kMinViewport pixels of content.
static int ClampSplit(int pos, int start, int end) {
    const int margin = kMinViewport + 2 * (kBorder + kRing);
    if (end - start < 2 * margin) return start + (end - start) / 2;
    return std::clamp(pos, start + margin, end - margin);
}

void SplitLines(int w, int h, const Arrangement& a, int& splitX, int& splitY) {
    w = std::max(w, 0);
    h = std::max(h, 0);
    const int top = std::min(kTopLine, h);
    const int frameH = h - top;
    // At 0.5 these are exactly the measured lines: column 868 of 1736, row 429 of 858.
    splitX = ClampSplit(static_cast<int>(a.splitX * w), 0, w);
    splitY = top + ClampSplit(static_cast<int>(a.splitY * frameH), 0, frameH);
}

double FractionForSplitX(int pos, int w) {
    if (w <= 0) return 0.5;
    return (ClampSplit(pos, 0, w) + 0.5) / w;          // + 0.5: lands back on `pos`
}

double FractionForSplitY(int pos, int h) {
    const int top = std::min(kTopLine, std::max(h, 0));
    const int frameH = h - top;
    if (frameH <= 0) return 0.5;
    return (ClampSplit(pos - top, 0, frameH) + 0.5) / frameH;
}

Layout ComputeViewportLayout(int w, int h, unsigned /*dpi*/, const Arrangement& a) {
    w = std::max(w, 0);
    h = std::max(h, 0);
    const int top = std::min(kTopLine, h);
    int splitX = 0, splitY = 0;
    SplitLines(w, h, a, splitX, splitY);

    // Outer rectangles; neighbours overlap on the split line.
    Rect outer[kCount] = {
        {0,      top,    splitX + 1, splitY + 1},
        {splitX, top,    w,          splitY + 1},
        {0,      splitY, splitX + 1, h},
        {splitX, splitY, w,          h},
    };
    if (a.maximized >= 0 && a.maximized < kCount) {
        for (int i = 0; i < kCount; ++i)
            outer[i] = (i == a.maximized) ? Rect{0, top, w, h} : Rect{0, 0, 0, 0};
    }

    Layout L;
    const int inset = kBorder + kRing;
    for (int i = 0; i < kCount; ++i) {
        L.view[i].outer = outer[i];
        if (outer[i].Width() == 0) { L.view[i].content = {0, 0, 0, 0}; continue; }
        Rect c{outer[i].left + inset, outer[i].top + inset, outer[i].right - inset, outer[i].bottom - inset};
        c.right = std::max(c.right, c.left);
        c.bottom = std::max(c.bottom, c.top);
        L.view[i].content = c;
    }
    return L;
}

} // namespace ui::viewports
