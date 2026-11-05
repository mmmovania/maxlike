#include "ui/StatusBar.h"
#include "ui/Bevel.h"
#include "ui/Theme.h"
#include "win/App.h"
#include <commctrl.h>
#include <windowsx.h>
#include <algorithm>
#include <cstdio>

namespace ui {

StatusBar::~StatusBar() {
    if (outputBrush_) DeleteObject(outputBrush_);
}

bool StatusBar::Create(HWND parent) {
    CreateParams p;
    p.title  = L"Status area";
    p.style  = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    p.x = p.y = 0;
    p.width = p.height = 0;
    p.parent = parent;
    if (!win::Window::Create(p))
        return false;

    outputBrush_ = CreateSolidBrush(theme::kListenerBg.Ref());

    // WS_EX_CLIENTEDGE draws the classic 2-px sunken edge, exactly as measured.
    output_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                              WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY,
                              0, 0, 0, 0, Hwnd(), nullptr, win::App::Instance(), nullptr);
    input_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                             WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                             0, 0, 0, 0, Hwnd(), nullptr, win::App::Instance(), nullptr);
    if (!output_ || !input_)
        return false;

    // A single-line EDIT ignores Enter. To catch it we "subclass" the control: put our
    // own window procedure in front of the EDIT's and pass everything else through.
    SetWindowLongPtrW(input_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    inputOldProc_ = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(input_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&StatusBar::InputProc)));

    // The button groups are the same Toolbar control as the main toolbar, with
    // different configs (16-px icons, explicit positions, no frame edges).
    if (!snapTools_.Create(Hwnd(), toolbar::StatusSnapConfig()) ||
        !animTools_.Create(Hwnd(), toolbar::StatusAnimConfig()))
        return false;
    auto forward = [this](const std::string& command) { if (onCommand) onCommand(command); };
    snapTools_.onCommand = forward;
    animTools_.onCommand = forward;
    animTools_.onEditCommitted = [this](const std::string&, const std::wstring& text) {
        // Accept a whole number; anything else just restores nothing (the caller
        // writes the current frame back via SetFrame).
        wchar_t* end = nullptr;
        const long value = wcstol(text.c_str(), &end, 10);
        if (end != text.c_str() && onFrameEntered) onFrameEntered(static_cast<int>(value));
    };

    // Tooltip for the time-tag field (a drawn field, so a rectangle "tool").
    tooltip_ = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
                               WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                               Hwnd(), nullptr, win::App::Instance(), nullptr);
    if (tooltip_) {
        TTTOOLINFOW ti{};
        ti.cbSize = TTTOOLINFOW_V2_SIZE;
        ti.uFlags = TTF_SUBCLASS;
        ti.hwnd = Hwnd();
        ti.uId = 1;
        ti.lpszText = const_cast<wchar_t*>(L"Time Tag");
        SendMessageW(tooltip_, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti));
    }
    return true;
}

void StatusBar::SetCommandChecked(const std::string& command, bool checked) {
    snapTools_.SetChecked(command, checked);
    animTools_.SetChecked(command, checked);
}

void StatusBar::SetFrame(int frame) {
    animTools_.SetEditText("anim.frame", std::to_wstring(frame));
}

LRESULT CALLBACK StatusBar::InputProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    auto* self = reinterpret_cast<StatusBar*>(GetWindowLongPtrW(h, GWLP_USERDATA));
    if (msg == WM_CHAR && wp == VK_RETURN) {
        if (self) {
            const int len = GetWindowTextLengthW(h);
            std::wstring text(static_cast<size_t>(len), L'\0');
            if (len > 0) GetWindowTextW(h, text.data(), len + 1);
            SetWindowTextW(h, L"");
            if (self->onListenerInput) self->onListenerInput(text);
        }
        return 0; // swallow it: no beep
    }
    return CallWindowProcW(self ? self->inputOldProc_ : DefWindowProcW, h, msg, wp, lp);
}

void StatusBar::SetPrompt(int line, const std::wstring& text) {
    if (line < 0 || line > 1) return;
    prompt_[line] = text;
    const Rect& r = line == 0 ? layout_.promptTop : layout_.promptBottom;
    RECT rc{r.left, r.top, r.right, r.bottom};
    InvalidateRect(Hwnd(), &rc, FALSE);
}

