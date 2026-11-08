#pragma once
#include "scene/Scene.h"
#include <string>
#include <vector>

// File > Summary Info (Phase 9): the scene's totals and its objects, as R3.1's dialog lists
// them (docs/reference/screenshots/r3.1-summary-*.png). Only geometry exists in maxlike,
// so Shapes, Lights, Cameras, Helpers and Space Warps are 0.
namespace scene {

struct SummaryRow {
    std::string name, type;      // "Teapot01", "Teapot" (shown as "Teapot01 (Teapot)")
    size_t verts = 0, faces = 0;
};

struct Summary {
    size_t objects = 0, total = 0;
    size_t verts = 0, faces = 0;     // Mesh Totals
    std::vector<SummaryRow> rows;    // in scene order
};

Summary Summarize(const Scene& scene);

} // namespace scene
