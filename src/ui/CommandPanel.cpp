#include "ui/CommandPanel.h"
#include "ui/Bevel.h"
#include "ui/Theme.h"
#include "win/App.h"
#include "../../res/resource.h"
#include <windowsx.h>

namespace ui {

using namespace cmdpanel;

// The colour that joins the selected tab to the body (measured: rows 28-29 under the
// Create tab are #C0C0C0, not the face colour).
static constexpr theme::Rgb kTabJoin{0xC0, 0xC0, 0xC0};

bool CommandPanel::Create(HWND parent) {
    icons_ = static_cast<HBITMAP>(LoadImageW(win::App::Instance(), MAKEINTRESOURCEW(IDB_PANEL_TAB_ICONS),
                                             IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    CreateParams p;
    p.title = L"Command panel";
    p.style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    p.parent = parent;
    return win::Window::Create(p);
}

void CommandPanel::SelectTab(int tab) {
    if (tab < 0 || tab >= kTabCount || tab == selected_) return;
    selected_ = tab;
    InvalidateRect(Hwnd(), nullptr, FALSE);
    if (onTabChanged) onTabChanged(tab);
}

// Tabs, measured (x relative to a tab's left edge L and right edge R, inclusive):
//   unselected: row 3 highlight L+2..R-2; row 4 highlight L+1, light L+2..R-2, dark R-1;
//               rows 5..27 highlight L, light L+1, face, shadow R-1, dark R.
//   selected:   the same shape 2 px wider on each side and starting 2 rows higher, its
//               sides continuing to row 28; rows 28-29 between its sides are #C0C0C0,
//               joining it to the body.
//   body:       row 28 highlight, row 29 light across the panel (the frame draws the
//               left and right edges).
// The selected tab is drawn last, over its neighbours.
void CommandPanel::PaintTabs(HDC dc) {
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    const COLORREF hi = theme::kButtonHighlight.Ref(), lt = theme::kLight.Ref();
    const COLORREF sh = theme::kButtonShadow.Ref(), dk = theme::kDarkShadow.Ref();
    auto px = [&](int x0, int y0, int x1, int y1, COLORREF c) {  // inclusive, in 100 % units
        if (x1 < x0 || y1 < y0) return;
        FillSolid(dc, {Scale(x0), Scale(y0), Scale(x1 + 1), Scale(y1 + 1)}, c);
    };
    const int panelRight = rc.right > 0 ? (rc.right * 96 + static_cast<int>(Dpi()) - 1) / static_cast<int>(Dpi()) - 1 : 0;

    // Body top edge (rows 28-29), from x 2 to the frame's right edge.
    px(2, kBodyTop, panelRight - 3, kBodyTop, hi);
    px(2, kBodyTop + 1, panelRight - 3, kBodyTop + 1, lt);

    auto tab = [&](int t, bool selected) {
        const Box b = TabBox(t, selected);
        const int L = b.left, R = b.right, top = b.top;
        const int sideBottom = selected ? kBodyTop : kTabBottom;
        px(L + 2, top, R - 2, top, hi);
        px(L + 1, top + 1, L + 1, top + 1, hi);
        px(L + 2, top + 1, R - 2, top + 1, lt);
        px(R - 1, top + 1, R - 1, top + 1, dk);
        px(L, top + 2, L, sideBottom, hi);
        px(L + 1, top + 2, L + 1, sideBottom, lt);
        px(L + 2, top + 2, R - 2, kTabBottom, theme::Face());
        px(R - 1, top + 2, R - 1, sideBottom, sh);
        px(R, top + 2, R, sideBottom, dk);
        if (selected) {
            px(L + 2, kBodyTop, R - 2, kBodyTop, kTabJoin.Ref());
            px(L + 2, kBodyTop + 1, R - 1, kBodyTop + 1, kTabJoin.Ref());
        }
    };
    for (int t = 0; t < kTabCount; ++t)
        if (t != selected_) tab(t, false);
    tab(selected_, true);

    // Icons (the same place whether the tab is selected or not).
    if (icons_) {
        HDC idc = CreateCompatibleDC(dc);
        HGDIOBJ old = SelectObject(idc, icons_);
        for (int t = 0; t < kTabCount; ++t)
            TransparentBlt(dc, Scale(IconLeft(t)), Scale(kIconTop), Scale(kIconWidth), Scale(kIconHeight),
                           idc, t * kIconWidth, 0, kIconWidth, kIconHeight, RGB(255, 0, 255));
        SelectObject(idc, old);
        DeleteDC(idc);
    }
}

void CommandPanel::OnPaint() {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(Hwnd(), &ps);
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    FillSolid(dc, rc, theme::Face());
    DrawFrameEdge(dc, rc, Edge::CommandPanel);
    PaintTabs(dc);
    EndPaint(Hwnd(), &ps);
}

LRESULT CommandPanel::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        OnPaint();
        return 0;
    case WM_LBUTTONDOWN: {
        // Hit-test in 100 % units.
        const int x = GET_X_LPARAM(lp) * 96 / static_cast<int>(Dpi());
        const int y = GET_Y_LPARAM(lp) * 96 / static_cast<int>(Dpi());
        const int t = TabAt(x, y, selected_);
        if (t >= 0) SelectTab(t);
        return 0;
    }
    case WM_SETFONT:
        font_ = reinterpret_cast<HFONT>(wp);
        return 0;
    case WM_GETFONT:
        return reinterpret_cast<LRESULT>(font_);
    case WM_DESTROY:
        if (icons_) { DeleteObject(icons_); icons_ = nullptr; }
        break;
    }
    return win::Window::HandleMessage(msg, wp, lp);
}

} // namespace ui
