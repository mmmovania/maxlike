#include "ui/Bevel.h"
#include "ui/Theme.h"

namespace ui {

void FillSolid(HDC dc, const RECT& r, COLORREF c) {
    // DC_BRUSH is a stock brush whose colour is set per DC, so no brush is
    // created or deleted here (no GDI handle to leak).
    SetDCBrushColor(dc, c);
    FillRect(dc, &r, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
}

void DrawRing(HDC dc, const RECT& r, COLORREF tl, COLORREF br) {
    if (r.right - r.left < 1 || r.bottom - r.top < 1) return;
    FillSolid(dc, {r.left, r.top, r.right - 1, r.top + 1}, tl);          // top
    FillSolid(dc, {r.left, r.top, r.left + 1, r.bottom - 1}, tl);        // left
    FillSolid(dc, {r.left, r.bottom - 1, r.right, r.bottom}, br);        // bottom
    FillSolid(dc, {r.right - 1, r.top, r.right, r.bottom}, br);          // right
}

static RECT Shrink(RECT r, int n) {
    InflateRect(&r, -n, -n);
    return r;
}

RECT DrawFrameEdge(HDC dc, RECT r, Edge edge) {
    switch (edge) {
    case Edge::None:
        return r;
    case Edge::Outline: {
        const COLORREF c = GetSysColor(COLOR_GRAYTEXT);
        DrawRing(dc, r, c, c);
        return Shrink(r, 1);
    }
    case Edge::Raised:
        DrawEdge(dc, &r, EDGE_RAISED, BF_RECT | BF_ADJUST); // BF_ADJUST shrinks r
        return r;
    case Edge::Sunken:
        DrawEdge(dc, &r, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
        return r;
    case Edge::ViewportFrame:
        DrawRing(dc, r, theme::kFrameOuterTL.Ref(), theme::kFrameOuterBR.Ref());
        r = Shrink(r, 1);
        DrawRing(dc, r, theme::kFrameInnerTL.Ref(), theme::kFrameInnerBR.Ref());
        return Shrink(r, 1);
    }
    return r;
}

} // namespace ui
