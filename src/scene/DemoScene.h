#pragma once
#include "scene/Scene.h"
#include <string>

namespace scene {

// Recreates objects from the verified JS-reader samples (docs/reference/
// max-reader-handoff.md) so they can be compared with R3.1:
//   "box"     Box 60 x 60 x 60, wire #1AB194
//   "plane"   Plane 40 (X) x 42 (Y), 4 x 4 segments, wire #B1581A
//   "pyramid" Pyramid 20 x 20 x 10, wire #390888
//   "cone"    Cone r1 10, r2 -20 (shown as 0), h 20, 24 sides, 5 height segs, #57E0C6
//   "tube"    Tube 10 / 6.185, h 10, 18 sides, 1 cap seg, 5 height segs, #1AB158
//   "sphere"  Sphere r 10, 32 segments, #860606
//   "geosphere" GeoSphere r 10, icosahedron, 4 segments, #99E4B8
//   "teapot"  Teapot radius 30, 4 segments, #C6E057
//   "cylinder", "torus"  Max's defaults (their samples were only checked visually)
//   "all"     all of them in a row along X
//   "instances" seven nodes sharing one Box + Edit Mesh (illustrative, see the .cpp)
// Each single sample sits at the origin (where the sample files put them is still to
// be confirmed). Returns false for an unknown name.
bool BuildDemoScene(Scene& scene, const std::string& which);

} // namespace scene
