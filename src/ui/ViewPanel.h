#pragma once
#include "win/Window.h"
#include "gl/GLContext.h"
#include "gl/LineRenderer.h"
#include "ui/Viewport.h"
#include "ui/ViewportLayout.h"
#include <array>

namespace ui {

// The 2x2 viewport area: four Viewport windows, the frames around them, and the
// OpenGL context they share.
class ViewPanel : public win::Window {
public:
    bool Create(HWND parent);
    int  Active() const { return active_; }
    void SetActive(int index);
    const gl::GLContext& Context() const { return context_; }

protected:
    const wchar_t* ClassName() const override { return L"maxlike.ViewPanel"; }
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    void Relayout();
    void ReleaseGpuObjects();
    void OnPaint();

    gl::GLContext context_;           // declared first: destroyed last
    gl::LineRenderer lines_;          // shared by all viewports (needs the context current)
    std::array<Viewport, viewports::kCount> views_;
    viewports::Layout layout_{};
    int active_ = viewports::kTopLeft; // the reference screenshot shows Top active
};

} // namespace ui
