#pragma once
#include "win/Window.h"
#include <functional>
#include <string>
#include "view/OrthoGrid.h"
#include "view/PerspectiveView.h"
#include "view/Navigation.h"
#include "ui/LabelText.h"

namespace gl { class GLContext; class LineRenderer; }
namespace scene { class Scene; }

namespace ui {

enum class ViewType { Top, Bottom, Front, Back, Left, Right, Perspective };
// What a left-drag in a viewport does (the status-area navigation buttons).
enum class NavMode { None, Zoom, ZoomAll, RegionZoom, Pan, ArcRotate };

enum class Shading { SmoothHighlights, Wireframe, Smooth, FacetsHighlights, Facets, LitWireframe, BoundingBox };

// One viewport: a child window drawn with OpenGL. Its frame (border and ring) is
// drawn by the ViewPanel around it.
class Viewport : public win::Window {
public:
    std::function<void()> onActivate;            // the user clicked into this viewport
    std::function<NavMode()> navMode;            // the panel's current navigation mode
    std::function<void()> onExitMode;            // right-click while a mode is active
    std::function<void(double)> onZoomAll;       // Zoom All: zoom the other views too

    // `lines` is the panel's shared line renderer (created lazily on first paint).
    bool Create(HWND parent, gl::GLContext& context, gl::LineRenderer& lines, ViewType type);
    ViewType Type() const { return type_; }
    const wchar_t* Name() const;
    void SetType(ViewType type);
    bool ShowGrid() const { return showGrid_; }
    void SetShowGrid(bool show);

    // Navigation (pure maths in view/Navigation.h).
    void Pan(int dx, int dy);
    void Zoom(double factor);
    void Orbit(int dx, int dy);
    void ZoomRegion(int x0, int y0, int x1, int y1);
    void ResetView();   // Zoom Extents with an empty scene: back to the default framing

    void SetScene(const scene::Scene* scene) { scene_ = scene; InvalidateRect(Hwnd(), nullptr, FALSE); }

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
    void DrawScene(int width, int height);
    void DrawRegionRectangle(int width, int height);
    void BeginDrag(NavMode op, int x, int y);
    void EndDrag(bool apply);
    HCURSOR CursorFor(NavMode mode) const;
    void ScreenAxes(double out[3][2]) const; // how X, Y, Z appear in this view

    gl::GLContext* context_ = nullptr;
    gl::LineRenderer* lines_ = nullptr;
    const scene::Scene* scene_ = nullptr;
    ViewType type_ = ViewType::Top;
    Shading shading_ = Shading::Wireframe;  // stored and shown; used once objects exist
    bool showGrid_ = true;
    view::OrthoView ortho_;              // what an ortho view shows (pan/zoom in step 2.5)
    view::PerspectiveView perspective_;  // the perspective camera (orbit in step 2.5)
    TextPixels label_;                   // the view's name, as pixels

    // The drag in progress (left button in a mode, or the middle button).
    NavMode drag_ = NavMode::None;
    POINT dragLast_{}, dragStart_{};
    bool regionVisible_ = false;
    UINT labelDpi_ = 0;                  // DPI the label was rendered for
};

} // namespace ui
