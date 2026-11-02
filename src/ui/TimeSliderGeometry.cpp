#include "ui/TimeSliderGeometry.h"
#include "ui/Metrics.h"
#include <algorithm>

namespace ui::timeslider {

// Pixels the block can travel: from x = 0 to the right edge of the slider.
static int Travel(int width, unsigned dpi) {
    return std::max(0, width - metrics::Scale(kBlockWidth, dpi));
}

Parts PartsAt(int frame, int start, int end, int width, int height, unsigned dpi) {
    const int range = std::max(1, end - start);
    const int f = std::clamp(frame, start, end);
    const int left = static_cast<int>((static_cast<long long>(f - start) * Travel(width, dpi) + range / 2) / range);
    const int a = metrics::Scale(kArrowWidth, dpi);
    const int t = metrics::Scale(kThumbWidth, dpi);
    Parts p;
    p.prev  = {left, 0, left + a, height};
    p.thumb = {left + a, 0, left + a + t, height};
    p.next  = {left + a + t, 0, left + a + t + a, height};
    return p;
}

int FrameAtBlockLeft(int blockLeft, int start, int end, int width, unsigned dpi) {
    const int travel = Travel(width, dpi);
    if (travel <= 0 || end <= start) return start;
    const int x = std::clamp(blockLeft, 0, travel);
    const long long range = end - start;
    return start + static_cast<int>((x * range + travel / 2) / travel);
}

} // namespace ui::timeslider
