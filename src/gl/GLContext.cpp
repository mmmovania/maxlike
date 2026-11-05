#include <glad/gl.h>   // must come before any other OpenGL header
#include <glad/wgl.h>
#include "gl/GLContext.h"

namespace gl {

// GLAD asks for each OpenGL function by name. On Windows they come from two places:
// wglGetProcAddress knows everything newer than OpenGL 1.1 (and the extensions);
// the 1.1 functions themselves are plain exports of opengl32.dll, for which
// wglGetProcAddress returns null (or, on some drivers, 1, 2, 3 or -1).
static GLADapiproc LoadProc(const char* name) {
    PROC p = wglGetProcAddress(name);
    const auto v = reinterpret_cast<intptr_t>(p);
    if (v == 0 || v == 1 || v == 2 || v == 3 || v == -1) {
        static HMODULE opengl32 = LoadLibraryW(L"opengl32.dll");
        p = GetProcAddress(opengl32, name);
    }
    return reinterpret_cast<GLADapiproc>(p);
}

GLContext::~GLContext() {
    if (context_) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(context_);
    }
}

bool GLContext::PrepareWindow(HWND window) {
    HDC dc = GetDC(window);
    if (pixelFormat_ == 0) {
        // What we ask for: a double-buffered RGBA window surface with depth and stencil.
        pfd_ = {};
        pfd_.nSize = sizeof(pfd_);
        pfd_.nVersion = 1;
        pfd_.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd_.iPixelType = PFD_TYPE_RGBA;
        pfd_.cColorBits = 32;
        pfd_.cDepthBits = 24;
        pfd_.cStencilBits = 8;
        pfd_.iLayerType = PFD_MAIN_PLANE;
        pixelFormat_ = ChoosePixelFormat(dc, &pfd_); // the closest format the driver has
    }
    // A window's pixel format can be set only once; setting the same one is fine.
    const bool ok = pixelFormat_ != 0 &&
                    (GetPixelFormat(dc) == pixelFormat_ || SetPixelFormat(dc, pixelFormat_, &pfd_));
    ReleaseDC(window, dc); // CS_OWNDC: the DC stays valid; this only balances GetDC
    return ok;
}

bool GLContext::Create() {
    // A context can draw into any window that has the same pixel format, so it is
    // created on a throw-away window. ("STATIC" is a predefined window class.)
    HWND helper = CreateWindowExW(0, L"STATIC", L"", WS_POPUP, 0, 0, 16, 16,
                                  nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!helper) {
        error_ = "Could not create the helper window.";
        return false;
    }
    const bool ok = CreateOn(helper);
    DestroyWindow(helper);
    return ok;
}

bool GLContext::CreateOn(HWND firstWindow) {
    if (!PrepareWindow(firstWindow)) {
        error_ = "No suitable pixel format (double-buffered RGBA with depth).";
        return false;
    }
    HDC dc = GetDC(firstWindow);

    // Step 1: a temporary old-style context. Only with a context current can we
    // ask the driver for the function that creates modern contexts.
    HGLRC temp = wglCreateContext(dc);
    if (!temp || !wglMakeCurrent(dc, temp)) {
        error_ = "Could not create an OpenGL context.";
        if (temp) wglDeleteContext(temp);
        ReleaseDC(firstWindow, dc);
        return false;
    }

    // Step 2: load the WGL extensions and create the real OpenGL 3.3 core context.
    gladLoadWGL(dc, LoadProc);
    if (GLAD_WGL_ARB_create_context && GLAD_WGL_ARB_create_context_profile) {
        const int attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0,
        };
        context_ = wglCreateContextAttribsARB(dc, nullptr, attribs);
    }

    // Step 3: retire the temporary context and switch to the real one.
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(temp);
    if (!context_) {
        error_ = "OpenGL 3.3 core profile is not available on this system.";
        ReleaseDC(firstWindow, dc);
        return false;
    }
    wglMakeCurrent(dc, context_);

    // Step 4: load every OpenGL function pointer for this context.
    if (!gladLoadGL(LoadProc)) {
        error_ = "Could not load the OpenGL 3.3 functions.";
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(context_);
        context_ = nullptr;
        ReleaseDC(firstWindow, dc);
        return false;
    }
    auto str = [](GLenum e) { auto s = glGetString(e); return std::string(s ? reinterpret_cast<const char*>(s) : "?"); };
    description_ = str(GL_VENDOR) + " | " + str(GL_RENDERER) + " | " + str(GL_VERSION);
    // Leave nothing current: every paint makes the context current on its own window.
    wglMakeCurrent(nullptr, nullptr);
    ReleaseDC(firstWindow, dc);
    return true;
}

bool GLContext::MakeCurrent(HDC dc) {
    return context_ && wglMakeCurrent(dc, context_);
}

} // namespace gl
