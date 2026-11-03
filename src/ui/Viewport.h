#pragma once
#include "win/Window.h"
#include <functional>
#include <string>

namespace gl { class GLContext; }

namespace ui {

enum class ViewType { Top, Front, Left, Perspective };

// One viewport: a child window drawn with OpenGL. Its frame (border and ring) is
// drawn by the ViewPanel around it.
class Viewport : public win::Window {
public:
    std::function<void()> onActivate; // the user clicked into this viewport

    bool Create(HWND parent, gl::GLContext& context, ViewType type);
    ViewType Type() const { return type_; }

protected:
    const wchar_t* ClassName() const override { return L"maxlike.Viewport"; }
    // CS_OWNDC: a private, permanent device context. OpenGL needs the same DC (and
    // its pixel format) every time; a shared, recycled DC would lose both.
    UINT ClassStyle() const override { return CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS; }
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    void Render(HDC dc);
    void PaintFallback(HDC dc); // GDI drawing when OpenGL is unavailable

    gl::GLContext* context_ = nullptr;
    ViewType type_ = ViewType::Top;
};

} // namespace ui
