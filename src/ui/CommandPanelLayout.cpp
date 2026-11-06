#include "ui/CommandPanelLayout.h"

namespace ui::cmdpanel {

Box TabBox(int tab, bool selected) {
    const int left = kFirstTabLeft + tab * kTabWidth;
    const int right = left + kTabWidth - 1;
    if (selected) return {left - 2, kSelectedTabTop, right + 2, kBodyTop + 1};
    return {left, kTabTop, right, kTabBottom};
}

int IconLeft(int tab) { return kFirstTabLeft + tab * kTabWidth + 3; }

int TabAt(int x, int y, int selected) {
    auto inside = [&](const Box& b) { return x >= b.left && x <= b.right && y >= b.top && y <= b.bottom; };
    if (selected >= 0 && selected < kTabCount && inside(TabBox(selected, true))) return selected;
    for (int t = 0; t < kTabCount; ++t)
        if (inside(TabBox(t, false))) return t;
    return -1;
}

} // namespace ui::cmdpanel
