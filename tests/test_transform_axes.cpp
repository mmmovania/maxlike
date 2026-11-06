// Unit tests for the axes drawn at a selected object's pivot, against pixels measured in
// R3.1 screenshots (Top view; screen coordinates).
#include "view/TransformAxes.h"
#include <cstdio>
#include <map>
#include <utility>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

using namespace view;

static std::map<std::pair<int, int>, uint32_t> Draw(AxesStyle st, const double s[3][2], int ox, int oy, const bool act[3]) {
    std::map<std::pair<int, int>, uint32_t> m;
    for (const ColoredPixel& p : TransformAxesPixels(st, s, ox, oy, act)) m[{p.x, p.y}] = p.rgb;
    return m;
}

// A helper instead of m[{x, y}] inside CHECK: the macro would split at the brace's comma.
static uint32_t At(std::map<std::pair<int, int>, uint32_t>& m, int x, int y) {
    auto it = m.find({x, y});
    return it == m.end() ? 0xFFFFFFFF : it->second;
}

int main() {
    const double top[3][2] = {{1, 0}, {0, 1}, {0, 0}};          // Top view: X right, Y up, Z at the viewer
    // Tripod, XY constraint (r3.1-box-created.png): pivot (434, 316).
    {
        const bool act[3] = {true, true, false};
        auto m = Draw(AxesStyle::Tripod, top, 434, 316, act);
        CHECK(At(m, 518, 316) == kTripodActive && At(m, 434, 232) == kTripodActive);   // tips of X and Y
        CHECK(At(m, 475, 316) == kTripodActive && At(m, 434, 270) == kTripodActive);   // the lines
        CHECK(At(m, 511, 309) == kTripodActive && At(m, 511, 323) == kTripodActive);   // X arrowhead strokes
        CHECK(At(m, 518, 307) == kTripodActive && At(m, 519, 307) == kTripodActive);   // "x": top row "##....##"
        CHECK(At(m, 434, 223) == kTripodActive && At(m, 435, 223) == kTripodActive);   // "y": top row
        CHECK(At(m, 435, 307) == kTripodInactive);                                    // "z" (grey) at the pivot
    }
    // Move gizmo, XY constraint (r3.1-move-selected.png): pivot (433, 314), from the shafts.
    {
        const bool act[3] = {true, true, false};
        auto m = Draw(AxesStyle::MoveGizmo, top, 433, 314, act);
        CHECK(At(m, 450, 314) == kGizmoActive && At(m, 493, 314) == kGizmoActive);   // yellow X shaft 450..493
        CHECK(At(m, 449, 314) != kGizmoActive);
        CHECK(At(m, 433, 297) == kGizmoActive && At(m, 433, 254) == kGizmoActive);   // yellow Y shaft rows 254..297
        CHECK(At(m, 505, 314) == kAxisColor[0] && At(m, 433, 243) == kAxisColor[1]);  // red and green heads
        CHECK(At(m, 518, 305) == kGizmoActive && At(m, 525, 305) == kGizmoActive);    // "x": top row at 518
        CHECK(At(m, 433, 221) == kGizmoActive && At(m, 440, 221) == kGizmoActive);    // "y": top row at 221
        CHECK(At(m, 517, 230) == kGizmoActive && At(m, 501, 230) == kGizmoActive && At(m, 517, 246) == kGizmoActive);  // XY corner
        bool anyBlue = false;
        for (const auto& [pt, c] : m) anyBlue = anyBlue || c == kAxisColor[2];
        CHECK(!anyBlue);                                                              // Z at the viewer: nothing
    }
    // Front view (X right, Z up, Y at the viewer), XY constraint: X yellow, Z blue, the XZ
    // corner coloured by axis (measured: the arm beside X blue, the arm beside Z red).
    {
        const double front[3][2] = {{1, 0}, {0, 0}, {0, 1}};
        const bool act[3] = {true, true, false};
        auto m = Draw(AxesStyle::MoveGizmo, front, 1300, 314, act);
        CHECK(At(m, 1330, 314) == kGizmoActive && At(m, 1300, 290) == kAxisColor[2]);
        CHECK(At(m, 1300 + 84 - 8, 314 - 84) == kAxisColor[2] && At(m, 1300 + 84, 314 - 84 + 8) == kAxisColor[0]);
    }
    if (failures == 0) std::printf("transform axes: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
