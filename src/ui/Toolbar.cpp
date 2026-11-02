#include "ui/Toolbar.h"
#include "ui/Bevel.h"
#include "ui/Theme.h"
#include "win/App.h"
#include "resource.h"   // res/resource.h
#include <windowsx.h>

namespace ui {

using toolbar::Kind;

Toolbar::~Toolbar() {
    if (icons_) DeleteObject(icons_);
}

bool Toolbar::Create(HWND parent) {
    items_ = toolbar::MainToolbarSpec();
    checked_.resize(items_.size());
    for (size_t i = 0; i < items_.size(); ++i)
        checked_[i] = items_[i].checked;

    // LR_CREATEDIBSECTION keeps the exact colours (no conversion to the display format).
    icons_ = static_cast<HBITMAP>(LoadImageW(win::App::Instance(),
                                             MAKEINTRESOURCEW(IDB_TOOLBAR_ICONS), IMAGE_BITMAP,
                                             0, 0, LR_CREATEDIBSECTION));
    CreateParams p;
    p.title  = L"Main toolbar";
    p.style  = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
    p.x = p.y = 0;
    p.width = p.height = 0;
    p.parent = parent;
    return win::Window::Create(p);
}

void Toolbar::ShowStrip(int index, const std::wstring& label) {
    strip_ = index;
    stripLabel_ = label;
    hot_ = pressed_ = -1;
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

void Toolbar::Relayout() {
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    rects_ = toolbar::LayoutToolbar(items_, rc.bottom, Dpi());
}

int Toolbar::HitTest(int x, int y) const {
    if (strip_ != 0) return -1;
    for (size_t i = 0; i < items_.size(); ++i) {
        const Rect& r = rects_[i];
        if (items_[i].kind == Kind::Button &&
            x >= r.left && x < r.right && y >= r.top && y < r.bottom)
            return static_cast<int>(i);
    }
    return -1;
}

void Toolbar::InvalidateItem(int index) {
    if (index < 0 || static_cast<size_t>(index) >= rects_.size()) return;
    const Rect& r = rects_[static_cast<size_t>(index)];
    RECT rc{r.left, r.top, r.right, r.bottom};
    InvalidateRect(Hwnd(), &rc, FALSE);
}

void Toolbar::Activate(size_t index) {
    const toolbar::Item& it = items_[index];
    if (!it.group.empty()) {
        // Radio group: exactly one button of the group is checked.
        for (size_t j = 0; j < items_.size(); ++j) {
            if (items_[j].group == it.group && checked_[j] != (j == index)) {
                checked_[j] = (j == index);
                InvalidateItem(static_cast<int>(j));
            }
        }
    }
    if (onCommand) onCommand(it.command);
}

// ---- painting ------------------------------------------------------------

// Measured button frame (R3 checked button), corners as in the reference:
// top-left colour owns the left column and the top row except its last pixel;
// bottom-right colour owns the right column and the bottom row except its first pixel.
static void ButtonRing(HDC dc, const Rect& r, COLORREF tl, COLORREF br) {
    FillSolid(dc, {r.left, r.top, r.left + 1, r.bottom}, tl);           // left
    FillSolid(dc, {r.left, r.top, r.right - 1, r.top + 1}, tl);         // top
    FillSolid(dc, {r.right - 1, r.top, r.right, r.bottom}, br);         // right
    FillSolid(dc, {r.left + 1, r.bottom - 1, r.right, r.bottom}, br);   // bottom
}

static Rect Inset(Rect r, int n) { return {r.left + n, r.top + n, r.right - n, r.bottom - n}; }

void Toolbar::PaintButton(HDC dc, HDC iconDc, size_t i) {
    const toolbar::Item& it = items_[i];
    const Rect& r = rects_[i];
    const bool down    = (static_cast<int>(i) == pressed_ && static_cast<int>(i) == hot_);
    const bool checked = checked_[i];
    const COLORREF face = theme::Face();

    if (checked || down) {
        // Sunken: outer ring shadow/highlight, inner ring face/highlight.
        ButtonRing(dc, r, theme::kButtonShadow.Ref(), theme::kButtonHighlight.Ref());
        ButtonRing(dc, Inset(r, 1), face, theme::kButtonHighlight.Ref());
        COLORREF fill = face;
        if (checked) fill = (it.group == "mode") ? theme::kButtonModeActive.Ref()
                                                 : theme::kButtonChecked.Ref();
        const Rect in = Inset(r, 2);
        FillSolid(dc, {in.left, in.top, in.right, in.bottom}, fill);
    } else if (static_cast<int>(i) == hot_) {
        // Hover: thin raised ring. NOTE: R3's hover look is not yet verified.
        ButtonRing(dc, r, theme::kButtonHighlight.Ref(), theme::kButtonShadow.Ref());
    }

    // Icon, 24x24 at (4,3), scaled with the DPI. Magenta pixels are transparent.
    if (icons_ && it.icon >= 0) {
        const int size = Scale(toolbar::kIconSize);
        TransparentBlt(dc, r.left + Scale(toolbar::kIconOffsetX), r.top + Scale(toolbar::kIconOffsetY),
                       size, size, iconDc, it.icon * toolbar::kIconSize, 0,
                       toolbar::kIconSize, toolbar::kIconSize, RGB(255, 0, 255));
    }

    // Flyout marker (measured): a small embossed notch in the bottom-right corner.
    // Cells are (row, column) relative to the button's top-left at 100%. A raised
    // button draws the band in highlight and the triangle in black; a sunken
    // (checked or pressed) button draws the band in black and the triangle in shadow.
    if (it.flyout) {
        static const int band[][2] = {{25, 29}, {26, 28}, {26, 29}, {27, 27}, {27, 28}, {28, 26}, {28, 27}};
        static const int tri[][2]  = {{27, 29}, {28, 28}, {28, 29}};
        const bool sunken = checked || down;
        const COLORREF bandColour = sunken ? RGB(0, 0, 0) : theme::kButtonHighlight.Ref();
        const COLORREF triColour  = sunken ? theme::kButtonShadow.Ref() : RGB(0, 0, 0);
        // Anchored to the bottom-right corner so it stays in the corner at any DPI.
        const int ox = r.right - toolbar::kButtonWidth, oy = r.bottom - toolbar::kButtonHeight;
        for (const auto& c : band) FillSolid(dc, {ox + c[1], oy + c[0], ox + c[1] + 1, oy + c[0] + 1}, bandColour);
        for (const auto& c : tri)  FillSolid(dc, {ox + c[1], oy + c[0], ox + c[1] + 1, oy + c[0] + 1}, triColour);
    }
}

void Toolbar::PaintSeparator(HDC dc, const Rect& r) {
    // Etched line (measured): shadow at +3, highlight at +4, rows 12..27.
    const int x = r.left + Scale(3);
    const int top = Scale(toolbar::kSeparatorTop);
    const int bottom = top + Scale(toolbar::kSeparatorHeight);
    FillSolid(dc, {x, top, x + 1, bottom}, theme::kButtonShadow.Ref());
    FillSolid(dc, {x + 1, top, x + 2, bottom}, theme::kButtonHighlight.Ref());
}

void Toolbar::PaintCombo(HDC dc, size_t i) {
    // Placeholder until step 1.4b replaces it with a real combo box.
    const Rect& r = rects_[i];
    RECT rc{r.left, r.top, r.right, r.bottom};
    DrawEdge(dc, &rc, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
    FillRect(dc, &rc, GetSysColorBrush(COLOR_WINDOW));
    rc.left += Scale(2);
    SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
    DrawTextW(dc, items_[i].text.c_str(), -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

void Toolbar::OnPaint() {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(Hwnd(), &ps);
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    FillSolid(dc, rc, theme::Face());
    // Frame shared with the tab panel above (measured): highlight column at x=0,
    // shadow column 2 px from the right.
    FillSolid(dc, {0, 0, 1, rc.bottom}, theme::kButtonHighlight.Ref());
    FillSolid(dc, {rc.right - 2, 0, rc.right - 1, rc.bottom}, theme::kButtonShadow.Ref());

    HGDIOBJ oldFont = SelectObject(dc, font_ ? font_ : GetStockObject(DEFAULT_GUI_FONT));
    SetBkMode(dc, TRANSPARENT);

    if (strip_ == 0) {
        HDC iconDc = CreateCompatibleDC(dc);
        HGDIOBJ oldBmp = SelectObject(iconDc, icons_);
        for (size_t i = 0; i < items_.size(); ++i) {
            switch (items_[i].kind) {
            case Kind::Button:    PaintButton(dc, iconDc, i); break;
            case Kind::Separator: PaintSeparator(dc, rects_[i]); break;
            case Kind::Combo:     PaintCombo(dc, i); break;
            case Kind::Gap:       break;
            }
        }
        SelectObject(iconDc, oldBmp);
        DeleteDC(iconDc);
    } else {
        SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
        const std::wstring note = stripLabel_ + L" strip: buttons are not built yet";
        DrawTextW(dc, note.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }

    SelectObject(dc, oldFont);
    EndPaint(Hwnd(), &ps);
}

// ---- messages ------------------------------------------------------------

LRESULT Toolbar::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_SIZE:
        Relayout();
        return 0;
    case WM_DPICHANGED_AFTERPARENT:
        Relayout();
        InvalidateRect(Hwnd(), nullptr, FALSE);
        return 0;
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

    case WM_MOUSEMOVE: {
        if (!trackingLeave_) {
            // Ask Windows for a WM_MOUSELEAVE when the mouse leaves us.
            TRACKMOUSEEVENT tme{sizeof(tme), TME_LEAVE, Hwnd(), 0};
            trackingLeave_ = TrackMouseEvent(&tme) != FALSE;
        }
        const int hit = HitTest(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        if (hit != hot_) {
            InvalidateItem(hot_);
            hot_ = hit;
            InvalidateItem(hot_);
        }
        return 0;
    }
    case WM_MOUSELEAVE:
        trackingLeave_ = false;
        if (pressed_ < 0) { InvalidateItem(hot_); hot_ = -1; }
        return 0;

    case WM_LBUTTONDOWN: {
        const int hit = HitTest(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        if (hit >= 0) {
            pressed_ = hit;
            hot_ = hit;
            SetCapture(Hwnd()); // keep getting mouse messages even outside the window
            InvalidateItem(hit);
        }
        return 0;
    }
    case WM_LBUTTONUP:
        if (pressed_ >= 0) {
            const int was = pressed_;
            const bool inside = (HitTest(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)) == was);
            pressed_ = -1;
            ReleaseCapture();
            InvalidateItem(was);
            if (inside) Activate(static_cast<size_t>(was)); // released over the same button
        }
        return 0;
    case WM_CAPTURECHANGED:
        if (pressed_ >= 0) { InvalidateItem(pressed_); pressed_ = -1; }
        return 0;
    }
    return win::Window::HandleMessage(msg, wp, lp);
}

} // namespace ui
