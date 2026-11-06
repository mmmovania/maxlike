#include "ui/CommandPanel.h"
#include "ui/Bevel.h"
#include "ui/Theme.h"
#include "win/App.h"
#include "../../res/resource.h"
#include <windowsx.h>
#include <commdlg.h>
#include <algorithm>

namespace ui {

using namespace cmdpanel;

// The colour that joins the selected tab to the body (measured: rows 28-29 under the
// Create tab are #C0C0C0, not the face colour).
static constexpr theme::Rgb kTabJoin{0xC0, 0xC0, 0xC0};
// The pressed category button's interior (measured).
static constexpr theme::Rgb kPressedFace{0xE1, 0xE1, 0xE1};
static constexpr int kDropdownId = 1;

bool CommandPanel::Create(HWND parent) {
    icons_ = static_cast<HBITMAP>(LoadImageW(win::App::Instance(), MAKEINTRESOURCEW(IDB_PANEL_TAB_ICONS),
                                             IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    categoryIcons_ = static_cast<HBITMAP>(LoadImageW(win::App::Instance(), MAKEINTRESOURCEW(IDB_PANEL_CATEGORY_ICONS),
                                                     IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    CreateParams p;
    p.title = L"Command panel";
    p.style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    p.parent = parent;
    if (!win::Window::Create(p)) return false;
    // A real Win32 combo box: with classic controls it draws exactly R3's dropdown. Its
    // closed height comes from the font (21 px with MS Sans Serif 8); the height given
    // here is for the open list.
    dropdown_ = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
                                Scale(kDropdown.left), Scale(kDropdown.top), Scale(kDropdown.right - kDropdown.left + 1),
                                Scale(200), Hwnd(), reinterpret_cast<HMENU>(static_cast<INT_PTR>(kDropdownId)),
                                win::App::Instance(), nullptr);
    FillDropdown();
    return true;
}

void CommandPanel::FillDropdown() {
    if (!dropdown_) return;
    SendMessageW(dropdown_, CB_RESETCONTENT, 0, 0);
    for (const std::wstring& item : Subcategories(category_))
        SendMessageW(dropdown_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.c_str()));
    SendMessageW(dropdown_, CB_SETCURSEL, 0, 0);
}

void CommandPanel::UpdateCreateControls() {
    if (dropdown_) ShowWindow(dropdown_, selected_ == kCreate ? SW_SHOW : SW_HIDE);
}

bool CommandPanel::ShowsRollouts() const {
    // NOTE: other categories and types have their own rollouts in R3 (later phases).
    if (selected_ != kCreate || category_ != kGeometry || !dropdown_) return false;
    return SendMessageW(dropdown_, CB_GETCURSEL, 0, 0) == 0;   // "Standard Primitives"
}

void CommandPanel::SetScroll(int scroll) {
    scroll = std::clamp(scroll, 0, rollup::MaxScroll(rollouts_));
    if (scroll == scroll_) return;
    scroll_ = scroll;
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

int CommandPanel::TypeButtonAt(int x, int y) const {
    if (!ShowsRollouts() || !rollouts_[0].open) return -1;
    const int titleTop = rollup::Layout(rollouts_, scroll_)[0].titleTop;
    for (int i = 0; i < 10; ++i) {
        const Box b = TypeButtonBox(i, titleTop);
        if (x >= b.left && x <= b.right && y >= b.top && y <= b.bottom && y >= rollup::kContainer.top + 1 &&
            y <= rollup::kContainer.bottom - 1)
            return i;
    }
    return -1;
}

// The rollup, measured: a 1-px sunken container; each rollout an etched frame with a
// raised title button over its top edge (2-px highlight top and left, 2-px shadow
// bottom and right), "-" when open at x 17..20, 8 rows below the title's top.
void CommandPanel::PaintRollouts(HDC dc) {
    auto px = [&](int x0, int y0, int x1, int y1, COLORREF c) {
        if (x1 < x0 || y1 < y0) return;
        FillSolid(dc, {Scale(x0), Scale(y0), Scale(x1 + 1), Scale(y1 + 1)}, c);
    };
    const COLORREF hi = theme::kButtonHighlight.Ref(), sh = theme::kButtonShadow.Ref();
    const rollup::Box& k = rollup::kContainer;
    px(k.left, k.top, k.right - 1, k.top, sh);          // sunken: shadow top and left,
    px(k.left, k.top, k.left, k.bottom, sh);              // (down to the bottom row, measured)
    px(k.right, k.top, k.right, k.bottom, hi);          // highlight right and bottom
    px(k.left + 1, k.bottom, k.right, k.bottom, hi);

    // Everything else stays inside the container.
    const int saved = SaveDC(dc);
    IntersectClipRect(dc, Scale(k.left + 1), Scale(k.top + 1), Scale(k.right), Scale(k.bottom));
    HGDIOBJ oldFont = SelectObject(dc, font_ ? font_ : GetStockObject(DEFAULT_GUI_FONT));
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(0, 0, 0));

    static const wchar_t* kTitles[] = {L"Object Type", L"Name and Color"};
    const auto placed = rollup::Layout(rollouts_, scroll_);
    for (size_t i = 0; i < placed.size(); ++i) {
        const rollup::Placed& r = placed[i];
        if (r.open) {
            // Etched frame, measured: a shadow rectangle, then a highlight rectangle 1 px
            // down and right drawn over it (so the highlight wins where they cross; not
            // quite DrawEdge's EDGE_ETCHED). Its top edge stops 1 px short of the title.
            const int fl = rollup::kFrameLeft, fr = rollup::kFrameRight, ft = r.frameTop, fb = r.frameBottom;
            auto outline = [&](int l, int t, int rr, int bb, COLORREF c) {
                px(l, t, rr, t, c); px(l, bb, rr, bb, c); px(l, t, l, bb, c); px(rr, t, rr, bb, c);
            };
            outline(fl, ft, fr - 1, fb - 1, sh);
            outline(fl + 1, ft + 1, fr, fb, hi);
            px(rollup::kTitleLeft - 1, ft, rollup::kTitleRight + 1, ft + 1, theme::Face());
            if (i == 0) PaintObjectType(dc, r.titleTop);
            if (i == 1) PaintNameAndColor(dc, r.titleTop);
        }
        // Title button over the frame's top edge.
        const int L = rollup::kTitleLeft, R = rollup::kTitleRight, T = r.titleTop, B = r.titleBottom;
        px(L, T, R, B, theme::Face());
        px(L, T, R - 1, T, hi);  px(L, T + 1, R - 2, T + 1, hi);       // top, 2 px
        px(L, T, L, B, hi);      px(L + 1, T, L + 1, B - 1, hi);       // left, 2 px
        px(R, T, R, B, sh);      px(R - 1, T + 1, R - 1, B, sh);       // right, 2 px
        px(L + 1, B, R, B, sh);  px(L + 2, B - 1, R, B - 1, sh);       // bottom, 2 px
        px(17, T + 8, 20, T + 8, RGB(0, 0, 0));                        // "-"
        if (!r.open) px(18, T + 6, 19, T + 10, RGB(0, 0, 0));          // "+" (NOTE: unverified)
        RECT text{Scale(L + 2), Scale(T + 2), Scale(R - 1), Scale(B - 1)};
        DrawTextW(dc, kTitles[i], -1, &text, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
    SelectObject(dc, oldFont);
    RestoreDC(dc, saved);
}

// Object Type for Standard Primitives, measured: the disabled AutoGrid checkbox, then
// ten 69 x 18 buttons (1-px highlight top and left; dark shadow right and bottom with a
// shadow line inside). A checked button looks pressed (NOTE: R3's checked look is
// unverified; the category row's pressed look is used).
void CommandPanel::PaintObjectType(HDC dc, int titleTop) {
    auto px = [&](int x0, int y0, int x1, int y1, COLORREF c) {
        if (x1 < x0 || y1 < y0) return;
        FillSolid(dc, {Scale(x0), Scale(y0), Scale(x1 + 1), Scale(y1 + 1)}, c);
    };
    const COLORREF hi = theme::kButtonHighlight.Ref(), sh = theme::kButtonShadow.Ref(), dk = RGB(0, 0, 0);

    // AutoGrid: a classic disabled checkbox, text on its left drawn embossed.
    RECT box{Scale(kAutoGridBoxLeft), Scale(titleTop + kAutoGridTop), Scale(kAutoGridBoxLeft + 13), Scale(titleTop + kAutoGridTop + 13)};
    DrawFrameControl(dc, &box, DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_INACTIVE);
    const wchar_t* autoGrid = L"AutoGrid";
    SIZE size{};
    GetTextExtentPoint32W(dc, autoGrid, 8, &size);
    DrawStateW(dc, nullptr, nullptr, reinterpret_cast<LPARAM>(autoGrid), 8,
               Scale(kAutoGridTextRight + 1) - size.cx, Scale(titleTop + kAutoGridTop + 13) - size.cy, 0, 0,
               DST_TEXT | DSS_DISABLED);

    const auto names = StandardPrimitiveNames();
    for (int i = 0; i < static_cast<int>(names.size()); ++i) {
        const Box b = TypeButtonBox(i, titleTop);
        const int L = b.left, T = b.top, R = b.right, B = b.bottom;
        const bool checked = i == createType_;
        px(L, T, R, B, checked ? RGB(0xE1, 0xE1, 0xE1) : theme::Face());
        if (checked) {
            px(L, T, R - 1, T, sh);  px(L, T, L, B, sh);
            px(R, T, R, B, hi);      px(L + 1, B, R, B, hi);
        } else {
            px(L, T, R - 1, T, hi);  px(L, T, L, B, hi);               // highlight top and left
            px(R, T, R, B, dk);      px(L + 1, B, R, B, dk);           // dark shadow right and bottom
            px(R - 1, T + 1, R - 1, B - 1, sh);                        // shadow inside them
            px(L + 2, B - 1, R - 1, B - 1, sh);
        }
        RECT text{Scale(L) + (checked ? 1 : 0), Scale(T) + (checked ? 1 : 0), Scale(R + 1), Scale(B)};
        DrawTextW(dc, names[static_cast<size_t>(i)].c_str(), -1, &text, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
}

// Name and Color, measured with nothing selected:
//   name field x 15..146, rows +23..+42 (title-relative): outer 1-px sunken edge (shadow
//   top and left, highlight right and bottom); inside it black top and left, face right
//   and bottom; then white: one row on top, two columns on each side; face inside
//   (the field is disabled until an object is selected; its enabled look: Phase 6).
//   swatch x 150..166, same rows: 1-px sunken edge filled with the colour.
void CommandPanel::PaintNameAndColor(HDC dc, int titleTop) {
    auto px = [&](int x0, int y0, int x1, int y1, COLORREF c) {
        if (x1 < x0 || y1 < y0) return;
        FillSolid(dc, {Scale(x0), Scale(y0), Scale(x1 + 1), Scale(y1 + 1)}, c);
    };
    const COLORREF hi = theme::kButtonHighlight.Ref(), sh = theme::kButtonShadow.Ref(), face = theme::Face();
    auto sunken = [&](const Box& b) {
        px(b.left, b.top, b.right - 1, b.top, sh);
        px(b.left, b.top, b.left, b.bottom, sh);
        px(b.right, b.top, b.right, b.bottom, hi);
        px(b.left + 1, b.bottom, b.right, b.bottom, hi);
    };
    const Box f = NameFieldBox(titleTop);
    sunken(f);
    px(f.left + 1, f.top + 1, f.right - 2, f.top + 1, RGB(0, 0, 0));      // black top
    px(f.left + 1, f.top + 1, f.left + 1, f.bottom - 1, RGB(0, 0, 0));    // black left
    px(f.right - 1, f.top + 1, f.right - 1, f.bottom - 1, face);          // face right
    px(f.left + 2, f.bottom - 1, f.right - 1, f.bottom - 1, face);        // face bottom
    px(f.left + 2, f.top + 2, f.right - 2, f.top + 2, hi);                // white top row
    px(f.left + 2, f.top + 2, f.left + 3, f.bottom - 2, hi);              // white left, 2 px
    px(f.right - 3, f.top + 2, f.right - 2, f.bottom - 2, hi);            // white right, 2 px
    px(f.left + 4, f.top + 3, f.right - 4, f.bottom - 2, face);           // disabled: face inside

    const Box w = SwatchBox(titleTop);
    sunken(w);
    const COLORREF c = RGB((objectColor_ >> 16) & 0xFF, (objectColor_ >> 8) & 0xFF, objectColor_ & 0xFF);
    px(w.left + 1, w.top + 1, w.right - 1, w.bottom - 1, c);
}

void CommandPanel::SetObjectColor(unsigned rgb) {
    objectColor_ = rgb & 0xFFFFFF;
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

// NOTE: R3 opens its own "Object Color" dialog (a fixed palette); until that is
// built, Windows' standard colour dialog stands in.
void CommandPanel::PickObjectColor() {
    static COLORREF custom[16] = {};
    CHOOSECOLORW cc{};
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = Hwnd();
    cc.rgbResult = RGB((objectColor_ >> 16) & 0xFF, (objectColor_ >> 8) & 0xFF, objectColor_ & 0xFF);
    cc.lpCustColors = custom;
    cc.Flags = CC_RGBINIT | CC_FULLOPEN;
    if (!ChooseColorW(&cc)) return;
    SetObjectColor((GetRValue(cc.rgbResult) << 16) | (GetGValue(cc.rgbResult) << 8) | GetBValue(cc.rgbResult));
    if (onObjectColorChanged) onObjectColorChanged(objectColor_);
}

void CommandPanel::SelectCategory(int c) {
    if (c < 0 || c >= kCategoryCount || c == category_) return;
    category_ = c;
    FillDropdown();
    InvalidateRect(Hwnd(), nullptr, FALSE);
    if (onCategoryChanged) onCategoryChanged(c);
    if (onSubcategoryChanged) onSubcategoryChanged(Subcategories(c).front());
}

// Category row (Create tab only), measured: the pressed button (x 9..32, rows 37..59)
// has a 1-px shadow top and left, a 2-px highlight right and bottom, a face-coloured
// inner line along its top and left, and #E1E1E1 inside; its icon moves 1 px down and
// right. Unpressed buttons show only their icon. NOTE: the hover look is unverified.
void CommandPanel::PaintCategories(HDC dc) {
    auto px = [&](int x0, int y0, int x1, int y1, COLORREF c) {
        if (x1 < x0 || y1 < y0) return;
        FillSolid(dc, {Scale(x0), Scale(y0), Scale(x1 + 1), Scale(y1 + 1)}, c);
    };
    const Box b = CategoryBox(category_);
    const int L = b.left, T = b.top, R = b.right, B = b.bottom;
    const COLORREF hi = theme::kButtonHighlight.Ref(), sh = theme::kButtonShadow.Ref();
    px(L, T, R - 1, T, sh);                 // top
    px(L, T, L, B, sh);                     // left
    px(R, T, R, B, hi);                     // right, 2 px
    px(R - 1, T + 1, R - 1, B, hi);
    px(L + 1, B, R, B, hi);                 // bottom, 2 px
    px(L + 2, B - 1, R, B - 1, hi);
    px(L + 1, T + 1, R - 2, T + 1, theme::Face());
    px(L + 1, T + 1, L + 1, B - 1, theme::Face());
    px(L + 2, T + 2, R - 2, B - 2, kPressedFace.Ref());

    if (categoryIcons_) {
        HDC idc = CreateCompatibleDC(dc);
        HGDIOBJ old = SelectObject(idc, categoryIcons_);
        for (int c = 0; c < kCategoryCount; ++c) {
            const bool pressed = c == category_;
            TransparentBlt(dc, Scale(CategoryIconLeft(c, pressed)), Scale(CategoryIconTop(pressed)),
                           Scale(kCategoryIconSize), Scale(kCategoryIconSize), idc, c * kCategoryIconSize, 0,
                           kCategoryIconSize, kCategoryIconSize, RGB(255, 0, 255));
        }
        SelectObject(idc, old);
        DeleteDC(idc);
    }
}

void CommandPanel::SelectTab(int tab) {
    if (tab < 0 || tab >= kTabCount || tab == selected_) return;
    selected_ = tab;
    UpdateCreateControls();
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
    if (selected_ == kCreate) PaintCategories(dc);
    if (ShowsRollouts()) PaintRollouts(dc);
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
        if (t >= 0) { SelectTab(t); return 0; }
        if (selected_ == kCreate) {
            const int c = CategoryAt(x, y);
            if (c >= 0) { SelectCategory(c); return 0; }
        }
        if (!ShowsRollouts()) return 0;
        // A title opens or closes its rollout.
        const int title = rollup::TitleAt(rollouts_, scroll_, x, y);
        if (title >= 0) {
            rollouts_[static_cast<size_t>(title)].open = !rollouts_[static_cast<size_t>(title)].open;
            scroll_ = std::clamp(scroll_, 0, rollup::MaxScroll(rollouts_));
            InvalidateRect(Hwnd(), nullptr, FALSE);
            return 0;
        }
        // The colour swatch opens the colour picker.
        if (rollouts_[1].open) {
            const Box w = SwatchBox(rollup::Layout(rollouts_, scroll_)[1].titleTop);
            if (x >= w.left && x <= w.right && y >= w.top && y <= w.bottom) { PickObjectColor(); return 0; }
        }
        // A primitive button checks (or unchecks) its type.
        const int type = TypeButtonAt(x, y);
        if (type >= 0) {
            createType_ = (type == createType_) ? -1 : type;
            InvalidateRect(Hwnd(), nullptr, FALSE);
            if (onCreateTypeChanged) onCreateTypeChanged(createType_);
            return 0;
        }
        // Anywhere else in the rollup: drag to scroll, if there is anything to scroll.
        if (rollup::MaxScroll(rollouts_) > 0 && y > rollup::kContainer.top && y < rollup::kContainer.bottom) {
            dragging_ = true;
            dragStartY_ = GET_Y_LPARAM(lp);
            dragStartScroll_ = scroll_;
            SetCapture(Hwnd());
        }
        return 0;
    }
    case WM_MOUSEMOVE:
        if (dragging_) SetScroll(dragStartScroll_ - (GET_Y_LPARAM(lp) - dragStartY_) * 96 / static_cast<int>(Dpi()));
        return 0;
    case WM_LBUTTONUP:
        if (dragging_) ReleaseCapture();
        return 0;
    case WM_CAPTURECHANGED:
        dragging_ = false;
        return 0;
    case WM_MOUSEWHEEL:
        if (ShowsRollouts()) SetScroll(scroll_ - GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA * 30);   // NOTE: step unverified
        return 0;
    case WM_SETCURSOR:
        // The hand shows where the rollup can be dragged (R3: over its empty space).
        if (LOWORD(lp) == HTCLIENT && ShowsRollouts() && rollup::MaxScroll(rollouts_) > 0) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(Hwnd(), &pt);
            const int x = pt.x * 96 / static_cast<int>(Dpi()), y = pt.y * 96 / static_cast<int>(Dpi());
            if (dragging_ || (y > rollup::kContainer.top && y < rollup::kContainer.bottom &&
                              rollup::TitleAt(rollouts_, scroll_, x, y) < 0 && TypeButtonAt(x, y) < 0)) {
                SetCursor(LoadCursorW(nullptr, IDC_HAND));
                return TRUE;
            }
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wp) == kDropdownId && HIWORD(wp) == CBN_SELCHANGE && onSubcategoryChanged) {
            const LRESULT i = SendMessageW(dropdown_, CB_GETCURSEL, 0, 0);
            const auto items = Subcategories(category_);
            if (i >= 0 && static_cast<size_t>(i) < items.size()) onSubcategoryChanged(items[static_cast<size_t>(i)]);
            InvalidateRect(Hwnd(), nullptr, FALSE);   // the rollouts depend on the choice
        }
        return 0;
    case WM_SETFONT:
        font_ = reinterpret_cast<HFONT>(wp);
        if (dropdown_) SendMessageW(dropdown_, WM_SETFONT, wp, lp);   // the dropdown uses the UI font
        return 0;
    case WM_GETFONT:
        return reinterpret_cast<LRESULT>(font_);
    case WM_DESTROY:
        if (icons_) { DeleteObject(icons_); icons_ = nullptr; }
        if (categoryIcons_) { DeleteObject(categoryIcons_); categoryIcons_ = nullptr; }
        break;
    }
    return win::Window::HandleMessage(msg, wp, lp);
}

} // namespace ui
