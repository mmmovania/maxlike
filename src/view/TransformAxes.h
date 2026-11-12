#pragma once
#include <cstdint>
#include <vector>

// The axes R3 draws at a selected object's pivot, as exact pixels. No Windows or OpenGL
// dependency (unit-tested in tests/test_transform_axes.cpp against R3.1 screenshots).
//
//   Tripod (Select, and while creating; screenshots r3.1-box-created.png,
//   r3.1-create-tripod-z.png): the object's own axes; lines from the pivot to 84 px with
//   open arrowheads (two 8-px strokes); colour #C82323 for axes of the active
//   constraint, #666666 for the others. An axis pointing at the viewer shows only its
//   letter at the pivot.
//   Move gizmo (Select and Move, measured from r3.1-move-selected.png; also Rotate and
//   Scale, which look the same in R3.1, confirmed by the project owner): the world axes;
//   a shaft from 17
//   to 60 px (yellow when in the active constraint, else the axis colour), a filled
//   arrowhead from 62 to 84.5 px in the axis colour (X red, Y green, Z blue), the letter
//   (yellow when active); and for each pair of visible axes an L-shaped plane corner at
//   84 px along both, arms 17 px (yellow when it is the active plane, else each arm in
//   the colour of the axis it runs beside). Axes pointing at the viewer are not drawn.
// Letters use the viewport tripod's glyphs, placed 9 rows above their tip (step 2.3).
namespace view {

struct ColoredPixel { int x, y; uint32_t rgb; };

enum class AxesStyle { Tripod, MoveGizmo };

// screenAxes[a] = (right, up) components of axis a on screen (as for the viewport
// tripod); origin = the pivot's pixel; active[a] = axis a is in the active constraint.
std::vector<ColoredPixel> TransformAxesPixels(AxesStyle style, const double screenAxes[3][2],
                                              int originX, int originY, const bool active[3]);

// The Move gizmo's part under the mouse (Phase 12.2), as an axis mask (1 X, 2 Y, 4 Z): an
// axis within 4 px of its shaft or arrowhead (17 to 84.5 along it), or a plane (two bits)
// inside its corner square (84 px along both axes, 17-px arms). 0: none. Uses exactly the
// geometry TransformAxesPixels draws; axes pointing at the viewer are never hit.
int GizmoPartAt(const double screenAxes[3][2], int originX, int originY, double mouseX, double mouseY);

inline constexpr uint32_t kTripodActive = 0xC82323, kTripodInactive = 0x666666;
inline constexpr uint32_t kGizmoActive = 0xFFFF00;
inline constexpr uint32_t kAxisColor[3] = {0xFF0000, 0x00FF00, 0x0000FF};

} // namespace view
