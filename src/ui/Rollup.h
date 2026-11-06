#pragma once
#include "ui/CommandPanelLayout.h"
#include <vector>

// The rollup: a sunken column holding rollouts (collapsible sections, each a title
// button over an etched frame). Measured on the R3.1 reference, in command-panel
// coordinates. No Windows dependency (unit-tested in tests/test_rollup.cpp).
namespace ui::rollup {

using cmdpanel::Box;

inline constexpr Box kContainer{4, 94, 178, 885};   // 1-px sunken edge; inside x 5..177, rows 95..884
inline constexpr int kFirstTitleGap = 5;             // container top -> first title top (row 99)
inline constexpr int kTitleLeft = 11, kTitleRight = 168, kTitleHeight = 16;
inline constexpr int kFrameLeft = 7, kFrameRight = 172;   // etched edge, 2 px
inline constexpr int kFrameTopOffset = 8;            // title top -> frame top (row 107)
// A collapsed rollout keeps a small frame behind its title (measured: Keyboard Entry
// collapsed, title row 356, frame rows 360..369).
inline constexpr int kClosedFrameTop = 4, kClosedFrameBottom = 13;
inline constexpr int kGap = 7;                       // frame bottom -> next title (also when collapsed)
inline constexpr int kBottomMargin = 5;              // space kept below the last rollout when scrolled to the end

struct Item {
    int contentHeight;   // rows from the title's bottom to the frame's bottom (Object Type: 127)
    bool open = true;
};

struct Placed {
    int titleTop, titleBottom;     // inclusive
    int frameTop, frameBottom;     // inclusive (a collapsed rollout's small frame too)
    bool open;
};

// Where each rollout is with the column scrolled up by `scroll` pixels.
std::vector<Placed> Layout(const std::vector<Item>& items, int scroll);

// Height of the whole column, from the first title's top to the last rollout's bottom.
int ContentHeight(const std::vector<Item>& items);

// How far the column can scroll (0 when everything fits).
int MaxScroll(const std::vector<Item>& items);

// Index of the rollout whose title button contains the point, or -1.
int TitleAt(const std::vector<Item>& items, int scroll, int x, int y);

} // namespace ui::rollup
