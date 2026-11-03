#pragma once

// Where every region of the main window goes, as a pure function of client size
// and DPI. No Windows types, so it is unit-tested in tests/test_frame_layout.cpp.
namespace ui {

struct Rect {
    int left = 0, top = 0, right = 0, bottom = 0;
    constexpr int Width()  const { return right - left; }
    constexpr int Height() const { return bottom - top; }
    constexpr bool operator==(const Rect& o) const {
        return left == o.left && top == o.top && right == o.right && bottom == o.bottom;
    }
};

struct FrameLayout {
    Rect tabPanel;      // tabs above the toolbar ("Main Toolbar", "Objects", ...)
    Rect mainToolbar;
    Rect viewports;     // the 2x2 viewport area
    Rect timeSlider;    // "0 / 100"
    Rect trackBar;
    Rect commandPanel;  // Create / Modify / ... panel on the right
    Rect statusArea;    // listener, prompt line, coordinates, animation and nav controls
};

// clientWidth/clientHeight in physical pixels; dpi 96 = 100%.
FrameLayout ComputeFrameLayout(int clientWidth, int clientHeight, unsigned dpi);

} // namespace ui
