#pragma once

// Layout metrics of the R3.1 main window at 96 DPI (100%), measured from
// docs/reference/screenshots/max-r3.1-default-1920x1040.png.
// Pure constants with no Windows dependency, so layout code can be unit-tested anywhere.
namespace metrics {

// Vertical stack, measured from the top of the client area (below the menu bar).
inline constexpr int TopGap            = 4;   // space above the tab panel
inline constexpr int TabPanelHeight    = 15;
inline constexpr int MainToolbarHeight = 38;

// Measured from the bottom of the client area.
inline constexpr int StatusAreaHeight  = 50;  // full width
inline constexpr int TrackBarHeight    = 16;  // left column only
inline constexpr int TimeSliderHeight  = 17;  // left column only

// Right column, from below the toolbar down to the status area.
inline constexpr int CommandPanelWidth = 184;

// Viewports (used from Phase 2).
inline constexpr int ViewportGap        = 6;
inline constexpr int ViewportLabelInset = 6;
inline constexpr int TripodSize         = 40;

inline constexpr int MinWindowWidth  = 800;
inline constexpr int MinWindowHeight = 600;

// Scale a 96-DPI pixel value to `dpi`, rounding to nearest (same result as MulDiv).
constexpr int Scale(int px96, unsigned dpi) {
    long long v = static_cast<long long>(px96) * dpi;
    return static_cast<int>(v >= 0 ? (v + 48) / 96 : -((-v + 48) / 96));
}

} // namespace metrics
