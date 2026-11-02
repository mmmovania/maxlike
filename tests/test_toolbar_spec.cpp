// Unit tests for the main toolbar description and layout. Expected x positions
// were measured from the R3.1 reference screenshot (100%).
#include "ui/ToolbarSpec.h"
#include <cstdio>
#include <map>
#include <set>
#include <string>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

using namespace ui::toolbar;

static void PositionsMatchReference() {
    const auto& items = MainToolbarSpec();
    const auto rects = LayoutToolbar(items, 38, 96);
    // Left edges of buttons, combos and separator lines, left to right.
    const int buttons[] = {4, 42, 74, 112, 144, 182, 220, 252, 362, 400, 432, 464, 572, 610,
                           642, 674, 706, 738, 776, 808, 840, 998, 1030, 1068, 1100, 1132, 1236};
    const int combos[]  = {286, 502, 872, 1166};
    const int sepLines[] = {39, 109, 179, 217, 359, 397, 499, 607, 773, 995, 1065};
    size_t b = 0, c = 0, sp = 0;
    for (size_t i = 0; i < items.size(); ++i) {
        switch (items[i].kind) {
        case Kind::Button:
            CHECK(b < std::size(buttons) && rects[i].left == buttons[b]);
            CHECK(rects[i].top == 4 && rects[i].Width() == 32 && rects[i].Height() == 31);
            ++b; break;
        case Kind::Combo:
            CHECK(c < std::size(combos) && rects[i].left == combos[c]);
            ++c; break;
        case Kind::Separator:
            CHECK(sp < std::size(sepLines) && rects[i].left + 3 == sepLines[sp]);
            ++sp; break;
        case Kind::Gap: break;
        }
    }
    CHECK(b == std::size(buttons));
    CHECK(c == std::size(combos));
    CHECK(sp == std::size(sepLines));
    CHECK(rects.back().right == 1268); // Render Last ends at x=1267
}

static void ItemsAreWellFormed() {
    std::set<std::string> commands;
    std::map<std::string, int> checkedPerGroup;
    std::set<int> icons;
    for (const Item& it : MainToolbarSpec()) {
        if (it.kind == Kind::Button) {
            CHECK(!it.command.empty());
            CHECK(commands.insert(it.command).second);   // unique
            CHECK(it.icon >= 0 && it.icon < kIconCount);
            CHECK(icons.insert(it.icon).second);         // each icon used once
            CHECK(!it.tooltip.empty());
            if (!it.group.empty()) checkedPerGroup[it.group] += it.checked ? 1 : 0;
            else CHECK(!it.checked);
        }
        if (it.kind == Kind::Combo) CHECK(it.width > 0 && !it.command.empty());
    }
    CHECK(static_cast<int>(icons.size()) == kIconCount);   // every icon used
    CHECK(checkedPerGroup.size() == 2);                     // "mode" and "axis"
    for (const auto& [group, n] : checkedPerGroup) CHECK(n == 1);
}

static void ScalesWithoutGaps() {
    const auto& items = MainToolbarSpec();
    const auto rects = LayoutToolbar(items, 57, 144);
    for (size_t i = 1; i < rects.size(); ++i)
        CHECK(rects[i].left == rects[i - 1].right);
}

int main() {
    PositionsMatchReference();
    ItemsAreWellFormed();
    ScalesWithoutGaps();
    if (failures == 0) std::printf("toolbar spec: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
