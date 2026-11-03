#pragma once
#include "ui/FrameLayout.h" // ui::Rect

// Layout of the R3 status area, with no Windows dependency (unit-tested in
// tests/test_status_layout.cpp).
//
// Measured at 100% in a 1920-wide window. Left part: listener, splitter, prompt
// lines (they stretch). Right part: fields anchored to the right edge.
namespace ui::statusbar {

inline constexpr int kDefaultListenerWidth = 148;

struct Layout {
    Rect listenerTop;     // pink box (output)
    Rect listenerBottom;  // white box (input)
    Rect splitter;        // drag to resize the listener
    Rect promptTop;       // "None Selected"
    Rect promptBottom;    // "Click or click-and-drag to select objects"
    Rect lock;            // selection lock toggle
    Rect x, y, z;         // coordinate display / type-in
    Rect grid;            // "Grid = 10.0"
    Rect timeTag;         // "Time Tag" field on the bottom row
    Rect snapTools;       // toggles and snaps, bottom row (step 1.5c)
    Rect animTools;       // Animate, playback, frame field, navigation (step 1.5c)
};

// `listenerWidth` is at 100% (default 148); it is clamped so the prompts keep room.
Layout ComputeStatusLayout(int width, int height, int listenerWidth, unsigned dpi);

} // namespace ui::statusbar
