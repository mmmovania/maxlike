#include "ui/Toolbar.h"
#include "ui/Bevel.h"
#include "ui/Theme.h"
#include "win/App.h"
#include "resource.h"   // res/resource.h
#include <commctrl.h>
#include <windowsx.h>

namespace ui {

using toolbar::Kind;

// Child-window IDs of the combo boxes: kComboIdBase + item index.
static constexpr int kComboIdBase = 1000;

// Holding a flyout button this long opens its strip. NOTE: R3's delay is not verified.
static constexpr UINT_PTR kFlyoutTimer = 1;
static constexpr UINT kFlyoutDelayMs = 300;

Toolbar::~Toolbar() {
    if (icons_) DeleteObject(icons_);
}

bool Toolbar::Create(HWND parent, toolbar::Config config) {
    config_ = std::move(config);
    items_ = config_.items;
    checked_.resize(items_.size());
    for (size_t i = 0; i < items_.size(); ++i)
        checked_[i] = items_[i].checked;

    // LR_CREATEDIBSECTION keeps the exact colours (no conversion to the display format).
    const int bitmapId = config_.icons == toolbar::IconStrip::Main ? IDB_TOOLBAR_ICONS : IDB_STATUS_ICONS;
    icons_ = static_cast<HBITMAP>(LoadImageW(win::App::Instance(), MAKEINTRESOURCEW(bitmapId),
                                             IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    CreateParams p;
    p.title  = L"Main toolbar";
    // WS_CLIPCHILDREN: our painting must not draw over the combo boxes.
    p.style  = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    p.x = p.y = 0;
    p.width = p.height = 0;
    p.parent = parent;
    if (!win::Window::Create(p))
        return false;
    CreateControls();
    CreateTooltips();
    return true;
}

void Toolbar::CreateControls() {
    combos_.assign(items_.size(), nullptr);
    editOldProcs_.assign(items_.size(), nullptr);
    for (size_t i = 0; i < items_.size(); ++i) {
        const toolbar::Item& it = items_[i];
        if (it.kind == Kind::Edit) {
            // A borderless EDIT; the toolbar draws R3's frame around it.
            HWND edit = CreateWindowExW(0, L"EDIT", it.text.c_str(),
                                        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                        0, 0, 0, 0, Hwnd(),
                                        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kComboIdBase + i)),
                                        win::App::Instance(), nullptr);
            // Subclass it to catch Enter (see StatusBar for the same technique).
            SetWindowLongPtrW(edit, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
            editOldProcs_[i] = reinterpret_cast<WNDPROC>(
                SetWindowLongPtrW(edit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Toolbar::EditProc)));
            combos_[i] = edit;
            continue;
        }
        if (it.kind != Kind::Combo) continue;
        // DROPDOWNLIST: pick from a list only. DROPDOWN: a list plus an edit box.
        const DWORD kindStyle = it.editable ? (CBS_DROPDOWN | CBS_AUTOHSCROLL) : CBS_DROPDOWNLIST;
        HWND combo = CreateWindowExW(0, L"COMBOBOX", L"",
                                     WS_CHILD | WS_VISIBLE | WS_VSCROLL | kindStyle,
                                     0, 0, 0, 0, Hwnd(),
                                     reinterpret_cast<HMENU>(static_cast<INT_PTR>(kComboIdBase + i)),
                                     win::App::Instance(), nullptr);
        for (const auto& choice : it.choices)
            SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(choice.c_str()));
        const LRESULT sel = SendMessageW(combo, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1),
                                         reinterpret_cast<LPARAM>(it.text.c_str()));
        if (sel != CB_ERR) SendMessageW(combo, CB_SETCURSEL, static_cast<WPARAM>(sel), 0);
        combos_[i] = combo;
    }
}

// The comctl32 version this app uses (v5, for the classic look) predates the last
// field of TTTOOLINFOW; passing the full sizeof makes TTM_ADDTOOL fail. Use the V2 size.
static TTTOOLINFOW MakeToolInfo(HWND owner) {
    TTTOOLINFOW ti{};
    ti.cbSize = TTTOOLINFOW_V2_SIZE;
    ti.hwnd = owner;
    return ti;
}

