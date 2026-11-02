#include "ui/TabPanel.h"
#include "ui/Bevel.h"
#include "ui/TabStrip.h"
#include "ui/Theme.h"
#include <windowsx.h> // GET_X_LPARAM

namespace ui {

bool TabPanel::Create(HWND parent) {
    CreateParams p;
    p.title  = L"Tab panel";
    p.style  = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
    p.x = p.y = 0;
    p.width = p.height = 0;
    p.parent = parent;
    return win::Window::Create(p);
}

void TabPanel::SetActiveTab(int index) {
    const int count = static_cast<int>(tabstrip::Tabs().size());
    if (index < 0 || index >= count || index == active_) return;
    active_ = index;
    if (Hwnd()) InvalidateRect(Hwnd(), nullptr, FALSE);
}

// One tab, drawn with 1-px lines exactly as measured at 100% (w = tab width,
// h = panel height, columns/rows relative to the tab's top-left corner):
//
//   row 0      : highlight cols 2..w-4, dark at col w-3         (chamfered top)
//   row 1      : highlight col 1, fill cols 2..w-3, dark col w-2
//   rows 2..h-2: highlight col 0, fill cols 1..w-2, dark col w-1
//   row h-1    : inactive -> highlight across the whole tab (bottom line)
//                active   -> highlight col 0 only; open towards the toolbar
//
// Active: fill = face, dark = black. Inactive: fill = #A0A0B3, dark = #A0A0A0.
void TabPanel::PaintTab(HDC dc, int index, int h) {
    const Rect r = tabstrip::TabRect(index, h, Dpi());
    const int x0 = r.left, w = r.Width();
    const bool active = (index == active_);

    const COLORREF hl   = theme::kTabHighlight.Ref();
    const COLORREF fill = active ? theme::Face() : theme::kTabInactive.Ref();
    const COLORREF dark = active ? theme::kTabActiveEdge.Ref() : theme::kTabInactiveEdge.Ref();

    auto px = [&](int col0, int col1, int row, COLORREF c) {   // horizontal run, cols inclusive
        if (col1 >= col0) FillSolid(dc, {x0 + col0, row, x0 + col1 + 1, row + 1}, c);
    };

    px(2, w - 4, 0, hl);   px(w - 3, w - 3, 0, dark);
    px(1, 1, 1, hl);       px(2, w - 3, 1, fill);   px(w - 2, w - 2, 1, dark);
    if (h > 3) {
        FillSolid(dc, {x0, 2, x0 + 1, h - 1}, hl);
        FillSolid(dc, {x0 + 1, 2, x0 + w - 1, h - 1}, fill);
        FillSolid(dc, {x0 + w - 1, 2, x0 + w, h - 1}, dark);
    }
    if (active) px(0, 0, h - 1, hl);
    else        px(0, w - 1, h - 1, hl);

    // Label: centred in the tab body.
    RECT text{x0 + 2, 1, x0 + w - 2, h - 1};
    const auto& label = tabstrip::Tabs()[static_cast<size_t>(index)].label;
    DrawTextW(dc, label.c_str(), -1, &text,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
}

void TabPanel::OnPaint() {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(Hwnd(), &ps);

    RECT rc;
    GetClientRect(Hwnd(), &rc);
    FillSolid(dc, rc, theme::Face());
    // Measured: the panel sits inside a frame that also surrounds the toolbar:
    // a highlight column at the far left and a shadow column 2 px from the right.
    FillSolid(dc, {0, 0, 1, rc.bottom}, theme::kTabHighlight.Ref());
    FillSolid(dc, {rc.right - 2, 0, rc.right - 1, rc.bottom}, theme::kTabInactiveEdge.Ref());

    HGDIOBJ oldFont = SelectObject(dc, font_ ? font_ : GetStockObject(DEFAULT_GUI_FONT));
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, GetSysColor(COLOR_BTNTEXT));
    const int count = static_cast<int>(tabstrip::Tabs().size());
    for (int i = 0; i < count; ++i)
        PaintTab(dc, i, rc.bottom);
    // The tabs' bottom highlight continues to 4 px from the right edge.
    const int tabsEnd = tabstrip::TabRect(count - 1, rc.bottom, Dpi()).right;
    if (rc.right - 4 > tabsEnd)
        FillSolid(dc, {tabsEnd, rc.bottom - 1, rc.right - 4, rc.bottom}, theme::kTabHighlight.Ref());
    SelectObject(dc, oldFont);

    EndPaint(Hwnd(), &ps);
}

LRESULT TabPanel::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        OnPaint();
        return 0;
    case WM_SETFONT:
        font_ = reinterpret_cast<HFONT>(wp);
        if (LOWORD(lp)) InvalidateRect(Hwnd(), nullptr, FALSE);
        return 0;
    case WM_GETFONT:
        return reinterpret_cast<LRESULT>(font_);
    case WM_LBUTTONDOWN: {
        const int count = static_cast<int>(tabstrip::Tabs().size());
        const int hit = tabstrip::TabAtX(GET_X_LPARAM(lp), count, Dpi());
        if (hit >= 0 && hit != active_) {
            SetActiveTab(hit);
            if (onTabChanged) onTabChanged(hit);
        }
        return 0;
    }
    }
    return win::Window::HandleMessage(msg, wp, lp);
}

} // namespace ui
