#pragma once
#include "scene/Scene.h"
#include <string>

namespace scene {

// Recreates objects from the verified JS-reader samples (docs/reference/
// max-reader-handoff.md) so they can be compared with R3.1:
//   "box"     Box 60 x 60 x 60, wire #1AB194
//   "plane"   Plane 40 (X) x 42 (Y), 4 x 4 segments, wire #B1581A
//   "pyramid" Pyramid 20 x 20 x 10, wire #390888
//   "all"     the three side by side
// Each single sample sits at the origin (where the sample files put them is still to
// be confirmed). Returns false for an unknown name.
bool BuildDemoScene(Scene& scene, const std::string& which);

} // namespace scene