void Toolbar::CreateTooltips() {
    tooltip_ = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
                               WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                               Hwnd(), nullptr, win::App::Instance(), nullptr);
    if (!tooltip_) return;
    for (size_t i = 0; i < items_.size(); ++i) {
        const toolbar::Item& it = items_[i];
        TTTOOLINFOW ti = MakeToolInfo(Hwnd());
        ti.lpszText = const_cast<wchar_t*>(it.tooltip.c_str());
        if (it.kind == Kind::Button) {
            // A rectangle of our window is the "tool"; TTF_SUBCLASS lets the tooltip
            // watch our mouse messages itself.
            ti.uFlags = TTF_SUBCLASS;
            ti.uId = i;
        } else if ((it.kind == Kind::Combo || it.kind == Kind::Edit) && combos_[i]) {
            // A whole child window is the "tool".
            ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
            ti.uId = reinterpret_cast<UINT_PTR>(combos_[i]);
        } else {
            continue;
        }
        SendMessageW(tooltip_, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti));
    }
}

LRESULT CALLBACK Toolbar::EditProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    auto* self = reinterpret_cast<Toolbar*>(GetWindowLongPtrW(h, GWLP_USERDATA));
    const size_t index = static_cast<size_t>(GetDlgCtrlID(h) - kComboIdBase);
    if (!self || index >= self->editOldProcs_.size())
        return DefWindowProcW(h, msg, wp, lp);
    if (msg == WM_SETFOCUS) {
        // Select everything on entry so typing replaces the number. (Classic EDIT
        // controls have no Ctrl+A.)
        const LRESULT r = CallWindowProcW(self->editOldProcs_[index], h, msg, wp, lp);
        PostMessageW(h, EM_SETSEL, 0, -1); // posted: the click that focused us comes first
        return r;
    }
    if (msg == WM_CHAR && wp == VK_RETURN) {
        const int len = GetWindowTextLengthW(h);
        std::wstring text(static_cast<size_t>(len), L'\0');
        if (len > 0) GetWindowTextW(h, text.data(), len + 1);
        if (self->onEditCommitted) self->onEditCommitted(self->items_[index].command, text);
        SetFocus(GetAncestor(h, GA_ROOT)); // done typing: keyboard back to the app
        return 0;
    }
    return CallWindowProcW(self->editOldProcs_[index], h, msg, wp, lp);
}

void Toolbar::SetEditText(const std::string& command, const std::wstring& text) {
    for (size_t i = 0; i < items_.size(); ++i)
        if (items_[i].kind == Kind::Edit && items_[i].command == command && combos_[i])
            SetWindowTextW(combos_[i], text.c_str());
}

void Toolbar::SetChecked(const std::string& command, bool checked) {
    for (size_t i = 0; i < items_.size(); ++i) {
        const toolbar::Item& it = items_[i];
        bool match = it.kind == Kind::Button && it.command == command;
        for (const toolbar::Alternative& a : it.flyout) match = match || a.command == command;
        if (match && checked_[i] != checked) {
            checked_[i] = checked;
            InvalidateItem(static_cast<int>(i));
        }
    }
}

