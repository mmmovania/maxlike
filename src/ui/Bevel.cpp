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

void DrawButtonRing(HDC dc, const RECT& r, COLORREF tl, COLORREF br) {
    FillSolid(dc, {r.left, r.top, r.left + 1, r.bottom}, tl);           // left
    FillSolid(dc, {r.left, r.top, r.right - 1, r.top + 1}, tl);         // top
    FillSolid(dc, {r.right - 1, r.top, r.right, r.bottom}, br);         // right
    FillSolid(dc, {r.left + 1, r.bottom - 1, r.right, r.bottom}, br);   // bottom
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
    case Edge::CommandPanel: {
        // Measured at 100% (x, y relative to the panel; w, h its size):
        //   row 0          : shadow across x 0..w-2 (the last column stays face)
        //   x 0, y 3..h-4  : highlight;  x 1, y 3..h-4 : light
        //   x w-3, y 28    : highlight;  x w-3, y 29..h-4 : shadow
        //   x w-2, y 28..h-2 : dark shadow
        //   bottom rows h-3 / h-2 / h-1 : (highlight, shadow..., dark) /
        //                                 dark... / (highlight, shadow...)
        // Rows 1..27 at the top hold the panel's own tabs (Phase 4).
        const int x = r.left, y = r.top, w = r.right - r.left, h = r.bottom - r.top;
        if (w < 8 || h < 40) return r;
        const COLORREF hi = theme::kButtonHighlight.Ref(), lt = theme::kLight.Ref();
        const COLORREF sh = theme::kButtonShadow.Ref(), dk = theme::kDarkShadow.Ref();
        auto fill = [&](int l, int t, int rr, int b, COLORREF c) { FillSolid(dc, {x + l, y + t, x + rr, y + b}, c); };
        fill(0, 0, w - 1, 1, sh);                    // top row
        fill(0, 3, 1, h - 3, hi);                    // left highlight
        fill(1, 3, 2, h - 3, lt);                    // left light
        fill(w - 3, 28, w - 2, 29, hi);              // right: top pixel
        fill(w - 3, 29, w - 2, h - 3, sh);           // right shadow
        fill(w - 2, 28, w - 1, h - 1, dk);           // right dark shadow
        fill(0, h - 3, 1, h - 2, hi);                // bottom row h-3
        fill(1, h - 3, w - 2, h - 2, sh);
        fill(0, h - 2, w - 1, h - 1, dk);            // bottom row h-2
        fill(0, h - 1, 1, h, hi);                    // bottom row h-1
        fill(1, h - 1, w - 1, h, sh);
        return {x + 2, y + 1, x + w - 3, y + h - 3};
    }
    case Edge::ViewportFrame:
        DrawRing(dc, r, theme::kFrameOuterTL.Ref(), theme::kFrameOuterBR.Ref());
        r = Shrink(r, 1);
        DrawRing(dc, r, theme::kFrameInnerTL.Ref(), theme::kFrameInnerBR.Ref());
        return Shrink(r, 1);
    }
    return r;
}

} // namespace ui
