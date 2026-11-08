#include "ui/ViewPanel.h"
#include "ui/Bevel.h"
#include "ui/Theme.h"
#include <windowsx.h>
#include <cstdlib>

namespace ui {

// GPU objects must be deleted while the context is current, which needs a window
// with the context's pixel format. WM_DESTROY reaches the panel *before* its child
// viewports are destroyed, so that is the moment to do it.
void ViewPanel::ReleaseGpuObjects() {
    if (!context_.IsValid() || !views_[0].Hwnd()) return;
    HDC dc = GetDC(views_[0].Hwnd());
    if (context_.MakeCurrent(dc)) {
        lines_.Release();
        meshes_.Release();
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
        if (!views_[static_cast<size_t>(i)].Create(Hwnd(), context_, lines_, meshes_, types[i]))
            return false;
    for (int i = 0; i < viewports::kCount; ++i) {
        Viewport& v = views_[static_cast<size_t>(i)];
        v.onActivate = [this, i] { SetActive(i); };
        v.SetIndex(i);
        v.creationActive = [this] { return creationActive && creationActive(); };
        v.onCreateEvent = [this](scene::Creator::Event e, int owner, const scene::ConstructionPlane& p,
                                 double u, double vv, int y, double upp) {
            return onCreateEvent && onCreateEvent(e, owner, p, u, vv, y, upp);
        };
        v.onCreateCancel = [this] { return onCreateCancel && onCreateCancel(); };
        v.onSelect = [this](const std::vector<scene::Node::Id>& ids, bool region, bool ctrl, bool alt) {
            if (onSelect) onSelect(ids, region, ctrl, alt);
        };
        v.onDeleteKey = [this] { if (onDeleteKey) onDeleteKey(); };
        v.onMoveBegin = [this] { if (onMoveBegin) onMoveBegin(); };
        v.onMoveDrag = [this](const math::Vec3& d, int up) { if (onMoveDrag) onMoveDrag(d, up); };
        v.onMoveEnd = [this](bool commit) { if (onMoveEnd) onMoveEnd(commit); };
        v.onCursorWorld = [this](bool ok, const math::Vec3& p) { if (onCursorWorld) onCursorWorld(ok, p); };
        v.onVertexSelect = [this](const std::vector<uint32_t>& vs, bool region, bool ctrl, bool alt) {
            if (onVertexSelect) onVertexSelect(vs, region, ctrl, alt);
        };
        v.navMode = [this] { return mode_; };
        v.onExitMode = [this] { SetNavMode(NavMode::None); };
        v.onZoomAll = [this, i](double factor) {
            for (int j = 0; j < viewports::kCount; ++j)
                if (j != i) views_[static_cast<size_t>(j)].Zoom(factor);
        };
    }
    return true;
}

void ViewPanel::SetActive(int index) {
    if (index == active_ || index < 0 || index >= viewports::kCount) return;
    active_ = index;
    InvalidateRect(Hwnd(), nullptr, FALSE); // only the frames change
}

void ViewPanel::SetNavMode(NavMode mode) {
    if (mode == mode_) return;
    mode_ = mode;
    if (onNavModeChanged) onNavModeChanged(mode_);
}

void ViewPanel::ZoomExtents(bool allViews) {
    for (int i = 0; i < viewports::kCount; ++i)
        if (allViews || i == active_) views_[static_cast<size_t>(i)].ResetView();
}

void ViewPanel::ToggleMaximized() {
    arrangement_.maximized = HasMaximizedView() ? -1 : active_;
    Relayout();
}

void ViewPanel::Relayout() {
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    layout_ = viewports::ComputeViewportLayout(rc.right, rc.bottom, Dpi(), arrangement_);
    HDWP dwp = BeginDeferWindowPos(viewports::kCount);
    for (int i = 0; i < viewports::kCount && dwp; ++i) {
        const Rect& c = layout_.view[i].content;
        const bool visible = c.Width() > 0 && c.Height() > 0;
        // Hidden viewports (one is maximised) are hidden, not just shrunk to nothing.
        dwp = DeferWindowPos(dwp, views_[static_cast<size_t>(i)].Hwnd(), nullptr,
                             c.left, c.top, c.Width(), c.Height(),
                             SWP_NOZORDER | SWP_NOACTIVATE | (visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
    }
    if (dwp) EndDeferWindowPos(dwp);
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

// The dividers are the frame lines between the viewports (part of this window, not
// of the viewports). A point within 3 px of the shared column/row is on a divider;
// near both, it is on the crossing and moves both.
int ViewPanel::DividerAt(int x, int y) const {
    if (HasMaximizedView()) return kNone;
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    int sx = 0, sy = 0;
    viewports::SplitLines(rc.right, rc.bottom, arrangement_, sx, sy);
    const int reach = Scale(3);
    int d = kNone;
    if (std::abs(x - sx) <= reach) d |= kDragX;
    if (std::abs(y - sy) <= reach) d |= kDragY;
    return d;
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
        if (i != active_ && layout_.view[i].outer.Width() > 0) frame(i);
    frame(HasMaximizedView() ? arrangement_.maximized : active_);
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

    case WM_SETCURSOR:
        if (LOWORD(lp) == HTCLIENT) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(Hwnd(), &pt);
            const int d = drag_ ? drag_ : DividerAt(pt.x, pt.y);
            if (d) {
                SetCursor(LoadCursorW(nullptr, d == kDragBoth ? IDC_SIZEALL : d == kDragX ? IDC_SIZEWE : IDC_SIZENS));
                return TRUE;
            }
        }
        break;
    case WM_LBUTTONDOWN:
        drag_ = DividerAt(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        if (drag_) SetCapture(Hwnd());
        return 0;
    case WM_MOUSEMOVE:
        if (drag_) {
            RECT rc;
            GetClientRect(Hwnd(), &rc);
            if (drag_ & kDragX) arrangement_.splitX = viewports::FractionForSplitX(GET_X_LPARAM(lp), rc.right);
            if (drag_ & kDragY) arrangement_.splitY = viewports::FractionForSplitY(GET_Y_LPARAM(lp), rc.bottom);
            Relayout();
            UpdateWindow(Hwnd()); // repaint the frames now, not after the drag
        }
        return 0;
    case WM_LBUTTONUP:
        if (drag_) ReleaseCapture();
        return 0;
    case WM_CAPTURECHANGED:
        drag_ = kNone;
        return 0;
    }
    return win::Window::HandleMessage(msg, wp, lp);
}

} // namespace ui

namespace ui {

void ViewPanel::ResetAllViews() {
    const ViewType types[viewports::kCount] = {ViewType::Top, ViewType::Front, ViewType::Left, ViewType::Perspective};
    for (size_t i = 0; i < views_.size(); ++i) {
        views_[i].SetType(types[i]);
        views_[i].ResetView();
    }
}

} // namespace ui
