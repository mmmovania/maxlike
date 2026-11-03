#include "ui/ViewPanel.h"
#include "ui/Bevel.h"
#include "ui/Theme.h"

namespace ui {

// GPU objects must be deleted while the context is current, which needs a window
// with the context's pixel format. WM_DESTROY reaches the panel *before* its child
// viewports are destroyed, so that is the moment to do it.
void ViewPanel::ReleaseGpuObjects() {
    if (!context_.IsValid() || !views_[0].Hwnd()) return;
    HDC dc = GetDC(views_[0].Hwnd());
    if (context_.MakeCurrent(dc)) {
        lines_.Release();
        wglMakeCurrent(nullptr, nullptr);
    }
    ReleaseDC(views_[0].Hwnd(), dc);
}

bool ViewPanel::Create(HWND parent) {
    CreateParams p;
    p.title = L"Viewports";
    p.style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    p.x = p.y = 0;
    p.width = p.height = 0;
    p.parent = parent;
    if (!win::Window::Create(p))
        return false;

    // Default R3 layout: Top, Front / Left, Perspective.
    const ViewType types[viewports::kCount] = {ViewType::Top, ViewType::Front,
                                               ViewType::Left, ViewType::Perspective};
    // The shared context first; each viewport then takes its pixel format on creation.
    context_.Create(); // on failure the viewports draw a message instead
    for (int i = 0; i < viewports::kCount; ++i)
        if (!views_[static_cast<size_t>(i)].Create(Hwnd(), context_, lines_, types[i]))
            return false;
    for (int i = 0; i < viewports::kCount; ++i)
        views_[static_cast<size_t>(i)].onActivate = [this, i] { SetActive(i); };
    return true;
}

void ViewPanel::SetActive(int index) {
    if (index == active_ || index < 0 || index >= viewports::kCount) return;
    active_ = index;
    InvalidateRect(Hwnd(), nullptr, FALSE); // only the frames change
}

void ViewPanel::Relayout() {
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    layout_ = viewports::ComputeViewportLayout(rc.right, rc.bottom, Dpi());
    HDWP dwp = BeginDeferWindowPos(viewports::kCount);
    for (int i = 0; i < viewports::kCount && dwp; ++i) {
        const Rect& c = layout_.view[i].content;
        dwp = DeferWindowPos(dwp, views_[static_cast<size_t>(i)].Hwnd(), nullptr,
                             c.left, c.top, c.Width(), c.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (dwp) EndDeferWindowPos(dwp);
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

// Frames (measured): a 2-px border, white for the active viewport and black for the
// others, then a 1-px #C0C0C0 ring. Inactive frames first, the active one last, so
// its colour wins on the shared line.
void ViewPanel::OnPaint() {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(Hwnd(), &ps);
    RECT rc;
    GetClientRect(Hwnd(), &rc);

    // The line above the viewports: highlight at x 0, shadow elsewhere.
    FillSolid(dc, {0, 0, 1, 1}, theme::kButtonHighlight.Ref());
    FillSolid(dc, {1, 0, rc.right, 1}, theme::kButtonShadow.Ref());

    auto frame = [&](int i) {
        const Rect& o = layout_.view[i].outer;
        const COLORREF border = (i == active_) ? theme::kViewportActiveBorder.Ref()
                                               : theme::kViewportBorder.Ref();
        const RECT r{o.left, o.top, o.right, o.bottom};
        DrawRing(dc, r, border, border);
        DrawRing(dc, {r.left + 1, r.top + 1, r.right - 1, r.bottom - 1}, border, border);
        const COLORREF ring = theme::kViewportRing.Ref();
        DrawRing(dc, {r.left + 2, r.top + 2, r.right - 2, r.bottom - 2}, ring, ring);
    };
    for (int i = 0; i < viewports::kCount; ++i)
        if (i != active_) frame(i);
    frame(active_);
    EndPaint(Hwnd(), &ps);
}

LRESULT ViewPanel::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_SIZE:
        Relayout();
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        OnPaint();
        return 0;
    case WM_DESTROY:
        ReleaseGpuObjects();
        return 0;
    }
    return win::Window::HandleMessage(msg, wp, lp);
}

} // namespace ui
