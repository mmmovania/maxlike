#pragma once
#include "win/Window.h"
#include <functional>
#include <string>
#include "view/OrthoGrid.h"
#include "view/PerspectiveView.h"
#include "ui/LabelText.h"

namespace gl { class GLContext; class LineRenderer; }

namespace ui {

enum class ViewType { Top, Bottom, Front, Back, Left, Right, Perspective };
enum class Shading { SmoothHighlights, Wireframe, Smooth, FacetsHighlights, Facets, LitWireframe, BoundingBox };

// One viewport: a child window drawn with OpenGL. Its frame (border and ring) is
// drawn by the ViewPanel around it.
class Viewport : public win::Window {
public:
    std::function<void()> onActivate; // the user clicked into this viewport

    // `lines` is the panel's shared line renderer (created lazily on first paint).
    bool Create(HWND parent, gl::GLContext& context, gl::LineRenderer& lines, ViewType type);
    ViewType Type() const { return type_; }
    const wchar_t* Name() const;
    void SetType(ViewType type);
    bool ShowGrid() const { return showGrid_; }
    void SetShowGrid(bool show);

protected:
    const wchar_t* ClassName() const override { return L"maxlike.Viewport"; }
    // CS_OWNDC: a private, permanent device context. OpenGL needs the same DC (and
    // its pixel format) every time; a shared, recycled DC would lose both.
    UINT ClassStyle() const override { return CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS; }
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    void Render(HDC dc);
    bool InLabel(int x, int y) const;       // the right-click area: the label text
    void ShowLabelMenu(POINT screen);       // R3's viewport menu
    void PaintFallback(HDC dc); // GDI drawing when OpenGL is unavailable

    void DrawOrthoGrid(int width, int height);
    void DrawPerspectiveGrid(int width, int height);
    void DrawLabelAndTripod(int width, int height);
    void ScreenAxes(double out[3][2]) const; // how X, Y, Z appear in this view

    gl::GLContext* context_ = nullptr;
    gl::LineRenderer* lines_ = nullptr;
    ViewType type_ = ViewType::Top;
    Shading shading_ = Shading::Wireframe;  // stored and shown; used once objects exist
    bool showGrid_ = true;
    view::OrthoView ortho_;              // what an ortho view shows (pan/zoom in step 2.5)
    view::PerspectiveView perspective_;  // the perspective camera (orbit in step 2.5)
    TextPixels label_;                   // the view's name, as pixels
    UINT labelDpi_ = 0;                  // DPI the label was rendered for
};

} // namespace ui
