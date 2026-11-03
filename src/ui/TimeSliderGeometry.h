#pragma once
#include "ui/FrameLayout.h" // ui::Rect

// Geometry of the R3 time slider, with no Windows dependency (unit-tested in
// tests/test_time_slider.cpp).
//
// Measured at 100%: the "<" button, the "0 / 100" thumb and the ">" button form one
// 157-px block that slides along the trough; at frame 0 it starts at x = 0.
namespace ui::timeslider {

inline constexpr int kArrowWidth = 16;
inline constexpr int kThumbWidth = 125;
inline constexpr int kBlockWidth = kArrowWidth + kThumbWidth + kArrowWidth; // 157

struct Parts {
    Rect prev;   // "<" button
    Rect thumb;  // "frame / end" label, dragged to scrub
    Rect next;   // ">" button
};

// Where the block sits for `frame` in [start, end], in a slider `width` x `height`.
Parts PartsAt(int frame, int start, int end, int width, int height, unsigned dpi);

// The frame whose block would start at `blockLeft` (rounded to the nearest frame,
// clamped to [start, end]). Inverse of PartsAt(...).prev.left.
int FrameAtBlockLeft(int blockLeft, int start, int end, int width, unsigned dpi);

} // namespace ui::timeslider
