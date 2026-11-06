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
        case Kind::Gap:
        case Kind::Edit: break;
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
        if (it.kind == Kind::Button && !it.flyout.empty()) {
            // Flyout: at least two choices; the button starts as the first one.
            CHECK(it.flyout.size() >= 2);
            CHECK(it.command == it.flyout[0].command);
            CHECK(it.icon == it.flyout[0].icon && it.tooltip == it.flyout[0].tooltip);
            for (const Alternative& a : it.flyout) {
                CHECK(!a.command.empty() && !a.tooltip.empty());
                CHECK(commands.insert(a.command).second);  // unique everywhere
                CHECK(a.icon >= 0 && a.icon < kIconCount);
                CHECK(icons.insert(a.icon).second);        // each icon used once
            }
        }
        if (it.kind == Kind::Button) {
            CHECK(!it.command.empty());
            if (it.flyout.empty()) {
                CHECK(commands.insert(it.command).second);   // unique
                CHECK(it.icon >= 0 && it.icon < kIconCount);
                CHECK(icons.insert(it.icon).second);         // each icon used once
            }
            CHECK(!it.tooltip.empty());
            if (!it.group.empty()) checkedPerGroup[it.group] += it.checked ? 1 : 0;
            else CHECK(!it.checked);
        }
        if (it.kind == Kind::Combo) {
            CHECK(it.width > 0 && !it.command.empty() && !it.tooltip.empty());
            if (it.editable) {
                CHECK(it.choices.empty() && it.text.empty());   // starts empty
            } else {
                // The initial text must be one of the entries, and entries are unique.
                std::set<std::wstring> unique(it.choices.begin(), it.choices.end());
                CHECK(unique.size() == it.choices.size());
                CHECK(unique.count(it.text) == 1);
            }
        }
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

static void FlyoutsMatchReference() {
    // The seven buttons with a corner notch in the reference screenshot.
    int flyouts = 0;
    for (const Item& it : MainToolbarSpec())
        if (!it.flyout.empty()) ++flyouts;
    CHECK(flyouts == 7);
}

// Status-area groups: every item inside its group, buttons never overlap, the
// status icons are each used exactly once, commands unique across all toolbars.
static void StatusGroupsAreWellFormed() {
    const Config groups[] = {StatusSnapConfig(), StatusAnimConfig()};
    const int widths[]  = {kStatusSnapWidth, kStatusAnimWidth};
    const int heights[] = {kStatusSnapHeight, 50};
    std::set<int> icons;
    std::set<std::string> commands;
    for (const Item& it : MainToolbarSpec()) {           // main toolbar commands
        if (it.flyout.empty()) commands.insert(it.command);
        for (const Alternative& a : it.flyout) commands.insert(a.command);
    }
    for (int g = 0; g < 2; ++g) {
        const Config& c = groups[g];
        CHECK(c.explicitLayout && c.icons == IconStrip::Status && !c.frameEdges && !c.hasStrips);
        const auto rects = LayoutItems(c, widths[g], heights[g], 96);
        for (size_t i = 0; i < c.items.size(); ++i) {
            const Item& it = c.items[i];
            const ui::Rect& r = rects[i];
            CHECK(r.left >= 0 && r.top >= 0 && r.right <= widths[g] && r.bottom <= heights[g]);
            if (it.kind == Kind::Button) {
                auto add = [&](const std::string& cmd, int icon) {
                    CHECK(commands.insert(cmd).second);
                    if (icon >= 0) { CHECK(icon < kStatusIconCount); CHECK(icons.insert(icon).second); }
                };
                if (it.flyout.empty()) add(it.command, it.icon);
                for (const Alternative& a : it.flyout) add(a.command, a.icon);
                CHECK(it.icon >= 0 || !it.text.empty());   // icon or a text label
                for (size_t j = 0; j < i; ++j) {            // no overlapping buttons
                    if (c.items[j].kind != Kind::Button) continue;
                    const ui::Rect& o = rects[j];
                    CHECK(r.right <= o.left || o.right <= r.left || r.bottom <= o.top || o.bottom <= r.top);
                }
            }
            if (it.kind == Kind::Edit) CHECK(commands.insert(it.command).second);
        }
    }
    CHECK(static_cast<int>(icons.size()) == kStatusIconCount);
}

static void StatusMatchesReference() {
    // Measured (status-area coordinates in the 1920-wide reference): the pressed
    // degradation toggle at 1518..1540 x 28..49 lies in the snap group placed at
    // (1456, 26); the Animate button at 1649..1696 x 3..46 in the anim group at (1645, 0).
    const Config snap = StatusSnapConfig();
    const auto sr = LayoutItems(snap, kStatusSnapWidth, kStatusSnapHeight, 96);
    CHECK(sr[2].left + 1456 == 1518 && sr[2].right + 1456 == 1541);
    CHECK(sr[2].top + 26 == 28 && sr[2].bottom + 26 == 50);
    CHECK(snap.items[2].checked);
    const Config anim = StatusAnimConfig();
    const auto ar = LayoutItems(anim, kStatusAnimWidth, 50, 96);
    CHECK(ar[0].left + 1645 == 1649 && ar[0].right + 1645 == 1697);
    CHECK(ar[0].top == 3 && ar[0].bottom == 47);
    CHECK(anim.items[0].text == L"Animate" && anim.items[0].alarm && anim.items[0].toggle);
}

static void ObjectsStrip() {
    // Measured (R3.1 screenshot r3.1-objects-tab.png): icons at x 8 + 32k for the 22
    // primitives (button = icon - 4), separators with their line at 711, 781, 851, 889,
    // and the last button's icon at 928.
    const auto& items = ObjectsToolbarSpec();
    const auto rects = LayoutToolbar(items, 38, 96);
    std::vector<int> buttons, seps;
    int icons = 0;
    for (size_t i = 0; i < items.size(); ++i) {
        if (items[i].kind == Kind::Button) { buttons.push_back(rects[i].left); if (items[i].icon >= 0) ++icons; }
        if (items[i].kind == Kind::Separator) seps.push_back(rects[i].left + 3);
    }
    CHECK(buttons.size() == 29 && icons == kObjectsIconCount);
    CHECK(buttons[0] == 4 && buttons[21] == 676 && buttons[22] == 714 && buttons.back() == 924);
    CHECK(seps.size() == 4 && seps[0] == 711 && seps[1] == 781 && seps[2] == 851 && seps[3] == 889);
    CHECK(items[0].command == "create.box" && items[9].command == "create.plane");
}

int main() {
    ObjectsStrip();
    FlyoutsMatchReference();
    StatusGroupsAreWellFormed();
    StatusMatchesReference();
    PositionsMatchReference();
    ItemsAreWellFormed();
    ScalesWithoutGaps();
    if (failures == 0) std::printf("toolbar spec: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
