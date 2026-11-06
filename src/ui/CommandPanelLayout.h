#pragma once
#include <string>
#include <vector>

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


// ---- Create panel: category row and the dropdown below it (measured, step 4.2) ----

enum Category { kGeometry = 0, kShapes, kLights, kCameras, kHelpers, kSpaceWarps, kSystems, kCategoryCount };

inline constexpr int kCategoryPitch = 24;     // buttons 24 x 23, side by side from x = 9
inline constexpr int kFirstCategoryLeft = 9;
inline constexpr int kCategoryTop = 37, kCategoryBottom = 59;
inline constexpr int kCategoryIconSize = 16;

Box CategoryBox(int category);
// Top-left of a category's 16 x 16 icon; a pressed button shows it 1 px down and right.
int CategoryIconLeft(int category, bool pressed);
int CategoryIconTop(bool pressed);
int CategoryAt(int x, int y);                 // -1 if none

// The dropdown (a classic Win32 combo box): sunken frame x 21..163, rows 66..86.
inline constexpr Box kDropdown{21, 66, 163, 86};

// What the dropdown lists for each category, first entry selected. Only Geometry's
// first entry ("Standard Primitives") is in the reference; the rest are from memory
// of R3.1 (NOTE: unverified).
std::vector<std::wstring> Subcategories(int category);
std::wstring CategoryName(int category);   // e.g. tooltips (NOTE: wording unverified)

} // namespace ui::cmdpanel
