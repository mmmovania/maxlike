#pragma once
#include <windows.h>

// Flicker-free painting (Phase 12.1): BeginPaint, then everything is drawn into an
// off-screen bitmap of the client area, which is copied to the window in one BitBlt when
// this goes out of scope. Windows that clear their whole client area before drawing
// otherwise show that blank state for an instant on every repaint.
namespace win {

class PaintBuffer {
public:
    explicit PaintBuffer(HWND hwnd) : hwnd_(hwnd) {
        screen_ = BeginPaint(hwnd_, &ps_);
        RECT rc;
        GetClientRect(hwnd_, &rc);
        w_ = rc.right > 0 ? rc.right : 1;
        h_ = rc.bottom > 0 ? rc.bottom : 1;
        mem_ = CreateCompatibleDC(screen_);
        bitmap_ = CreateCompatibleBitmap(screen_, w_, h_);
        old_ = SelectObject(mem_, bitmap_);
    }
    ~PaintBuffer() {
        BitBlt(screen_, 0, 0, w_, h_, mem_, 0, 0, SRCCOPY);
        SelectObject(mem_, old_);
        DeleteObject(bitmap_);
        DeleteDC(mem_);
        EndPaint(hwnd_, &ps_);
    }
    PaintBuffer(const PaintBuffer&) = delete;
    PaintBuffer& operator=(const PaintBuffer&) = delete;
    HDC Dc() const { return mem_; }
    const PAINTSTRUCT& Ps() const { return ps_; }

private:
    HWND hwnd_;
    PAINTSTRUCT ps_{};
    HDC screen_ = nullptr, mem_ = nullptr;
    HBITMAP bitmap_ = nullptr;
    HGDIOBJ old_ = nullptr;
    int w_ = 1, h_ = 1;
};

} // namespace win
