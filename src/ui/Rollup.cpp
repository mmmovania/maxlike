#include "ui/Rollup.h"
#include <cmath>
#include <algorithm>

namespace ui::rollup {

std::vector<Placed> Layout(const std::vector<Item>& items, int scroll, const Box& container) {
    std::vector<Placed> out;
    int top = container.top + kFirstTitleGap - scroll;
    for (const Item& it : items) {
        Placed p{};
        p.open = it.open;
        p.titleTop = top;
        p.titleBottom = top + kTitleHeight - 1;
        p.frameTop = top + (it.open ? kFrameTopOffset : kClosedFrameTop);
        p.frameBottom = it.open ? p.titleBottom + it.contentHeight : top + kClosedFrameBottom;
        out.push_back(p);
        top = p.frameBottom + kGap;
    }
    return out;
}

int ContentHeight(const std::vector<Item>& items) {
    if (items.empty()) return 0;
    const std::vector<Placed> p = Layout(items, 0);
    return p.back().frameBottom - p.front().titleTop + 1;
}

int MaxScroll(const std::vector<Item>& items, const Box& container) {
    const int room = (container.bottom - 1) - (container.top + kFirstTitleGap) + 1 - kBottomMargin;
    return std::max(0, ContentHeight(items) - room);
}

int TitleAt(const std::vector<Item>& items, int scroll, int x, int y, const Box& container) {
    if (x < kTitleLeft || x > kTitleRight) return -1;
    const std::vector<Placed> p = Layout(items, scroll, container);
    for (size_t i = 0; i < p.size(); ++i)
        if (y >= p[i].titleTop && y <= p[i].titleBottom) return static_cast<int>(i);
    return -1;
}

Thumb ScrollThumb(const Box& k, int scroll, int maxScroll) {
    // H rows of indicator stand for H + maxScroll rows of column; the end rounds up.
    const int H = (k.bottom - 1) - (k.top + 1) + 1;
    const double C = H + maxScroll;
    return {k.top + 1 + static_cast<int>(std::floor(scroll * H / C)),
            k.top + 1 + static_cast<int>(std::ceil((scroll + H) * H / C)) - 1};
}

} // namespace ui::rollup