void Toolbar::ShowStrip(int index, const std::wstring& label) {
    strip_ = index;
    stripLabel_ = label;
    hot_ = pressed_ = -1;
    const bool main = (index == 0);
    for (HWND combo : combos_)
        if (combo) ShowWindow(combo, main ? SW_SHOW : SW_HIDE);
    if (tooltip_) SendMessageW(tooltip_, TTM_ACTIVATE, main, 0);
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

void Toolbar::Relayout() {
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    rects_ = toolbar::LayoutItems(config_, rc.right, rc.bottom, Dpi());

    for (size_t i = 0; i < items_.size(); ++i) {
        const Rect& r = rects_[i];
        if (combos_.size() > i && combos_[i] && items_[i].kind == Kind::Edit) {
            // Inside the 2-px frame the toolbar draws.
            SetWindowPos(combos_[i], nullptr, r.left + 2, r.top + 2, r.Width() - 4, r.Height() - 4,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        } else if (combos_.size() > i && combos_[i]) {
            // For a combo box the height passed here is the *dropped-down* height;
            // the closed height follows from the font (21 px with our 8 pt font).
            SetWindowPos(combos_[i], nullptr, r.left, r.top, r.Width(), Scale(200),
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
        if (tooltip_ && items_[i].kind == Kind::Button) {
            TTTOOLINFOW ti = MakeToolInfo(Hwnd());
            ti.uId = i;
            ti.rect = {r.left, r.top, r.right, r.bottom};
            SendMessageW(tooltip_, TTM_NEWTOOLRECTW, 0, reinterpret_cast<LPARAM>(&ti));
        }
    }
}

void Toolbar::OnComboCommand(size_t index, int code) {
    HWND combo = combos_[index];
    if (code == CBN_SELCHANGE) {
        const LRESULT sel = SendMessageW(combo, CB_GETCURSEL, 0, 0);
        if (sel == CB_ERR) return;
        const LRESULT len = SendMessageW(combo, CB_GETLBTEXTLEN, static_cast<WPARAM>(sel), 0);
        std::wstring text(static_cast<size_t>(len > 0 ? len : 0), L'\0');
        if (len > 0)
            SendMessageW(combo, CB_GETLBTEXT, static_cast<WPARAM>(sel), reinterpret_cast<LPARAM>(text.data()));
        if (onComboChanged) onComboChanged(items_[index].command, text);
    } else if (code == CBN_CLOSEUP && !items_[index].editable) {
        // Hand the keyboard back to the main window, as R3 does, so shortcut keys
        // keep working instead of scrolling through the list.
        SetFocus(GetAncestor(Hwnd(), GA_ROOT));
    }
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

void Toolbar::OpenFlyout() {
    if (pressed_ < 0 || pressed_ != hot_) return; // released or dragged away already
    const size_t i = static_cast<size_t>(pressed_);
    if (items_[i].flyout.empty()) return;

    // The strip's first cell goes exactly over the button, in screen coordinates.
    const Rect& r = rects_[i];
    POINT tl{r.left, r.top}, br{r.right, r.bottom};
    ClientToScreen(Hwnd(), &tl);
    ClientToScreen(Hwnd(), &br);
    int current = 0;
    for (size_t a = 0; a < items_[i].flyout.size(); ++a)
        if (items_[i].flyout[a].command == items_[i].command) current = static_cast<int>(a);
    if (flyout_.Open(Hwnd(), items_[i].flyout, current, icons_, RECT{tl.x, tl.y, br.x, br.y},
                     config_.iconSize, config_.iconOffsetX, config_.iconOffsetY)) {
        flyoutItem_ = pressed_;
        POINT pt;
        GetCursorPos(&pt);
        flyout_.SetHot(flyout_.CellAt(pt));
    }
}

void Toolbar::CloseFlyout() {
    KillTimer(Hwnd(), kFlyoutTimer);
    flyout_.Close();
    flyoutItem_ = -1;
}

void Toolbar::ChooseAlternative(size_t index, int alternative) {
    toolbar::Item& it = items_[index];
    const toolbar::Alternative& a = it.flyout[static_cast<size_t>(alternative)];
    it.command = a.command;
    it.tooltip = a.tooltip;
    it.icon    = a.icon;
    if (tooltip_) {
        // The tooltip keeps a pointer to the text, so tell it about the new string.
        TTTOOLINFOW ti{};
        ti.cbSize = TTTOOLINFOW_V2_SIZE;
        ti.hwnd = Hwnd();
        ti.uId = index;
        ti.lpszText = const_cast<wchar_t*>(it.tooltip.c_str());
        SendMessageW(tooltip_, TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&ti));
    }
    InvalidateItem(static_cast<int>(index));
}

void Toolbar::Activate(size_t index) {
    const toolbar::Item& it = items_[index];
    if (it.toggle) {
        checked_[index] = !checked_[index];
        InvalidateItem(static_cast<int>(index));
    }
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

static void ButtonRing(HDC dc, const Rect& r, COLORREF tl, COLORREF br) {
    DrawButtonRing(dc, RECT{r.left, r.top, r.right, r.bottom}, tl, br);
}

static Rect Inset(Rect r, int n) { return {r.left + n, r.top + n, r.right - n, r.bottom - n}; }

void Toolbar::PaintButton(HDC dc, HDC iconDc, size_t i) {
    const toolbar::Item& it = items_[i];
    const Rect& r = rects_[i];
    const bool down    = (static_cast<int>(i) == pressed_ && static_cast<int>(i) == hot_);
    const bool checked = checked_[i];
    const COLORREF face = theme::Face();

    const bool labelled = it.icon < 0 && !it.text.empty(); // e.g. "Animate"
    if (checked || down) {
        // Sunken: outer ring shadow/highlight, inner ring face/highlight.
        ButtonRing(dc, r, theme::kButtonShadow.Ref(), theme::kButtonHighlight.Ref());
        ButtonRing(dc, Inset(r, 1), face, theme::kButtonHighlight.Ref());
        COLORREF fill = face;
        if (checked) fill = it.alarm            ? theme::kButtonAlarm.Ref()
                          : (it.group == "mode") ? theme::kButtonModeActive.Ref()
                                                 : theme::kButtonChecked.Ref();
        const Rect in = Inset(r, 2);
        FillSolid(dc, {in.left, in.top, in.right, in.bottom}, fill);
    } else if (labelled) {
        // Labelled buttons are always raised (measured on Animate): two rings
        // highlight / shadow, R3 corner convention.
        ButtonRing(dc, r, theme::kButtonHighlight.Ref(), theme::kButtonShadow.Ref());
        ButtonRing(dc, Inset(r, 1), theme::kButtonHighlight.Ref(), theme::kButtonShadow.Ref());
    } else if (static_cast<int>(i) == hot_) {
        // Hover: thin raised ring. NOTE: R3's hover look is not yet verified.
        ButtonRing(dc, r, theme::kButtonHighlight.Ref(), theme::kButtonShadow.Ref());
    }

    // Icon at the config's offset, scaled with the DPI. Magenta pixels are transparent.
    if (icons_ && it.icon >= 0) {
        const int cell = config_.iconSize;
        const int size = Scale(cell);
        TransparentBlt(dc, r.left + Scale(config_.iconOffsetX), r.top + Scale(config_.iconOffsetY),
                       size, size, iconDc, it.icon * cell, 0, cell, cell, RGB(255, 0, 255));
    }
    if (labelled) {
        RECT t{r.left, r.top, r.right, r.bottom};
        if (checked || down) OffsetRect(&t, 1, 1);
        SetTextColor(dc, GetSysColor(COLOR_BTNTEXT));
        DrawTextW(dc, it.text.c_str(), -1, &t, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }

    // Flyout marker (measured): a small embossed notch in the bottom-right corner.
    // Cells are (row, column) relative to the button's top-left at 100%. A raised
    // button draws the band in highlight and the triangle in black; a sunken
    // (checked or pressed) button draws the band in black and the triangle in shadow.
    if (!it.flyout.empty()) {
        static const int band[][2] = {{25, 29}, {26, 28}, {26, 29}, {27, 27}, {27, 28}, {28, 26}, {28, 27}};
        static const int tri[][2]  = {{27, 29}, {28, 28}, {28, 29}};
        const bool sunken = checked || down;
        const COLORREF bandColour = sunken ? RGB(0, 0, 0) : theme::kButtonHighlight.Ref();
        const COLORREF triColour  = sunken ? theme::kButtonShadow.Ref() : RGB(0, 0, 0);
        // Anchored to the bottom-right corner so it stays in the corner at any DPI.
        const int ox = r.right - toolbar::kButtonWidth + config_.flyoutMarkerDx;
        const int oy = r.bottom - toolbar::kButtonHeight;
        for (const auto& c : band) FillSolid(dc, {ox + c[1], oy + c[0], ox + c[1] + 1, oy + c[0] + 1}, bandColour);
        for (const auto& c : tri)  FillSolid(dc, {ox + c[1], oy + c[0], ox + c[1] + 1, oy + c[0] + 1}, triColour);
    }
}

void Toolbar::PaintSeparator(HDC dc, const Rect& r) {
    // Etched line (measured): shadow at +3, highlight at +4, over the item's height.
    const int x = r.left + Scale(3);
    FillSolid(dc, {x, r.top, x + 1, r.bottom}, theme::kButtonShadow.Ref());
    FillSolid(dc, {x + 1, r.top, x + 2, r.bottom}, theme::kButtonHighlight.Ref());
}

// R3's own edit frame (measured on the frame field): outer ring shadow / highlight,
// inner ring black / face, both with the R3 corner convention.
void Toolbar::PaintEditFrame(HDC dc, const Rect& r) {
    ButtonRing(dc, r, theme::kButtonShadow.Ref(), theme::kButtonHighlight.Ref());
    ButtonRing(dc, Inset(r, 1), RGB(0, 0, 0), theme::Face());
}

void Toolbar::OnPaint() {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(Hwnd(), &ps);
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    FillSolid(dc, rc, theme::Face());
    if (config_.frameEdges) {
        // Frame shared with the tab panel above (measured): highlight column at x=0,
        // shadow column 2 px from the right.
        FillSolid(dc, {0, 0, 1, rc.bottom}, theme::kButtonHighlight.Ref());
        FillSolid(dc, {rc.right - 2, 0, rc.right - 1, rc.bottom}, theme::kButtonShadow.Ref());
    }

    HGDIOBJ oldFont = SelectObject(dc, font_ ? font_ : GetStockObject(DEFAULT_GUI_FONT));
    SetBkMode(dc, TRANSPARENT);

    if (strip_ == 0) {
        HDC iconDc = CreateCompatibleDC(dc);
        HGDIOBJ oldBmp = SelectObject(iconDc, icons_);
        for (size_t i = 0; i < items_.size(); ++i) {
            switch (items_[i].kind) {
            case Kind::Button:    PaintButton(dc, iconDc, i); break;
            case Kind::Separator: PaintSeparator(dc, rects_[i]); break;
            case Kind::Combo:     break; // real child windows draw themselves
            case Kind::Edit:      PaintEditFrame(dc, rects_[i]); break;
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
        // Pass it on: combo boxes and tooltips use our font, as in R3.
        for (HWND combo : combos_)
            if (combo) SendMessageW(combo, WM_SETFONT, wp, lp);
        if (tooltip_) SendMessageW(tooltip_, WM_SETFONT, wp, FALSE);
        Relayout(); // the combo height depends on the font
        if (LOWORD(lp)) InvalidateRect(Hwnd(), nullptr, FALSE);
        return 0;

    case WM_COMMAND: {
        // Notifications from our combo boxes: control ID in LOWORD, code in HIWORD.
        const int id = LOWORD(wp) - kComboIdBase;
        if (id >= 0 && static_cast<size_t>(id) < combos_.size() && combos_[static_cast<size_t>(id)] &&
            items_[static_cast<size_t>(id)].kind == Kind::Combo) {
            OnComboCommand(static_cast<size_t>(id), HIWORD(wp));
            return 0;
        }
        break;
    }
    case WM_GETFONT:
        return reinterpret_cast<LRESULT>(font_);

    case WM_MOUSEMOVE: {
        if (!trackingLeave_) {
            // Ask Windows for a WM_MOUSELEAVE when the mouse leaves us.
            TRACKMOUSEEVENT tme{sizeof(tme), TME_LEAVE, Hwnd(), 0};
            trackingLeave_ = TrackMouseEvent(&tme) != FALSE;
        }
        if (flyout_.IsOpen()) {
            // While a strip is open the mouse is still captured by us; just tell
            // the strip which cell is under the mouse.
            POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            ClientToScreen(Hwnd(), &pt);
            flyout_.SetHot(flyout_.CellAt(pt));
            return 0;
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
            if (!items_[static_cast<size_t>(hit)].flyout.empty())
                SetTimer(Hwnd(), kFlyoutTimer, kFlyoutDelayMs, nullptr); // press-and-hold
        }
        return 0;
    }
    case WM_TIMER:
        if (wp == kFlyoutTimer) {
            KillTimer(Hwnd(), kFlyoutTimer); // one-shot
            OpenFlyout();
            return 0;
        }
        break;
    case WM_LBUTTONUP:
        KillTimer(Hwnd(), kFlyoutTimer); // a quick click never opens the strip
        if (flyout_.IsOpen()) {
            // Read everything before releasing capture: ReleaseCapture sends
            // WM_CAPTURECHANGED immediately, which resets the state.
            const int item = flyoutItem_;
            const int choice = flyout_.Hot();
            CloseFlyout();
            pressed_ = -1;
            ReleaseCapture();
            InvalidateItem(item);
            if (choice >= 0) {
                ChooseAlternative(static_cast<size_t>(item), choice);
                Activate(static_cast<size_t>(item));
            }
            return 0;
        }
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
        // Someone else took the mouse (e.g. a dialog popped up): cancel everything.
        CloseFlyout();
        if (pressed_ >= 0) { InvalidateItem(pressed_); pressed_ = -1; }
        return 0;
    }
    return win::Window::HandleMessage(msg, wp, lp);
}

} // namespace ui
