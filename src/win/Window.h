#pragma once
#include <windows.h>

namespace win {

// Thin base class over a Win32 window.
// Derived classes override ClassName() and HandleMessage(); `this` is stored in
// GWLP_USERDATA so messages reach the C++ object.
class Window {
public:
    struct CreateParams {
        const wchar_t* title   = L"";
        DWORD style            = WS_OVERLAPPEDWINDOW;
        DWORD exStyle          = 0;
        int x = CW_USEDEFAULT, y = CW_USEDEFAULT;
        int width = CW_USEDEFAULT, height = CW_USEDEFAULT;
        HWND parent            = nullptr;
        HMENU menuOrId         = nullptr;
    };

    Window() = default;
    virtual ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool Create(const CreateParams& p);
    HWND Hwnd() const { return hwnd_; }

    // DPI of the monitor this window is on (96 = 100%).
    UINT Dpi() const;
    // Scale a 96-DPI pixel value to this window's DPI.
    int Scale(int px96) const { return MulDiv(px96, static_cast<int>(Dpi()), 96); }

protected:
    virtual const wchar_t* ClassName() const = 0;
    virtual UINT ClassStyle() const { return CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS; }
    virtual HCURSOR ClassCursor() const { return LoadCursorW(nullptr, IDC_ARROW); }

    // Default forwards to DefWindowProcW.
    virtual LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp);

private:
    static LRESULT CALLBACK StaticWndProc(HWND, UINT, WPARAM, LPARAM);
    bool EnsureClassRegistered() const;

    HWND hwnd_ = nullptr;
};

} // namespace win
