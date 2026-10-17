#pragma once
#include <windows.h>
#include <cstdint>

// All colours and pixel metrics of the R3.1 look, from docs/spec/01-ui-look.md.
// Metrics are 96-DPI values; scale with Window::Scale() or theme::Scale().
namespace theme {

struct Rgb {
    uint8_t r, g, b;
    constexpr COLORREF Ref() const { return RGB(r, g, b); }
    constexpr float R() const { return r / 255.0f; }
    constexpr float G() const { return g / 255.0f; }
    constexpr float B() const { return b / 255.0f; }
};

// ---- Viewports (measured) ----
inline constexpr Rgb kViewportBg     {0xB0, 0xB0, 0xB0};
inline constexpr Rgb kGridMinor      {0x8D, 0x8D, 0x8D};
inline constexpr Rgb kGridMajor      {0x74, 0x74, 0x74};
inline constexpr Rgb kGridAxis       {0x00, 0x00, 0x00};
inline constexpr Rgb kViewportLabel  {0xE5, 0xE5, 0xE5};
inline constexpr Rgb kTripodX        {0xFF, 0x00, 0x00};
inline constexpr Rgb kTripodY        {0x00, 0xFF, 0x00};
inline constexpr Rgb kTripodZ        {0x00, 0x00, 0xFF};

// Viewport frame bevel (outer then inner ring).
inline constexpr Rgb kFrameOuterTL   {0xA0, 0xA0, 0xA0};
inline constexpr Rgb kFrameOuterBR   {0xFF, 0xFF, 0xFF};
inline constexpr Rgb kFrameInnerTL   {0x00, 0x00, 0x00};
inline constexpr Rgb kFrameInnerBR   {0xC0, 0xC0, 0xC0};

// TODO(spec): active viewport highlight colour is unverified.
inline constexpr Rgb kViewportActive {0xFF, 0xFF, 0xFF};

// ---- Status area ----
inline constexpr Rgb kListenerBg     {0xDB, 0xC3, 0xC3};

// ---- Grid ----
inline constexpr float kGridSpacing        = 10.0f;  // world units
inline constexpr int   kGridMajorEvery     = 10;     // minor lines per major line
inline constexpr float kOrthoUnitsPerPixel = 0.46f;  // default ortho zoom at 100%
inline constexpr float kPerspectiveFovDeg  = 45.0f;

// ---- Layout metrics at 96 DPI (approximate, refine in Phase 1) ----
struct Metrics {
    static constexpr int TabPanelHeight     = 16;
    static constexpr int MainToolbarHeight  = 38;
    static constexpr int CommandPanelWidth  = 180;
    static constexpr int ViewportGap        = 6;   // between viewports, incl. frames
    static constexpr int ViewportLabelInset = 6;
    static constexpr int TripodSize         = 40;
    static constexpr int TimeSliderHeight   = 18;
    static constexpr int TrackBarHeight     = 16;
    static constexpr int StatusAreaHeight   = 50;
    static constexpr int MinWindowWidth     = 800;
    static constexpr int MinWindowHeight    = 600;
};

inline int Scale(int px96, UINT dpi) { return MulDiv(px96, static_cast<int>(dpi), 96); }

// System UI face colour (#F0F0F0 on Win10, #D4D0C8 on Win2000).
inline COLORREF Face()      { return GetSysColor(COLOR_BTNFACE); }
inline HBRUSH   FaceBrush() { return GetSysColorBrush(COLOR_BTNFACE); }

// The UI font: MS Sans Serif 8 pt at 100% (the bitmap face R3 used),
// Microsoft Sans Serif (TrueType, same design) at other DPIs so it scales cleanly.
// Caller owns the returned HFONT.
HFONT CreateUiFont(UINT dpi);

} // namespace theme
