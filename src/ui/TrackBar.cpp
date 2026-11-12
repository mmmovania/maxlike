#include "win/PaintBuffer.h"
#include "ui/TrackBar.h"
#include "ui/Bevel.h"
#include "ui/Theme.h"
#include <algorithm>

namespace ui {

bool TrackBar::Create(HWND parent) {
    CreateParams p;
    p.title  = L"Track bar";
    p.style  = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
    p.x = p.y = 0;
    p.width = p.height = 0;
    p.parent = parent;
    return win::Window::Create(p);
}

void TrackBar::SetRange(int start, int end) {
    start_ = start;
    end_ = std::max(start, end);
    if (Hwnd()) InvalidateRect(Hwnd(), nullptr, FALSE);
}

void TrackBar::SetFrame(int frame) {
    frame_ = std::clamp(frame, start_, end_);
    if (Hwnd()) InvalidateRect(Hwnd(), nullptr, FALSE);
}

// Measured rows at 100% (16 high): 0-1 white, 2 shadow, 3-13 band #E1E1E1,
// 14 white, 15 shadow. The last column stays #E1E1E1, like the time slider trough.
// Marker (measured at frame 0, unverified meaning): a "[" in #1E1E1E spanning the band.
void TrackBar::OnPaint() {
    win::PaintBuffer paint(Hwnd());                       // flicker-free (12.1)
    HDC dc = paint.Dc();
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    const int w = rc.right, h = rc.bottom;
    const int right = std::max(0, w - 1);

    FillSolid(dc, rc, theme::kSliderTrough.Ref());
    if (h >= 16) {
        FillSolid(dc, {0, 0, right, 2}, theme::kButtonHighlight.Ref());
        FillSolid(dc, {0, 2, right, 3}, theme::kButtonShadow.Ref());
        FillSolid(dc, {0, h - 2, right, h - 1}, theme::kButtonHighlight.Ref());
        FillSolid(dc, {0, h - 1, right, h}, theme::kButtonShadow.Ref());

        // Current frame marker, left-aligned at its frame position.
        const int range = std::max(1, end_ - start_);
        const int x = (frame_ - start_) * std::max(0, right - 2) / range;
        const COLORREF ink = theme::kTrackMarker.Ref();
        FillSolid(dc, {x, 3, x + 1, h - 2}, ink);         // vertical stroke
        FillSolid(dc, {x + 1, 3, x + 2, 4}, ink);         // top serif
        FillSolid(dc, {x + 1, h - 3, x + 2, h - 2}, ink); // bottom serif
    }
}

LRESULT TrackBar::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        OnPaint();
        return 0;
    }
    return win::Window::HandleMessage(msg, wp, lp);
}

} // namespace ui
