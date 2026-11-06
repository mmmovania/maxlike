#include "ui/Rollup.h"
#include <algorithm>

namespace ui::rollup {

std::vector<Placed> Layout(const std::vector<Item>& items, int scroll) {
    std::vector<Placed> out;
    int top = kContainer.top + kFirstTitleGap - scroll;
    for (const Item& it : items) {
        Placed p{};
        p.open = it.open;
        p.titleTop = top;
        p.titleBottom = top + kTitleHeight - 1;
        p.frameTop = top + kFrameTopOffset;
        p.frameBottom = it.open ? p.titleBottom + it.contentHeight : p.titleBottom;
        out.push_back(p);
        // NOTE: the spacing after a collapsed rollout is not measured; the same gap is used.
        top = p.frameBottom + kGap;
    }
    return out;
}

int ContentHeight(const std::vector<Item>& items) {
    if (items.empty()) return 0;
    const std::vector<Placed> p = Layout(items, 0);
    return p.back().frameBottom - p.front().titleTop + 1;
}

int MaxScroll(const std::vector<Item>& items) {
    const int room = (kContainer.bottom - 1) - (kContainer.top + kFirstTitleGap) + 1 - kBottomMargin;
    return std::max(0, ContentHeight(items) - room);
}

int TitleAt(const std::vector<Item>& items, int scroll, int x, int y) {
    if (x < kTitleLeft || x > kTitleRight) return -1;
    const std::vector<Placed> p = Layout(items, scroll);
    for (size_t i = 0; i < p.size(); ++i)
        if (y >= p[i].titleTop && y <= p[i].titleBottom) return static_cast<int>(i);
    return -1;
}

} // namespace ui::rollup
