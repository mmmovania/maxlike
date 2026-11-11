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
    // Creation hooks, passed to every viewport (see Viewport).
    std::function<bool()> creationActive;
    std::function<bool(scene::Creator::Event, int, const scene::ConstructionPlane&, double, double, int, double)> onCreateEvent;
    std::function<bool()> onCreateCancel;
    std::function<void(const std::vector<scene::Node::Id>&, bool, bool, bool)> onSelect;
    std::function<void()> onDeleteKey;
    std::function<void()> onMoveBegin;
    std::function<void(const math::Vec3&, int)> onMoveDrag;
    std::function<void(bool)> onMoveEnd;
    std::function<void(bool, const math::Vec3&)> onCursorWorld;

    bool Create(HWND parent);
    NavMode Mode() const { return mode_; }
    void SetNavMode(NavMode mode);          // None leaves the current mode
    void ZoomExtents(bool allViews);        // empty scene: back to the default framing
    void SetScene(const scene::Scene* scene) { for (auto& v : views_) v.SetScene(scene); }
    void SetAxesDisplay(view::AxesStyle style, const bool active[3]) { for (auto& v : views_) v.SetAxesDisplay(style, active); }
    void SetTransformTool(bool on) { for (auto& v : views_) v.SetTransformTool(on); }
    void SetReferenceSystem(scene::move::RefCoord s, int centre) { for (auto& v : views_) v.SetReferenceSystem(s, centre); }
    void SetGizmoEditing(const Viewport::GizmoEditing& g) { for (auto& v : views_) v.SetGizmoEditing(g); }
    void SetVertexEditing(const Viewport::VertexEditing& e) { for (auto& v : views_) v.SetVertexEditing(e); }
    std::function<void(const std::vector<uint32_t>&, bool, bool, bool)> onVertexSelect;
    std::function<void(uint32_t, uint32_t)> onWeldTarget;
    std::function<void(const Viewport::ToolClick&)> onToolClick;
    scene::ConstructionPlane ActivePlane() const { return views_[static_cast<size_t>(active_)].CreationPlane(); }
    void Redraw() { for (auto& v : views_) if (v.Hwnd()) InvalidateRect(v.Hwnd(), nullptr, FALSE); }
    // File > Reset: the startup types and views (Top, Front, Left, Perspective).
    void ResetAllViews();
    // A viewport (0..3 in layout order) restored from a file: its type and its view.
    void ApplySavedView(int slot, ViewType type, const view::OrthoView& ortho, const view::PerspectiveView& perspective) {
        if (slot < 0 || slot >= static_cast<int>(views_.size())) return;
        views_[static_cast<size_t>(slot)].SetType(type);
        views_[static_cast<size_t>(slot)].SetViewState(ortho, perspective);
    }
    int  Active() const { return active_; }
    const Viewport& ViewAt(int slot) const { return views_[static_cast<size_t>(slot)]; }   // 0..3, layout order
    Viewport& ActiveView() { return views_[static_cast<size_t>(active_)]; }
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
