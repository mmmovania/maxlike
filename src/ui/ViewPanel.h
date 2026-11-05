#pragma once
#include "win/Window.h"
#include "gl/GLContext.h"
#include "gl/LineRenderer.h"
#include "gl/MeshRenderer.h"
#include "ui/Viewport.h"
#include "ui/ViewportLayout.h"
#include <array>
#include <functional>

namespace ui {

// The 2x2 viewport area: four Viewport windows, the frames around them, and the
// OpenGL context they share.
class ViewPanel : public win::Window {
public:
    std::function<void(NavMode)> onNavModeChanged;

    bool Create(HWND parent);
    NavMode Mode() const { return mode_; }
    void SetNavMode(NavMode mode);          // None leaves the current mode
    void ZoomExtents(bool allViews);        // empty scene: back to the default framing
    void SetScene(const scene::Scene* scene) { for (auto& v : views_) v.SetScene(scene); }
    int  Active() const { return active_; }
    void SetActive(int index);
    void ToggleMaximized();                 // Min/Max: the active viewport fills the area
    // (Not "IsMaximized": windows.h defines that name as a macro for IsZoomed.)
    bool HasMaximizedView() const { return arrangement_.maximized >= 0; }
    const gl::GLContext& Context() const { return context_; }

protected:
    const wchar_t* ClassName() const override { return L"maxlike.ViewPanel"; }
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    enum Drag { kNone = 0, kDragX = 1, kDragY = 2, kDragBoth = 3 };
    int  DividerAt(int x, int y) const;     // which divider(s) a point is on
    void Relayout();
    void ReleaseGpuObjects();
    void OnPaint();

    gl::GLContext context_;           // declared first: destroyed last
    gl::LineRenderer lines_;          // shared by all viewports (needs the context current)
    gl::MeshRenderer meshes_;         // likewise, for shaded display
    std::array<Viewport, viewports::kCount> views_;
    viewports::Layout layout_{};
    int active_ = viewports::kTopLeft; // the reference screenshot shows Top active
    viewports::Arrangement arrangement_;
    int drag_ = kNone;
    NavMode mode_ = NavMode::None;
};

} // namespace ui
