#pragma once
#include "view/OrthoGrid.h"
#include "view/PerspectiveView.h"
#include <cstdint>
#include <string>
#include <vector>

// The viewports saved in a .max file (Config stream, chunk 0x2080 > 0x2810..0x2813, one
// per viewport in layout order; docs/reference/max-format.md §11). R3 restores them when
// a file is opened. No Windows dependency (tests/test_max_viewports.cpp).
namespace io {

struct MaxViewport {
    int slot = 0;                  // 0..3: top-left, top-right, bottom-left, bottom-right
    int type = -1;                 // the SDK's VIEW_*: 0 Left, 1 Right, 2 Top, 3 Bottom, 4 Front, 5 Back, 6 User, 7 Perspective
    double tm[4][3] = {};          // world -> view, rows (row vectors: v = w * R + T)
    double zoom = 1;               // ortho: the visible width is 400 * zoom units
    double fov = 0.7853981634;     // perspective: horizontal field of view, radians
    double distance = 250;         // perspective: the distance to the target
};

// False only if the file cannot be read; a file without saved viewports gives none.
bool ReadViewports(const std::vector<uint8_t>& fileBytes, std::vector<MaxViewport>& out, std::string& error);

// maxlike's view for a saved ortho view: the visible width and the world point at the
// centre (the negated translation, along the view's own right and up).
view::OrthoView ToOrthoView(const MaxViewport& v);
// maxlike's perspective camera: eye = -T * R^T, looking along the negated third column
// of R, the target `distance` ahead.
view::PerspectiveView ToPerspectiveView(const MaxViewport& v);

} // namespace io
