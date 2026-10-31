#pragma once
#include <windows.h>

namespace ui {

enum class Edge {
    None,          // nothing
    Outline,       // 1 px grey outline: marks a placeholder region, not part of the R3 look
    Raised,        // classic raised 3D edge (DrawEdge EDGE_RAISED), e.g. command panel
    Sunken,        // classic sunken 3D edge (DrawEdge EDGE_SUNKEN)
    ViewportFrame, // the two-ring frame around viewports (refined in Phase 2)
};

// Fill a rectangle with a colour, without creating a brush.
void FillSolid(HDC dc, const RECT& r, COLORREF c);

// One 1-px ring: top and left in `tl`, bottom and right in `br`.
void DrawRing(HDC dc, const RECT& r, COLORREF tl, COLORREF br);

// Draw `edge` just inside `r` and return the rectangle left inside it.
RECT DrawFrameEdge(HDC dc, RECT r, Edge edge);

} // namespace ui
