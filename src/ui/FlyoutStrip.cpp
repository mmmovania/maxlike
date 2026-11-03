#include "ui/FlyoutStrip.h"
#include "ui/Bevel.h"
#include "ui/Theme.h"

namespace ui {

bool FlyoutStrip::Open(HWND owner, const std::vector<toolbar::Alternative>& alternatives,
                       int current, HBITMAP icons, const RECT& b,
                       int iconSize, int iconOffsetX, int iconOffsetY) {
    if (IsOpen()) Close();
    iconSize_ = iconSize; iconOffsetX_ = iconOffsetX; iconOffsetY_ = iconOffsetY;
    alternatives_ = alternatives;
    current_ = current;
    hot_ = -1;
    icons_ = icons;
    cellW_ = b.right - b.left;
    cellH_ = b.bottom - b.top;
    border_ = 2; // classic raised edge

    const int n = static_cast<int>(alternatives_.size());
    CreateParams p;
    p.title   = L"";
    p.style   = WS_POPUP;
    // TOOLWINDOW: no taskbar button. NOACTIVATE: the main window keeps the focus.
    p.exStyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE;
    p.x = b.left - border_;
    p.y = b.top - border_;
    p.width  = cellW_ + 2 * border_;
    p.height = n * cellH_ + 2 * border_;
    p.parent = owner;  // for a popup this is the *owner*: it closes with it
    if (!win::Window::Create(p))
        return false;
    ShowWindow(Hwnd(), SW_SHOWNOACTIVATE);
    UpdateWindow(Hwnd());
    return true;
}

void FlyoutStrip::Close() {
    if (Hwnd()) DestroyWindow(Hwnd()); // WM_NCDESTROY clears our handle
}

RECT FlyoutStrip::CellRect(int cell) const {
    const int top = border_ + cell * cellH_;
    return {border_, top, border_ + cellW_, top + cellH_};
}

int FlyoutStrip::CellAt(POINT pt) const {
    if (!IsOpen()) return -1;
    ScreenToClient(Hwnd(), &pt);
    for (int i = 0; i < static_cast<int>(alternatives_.size()); ++i) {
        const RECT r = CellRect(i);
        if (PtInRect(&r, pt)) return i;
    }
    return -1;
}

void FlyoutStrip::SetHot(int cell) {
    if (cell == hot_ || !IsOpen()) return;
    hot_ = cell;
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

// NOTE: R3's flyout look is not verified yet. This draws a raised strip; the cell
// under the mouse is sunken, like a pressed toolbar button.
void FlyoutStrip::OnPaint() {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(Hwnd(), &ps);
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    FillSolid(dc, rc, theme::Face());
    DrawEdge(dc, &rc, EDGE_RAISED, BF_RECT);

    HDC iconDc = CreateCompatibleDC(dc);
    HGDIOBJ oldBmp = SelectObject(iconDc, icons_);
    const int size = Scale(iconSize_);
    for (int i = 0; i < static_cast<int>(alternatives_.size()); ++i) {
        const RECT r = CellRect(i);
        if (i == hot_) {
            DrawButtonRing(dc, r, theme::kButtonShadow.Ref(), theme::kButtonHighlight.Ref());
            DrawButtonRing(dc, {r.left + 1, r.top + 1, r.right - 1, r.bottom - 1},
                           theme::Face(), theme::kButtonHighlight.Ref());
        }
        TransparentBlt(dc, r.left + Scale(iconOffsetX_), r.top + Scale(iconOffsetY_),
                       size, size, iconDc, alternatives_[static_cast<size_t>(i)].icon * iconSize_, 0,
                       iconSize_, iconSize_, RGB(255, 0, 255));
    }
    SelectObject(iconDc, oldBmp);
    DeleteDC(iconDc);
    EndPaint(Hwnd(), &ps);
}

LRESULT FlyoutStrip::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        OnPaint();
        return 0;
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE; // clicking the strip must not steal activation
    }
    return win::Window::HandleMessage(msg, wp, lp);
}

} // namespace ui
