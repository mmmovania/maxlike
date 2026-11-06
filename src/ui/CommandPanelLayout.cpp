#include "ui/CommandPanelLayout.h"

namespace ui::cmdpanel {

Box TabBox(int tab, bool selected) {
    const int left = kFirstTabLeft + tab * kTabWidth;
    const int right = left + kTabWidth - 1;
    if (selected) return {left - 2, kSelectedTabTop, right + 2, kBodyTop + 1};
    return {left, kTabTop, right, kTabBottom};
}

int IconLeft(int tab) { return kFirstTabLeft + tab * kTabWidth + 3; }

int TabAt(int x, int y, int selected) {
    auto inside = [&](const Box& b) { return x >= b.left && x <= b.right && y >= b.top && y <= b.bottom; };
    if (selected >= 0 && selected < kTabCount && inside(TabBox(selected, true))) return selected;
    for (int t = 0; t < kTabCount; ++t)
        if (inside(TabBox(t, false))) return t;
    return -1;
}


Box CategoryBox(int c) {
    const int left = kFirstCategoryLeft + c * kCategoryPitch;
    return {left, kCategoryTop, left + kCategoryPitch - 1, kCategoryBottom};
}

int CategoryIconLeft(int c, bool pressed) { return CategoryBox(c).left + 4 + (pressed ? 1 : 0); }
int CategoryIconTop(bool pressed) { return 40 + (pressed ? 1 : 0); }

int CategoryAt(int x, int y) {
    for (int c = 0; c < kCategoryCount; ++c) {
        const Box b = CategoryBox(c);
        if (x >= b.left && x <= b.right && y >= b.top && y <= b.bottom) return c;
    }
    return -1;
}

std::vector<std::wstring> Subcategories(int c) {
    switch (c) {
    case kGeometry:   return {L"Standard Primitives", L"Extended Primitives", L"Compound Objects",
                              L"Particle Systems", L"Patch Grids", L"NURBS Surfaces", L"Dynamics Objects"};
    case kShapes:     return {L"Splines", L"NURBS Curves"};
    case kLights:     return {L"Standard"};
    case kCameras:    return {L"Standard"};
    case kHelpers:    return {L"Standard", L"Atmospheric Apparatus", L"Camera Match"};
    case kSpaceWarps: return {L"Particles & Dynamics", L"Modifier-Based", L"Particles Only", L"Dynamics Interface"};
    case kSystems:    return {L"Standard"};
    }
    return {};
}

std::wstring CategoryName(int c) {
    static const wchar_t* kNames[kCategoryCount] = {L"Geometry", L"Shapes", L"Lights", L"Cameras",
                                                    L"Helpers", L"Space Warps", L"Systems"};
    return (c >= 0 && c < kCategoryCount) ? kNames[c] : L"";
}

std::vector<std::wstring> StandardPrimitiveNames() {
    return {L"Box", L"Cone", L"Sphere", L"GeoSphere", L"Cylinder", L"Tube", L"Torus", L"Pyramid", L"Teapot", L"Plane"};
}

Box TypeButtonBox(int i, int titleTop) {
    const int left = kTypeButtonLeft[i % 2];
    const int top = titleTop + kTypeButtonTop + (i / 2) * kTypeButtonPitch;
    return {left, top, left + kTypeButtonWidth - 1, top + kTypeButtonHeight - 1};
}

} // namespace ui::cmdpanel
