#include "ui/StatusLayout.h"
#include "ui/Metrics.h"
#include <algorithm>

namespace ui::statusbar {

Layout ComputeStatusLayout(int w, int /*h*/, int listenerWidth, unsigned dpi) {
    auto s = [dpi](int v) { return metrics::Scale(v, dpi); };
    auto fromRight = [&](int left, int right) { return std::pair<int, int>{w - s(left), w - s(right)}; };

    // Rows (measured): top fields y 4..24, bottom fields y 29..49 (21 high each);
    // listener boxes y 2..24 and 25..47 (23 high each).
    const int topY = s(4), botY = s(29), rowH = s(21);
    Layout L;

    // Right-anchored fields: offsets from the right edge at 100% (left, right).
    auto field = [&](int top, int left, int right) {
        const auto [l, r] = fromRight(left, right);
        return Rect{std::max(0, l), top, std::max(0, r), top + rowH};
    };
    L.lock  = field(topY, 620, 591);
    L.x     = field(topY, 591, 521);
    L.y     = field(topY, 519, 449);
    L.z     = field(topY, 447, 377);
    L.grid  = field(topY, 375, 275);
    L.timeTag = field(botY, 564, 464);
    // The two button groups (step 1.5c): fixed size, anchored to the right edge.
    L.snapTools = {std::max(0, w - s(464)), s(26), std::max(0, w - s(275)), s(50)};
    L.animTools = {std::max(0, w - s(275)), 0, w, s(50)};

    // Listener: x = 1, width clamped so the top prompt keeps at least 40 px.
    const int maxListener = std::max(0, (L.lock.left - s(1 + 2 + 4 + 2 + 40)) * 96 / static_cast<int>(dpi));
    const int lw = s(std::clamp(listenerWidth, 0, maxListener));
    const int lx = s(1);
    L.listenerTop    = {lx, s(2),  lx + lw, s(2) + s(23)};
    L.listenerBottom = {lx, s(25), lx + lw, s(25) + s(23)};

    // Splitter: 2 px after the listener, 4 px wide; prompts start 2 px after it.
    const int splitLeft = lx + lw + s(2);
    L.splitter = {splitLeft, 0, splitLeft + s(4), s(50)};
    const int promptLeft = L.splitter.right + s(2);
    L.promptTop    = {promptLeft, topY, std::max(promptLeft, L.lock.left), topY + rowH};
    L.promptBottom = {promptLeft, botY, std::max(promptLeft, w - s(566)), botY + rowH};
    return L;
}

} // namespace ui::statusbar
