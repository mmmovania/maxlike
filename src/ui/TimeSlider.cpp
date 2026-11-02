#include "ui/TimeSlider.h"
#include "ui/Bevel.h"
#include "ui/Theme.h"
#include <windowsx.h>
#include <algorithm>
#include <cstdio>

namespace ui {

using timeslider::Parts;

// Arrow buttons auto-repeat while held: a pause, then a steady rate.
// NOTE: R3's exact repeat timing is not verified.
static constexpr UINT_PTR kRepeatTimer = 1;
static constexpr UINT kRepeatDelayMs = 400;
static constexpr UINT kRepeatRateMs = 50;

bool TimeSlider::Create(HWND parent) {
    CreateParams p;
    p.title  = L"Time slider";
    p.style  = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
    p.x = p.y = 0;
    p.width = p.height = 0;
    p.parent = parent;
    return win::Window::Create(p);
}

void TimeSlider::SetRange(int start, int end) {
    start_ = start;
    end_ = std::max(start, end);
    frame_ = std::clamp(frame_, start_, end_);
    if (Hwnd()) InvalidateRect(Hwnd(), nullptr, FALSE);
}

void TimeSlider::SetFrame(int frame) {
    frame = std::clamp(frame, start_, end_);
    if (frame == frame_) return;
    frame_ = frame;
    if (Hwnd()) InvalidateRect(Hwnd(), nullptr, FALSE);
}

void TimeSlider::ChangeFrame(int frame) {
    const int before = frame_;
    SetFrame(frame);
    if (frame_ != before && onFrameChanged) onFrameChanged(frame_);
}

Parts TimeSlider::CurrentParts() const {
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    return timeslider::PartsAt(frame_, start_, end_, rc.right, rc.bottom, Dpi());
}

TimeSlider::Part TimeSlider::HitTest(int x, int y) const {
    auto in = [&](const Rect& r) { return x >= r.left && x < r.right && y >= r.top && y < r.bottom; };
    const Parts p = CurrentParts();
    if (in(p.prev))  return Part::Prev;
    if (in(p.thumb)) return Part::Thumb;
    if (in(p.next))  return Part::Next;
    return Part::Trough;
}

// ---- painting (all measured from the reference at 100%) -------------------

static void Px(HDC dc, int x, int y, COLORREF c) { FillSolid(dc, {x, y, x + 1, y + 1}, c); }

// Classic raised button: outer ring white / dark shadow, inner ring light / shadow
// (Windows' corner convention: the bottom-right colour owns both odd corners).
// While pressed: a single flat shadow ring and the glyph moves by one pixel.
void TimeSlider::PaintArrow(HDC dc, const Rect& r, bool left, bool down) {
    RECT rc{r.left, r.top, r.right, r.bottom};
    FillSolid(dc, rc, theme::Face());
    if (down) {
        DrawRing(dc, rc, theme::kButtonShadow.Ref(), theme::kButtonShadow.Ref());
    } else {
        DrawRing(dc, rc, theme::kButtonHighlight.Ref(), theme::kDarkShadow.Ref());
        DrawRing(dc, {rc.left + 1, rc.top + 1, rc.right - 1, rc.bottom - 1},
                 theme::kLight.Ref(), theme::kButtonShadow.Ref());
    }
    // 1-px chevron, 7 rows tall, tip at row 8 (measured: "<" spans cols 6..9 with its
    // tip at 6; ">" spans cols 6..9 with its tip at 9).
    const int shift = down ? 1 : 0;
    const int y0 = r.top + 5 + shift;
    const COLORREF ink = GetSysColor(COLOR_BTNTEXT);
    for (int i = 0; i < 7; ++i) {
        const int d = (i <= 3) ? (3 - i) : (i - 3); // 3,2,1,0,1,2,3
        const int x = left ? (r.left + 6 + d) : (r.left + 9 - d);
        Px(dc, x + shift, y0 + i, ink);
    }
}

// The thumb: outer ring white / black (R3 corner convention), an inner shadow on the
// right and bottom, and one white pixel at the inner bottom-left; label centred.
void TimeSlider::PaintThumb(HDC dc, const Rect& r) {
    RECT rc{r.left, r.top, r.right, r.bottom};
    FillSolid(dc, rc, theme::Face());
    DrawButtonRing(dc, rc, theme::kButtonHighlight.Ref(), RGB(0, 0, 0));
    FillSolid(dc, {r.right - 2, r.top + 1, r.right - 1, r.bottom - 1}, theme::kButtonShadow.Ref());
    FillSolid(dc, {r.left + 2, r.bottom - 2, r.right - 1, r.bottom - 1}, theme::kButtonShadow.Ref());
    Px(dc, r.left + 1, r.bottom - 2, theme::kButtonHighlight.Ref());

    wchar_t text[32];
    swprintf_s(text, L"%d / %d", frame_, end_);
    RECT t{r.left + 2, r.top + 1, r.right - 2, r.bottom - 2};
    SetTextColor(dc, GetSysColor(COLOR_BTNTEXT));
    DrawTextW(dc, text, -1, &t, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

void TimeSlider::OnPaint() {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(Hwnd(), &ps);
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    FillSolid(dc, rc, theme::kSliderTrough.Ref());

    HGDIOBJ oldFont = SelectObject(dc, font_ ? font_ : GetStockObject(DEFAULT_GUI_FONT));
    SetBkMode(dc, TRANSPARENT);
    const Parts p = CurrentParts();
    PaintArrow(dc, p.prev, true, pressed_ == Part::Prev);
    PaintThumb(dc, p.thumb);
    PaintArrow(dc, p.next, false, pressed_ == Part::Next);
    SelectObject(dc, oldFont);
    EndPaint(Hwnd(), &ps);
}

// ---- input -----------------------------------------------------------------

LRESULT TimeSlider::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
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
        const int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp);
        pressed_ = HitTest(x, y);
        SetCapture(Hwnd());
        switch (pressed_) {
        case Part::Prev:
        case Part::Next:
            ChangeFrame(frame_ + (pressed_ == Part::Next ? 1 : -1));
            SetTimer(Hwnd(), kRepeatTimer, kRepeatDelayMs, nullptr);
            break;
        case Part::Thumb:
            dragOffset_ = x - CurrentParts().prev.left;
            break;
        case Part::Trough: {
            // Jump so the thumb's centre lands under the mouse, then keep dragging.
            const Parts p = CurrentParts();
            dragOffset_ = (p.thumb.left + p.thumb.right) / 2 - p.prev.left;
            RECT rc;
            GetClientRect(Hwnd(), &rc);
            ChangeFrame(timeslider::FrameAtBlockLeft(x - dragOffset_, start_, end_, rc.right, Dpi()));
            pressed_ = Part::Thumb;
            break;
        }
        case Part::None:
            break;
        }
        InvalidateRect(Hwnd(), nullptr, FALSE);
        return 0;
    }
    case WM_MOUSEMOVE:
        if (pressed_ == Part::Thumb) {
            RECT rc;
            GetClientRect(Hwnd(), &rc);
            ChangeFrame(timeslider::FrameAtBlockLeft(GET_X_LPARAM(lp) - dragOffset_,
                                                     start_, end_, rc.right, Dpi()));
        }
        return 0;
    case WM_TIMER:
        if (wp == kRepeatTimer && (pressed_ == Part::Prev || pressed_ == Part::Next)) {
            ChangeFrame(frame_ + (pressed_ == Part::Next ? 1 : -1));
            SetTimer(Hwnd(), kRepeatTimer, kRepeatRateMs, nullptr); // faster after the first
            return 0;
        }
        break;
    case WM_LBUTTONUP:
        if (pressed_ != Part::None) ReleaseCapture(); // WM_CAPTURECHANGED resets the state
        return 0;
    case WM_CAPTURECHANGED:
        KillTimer(Hwnd(), kRepeatTimer);
        if (pressed_ != Part::None) {
            pressed_ = Part::None;
            InvalidateRect(Hwnd(), nullptr, FALSE);
        }
        return 0;
    }
    return win::Window::HandleMessage(msg, wp, lp);
}

} // namespace ui
