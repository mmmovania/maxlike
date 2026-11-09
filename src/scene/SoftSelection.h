#pragma once
#include "scene/Mesh.h"
#include <cstdint>
#include <vector>

// Edit Mesh's Soft Selection (Phase 10.6): unselected vertices near the selection follow
// a transform by a weight that falls off with distance.
namespace scene::soft {

// Max's "affect region" curve: 1 at distance 0, 0 at `falloff` and beyond; Pinch sharpens
// the top, Bubble swells the middle. With both 0 it is 3u^2 - 2u^3, u = 1 - d / falloff.
double AffectRegion(double distance, double falloff, double pinch, double bubble);

// One weight per vertex: 1 for the selected, the curve of the distance to the nearest
// selected vertex for the rest. `edgeLimit` >= 0 (Edge Distance): only vertices within that
// many edges of the selection count. NOTE: distances in the mesh's own space; R3's
// Affect Backfacing is not applied.
std::vector<double> Weights(const Mesh& mesh, const std::vector<uint32_t>& selected, double falloff, double pinch,
                            double bubble, int edgeLimit = -1);

} // namespace scene::soft
