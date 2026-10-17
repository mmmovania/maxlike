#include "win/Window.h"
#include "win/App.h"

namespace win {

Window::~Window() {
    if (hwnd_) {
        SetWindowLongPtrW(hwnd_, GWLP_USERDATA, 0);
        DestroyWindow(hwnd_);
    }
}

bool Window::EnsureClassRegistered() const {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    if (GetClassInfoExW(App::Instance(), ClassName(), &wc))
        return true;
    wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = ClassStyle();
    wc.lpfnWndProc   = &Window::StaticWndProc;
    wc.hInstance     = App::Instance();
    wc.hCursor       = ClassCursor();
    wc.hbrBackground = nullptr; // every window paints its own background
    wc.lpszClassName = ClassName();
    return RegisterClassExW(&wc) != 0;
}

bool Window::Create(const CreateParams& p) {
    if (hwnd_ || !EnsureClassRegistered())
        return false;
    HWND h = CreateWindowExW(p.exStyle, ClassName(), p.title, p.style,
                             p.x, p.y, p.width, p.height,
                             p.parent, p.menuOrId, App::Instance(), this);
    return h != nullptr; // hwnd_ is set in WM_NCCREATE
}

UINT Window::Dpi() const {
    return hwnd_ ? GetDpiForWindow(hwnd_) : 96;
}

LRESULT Window::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    return DefWindowProcW(hwnd_, msg, wp, lp);
}

LRESULT CALLBACK Window::StaticWndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    Window* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<Window*>(cs->lpCreateParams);
        self->hwnd_ = h;
        SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<Window*>(GetWindowLongPtrW(h, GWLP_USERDATA));
    }

    if (!self)
        return DefWindowProcW(h, msg, wp, lp);

    LRESULT r = self->HandleMessage(msg, wp, lp);
    if (msg == WM_NCDESTROY) {
        SetWindowLongPtrW(h, GWLP_USERDATA, 0);
        self->hwnd_ = nullptr;
    }
    return r;
}

} // namespace win
