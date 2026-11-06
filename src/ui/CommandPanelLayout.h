#pragma once

// Geometry of the command panel's tab row, measured at 100 % on the R3.1 reference
// (coordinates relative to the panel's client area, 184 px wide). No Windows
// dependency (unit-tested in tests/test_command_panel_layout.cpp).
namespace ui::cmdpanel {

enum Tab { kCreate = 0, kModify, kHierarchy, kMotion, kDisplay, kUtilities, kTabCount };

// Inclusive pixel rectangle.
struct Box { int left, top, right, bottom; };

inline constexpr int kTabWidth = 29;          // unselected tabs: 29 px, side by side
inline constexpr int kFirstTabLeft = 2;
inline constexpr int kTabTop = 3;             // unselected tabs start at row 3 ...
inline constexpr int kSelectedTabTop = 1;     // ... the selected one at row 1
inline constexpr int kTabBottom = 27;         // last row of a tab
inline constexpr int kBodyTop = 28;           // the panel body's top edge
inline constexpr int kIconWidth = 22, kIconHeight = 20;

// A tab's outline. The selected tab is 2 px wider on each side and 2 px taller, and
// continues down to the body (rows 28-29 join it to the body).
Box TabBox(int tab, bool selected);

// Top-left corner of a tab's 22 x 20 icon (the same whether selected or not).
int IconLeft(int tab);
inline constexpr int kIconTop = 6;

// Which tab a point is on (-1 if none). The selected tab is checked first, because
// it is drawn over its neighbours.
int TabAt(int x, int y, int selected);

} // namespace ui::cmdpanel
