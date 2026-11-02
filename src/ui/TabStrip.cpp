#include "ui/TabStrip.h"
#include "ui/Metrics.h"

namespace ui::tabstrip {

const std::vector<Tab>& Tabs() {
    static const std::vector<Tab> tabs = {
        {L"Main Toolbar",     "tab.main_toolbar"},
        {L"Objects",          "tab.objects"},
        {L"Shapes",           "tab.shapes"},
        {L"Compounds",        "tab.compounds"},
        {L"Lights & Cameras", "tab.lights_cameras"},
        {L"Particles",        "tab.particles"},
        {L"Helpers",          "tab.helpers"},
        {L"SpaceWarps",       "tab.spacewarps"},
        {L"Modifiers",        "tab.modifiers"},
        {L"Modeling",         "tab.modeling"},
        {L"Rendering",        "tab.rendering"},
    };
    return tabs;
}

Rect TabRect(int index, int height, unsigned dpi) {
    const int left  = metrics::Scale(kLeft + index * kPitch, dpi);
    const int right = metrics::Scale(kLeft + (index + 1) * kPitch, dpi);
    return {left, 0, right, height};
}

int TabAtX(int x, int count, unsigned dpi) {
    for (int i = 0; i < count; ++i) {
        const Rect r = TabRect(i, 1, dpi);
        if (x >= r.left && x < r.right)
            return i;
    }
    return -1;
}

} // namespace ui::tabstrip