void StatusBar::SetListenerOutput(const std::wstring& text) {
    SetWindowTextW(output_, text.c_str());
}

void StatusBar::SetCoordinates(const std::wstring& x, const std::wstring& y, const std::wstring& z) {
    x_ = x; y_ = y; z_ = z;
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

void StatusBar::SetGridSpacing(double spacing) {
    wchar_t text[64];
    swprintf_s(text, L"Grid = %.1f", spacing);
    grid_ = text;
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

void StatusBar::Relayout() {
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    layout_ = statusbar::ComputeStatusLayout(rc.right, rc.bottom, listenerWidth_, Dpi());
    auto place = [](HWND h, const Rect& r) {
        SetWindowPos(h, nullptr, r.left, r.top, r.Width(), r.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
    };
    place(output_, layout_.listenerTop);
    place(input_, layout_.listenerBottom);
    place(snapTools_.Hwnd(), layout_.snapTools);
    place(animTools_.Hwnd(), layout_.animTools);
    if (tooltip_) {
        TTTOOLINFOW ti{};
        ti.cbSize = TTTOOLINFOW_V2_SIZE;
        ti.hwnd = Hwnd();
        ti.uId = 1;
        const Rect& t = layout_.timeTag;
        ti.rect = {t.left, t.top, t.right, t.bottom};
        SendMessageW(tooltip_, TTM_NEWTOOLRECTW, 0, reinterpret_cast<LPARAM>(&ti));
    }
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

// ---- painting --------------------------------------------------------------

// Thin sunken field (measured): shadow top/left, highlight bottom/right, R3 corner
// convention; text 7 px from the left, vertically centred.
void StatusBar::PaintField(HDC dc, const Rect& r, const std::wstring& text) {
    if (r.Width() <= 2 || r.Height() <= 2) return;
    DrawButtonRing(dc, {r.left, r.top, r.right, r.bottom},
                   theme::kButtonShadow.Ref(), theme::kButtonHighlight.Ref());
    if (text.empty()) return;
    RECT t{r.left + Scale(7), r.top + 1, r.right - 2, r.bottom - 1};
    DrawTextW(dc, text.c_str(), -1, &t, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
}

// Selection lock: a flat button with an original padlock glyph; sunken when on.
void StatusBar::PaintLock(HDC dc, const Rect& r) {
    if (locked_ || lockPressed_) {
        DrawButtonRing(dc, {r.left, r.top, r.right, r.bottom},
                       theme::kButtonShadow.Ref(), theme::kButtonHighlight.Ref());
        FillSolid(dc, {r.left + 1, r.top + 1, r.right - 1, r.bottom - 1},
                  locked_ ? theme::kButtonChecked.Ref() : theme::Face());
    }
    // 9 x 11 padlock, centred: shackle (rows 0-4), body (rows 5-10).
    const int s = Dpi() >= 144 ? 2 : 1; // simple pixel doubling at high DPI
    const int x0 = (r.left + r.right) / 2 - 9 * s / 2;
    const int y0 = (r.top + r.bottom) / 2 - 11 * s / 2;
    const COLORREF ink = GetSysColor(COLOR_BTNTEXT);
    auto px = [&](int c0, int c1, int row) {
        FillSolid(dc, {x0 + c0 * s, y0 + row * s, x0 + (c1 + 1) * s, y0 + (row + 1) * s}, ink);
    };
    px(3, 5, 0);
    px(2, 2, 1); px(6, 6, 1);
    for (int row = 2; row <= 4; ++row) { px(1, 1, row); px(7, 7, row); }
    for (int row = 5; row <= 10; ++row) px(0, 8, row);
    FillSolid(dc, {x0 + 4 * s, y0 + 7 * s, x0 + 5 * s, y0 + 9 * s}, theme::Face()); // keyhole
}

void StatusBar::OnPaint() {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(Hwnd(), &ps);
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    FillSolid(dc, rc, theme::Face());

    // Splitter (measured): columns highlight, face, shadow, black; its top row is
    // highlight across the first three columns.
    const Rect& sp = layout_.splitter;
    FillSolid(dc, {sp.left, 0, sp.left + 1, rc.bottom}, theme::kButtonHighlight.Ref());
    FillSolid(dc, {sp.left + 2, 0, sp.left + 3, rc.bottom}, theme::kButtonShadow.Ref());
    FillSolid(dc, {sp.left + 3, 0, sp.left + 4, rc.bottom}, RGB(0, 0, 0));
    FillSolid(dc, {sp.left, 0, sp.left + 3, 1}, theme::kButtonHighlight.Ref());

    HGDIOBJ oldFont = SelectObject(dc, font_ ? font_ : GetStockObject(DEFAULT_GUI_FONT));
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, GetSysColor(COLOR_BTNTEXT));
    PaintField(dc, layout_.promptTop, prompt_[0]);
    PaintField(dc, layout_.promptBottom, prompt_[1]);
    PaintField(dc, layout_.x, x_);
    PaintField(dc, layout_.y, y_);
    PaintField(dc, layout_.z, z_);
    PaintField(dc, layout_.grid, grid_);
    PaintField(dc, layout_.timeTag, L"");   // Time Tag (empty until tags exist)
    PaintLock(dc, layout_.lock);
    SelectObject(dc, oldFont);
    EndPaint(Hwnd(), &ps);
}

// ---- messages --------------------------------------------------------------

static bool In(const Rect& r, int x, int y) {
    return x >= r.left && x < r.right && y >= r.top && y < r.bottom;
}

LRESULT StatusBar::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_SIZE:
        Relayout();
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        OnPaint();
        return 0;
    case WM_SETFONT:
        font_ = reinterpret_cast<HFONT>(wp);
        SendMessageW(output_, WM_SETFONT, wp, lp);
        SendMessageW(input_, WM_SETFONT, wp, lp);
        SendMessageW(snapTools_.Hwnd(), WM_SETFONT, wp, lp);
        SendMessageW(animTools_.Hwnd(), WM_SETFONT, wp, lp);
        if (tooltip_) SendMessageW(tooltip_, WM_SETFONT, wp, FALSE);
        if (LOWORD(lp)) InvalidateRect(Hwnd(), nullptr, FALSE);
        return 0;
    case WM_GETFONT:
        return reinterpret_cast<LRESULT>(font_);

    case WM_CTLCOLORSTATIC:
        // A read-only EDIT asks its parent for colours with WM_CTLCOLORSTATIC.
        if (reinterpret_cast<HWND>(lp) == output_) {
            HDC dc = reinterpret_cast<HDC>(wp);
            SetBkColor(dc, theme::kListenerBg.Ref());
            SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
            return reinterpret_cast<LRESULT>(outputBrush_);
        }
        break;

    case WM_SETCURSOR:
        if (LOWORD(lp) == HTCLIENT) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(Hwnd(), &pt);
            if (draggingSplitter_ || In(layout_.splitter, pt.x, pt.y)) {
                SetCursor(LoadCursorW(nullptr, IDC_SIZEWE));
                return TRUE;
            }
        }
        break;

    case WM_LBUTTONDOWN: {
        const int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp);
        if (In(layout_.splitter, x, y)) {
            draggingSplitter_ = true;
            dragStartX_ = x;
            dragStartWidth_ = listenerWidth_;
            SetCapture(Hwnd());
        } else if (In(layout_.lock, x, y)) {
            lockPressed_ = true;
            SetCapture(Hwnd());
            InvalidateRect(Hwnd(), nullptr, FALSE);
        }
        return 0;
    }
    case WM_MOUSEMOVE:
        if (draggingSplitter_) {
            // Width is kept at 100% so it survives DPI changes.
            const int dx = (GET_X_LPARAM(lp) - dragStartX_) * 96 / static_cast<int>(Dpi());
            listenerWidth_ = std::max(0, dragStartWidth_ + dx);
            Relayout();
        }
        return 0;
    case WM_LBUTTONUP:
        if (lockPressed_ && In(layout_.lock, GET_X_LPARAM(lp), GET_Y_LPARAM(lp))) {
            locked_ = !locked_;
            if (onLockChanged) onLockChanged(locked_);
        }
        if (draggingSplitter_ || lockPressed_) ReleaseCapture();
        return 0;
    case WM_CAPTURECHANGED:
        draggingSplitter_ = false;
        if (lockPressed_) { lockPressed_ = false; InvalidateRect(Hwnd(), nullptr, FALSE); }
        return 0;
    }
    return win::Window::HandleMessage(msg, wp, lp);
}

} // namespace ui
