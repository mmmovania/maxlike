#include "ui/FrameLayout.h"
#include "ui/Metrics.h"
#include <algorithm>

namespace ui {

FrameLayout ComputeFrameLayout(int w, int h, unsigned dpi) {
    auto s = [dpi](int px96) { return metrics::Scale(px96, dpi); };
    w = std::max(w, 0);
    h = std::max(h, 0);

    FrameLayout L;

    // Top stack: gap, tab panel, toolbar. Clamped so nothing goes past the bottom.
    const int tabTop     = std::min(s(metrics::TopGap), h);
    const int toolbarTop = std::min(tabTop + s(metrics::TabPanelHeight), h);
    const int bodyTop    = std::min(toolbarTop + s(metrics::MainToolbarHeight), h);
    L.tabPanel    = {0, tabTop, w, toolbarTop};
    L.mainToolbar = {0, toolbarTop, w, bodyTop};

    // Bottom: status area spans the full width.
    const int statusTop = std::max(bodyTop, h - s(metrics::StatusAreaHeight));
    L.statusArea = {0, statusTop, w, h};

    // Right column: command panel between the toolbar and the status area.
    const int split = std::max(0, w - s(metrics::CommandPanelWidth));
    L.commandPanel = {split, bodyTop, w, statusTop};

    // Left column, bottom up: track bar, time slider, then viewports fill the rest.
    const int trackTop  = std::max(bodyTop, statusTop - s(metrics::TrackBarHeight));
    const int sliderTop = std::max(bodyTop, trackTop - s(metrics::TimeSliderHeight));
    L.trackBar   = {0, trackTop, split, statusTop};
    L.timeSlider = {0, sliderTop, split, trackTop};
    L.viewports  = {0, bodyTop, split, sliderTop};
    return L;
}

} // namespace ui
