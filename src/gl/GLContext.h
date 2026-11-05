#pragma once
#include <windows.h>
#include <string>

namespace gl {

// One OpenGL 3.3 core-profile context shared by every viewport window.
//
// OpenGL draws into a window through its device context (HDC). Every window that
// will use the context must first get the same "pixel format" (PrepareWindow);
// then the context can be made current on any of them in turn (MakeCurrent).
class GLContext {
public:
    ~GLContext();

    // Creates the context on a hidden helper window (then destroyed), so every
    // viewport window is treated the same. On failure returns false; Error() says why.
    bool Create();
    bool IsValid() const { return context_ != nullptr; }
    const std::string& Error() const { return error_; }
    const std::string& Description() const { return description_; } // vendor, renderer, version

    // Gives `window` the same pixel format. Required once per window, before use.
    bool PrepareWindow(HWND window);

    // Makes the context current on `dc` (the window's own DC, see CS_OWNDC).
    bool MakeCurrent(HDC dc);

private:
    bool CreateOn(HWND window);

    HGLRC context_ = nullptr;
    int pixelFormat_ = 0;
    PIXELFORMATDESCRIPTOR pfd_{};
    std::string error_;
    std::string description_;
};

} // namespace gl
