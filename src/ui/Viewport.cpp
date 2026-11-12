#include <glad/gl.h>
#include <windowsx.h>
#include <algorithm>
#include <cmath>
#include "ui/Viewport.h"
#include "ui/Cursors.h"
#include "scene/SoftSelection.h"
#include "scene/Groups.h"
#include "scene/TypeIn.h"
#include "scene/MeshTopology.h"
#include "scene/MoveMath.h"
#include "gl/GLContext.h"
#include "gl/LineRenderer.h"
#include "gl/MeshRenderer.h"
#include "ui/Bevel.h"
#include "ui/Theme.h"
#include "view/Tripod.h"
#include "scene/Scene.h"

namespace ui {

bool Viewport::Create(HWND parent, gl::GLContext& context, gl::LineRenderer& lines,
                      gl::MeshRenderer& meshes, ViewType type) {
    context_ = &context;
    lines_ = &lines;
    meshes_ = &meshes;
    // R3's defaults: the perspective view is shaded, the ortho views are wireframe.
    shading_ = type == ViewType::Perspective ? Shading::SmoothHighlights : Shading::Wireframe;
    type_ = type;
    CreateParams p;
    p.title = L"Viewport";
    // An OpenGL window must not have its pixels drawn over by siblings or children.
    p.style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    p.x = p.y = 0;
    p.width = p.height = 1;
    p.parent = parent;
    return win::Window::Create(p);
}

void Viewport::Render(HDC dc) {
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    context_->MakeCurrent(dc);
    glViewport(0, 0, rc.right, rc.bottom);
    glClearColor(theme::kViewportBg.R(), theme::kViewportBg.G(), theme::kViewportBg.B(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // OpenGL objects must be created with the context current, so the shared line
    // renderer is created on the first paint of whichever viewport comes first.
    if (!lines_->IsValid()) lines_->Create();
    if (!meshes_->IsValid()) meshes_->Create();
    if (showGrid_) {
        if (type_ == ViewType::Perspective)
            DrawPerspectiveGrid(rc.right, rc.bottom);
        else
            DrawOrthoGrid(rc.right, rc.bottom);
    }
    DrawScene(rc.right, rc.bottom);
    DrawVertices(rc.right, rc.bottom);
    DrawTransformAxes(rc.right, rc.bottom);
    DrawLabelAndTripod(rc.right, rc.bottom);
    if (regionVisible_) DrawRegionRectangle(rc.right, rc.bottom);

    SwapBuffers(dc); // show the finished back buffer
}

// Minor lines first, then major, then the axes, so darker lines are always on top.
static const theme::Rgb kGridColours[3] = {theme::kGridMinor, theme::kGridMajor, theme::kGridAxis};

// Ortho: lines computed in pixels (exactly R3's), drawn through pixel centres and
// extended past both ends so no end pixel is lost.
void Viewport::DrawOrthoGrid(int w, int h) {
    using Line = gl::LineRenderer::Line;
    std::vector<Line> batch[3];
    for (const view::GridLine& l : view::OrthoGridLines(w, h, ortho_)) {
        const float c = static_cast<float>(l.pixel) + 0.5f;
        const Line line = l.vertical ? Line{c, -1.0f, 0, c, static_cast<float>(h) + 1.0f, 0}
                                     : Line{-1.0f, c, 0, static_cast<float>(w) + 1.0f, c, 0};
        batch[static_cast<int>(l.kind)].push_back(line);
    }
    const math::Mat4 toClip = math::PixelToClip(w, h);
    for (int k = 0; k < 3; ++k)
        if (k == 0) {
            // Minor lines: a grey level that depends on their spacing (OrthoMinorShade).
            const float g = static_cast<float>(view::OrthoMinorShade(w, ortho_)) / 255.0f;
            lines_->Draw(batch[k], toClip, g, g, g);
        } else {
            lines_->Draw(batch[k], toClip, kGridColours[k].R(), kGridColours[k].G(), kGridColours[k].B());
        }
}

// Perspective: the home grid in world space, through the camera's matrices.
void Viewport::DrawPerspectiveGrid(int w, int h) {
    using Line = gl::LineRenderer::Line;
    std::vector<Line> batch[3];
    for (const view::WorldLine& l : view::HomeGridLines()) {
        batch[l.kind].push_back({static_cast<float>(l.a.x), static_cast<float>(l.a.y), static_cast<float>(l.a.z),
                                 static_cast<float>(l.b.x), static_cast<float>(l.b.y), static_cast<float>(l.b.z)});
    }
    const math::Mat4 viewProj = perspective_.ProjectionMatrix(w, h) * perspective_.ViewMatrix();
    for (int k = 0; k < 3; ++k)
        lines_->Draw(batch[k], viewProj, kGridColours[k].R(), kGridColours[k].G(), kGridColours[k].B());
}

const wchar_t* Viewport::Name() const {
    switch (type_) {
    case ViewType::Top:         return L"Top";
    case ViewType::Bottom:      return L"Bottom";
    case ViewType::Front:       return L"Front";
    case ViewType::Back:        return L"Back";
    case ViewType::Left:        return L"Left";
    case ViewType::Right:       return L"Right";
    case ViewType::Perspective: return L"Perspective";
    }
    return L"";
}

void Viewport::SetType(ViewType type) {
    if (type == type_) return;
    type_ = type;
    labelDpi_ = 0;                 // the label text changed: render it again
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

// ---- navigation --------------------------------------------------------------

void Viewport::Pan(int dx, int dy) {
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    if (type_ == ViewType::Perspective) perspective_ = view::PanPerspective(perspective_, dx, dy, rc.right);
    else ortho_ = view::PanOrtho(ortho_, dx, dy, rc.right, rc.bottom);
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

void Viewport::Zoom(double factor) {
    if (type_ == ViewType::Perspective) perspective_ = view::ZoomPerspective(perspective_, factor);
    else ortho_ = view::ZoomOrtho(ortho_, factor);
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

// Arc Rotate turns the perspective camera. In R3 it also turns an ortho view into a
// rotated "User" view; User views are not implemented yet, so ortho views ignore it.
void Viewport::Orbit(int dx, int dy) {
    if (type_ != ViewType::Perspective) return;
    perspective_ = view::OrbitPerspective(perspective_, dx, dy);
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

// Region Zoom for ortho views. (In R3 the perspective view offers Field-of-View
// instead; not implemented yet.)
void Viewport::ZoomRegion(int x0, int y0, int x1, int y1) {
    if (type_ == ViewType::Perspective) return;
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    ortho_ = view::ZoomOrthoRegion(ortho_, x0, y0, x1, y1, rc.right, rc.bottom);
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

bool Viewport::PointOnPlane(int x, int y, scene::ConstructionPlane& plane, double& u, double& v,
                            double& unitsPerPixel) const {
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    const int w = rc.right, h = rc.bottom;
    if (w <= 0 || h <= 0) return false;
    if (type_ != ViewType::Perspective) {
        // Ortho: the plane is the view's own (right, up); the grid formula, reversed.
        math::Vec3 right, up;
        OrthoBasis(right, up);
        plane = {right, up, math::Cross(right, up)};
        u = view::UOfColumn(x, w, h, ortho_);
        v = view::VOfRow(y, w, h, ortho_);
        unitsPerPixel = view::OrthoUnitsPerPixel(ortho_, w);
        return true;
    }
    // Perspective: a ray from the eye through the pixel, hitting the ground (z = 0).
    const math::Mat4 view = perspective_.ViewMatrix();
    const math::Vec3 right{view.m[0][0], view.m[1][0], view.m[2][0]};
    const math::Vec3 up{view.m[0][1], view.m[1][1], view.m[2][1]};
    const math::Vec3 eye = perspective_.Eye();
    const math::Vec3 forward = math::Normalize(perspective_.target - eye);
    const double f = (w / 2.0) / std::tan(perspective_.fovDeg * 3.14159265358979323846 / 360.0);
    const math::Vec3 dir = forward * f + right * (x + 0.5 - w / 2.0) - up * (y + 0.5 - h / 2.0);
    if (std::fabs(dir.z) < 1e-12) return false;
    const double t = -eye.z / dir.z;
    if (t <= 0) return false;                              // the ground is behind the ray
    const math::Vec3 hit = eye + dir * t;
    plane = scene::TopPlane();
    u = hit.x;
    v = hit.y;
    unitsPerPixel = 2.0 * perspective_.distance * std::tan(perspective_.fovDeg * 3.14159265358979323846 / 360.0) / w;
    return true;
}

scene::ConstructionPlane Viewport::CreationPlane() const {
    if (type_ == ViewType::Perspective) return scene::TopPlane();
    math::Vec3 right, up;
    OrthoBasis(right, up);
    return {right, up, math::Cross(right, up)};
}

bool Viewport::CreateEvent(scene::Creator::Event e, LPARAM lp) {
    if (!onCreateEvent) return false;
    scene::ConstructionPlane plane;
    double u = 0, v = 0, upp = 1;
    if (!PointOnPlane(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), plane, u, v, upp)) return false;
    return onCreateEvent(e, index_, plane, u, v, GET_Y_LPARAM(lp), upp);
}

void Viewport::ResetView() {
    ortho_ = view::OrthoView{};
    perspective_ = view::PerspectiveView{};
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

// NOTE: R3 has its own navigation cursors (magnifier, hand, ...); these are the
// closest standard ones until the real ones are drawn.
HCURSOR Viewport::CursorFor(NavMode mode) const {
    switch (mode) {
    case NavMode::Pan:        return LoadCursorW(nullptr, IDC_HAND);
    case NavMode::Zoom:
    case NavMode::ZoomAll:    return LoadCursorW(nullptr, IDC_SIZENS);
    case NavMode::RegionZoom: return LoadCursorW(nullptr, IDC_CROSS);
    case NavMode::ArcRotate:  return LoadCursorW(nullptr, IDC_SIZEALL);
    case NavMode::None:       break;
    }
    return LoadCursorW(nullptr, IDC_ARROW);
}

void Viewport::BeginDrag(NavMode op, int x, int y) {
    drag_ = op;
    dragLast_ = dragStart_ = {x, y};
    regionVisible_ = false;
    SetCapture(Hwnd());
}

void Viewport::EndDrag(bool apply) {
    if (apply && drag_ == NavMode::RegionZoom && regionVisible_)
        ZoomRegion(dragStart_.x, dragStart_.y, dragLast_.x, dragLast_.y);
    drag_ = NavMode::None;
    if (regionVisible_) { regionVisible_ = false; InvalidateRect(Hwnd(), nullptr, FALSE); }
}

// The region being dragged, as a 1-px white rectangle. NOTE: R3 draws a dashed
// rectangle; its exact look is not measured yet.
void Viewport::DrawRegionRectangle(int w, int h) {
    const float x0 = static_cast<float>(std::min(dragStart_.x, dragLast_.x)) + 0.5f;
    const float x1 = static_cast<float>(std::max(dragStart_.x, dragLast_.x)) + 0.5f;
    const float y0 = static_cast<float>(std::min(dragStart_.y, dragLast_.y)) + 0.5f;
    const float y1 = static_cast<float>(std::max(dragStart_.y, dragLast_.y)) + 0.5f;
    const std::vector<gl::LineRenderer::Line> box = {
        {x0, y0, 0, x1, y0, 0}, {x1, y0, 0, x1, y1, 0}, {x1, y1, 0, x0, y1, 0}, {x0, y1, 0, x0, y0, 0}};
    lines_->Draw(box, math::PixelToClip(w, h), 1.0f, 1.0f, 1.0f);
}

// Every node's mesh as a wireframe in the node's colour: visible edges only, all of
// them (no hidden-line removal), over the grid and under the label and tripod.
void Viewport::OrthoBasis(math::Vec3& right, math::Vec3& up) const {
    double axes[3][2];
    ScreenAxes(axes);
    right = {axes[0][0], axes[1][0], axes[2][0]};
    up = {axes[0][1], axes[1][1], axes[2][1]};
}

// A mesh made of an object's bounding box, for the Bounding Box display mode.
static scene::Mesh BoxOutline(const scene::Bounds& b) {
    scene::Mesh m;
    for (int i = 0; i < 8; ++i)
        m.verts.push_back({(i & 1) ? b.max.x : b.min.x, (i & 2) ? b.max.y : b.min.y, (i & 4) ? b.max.z : b.min.z});
    // Twelve edges as degenerate faces with one visible edge each (only edges are drawn).
    const int e[12][2] = {{0, 1}, {2, 3}, {4, 5}, {6, 7}, {0, 2}, {1, 3}, {4, 6}, {5, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    for (const auto& ed : e) {
        scene::Face f;
        f.v[0] = static_cast<uint32_t>(ed[0]); f.v[1] = static_cast<uint32_t>(ed[1]); f.v[2] = static_cast<uint32_t>(ed[1]);
        f.edgeVisible = 0x1;
        m.faces.push_back(f);
    }
    return m;
}

void Viewport::DrawScene(int w, int h) {
    if (!scene_) return;
    switch (shading_) {
    case Shading::SmoothHighlights: DrawShaded(w, h, false, true); return;
    case Shading::Smooth:           DrawShaded(w, h, false, false); return;
    case Shading::FacetsHighlights: DrawShaded(w, h, true, true); return;
    case Shading::Facets:           DrawShaded(w, h, true, false); return;
    case Shading::Wireframe:
    case Shading::LitWireframe:     // NOTE: drawn like Wireframe for now
    case Shading::BoundingBox:      break;
    }
    using Line = gl::LineRenderer::Line;
    const bool persp = type_ == ViewType::Perspective;
    // Ortho: the view's right and up directions in world space (from the same table
    // as the tripod), so a world point maps to (u, v) and then to pixels exactly like
    // the grid does.
    math::Vec3 right, up;
    OrthoBasis(right, up);
    const math::Mat4 toClip = persp ? perspective_.ProjectionMatrix(w, h) * perspective_.ViewMatrix()
                                    : math::PixelToClip(w, h);

    for (const auto& node : scene_->Nodes()) {
        if (!node->GetObject()) continue;
        const scene::Mesh mesh = shading_ == Shading::BoundingBox
            ? BoxOutline(node->GetObject()->Evaluate().GetBounds())
            : node->GetObject()->Evaluate();
        const math::Mat4 m = node->ObjectMatrix();
        std::vector<math::Vec3> p(mesh.verts.size());
        for (size_t i = 0; i < p.size(); ++i) p[i] = math::TransformPoint(m, mesh.verts[i]);

        // R3.1 culls back faces in wireframe (measured: r3.1-open-*.png show one hemisphere
        // of a sphere): a face is shown when its world-space winding faces the viewer.
        std::vector<bool> shown(mesh.faces.size());
        const math::Vec3 outOfScreen = math::Cross(right, up);
        const math::Vec3 eye = persp ? perspective_.Eye() : math::Vec3{0, 0, 0};
        const double flip = math::Mirrors(m) ? -1.0 : 1.0;      // a mirrored object winds the other way
        for (size_t i = 0; i < mesh.faces.size(); ++i) {
            const scene::Face& f = mesh.faces[i];
            const math::Vec3 n = math::Cross(p[f.v[1]] - p[f.v[0]], p[f.v[2]] - p[f.v[0]]) * flip;
            const math::Vec3 toViewer = persp ? eye - p[f.v[0]] : outOfScreen;
            shown[i] = math::Dot(n, toViewer) > 0;
        }
        std::vector<Line> lines;
        // Backface Cull off (11.4b): every visible edge, front or back.
        for (const auto& [a, b] : mesh.VisibleEdges(shading_ == Shading::BoundingBox || !node->BackfaceCull() ? nullptr : &shown)) {
            if (persp) {
                lines.push_back({static_cast<float>(p[a].x), static_cast<float>(p[a].y), static_cast<float>(p[a].z),
                                 static_cast<float>(p[b].x), static_cast<float>(p[b].y), static_cast<float>(p[b].z)});
                continue;
            }
            // Snap both ends to pixel centres (as the grid lines are). OpenGL leaves out a
            // line's last pixel (the "diamond-exit" rule), so extend only the end by half
            // a pixel; extending the start too would add a pixel before it.
            auto px = [&](const math::Vec3& q, float& x, float& y) {
                x = static_cast<float>(std::floor(view::ColumnF(math::Dot(q, right), w, h, ortho_))) + 0.5f;
                y = static_cast<float>(std::floor(view::RowF(math::Dot(q, up), w, h, ortho_))) + 0.5f;
            };
            float x0, y0, x1, y1;
            px(p[a], x0, y0);
            px(p[b], x1, y1);
            const float dx = x1 - x0, dy = y1 - y0, len = std::sqrt(dx * dx + dy * dy);
            const float ex = len > 0 ? 0.5f * dx / len : 0.5f, ey = len > 0 ? 0.5f * dy / len : 0.0f;
            lines.push_back({x0, y0, 0, x1 + ex, y1 + ey, 0});
        }
        // Selected objects are white (measured, R3.1 screenshots); so are the members of a
        // selected group.
        const uint32_t c = scene::groups::ShownSelected(*scene_, *node) ? 0xFFFFFF : node->WireColor();
        lines_->Draw(lines, toClip, ((c >> 16) & 0xFF) / 255.0f, ((c >> 8) & 0xFF) / 255.0f, (c & 0xFF) / 255.0f);
    }
    DrawOpenGroups(w, h);
    DrawGizmo(w, h);
}

// The gizmo: its lattice (the input's box, deformed by the modifier) and the centre as a
// small cross. NOTE: R3's colours from memory: orange, yellow at the sub-object level
// being edited.
void Viewport::DrawGizmo(int w, int h) {
    const GizmoEditing& g = gizmoEditing_;
    if (!g.on || !g.mod || !g.stack || !scene_) return;
    const scene::Node* node = scene_->FindById(g.node);
    if (!node) return;
    const math::Mat4 m = node->ObjectMatrix();
    const scene::Mesh input = g.stack->EvaluateUpTo(g.index);
    std::vector<gl::LineRenderer::Line> lattice, centre;
    for (const auto& [a, b] : g.mod->GizmoLines(input)) {
        double ax, ay, bx, by, d;
        if (!WorldToWindow(math::TransformPoint(m, a), ax, ay, d) || !WorldToWindow(math::TransformPoint(m, b), bx, by, d)) continue;
        lattice.push_back({static_cast<float>(ax), static_cast<float>(ay), 0.0f, static_cast<float>(bx), static_cast<float>(by), 0.0f});
    }
    double cx, cy, d;
    if (WorldToWindow(math::TransformPoint(m, g.mod->CenterPoint()), cx, cy, d)) {
        const float x = static_cast<float>(std::floor(cx)) + 0.5f, y = static_cast<float>(std::floor(cy)) + 0.5f;
        centre.push_back({x - 4, y, 0, x + 5, y, 0});
        centre.push_back({x, y - 4, 0, x, y + 5, 0});
    }
    const math::Mat4 toClip = math::PixelToClip(w, h);
    const float orange[3] = {1.0f, 0.55f, 0.0f}, yellow[3] = {1.0f, 1.0f, 0.0f};
    const float* lc = g.level == 0 ? yellow : orange;
    const float* cc = g.level == 1 ? yellow : orange;
    if (!lattice.empty()) lines_->Draw(lattice, toClip, lc[0], lc[1], lc[2]);
    if (!centre.empty()) lines_->Draw(centre, toClip, cc[0], cc[1], cc[2]);
}

// An open group shows its members' bounding box (NOTE: pink in R3, from memory; drawn
// here as the box's twelve edges in #FF7FB2, measured in 11.3 from r3.1-group-open.png).
void Viewport::DrawOpenGroups(int w, int h) {
    std::vector<gl::LineRenderer::Line> lines;
    for (const auto& node : scene_->Nodes()) {
        if (!node->IsGroupHead() || !node->GroupOpen()) continue;
        math::Vec3 lo, hi;
        if (!scene::groups::Bounds(*node, lo, hi)) continue;
        view::Point2 px[8];
        bool visible = true;
        for (int c = 0; c < 8; ++c) {
            const math::Vec3 p{c & 1 ? hi.x : lo.x, c & 2 ? hi.y : lo.y, c & 4 ? hi.z : lo.z};
            double x = 0, y = 0, depth = 0;
            visible = visible && WorldToWindow(p, x, y, depth);
            px[c] = {x, y};
        }
        if (!visible) continue;
        for (int c = 0; c < 8; ++c)
            for (int k = 1; k <= 4; k <<= 1)
                if (!(c & k))
                    lines.push_back({static_cast<float>(px[c].x), static_cast<float>(px[c].y), 0.0f,
                                     static_cast<float>(px[c | k].x), static_cast<float>(px[c | k].y), 0.0f});
    }
    if (!lines.empty()) lines_->Draw(lines, math::PixelToClip(w, h), 1.0f, 127.0f / 255.0f, 178.0f / 255.0f);   // #FF7FB2, measured (r3.1-group-open.png)
}

// ---- The Move gizmo under the mouse (Phase 12.2) -----------------------------------------

void Viewport::EffectiveConstraint(bool out[3]) const {
    const int mask = moving_ ? dragMask_ : hoverMask_;
    for (int a = 0; a < 3; ++a) out[a] = mask ? (mask & (1 << a)) != 0 : axesActive_[a];
}

bool Viewport::GizmoScreen(double axes[3][2], int& ox, int& oy) const {
    if (!scene_ || scene_->Selection().empty() || axesStyle_ != view::AxesStyle::MoveGizmo) return false;
    double px = 0, py = 0, depth = 0;
    if (!WorldToWindow(TransformCenter(), px, py, depth)) return false;
    const scene::move::Frame frame = CurrentFrame();       // as DrawTransformAxes
    math::Vec3 right, up;
    OrthoBasis(right, up);
    for (int a = 0; a < 3; ++a) {
        axes[a][0] = math::Dot(frame.axis[a], right);
        axes[a][1] = math::Dot(frame.axis[a], up);
    }
    ox = static_cast<int>(std::floor(px));
    oy = static_cast<int>(std::floor(py));
    return true;
}

// The axes at the selection's pivot (one object: its pivot; several: the average of
// their pivots; NOTE: R3's choice for several objects is unverified). The Move gizmo
// shows the world axes, the tripod the object's own axes (measured, step 6.4a).
void Viewport::DrawTransformAxes(int w, int h) {
    if (!scene_ || scene_->Selection().empty()) return;
    const scene::Node* first = nullptr;
    int count = 0;
    for (scene::Node::Id id : scene_->Selection())
        if (const scene::Node* n = scene_->FindById(id)) {
            if (!first) first = n;
            ++count;
        }
    if (!count) return;
    const math::Vec3 pivot = TransformCenter();
    double px = 0, py = 0, depth = 0;
    if (!WorldToWindow(pivot, px, py, depth)) return;

    // Screen directions: the reference system's axes; a single object's tripod its own.
    double axes[3][2];
    {
        const scene::move::Frame frame = CurrentFrame();
        math::Vec3 right, up;
        OrthoBasis(right, up);
        for (int a = 0; a < 3; ++a) {
            axes[a][0] = math::Dot(frame.axis[a], right);
            axes[a][1] = math::Dot(frame.axis[a], up);
        }
    }
    if (axesStyle_ == view::AxesStyle::Tripod && count == 1) {
        const math::Mat4 m = first->WorldMatrix();
        double world[3][2];
        ScreenAxes(world);
        for (int a = 0; a < 3; ++a) {
            const math::Vec3 d = math::Normalize({m.m[a][0], m.m[a][1], m.m[a][2]});
            axes[a][0] = d.x * world[0][0] + d.y * world[1][0] + d.z * world[2][0];
            axes[a][1] = d.x * world[0][1] + d.y * world[1][1] + d.z * world[2][1];
        }
    }
    bool active[3];
    EffectiveConstraint(active);                           // a hovered gizmo part shows yellow (12.2)
    const auto pixels = view::TransformAxesPixels(axesStyle_, axes, static_cast<int>(std::floor(px)),
                                                  static_cast<int>(std::floor(py)), active);
    // Group by colour, in order, so later pixels still cover earlier ones.
    std::vector<float> run;
    uint32_t colour = pixels.empty() ? 0 : pixels.front().rgb;
    const math::Mat4 toClip = math::PixelToClip(w, h);
    auto flush = [&] {
        if (!run.empty())
            lines_->DrawPoints(run, toClip, ((colour >> 16) & 0xFF) / 255.0f, ((colour >> 8) & 0xFF) / 255.0f, (colour & 0xFF) / 255.0f);
        run.clear();
    };
    for (const view::ColoredPixel& p : pixels) {
        if (p.rgb != colour) { flush(); colour = p.rgb; }
        run.insert(run.end(), {p.x + 0.5f, p.y + 0.5f, 0.0f});
    }
    flush();
}

// Edit Mesh's output (its input from the stack below it, edited), in window pixels and
// world space, with the maps back to input indices. Points behind the camera are placed far
// outside the view. If the input no longer matches what Edit Mesh recorded, its edits do
// not apply: the input is shown with identity maps.
Viewport::EditedMeshView Viewport::EditedView() const {
    EditedMeshView v;
    if (!vertexEditing_.on || !vertexEditing_.stack || !vertexEditing_.mod || !scene_ || vertexEditing_.upTo == 0) return v;
    const scene::Node* node = scene_->FindById(vertexEditing_.node);
    if (!node) return v;
    const scene::Mesh input = vertexEditing_.stack->EvaluateUpTo(vertexEditing_.upTo - 1);
    if (input.verts.size() == vertexEditing_.mod->ExpectedVertexCount()) {
        v.mesh = vertexEditing_.mod->Edited(input, &v.vertexMap, &v.faceMap);
    } else {
        v.mesh = input;
        for (uint32_t i = 0; i < input.verts.size(); ++i) v.vertexMap.push_back(i);
        for (uint32_t i = 0; i < input.faces.size(); ++i) v.faceMap.push_back(i);
    }
    const math::Mat4 m = node->ObjectMatrix();
    for (const auto& p : v.mesh.verts) {
        const math::Vec3 w = math::TransformPoint(m, p);
        v.world.push_back(w);
        double x = 0, y = 0, depth = 0;
        if (WorldToWindow(w, x, y, depth)) { v.pixels.push_back({x, y}); v.depth.push_back(depth); }
        else { v.pixels.push_back({-1e9, -1e9}); v.depth.push_back(1e300); }
    }
    // Hidden vertices (Vertex level): placed off-screen, so neither drawn nor picked.
    if (vertexEditing_.level == 0) {
        const auto& hidden = vertexEditing_.mod->HiddenVertices();
        for (size_t i = 0; i < v.pixels.size(); ++i)
            if (std::binary_search(hidden.begin(), hidden.end(), v.vertexMap[i])) v.pixels[i] = {-1e9, -1e9};
    }
    math::Vec3 right, up;
    OrthoBasis(right, up);
    const bool persp = type_ == ViewType::Perspective;
    const math::Vec3 eye = persp ? perspective_.Eye() : math::Vec3{0, 0, 0};
    const math::Vec3 outOfScreen = math::Cross(right, up);
    for (uint32_t fi = 0; fi < v.mesh.faces.size(); ++fi) {
        const scene::Face& f = v.mesh.faces[fi];
        v.tris.push_back({f.v[0], f.v[1], f.v[2]});
        const math::Vec3 n = math::Cross(v.world[f.v[1]] - v.world[f.v[0]], v.world[f.v[2]] - v.world[f.v[0]]) * (math::Mirrors(m) ? -1.0 : 1.0);
        v.backFacing.push_back(math::Dot(n, persp ? eye - v.world[f.v[0]] : outOfScreen) <= 0);
        for (int k = 0; k < 3; ++k)
            if (f.edgeVisible & (1u << k)) {
                v.segs.push_back({f.v[k], f.v[(k + 1) % 3]});
                v.segEdge.push_back(scene::EdgeId(v.faceMap[fi], k));
                v.segFace.push_back(fi);
            }
    }
    return v;
}

const std::vector<uint32_t>& Viewport::SubObjectSelection() const {
    const auto& m = *vertexEditing_.mod;
    return vertexEditing_.level == 0 ? m.SelectedVertices() : vertexEditing_.level == 1 ? m.SelectedEdges() : m.SelectedFaces();
}

std::vector<uint32_t> Viewport::SubObjectsAt(const EditedMeshView& v, double x, double y, math::Vec3* pivot) const {
    const int level = vertexEditing_.level;
    const bool ignoreBack = vertexEditing_.mod->Params().Int(scene::EditMeshModifier::kIgnoreBackfacing, 0) != 0;
    if (level == 0) {
        const int hit = view::PickNearestPoint(v.pixels, x, y, 4.0);
        if (hit < 0) return {};
        if (pivot) *pivot = v.world[static_cast<size_t>(hit)];
        return {v.vertexMap[static_cast<size_t>(hit)]};
    }
    if (level == 1) {
        // Visible edges; with Ignore Backfacing only those of faces turned towards the viewer.
        std::vector<view::Seg> segs;
        std::vector<size_t> which;
        for (size_t i = 0; i < v.segs.size(); ++i)
            if (!ignoreBack || !v.backFacing[v.segFace[i]]) { segs.push_back(v.segs[i]); which.push_back(i); }
        const int hit = view::PickNearestSegment(v.pixels, segs, x, y, 4.0);
        if (hit < 0) return {};
        const size_t s = which[static_cast<size_t>(hit)];
        if (pivot) *pivot = (v.world[v.segs[s].a] + v.world[v.segs[s].b]) * 0.5;
        return {v.segEdge[s]};
    }
    const int hit = view::PickFace(v.pixels, v.depth, v.tris, ignoreBack ? &v.backFacing : nullptr, x, y);
    if (hit < 0) return {};
    const view::Tri& t = v.tris[static_cast<size_t>(hit)];
    if (pivot) *pivot = (v.world[t.a] + v.world[t.b] + v.world[t.c]) * (1.0 / 3.0);
    std::vector<uint32_t> faces{static_cast<uint32_t>(hit)};
    if (level == 3) faces = scene::ExpandToPolygons(v.mesh, faces);
    if (level == 4) faces = scene::ExpandToElements(v.mesh, faces);
    std::vector<uint32_t> out;
    for (uint32_t f : faces) out.push_back(v.faceMap[f]);
    return out;
}

std::vector<uint32_t> Viewport::SubObjectsIn(const EditedMeshView& v, double x0, double y0, double x1, double y1) const {
    const int level = vertexEditing_.level;
    const bool ignoreBack = vertexEditing_.mod->Params().Int(scene::EditMeshModifier::kIgnoreBackfacing, 0) != 0;
    std::vector<uint32_t> out;
    if (level == 0) {
        for (int i : view::PickPointsInRect(v.pixels, x0, y0, x1, y1)) out.push_back(v.vertexMap[static_cast<size_t>(i)]);
        return out;
    }
    if (level == 1) {
        for (int i : view::PickSegmentsInRect(v.pixels, v.segs, x0, y0, x1, y1))
            if (!ignoreBack || !v.backFacing[v.segFace[static_cast<size_t>(i)]]) out.push_back(v.segEdge[static_cast<size_t>(i)]);
        return out;
    }
    std::vector<uint32_t> faces;
    for (int i : view::PickFacesInRect(v.pixels, v.tris, ignoreBack ? &v.backFacing : nullptr, x0, y0, x1, y1))
        faces.push_back(static_cast<uint32_t>(i));
    if (level == 3) faces = scene::ExpandToPolygons(v.mesh, faces);
    if (level == 4) faces = scene::ExpandToElements(v.mesh, faces);
    for (uint32_t f : faces) out.push_back(v.faceMap[f]);
    return out;
}

std::vector<view::Point2> Viewport::VertexPixels(std::vector<math::Vec3>* worldOut) const {
    EditedMeshView v = EditedView();
    if (worldOut) *worldOut = v.world;
    return v.pixels;
}

// Measured (r3.1-modify-editmesh-vertex.png): each vertex a 5-px diamond (rows 1, 3, 5, 3,
// 1) centred on its pixel, red #FF0000 when selected, blue #0000FF otherwise; red is
// drawn last.
void Viewport::DrawVertices(int w, int h) {
    const EditedMeshView v = EditedView();
    const auto& pts = v.pixels;
    if (pts.empty()) return;
    const math::Mat4 toClip = math::PixelToClip(w, h);
    if (vertexEditing_.slicePlane && scene_) {
        // The slice plane (10.9): a yellow square through its point, sized to the object
        // (NOTE: R3's plane gizmo look from memory).
        const scene::Node* node = scene_->FindById(vertexEditing_.node);
        const scene::Bounds b = v.mesh.GetBounds();
        if (node && !b.Empty()) {
            const math::Vec3 n = math::Normalize(vertexEditing_.planeNormal);
            const math::Vec3 a = math::Normalize(std::fabs(n.z) < 0.9 ? math::Cross(n, {0, 0, 1}) : math::Cross(n, {1, 0, 0}));
            const math::Vec3 c = math::Cross(n, a);
            const double r = 0.6 * math::Length(b.max - b.min);
            const math::Mat4 m = node->ObjectMatrix();
            math::Vec3 corner[4] = {vertexEditing_.planePoint + (a + c) * r, vertexEditing_.planePoint + (c - a) * r,
                                    vertexEditing_.planePoint - (a + c) * r, vertexEditing_.planePoint + (a - c) * r};
            std::vector<gl::LineRenderer::Line> lines;
            for (int i = 0; i < 4; ++i) {
                double x0, y0, x1, y1, d;
                if (!WorldToWindow(math::TransformPoint(m, corner[i]), x0, y0, d) || !WorldToWindow(math::TransformPoint(m, corner[(i + 1) % 4]), x1, y1, d)) continue;
                lines.push_back({static_cast<float>(x0), static_cast<float>(y0), 0.0f, static_cast<float>(x1), static_cast<float>(y1), 0.0f});
            }
            if (!lines.empty()) lines_->Draw(lines, toClip, 1.0f, 1.0f, 0.0f);
        }
    }
    if (vertexEditing_.level > 0) {
        // Edge level: the selected edges in red. Face level: the selected faces' three
        // edges; Polygon and Element: the selected faces' visible edges (their outlines).
        // NOTE: R3's look at these levels is unverified; vertices are not shown.
        const int level = vertexEditing_.level;
        std::vector<gl::LineRenderer::Line> lines;
        auto add = [&](uint32_t i, uint32_t j) {
            lines.push_back({static_cast<float>(pts[i].x), static_cast<float>(pts[i].y), 0.0f,
                             static_cast<float>(pts[j].x), static_cast<float>(pts[j].y), 0.0f});
        };
        if (level == 1) {
            const auto& sel = vertexEditing_.mod->SelectedEdges();
            for (size_t s = 0; s < v.segs.size(); ++s)
                if (std::binary_search(sel.begin(), sel.end(), v.segEdge[s])) add(v.segs[s].a, v.segs[s].b);
        } else {
            const auto& sel = vertexEditing_.mod->SelectedFaces();
            std::vector<float> dots;
            for (size_t f = 0; f < v.tris.size(); ++f) {
                if (!std::binary_search(sel.begin(), sel.end(), v.faceMap[f])) continue;
                const uint32_t idx[3] = {v.tris[f].a, v.tris[f].b, v.tris[f].c};
                for (int k = 0; k < 3; ++k) {
                    const bool visible = (v.mesh.faces[f].edgeVisible & (1u << k)) != 0;
                    if (visible) { add(idx[k], idx[(k + 1) % 3]); continue; }
                    if (level != 2) continue;
                    // Face level: hidden edges dotted, a pixel every 4 along the line
                    // (measured, r3.1-editmesh-face-a.png).
                    const view::Point2 &a = pts[idx[k]], &b = pts[idx[(k + 1) % 3]];
                    const double len = std::hypot(b.x - a.x, b.y - a.y);
                    for (double t = 0; t <= len; t += 4) {
                        const double x = a.x + (b.x - a.x) * t / len, y = a.y + (b.y - a.y) * t / len;
                        dots.insert(dots.end(), {static_cast<float>(std::floor(x)) + 0.5f, static_cast<float>(std::floor(y)) + 0.5f, 0.0f});
                    }
                }
            }
            if (!dots.empty()) lines_->DrawPoints(dots, toClip, 1.0f, 0.0f, 0.0f);
        }
        if (!lines.empty()) lines_->Draw(lines, toClip, 1.0f, 0.0f, 0.0f);
        return;
    }
    const auto& sel = vertexEditing_.mod->SelectedVertices();
    // Each vertex's colour group: 0 unselected (blue), 17 selected (red); with Soft Selection
    // on, 1..16 by weight, from blue through green and yellow to red (NOTE: R3's exact hues
    // unverified).
    std::vector<int> group(pts.size(), 0);
    std::vector<uint32_t> selOut;
    for (size_t i = 0; i < pts.size(); ++i)
        if (std::binary_search(sel.begin(), sel.end(), v.vertexMap[i])) { group[i] = 17; selOut.push_back(static_cast<uint32_t>(i)); }
    const auto& sp = vertexEditing_.mod->Params();
    if (sp.Int(scene::EditMeshModifier::kUseSoftSelection, 0) != 0 && !selOut.empty()) {
        const int edges = sp.Int(scene::EditMeshModifier::kEdgeDistance, 0) != 0 ? sp.Int(scene::EditMeshModifier::kEdgeIterations, 1) : -1;
        const auto w = scene::soft::Weights(v.mesh, selOut, sp.Float(scene::EditMeshModifier::kFalloff, 20),
                                            sp.Float(scene::EditMeshModifier::kPinch, 0), sp.Float(scene::EditMeshModifier::kBubble, 0), edges);
        for (size_t i = 0; i < pts.size() && i < w.size(); ++i)
            if (group[i] == 0 && w[i] > 0) group[i] = 1 + std::min(15, static_cast<int>(w[i] * 16));
    }
    for (int g = 0; g <= 17; ++g) {
        std::vector<float> run;
        for (size_t i = 0; i < pts.size(); ++i) {
            if (group[i] != g) continue;
            const int cx = static_cast<int>(std::floor(pts[i].x)), cy = static_cast<int>(std::floor(pts[i].y));
            static const int kHalf[5] = {0, 1, 2, 1, 0};
            for (int dy = -2; dy <= 2; ++dy)
                for (int dx = -kHalf[dy + 2]; dx <= kHalf[dy + 2]; ++dx)
                    run.insert(run.end(), {cx + dx + 0.5f, cy + dy + 0.5f, 0.0f});
        }
        if (run.empty()) continue;
        float r = 0, gr = 0, b = 1;                                   // unselected: blue
        if (g == 17) { r = 1; gr = 0; b = 0; }                        // selected: red
        else if (g > 0) {
            // Hue from 240 (blue, weight ~0) to 0 (red, weight 1).
            const double hue = 240.0 * (1.0 - (g - 0.5) / 16.0), x = 1.0 - std::fabs(std::fmod(hue / 60.0, 2.0) - 1.0);
            const int sector = static_cast<int>(hue / 60.0);
            const double rgb[6][3] = {{1, x, 0}, {x, 1, 0}, {0, 1, x}, {0, x, 1}, {x, 0, 1}, {1, 0, x}};
            r = static_cast<float>(rgb[sector][0]); gr = static_cast<float>(rgb[sector][1]); b = static_cast<float>(rgb[sector][2]);
        }
        lines_->DrawPoints(run, toClip, r, gr, b);
    }
}

math::Vec3 Viewport::SelectionPivot() const {
    math::Vec3 sum{0, 0, 0};
    int count = 0;
    for (scene::Node::Id id : scene_->Selection())
        if (const scene::Node* n = scene_->FindById(id)) { sum = sum + math::TransformPoint(n->WorldMatrix(), {0, 0, 0}); ++count; }
    return count ? sum * (1.0 / count) : sum;
}

// The ray under a pixel: from the eye in perspective; in ortho views from the point on
// the view plane, along the view direction.
bool Viewport::MouseRay(int x, int y, math::Vec3& origin, math::Vec3& dir) const {
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    const int w = rc.right, h = rc.bottom;
    if (w <= 0 || h <= 0) return false;
    if (type_ != ViewType::Perspective) {
        math::Vec3 right, up;
        OrthoBasis(right, up);
        const math::Vec3 toward = math::Cross(right, up);
        origin = right * view::UOfColumn(x, w, h, ortho_) + up * view::VOfRow(y, w, h, ortho_) + toward * 1e5;
        dir = toward * -1.0;
        return true;
    }
    const math::Mat4 view = perspective_.ViewMatrix();
    const math::Vec3 right{view.m[0][0], view.m[1][0], view.m[2][0]};
    const math::Vec3 up{view.m[0][1], view.m[1][1], view.m[2][1]};
    origin = perspective_.Eye();
    const math::Vec3 forward = math::Normalize(perspective_.target - origin);
    const double f = (w / 2.0) / std::tan(perspective_.fovDeg * 3.14159265358979323846 / 360.0);
    dir = math::Normalize(forward * f + right * (x + 0.5 - w / 2.0) - up * (y + 0.5 - h / 2.0));
    return true;
}

// Ortho: the mouse moves on the view plane; axes outside the constraint are dropped
// (Front with XY moves along X only). Perspective: the mouse ray meets a plane through
// the pivot (the constraint's plane, or for one axis the plane containing it that faces
// the camera), and the movement is kept to the allowed axes.
math::Vec3 Viewport::MoveDelta(int x, int y) const {
    // Constraints and drag planes in the reference coordinate system (9.9).
    const scene::move::Frame frame = CurrentFrame();
    math::Vec3 o0, d0, o1, d1;
    if (!MouseRay(moveStart_.x, moveStart_.y, o0, d0) || !MouseRay(x, y, o1, d1)) return {0, 0, 0};
    bool active[3];
    EffectiveConstraint(active);
    if (type_ != ViewType::Perspective) return scene::move::ConstrainInFrame(o1 - o0, active, frame);
    math::Vec3 n, h0, h1;
    const math::Vec3 forward = math::Normalize(perspective_.target - perspective_.Eye());
    if (!scene::move::DragPlaneNormalInFrame(active, forward, frame, n)) n = forward;   // all axes: the view plane
    if (!scene::move::RayPlane(o0, d0, movePivot_, n, h0) || !scene::move::RayPlane(o1, d1, movePivot_, n, h1))
        return {0, 0, 0};
    return scene::move::ConstrainInFrame(h1 - h0, axesActive_, frame);
}

// The reference frame for this view (View and Screen depend on it) and the first
// selected object (Parent, Local).
scene::move::Frame Viewport::CurrentFrame() const {
    math::Vec3 right, up;
    OrthoBasis(right, up);
    const scene::Node* first = nullptr;
    if (scene_)
        for (scene::Node::Id id : scene_->Selection())
            if ((first = scene_->FindById(id))) break;
    math::Quat parentQ, localQ;
    math::Vec3 parentPivot{0, 0, 0}, localPivot{0, 0, 0};
    const bool hasParent = first && first->Parent();
    if (first) {
        localQ = scene::typein::WorldRotation(*first);
        localPivot = math::TransformPoint(first->WorldMatrix(), {0, 0, 0});
    }
    if (hasParent) {
        parentQ = scene::typein::WorldRotation(*first->Parent());
        parentPivot = math::TransformPoint(first->Parent()->WorldMatrix(), {0, 0, 0});
    }
    return scene::move::MakeFrame(refCoord_, type_ == ViewType::Perspective, right, up,
                                  hasParent ? &parentQ : nullptr, parentPivot, first ? &localQ : nullptr, localPivot);
}

// Where transforms centre and the gizmo sits: the pivots' average (Use Pivot Point
// Center), the centre of the selection's world bounding box (Use Selection Center), or
// the reference system's origin (Use Transform Coordinate Center).
math::Vec3 Viewport::TransformCenter() const {
    if (centre_ == 2) return CurrentFrame().origin;
    math::Vec3 sum{0, 0, 0}, lo{1e300, 1e300, 1e300}, hi{-1e300, -1e300, -1e300};
    int count = 0;
    for (scene::Node::Id id : scene_->Selection()) {
        const scene::Node* n = scene_->FindById(id);
        if (!n) continue;
        sum = sum + math::TransformPoint(n->WorldMatrix(), {0, 0, 0});
        ++count;
        if (centre_ == 1 && n->GetObject()) {
            const math::Mat4 m = n->ObjectMatrix();
            for (const auto& v : n->GetObject()->Evaluate().verts) {
                const math::Vec3 w = math::TransformPoint(m, v);
                lo = {std::min(lo.x, w.x), std::min(lo.y, w.y), std::min(lo.z, w.z)};
                hi = {std::max(hi.x, w.x), std::max(hi.y, w.y), std::max(hi.z, w.z)};
            }
        }
    }
    if (centre_ == 1 && lo.x <= hi.x) return (lo + hi) * 0.5;
    return count ? sum * (1.0 / count) : sum;
}

bool Viewport::WorldToWindow(const math::Vec3& p, double& x, double& y, double& depth) const {
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    const int w = rc.right, h = rc.bottom;
    if (type_ == ViewType::Perspective) {
        const math::Mat4 vp = perspective_.ProjectionMatrix(w, h) * perspective_.ViewMatrix();
        const math::Vec4 c = vp * math::Vec4{p.x, p.y, p.z, 1.0};
        if (c.w <= 1e-9) return false;
        depth = c.w;
        return math::ProjectToPixel(vp, p, w, h, x, y);
    }
    math::Vec3 right, up;
    OrthoBasis(right, up);
    x = view::ColumnF(math::Dot(p, right), w, h, ortho_);
    y = view::RowF(math::Dot(p, up), w, h, ortho_);
    depth = -math::Dot(p, math::Cross(right, up));
    return true;
}

void Viewport::CollectPickData(std::vector<scene::Node::Id>& ids, std::vector<std::vector<view::Segment2>>& outlines,
                               std::vector<std::vector<view::Triangle2>>* surfaces) const {
    if (!scene_) return;
    for (const auto& node : scene_->Nodes()) {
        if (!node->GetObject()) continue;
        const scene::Mesh mesh = node->GetObject()->Evaluate();
        const math::Mat4 m = node->ObjectMatrix();
        std::vector<double> X(mesh.verts.size()), Y(mesh.verts.size()), D(mesh.verts.size());
        std::vector<bool> ok(mesh.verts.size());
        for (size_t i = 0; i < mesh.verts.size(); ++i)
            ok[i] = WorldToWindow(math::TransformPoint(m, mesh.verts[i]), X[i], Y[i], D[i]);
        ids.push_back(node->GetId());
        outlines.emplace_back();
        for (const auto& [a, b] : mesh.VisibleEdges())
            if (ok[a] && ok[b]) outlines.back().push_back({X[a], Y[a], X[b], Y[b]});
        if (surfaces) {
            surfaces->emplace_back();
            for (const scene::Face& f : mesh.faces)
                if (ok[f.v[0]] && ok[f.v[1]] && ok[f.v[2]])
                    surfaces->back().push_back({{X[f.v[0]], X[f.v[1]], X[f.v[2]]}, {Y[f.v[0]], Y[f.v[1]], Y[f.v[2]]},
                                                (D[f.v[0]] + D[f.v[1]] + D[f.v[2]]) / 3});
        }
    }
}

// In shaded views clicks pick surfaces; in wireframe views, edges within 4 px.
scene::Node::Id Viewport::PickAt(int x, int y) const {
    const bool shaded = shading_ != Shading::Wireframe && shading_ != Shading::LitWireframe &&
                        shading_ != Shading::BoundingBox;
    std::vector<scene::Node::Id> ids;
    std::vector<std::vector<view::Segment2>> outlines;
    std::vector<std::vector<view::Triangle2>> surfaces;
    CollectPickData(ids, outlines, shaded ? &surfaces : nullptr);
    int hit = shaded ? view::PickSurface(surfaces, x + 0.5, y + 0.5) : -1;
    if (hit < 0) hit = view::PickNearest(outlines, x + 0.5, y + 0.5, 4.0);
    return hit >= 0 ? ids[static_cast<size_t>(hit)] : 0;
}

std::vector<scene::Node::Id> Viewport::PickRect(int x0, int y0, int x1, int y1) const {
    std::vector<scene::Node::Id> ids;
    std::vector<std::vector<view::Segment2>> outlines;
    CollectPickData(ids, outlines, nullptr);
    std::vector<scene::Node::Id> out;
    const auto picked = regionWindow_ ? view::PickRegionWindow(outlines, x0 + 0.5, y0 + 0.5, x1 + 0.5, y1 + 0.5)
                                      : view::PickRegion(outlines, x0 + 0.5, y0 + 0.5, x1 + 0.5, y1 + 0.5);
    for (int i : picked) out.push_back(ids[static_cast<size_t>(i)]);
    return out;
}

// Selected objects in shaded views: white brackets at the corners of the object's
// bounding box, each a quarter of the box's edge (measured roughly from R3.1's Box
// screenshot). Drawn over the shaded objects.
void Viewport::DrawBrackets(int /*w*/, int /*h*/, const math::Mat4& worldToClip) {
    using Line = gl::LineRenderer::Line;
    std::vector<Line> lines;
    for (const auto& node : scene_->Nodes()) {
        // One box per selected object; a selected group gets one box around all its
        // members (world-aligned), and its members none of their own.
        if (!scene_->IsSelected(node->GetId())) continue;
        double lo[3], hi[3];
        math::Mat4 m = math::Translation({0, 0, 0});
        if (node->IsGroupHead()) {
            math::Vec3 a, b;
            if (!scene::groups::Bounds(*node, a, b)) continue;
            lo[0] = a.x; lo[1] = a.y; lo[2] = a.z; hi[0] = b.x; hi[1] = b.y; hi[2] = b.z;
        } else {
            if (!node->GetObject()) continue;
            const scene::Bounds b = node->GetObject()->Evaluate().GetBounds();
            if (b.Empty()) continue;
            m = node->ObjectMatrix();
            lo[0] = b.min.x; lo[1] = b.min.y; lo[2] = b.min.z; hi[0] = b.max.x; hi[1] = b.max.y; hi[2] = b.max.z;
        }
        for (int corner = 0; corner < 8; ++corner) {
            double c[3];
            for (int k = 0; k < 3; ++k) c[k] = (corner >> k) & 1 ? hi[k] : lo[k];
            const math::Vec3 from = math::TransformPoint(m, {c[0], c[1], c[2]});
            for (int k = 0; k < 3; ++k) {
                double e[3] = {c[0], c[1], c[2]};
                e[k] += ((corner >> k) & 1 ? -1 : 1) * 0.25 * (hi[k] - lo[k]);
                const math::Vec3 to = math::TransformPoint(m, {e[0], e[1], e[2]});
                lines.push_back({static_cast<float>(from.x), static_cast<float>(from.y), static_cast<float>(from.z),
                                 static_cast<float>(to.x), static_cast<float>(to.y), static_cast<float>(to.z)});
            }
        }
    }
    if (!lines.empty()) lines_->Draw(lines, worldToClip, 1.0f, 1.0f, 1.0f);
}

// Lit triangles with the depth test on, so nearer surfaces hide farther ones. Normals
// come from smoothing groups (Smooth modes) or the faces themselves (Facets modes).
void Viewport::DrawShaded(int w, int h, bool facets, bool highlights) {
    const bool persp = type_ == ViewType::Perspective;
    math::Vec3 right, up, toward;
    gl::MeshRenderer::Lighting light;
    math::Mat4 toClip;
    if (persp) {
        const math::Mat4 view = perspective_.ViewMatrix();
        right = {view.m[0][0], view.m[1][0], view.m[2][0]};
        up = {view.m[0][1], view.m[1][1], view.m[2][1]};
        toward = math::Normalize(perspective_.Eye() - perspective_.target);
        toClip = perspective_.ProjectionMatrix(w, h) * view;
        light.eye = perspective_.Eye();
    } else {
        OrthoBasis(right, up);
        toward = math::Cross(right, up);
        toClip = view::OrthoWorldToClip(right, up, w, h, ortho_);
        light.ortho = true;
    }
    // A light above, to the left and behind the viewer (NOTE: R3's placement unverified).
    light.toLight = math::Normalize(toward + up * 0.6 - right * 0.4);
    light.viewDir = toward;
    light.highlights = highlights;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    for (const auto& node : scene_->Nodes()) {
        if (!node->GetObject()) continue;
        const scene::Mesh mesh = node->GetObject()->Evaluate();
        const math::Mat4 m = node->ObjectMatrix();
        // Normals transform with the inverse transpose (correct under non-uniform scale).
        const math::Mat4 inv = math::AffineInverse(m);
        const bool mirrored = math::Mirrors(m);
        auto normalToWorld = [&](const math::Vec3& n) {
            return math::Normalize(math::Vec3{inv.m[0][0] * n.x + inv.m[0][1] * n.y + inv.m[0][2] * n.z,
                                              inv.m[1][0] * n.x + inv.m[1][1] * n.y + inv.m[1][2] * n.z,
                                              inv.m[2][0] * n.x + inv.m[2][1] * n.y + inv.m[2][2] * n.z});
        };
        const std::vector<math::Vec3> corner = facets ? std::vector<math::Vec3>{} : mesh.CornerNormals();
        std::vector<float> v;
        v.reserve(mesh.faces.size() * 18);
        for (size_t i = 0; i < mesh.faces.size(); ++i) {
            const math::Vec3 fn = facets ? mesh.FaceNormal(i) : math::Vec3{};
            for (int j = 0; j < 3; ++j) {
                // A mirrored object's corners in reverse order, so its winding (which the
                // shader's two-sided lighting reads) agrees with its outward normals again.
                const int k = mirrored ? (3 - j) % 3 : j;
                const math::Vec3 p = math::TransformPoint(m, mesh.verts[mesh.faces[i].v[k]]);
                const math::Vec3 n = normalToWorld(facets ? fn : corner[i * 3 + static_cast<size_t>(k)]);
                v.insert(v.end(), {static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z),
                                   static_cast<float>(n.x), static_cast<float>(n.y), static_cast<float>(n.z)});
            }
        }
        const uint32_t c = node->HasMaterialColor() ? node->MaterialColor() : node->WireColor();   // a material's diffuse (11.4b)
        meshes_->Draw(v, toClip, ((c >> 16) & 0xFF) / 255.0f, ((c >> 8) & 0xFF) / 255.0f, (c & 0xFF) / 255.0f, light);
    }
    glDisable(GL_DEPTH_TEST);
    DrawBrackets(w, h, toClip);
    DrawOpenGroups(w, h);
    DrawGizmo(w, h);
}

void Viewport::SetShowGrid(bool show) {
    showGrid_ = show;
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

// Right-clicking the label opens the menu. The area is the label's ink plus a few
// pixels around it.
bool Viewport::InLabel(int x, int y) const {
    if (label_.pixels.empty()) return false;
    LONG right = 0, bottom = 0;
    for (const POINT& p : label_.pixels) { right = std::max(right, p.x); bottom = std::max(bottom, p.y); }
    return x >= 0 && y >= 0 && x <= right + Scale(4) && y <= bottom + Scale(3);
}

// R3's viewport menu, reconstructed from memory: items, order and names are still to
// be verified against R3.1. Entries that need features from later phases are greyed.
// Keyboard-shortcut text (R3 shows e.g. "Top  T") is left out until shortcuts exist.
void Viewport::ShowLabelMenu(POINT screen) {
    enum : UINT {
        kSmoothHi = 1, kWire, kSmooth, kFacetsHi, kFacets, kLitWire, kBox,
        kEdgedFaces, kShowGrid, kShowBackground, kSafeFrame, kDisable, kTexCorrection,
        kTop, kBottom, kFront, kBack, kLeft, kRight, kUser, kPersp, kCamera, kLight, kShape, kGrid,
        kUndo, kRedo, kConfigure,
    };
    auto check = [](bool on) { return on ? MF_CHECKED : MF_UNCHECKED; };
    const UINT grey = MF_GRAYED;

    HMENU other = CreatePopupMenu();
    AppendMenuW(other, MF_STRING | check(shading_ == Shading::Smooth), kSmooth, L"Smooth");
    AppendMenuW(other, MF_STRING | check(shading_ == Shading::FacetsHighlights), kFacetsHi, L"Facets + Highlights");
    AppendMenuW(other, MF_STRING | check(shading_ == Shading::Facets), kFacets, L"Facets");
    AppendMenuW(other, MF_STRING | check(shading_ == Shading::LitWireframe), kLitWire, L"Lit Wireframes");
    AppendMenuW(other, MF_STRING | check(shading_ == Shading::BoundingBox), kBox, L"Bounding Box");

    HMENU views = CreatePopupMenu();
    AppendMenuW(views, MF_STRING | check(type_ == ViewType::Top), kTop, L"Top");
    AppendMenuW(views, MF_STRING | check(type_ == ViewType::Bottom), kBottom, L"Bottom");
    AppendMenuW(views, MF_STRING | check(type_ == ViewType::Front), kFront, L"Front");
    AppendMenuW(views, MF_STRING | check(type_ == ViewType::Back), kBack, L"Back");
    AppendMenuW(views, MF_STRING | check(type_ == ViewType::Left), kLeft, L"Left");
    AppendMenuW(views, MF_STRING | check(type_ == ViewType::Right), kRight, L"Right");
    AppendMenuW(views, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(views, MF_STRING | grey, kUser, L"User");
    AppendMenuW(views, MF_STRING | check(type_ == ViewType::Perspective), kPersp, L"Perspective");
    AppendMenuW(views, MF_STRING | grey, kCamera, L"Camera");
    AppendMenuW(views, MF_STRING | grey, kLight, L"Spot/Directional Light");
    AppendMenuW(views, MF_STRING | grey, kShape, L"Shape");
    AppendMenuW(views, MF_STRING | grey, kGrid, L"Grid");

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING | check(shading_ == Shading::SmoothHighlights), kSmoothHi, L"Smooth + Highlights");
    AppendMenuW(menu, MF_STRING | check(shading_ == Shading::Wireframe), kWire, L"Wireframe");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(other), L"Other");
    AppendMenuW(menu, MF_STRING | grey, kEdgedFaces, L"Edged Faces");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | check(showGrid_), kShowGrid, L"Show Grid");
    AppendMenuW(menu, MF_STRING | grey, kShowBackground, L"Show Background");
    AppendMenuW(menu, MF_STRING | grey, kSafeFrame, L"Show Safe Frame");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | grey, kDisable, L"Disable View");
    AppendMenuW(menu, MF_STRING | grey, kTexCorrection, L"Texture Correction");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(views), L"Views");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | grey, kUndo, L"Undo View Change");
    AppendMenuW(menu, MF_STRING | grey, kRedo, L"Redo View Change");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | grey, kConfigure, L"Configure...");

    // TPM_RETURNCMD: return the chosen item instead of sending WM_COMMAND.
    const UINT cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_TOPALIGN,
                                    screen.x, screen.y, 0, Hwnd(), nullptr);
    DestroyMenu(menu); // also destroys the submenus it owns

    switch (cmd) {
    case kSmoothHi: shading_ = Shading::SmoothHighlights; break;
    case kWire:     shading_ = Shading::Wireframe; break;
    case kSmooth:   shading_ = Shading::Smooth; break;
    case kFacetsHi: shading_ = Shading::FacetsHighlights; break;
    case kFacets:   shading_ = Shading::Facets; break;
    case kLitWire:  shading_ = Shading::LitWireframe; break;
    case kBox:      shading_ = Shading::BoundingBox; break;
    case kShowGrid: SetShowGrid(!showGrid_); break;
    case kTop:      SetType(ViewType::Top); break;
    case kBottom:   SetType(ViewType::Bottom); break;
    case kFront:    SetType(ViewType::Front); break;
    case kBack:     SetType(ViewType::Back); break;
    case kLeft:     SetType(ViewType::Left); break;
    case kRight:    SetType(ViewType::Right); break;
    case kPersp:    SetType(ViewType::Perspective); break;
    default: break; // cancelled (0) or not implemented yet
    }
    InvalidateRect(Hwnd(), nullptr, FALSE); // the shading may have changed
}

// How world X, Y, Z appear on screen: components along screen-right and screen-up.
void Viewport::ScreenAxes(double out[3][2]) const {
    auto set = [&](double xr, double xu, double yr, double yu, double zr, double zu) {
        out[0][0] = xr; out[0][1] = xu; out[1][0] = yr; out[1][1] = yu; out[2][0] = zr; out[2][1] = zu;
    };
    switch (type_) {
    case ViewType::Top:    set(1, 0, 0, 1, 0, 0);   break; // looking down -Z
    case ViewType::Bottom: set(1, 0, 0, -1, 0, 0);  break; // looking up +Z: +Y points down
    case ViewType::Front:  set(1, 0, 0, 0, 0, 1);   break; // looking along +Y
    case ViewType::Back:   set(-1, 0, 0, 0, 0, 1);  break; // looking along -Y: +X points left
    case ViewType::Left:   set(0, 0, -1, 0, 0, 1);  break; // looking along +X: +Y points left
    case ViewType::Right:  set(0, 0, 1, 0, 0, 1);   break; // looking along -X: +Y points right
    case ViewType::Perspective: {
        // The camera's right and up directions, from its view matrix rows.
        const math::Mat4 v = perspective_.ViewMatrix();
        for (int a = 0; a < 3; ++a) {
            out[a][0] = v.m[a][0];   // world axis a . camera right
            out[a][1] = v.m[a][1];   // world axis a . camera up
        }
        break;
    }
    }
}

// The label (top-left) and the axis tripod (bottom-left), as single pixels drawn
// over the grid, in viewport pixel coordinates.
void Viewport::DrawLabelAndTripod(int w, int h) {
    const math::Mat4 toClip = math::PixelToClip(w, h);
    auto centre = [](int x, int y, std::vector<float>& v) {
        v.push_back(static_cast<float>(x) + 0.5f);
        v.push_back(static_cast<float>(y) + 0.5f);
        v.push_back(0.0f);
    };

    // Label: rendered once per DPI (measured: text starts at x 4, ink top at y 4).
    if (labelDpi_ != Dpi()) {
        HFONT font = CreateViewportLabelFont(Dpi());
        label_ = RenderText(Name(), font, Scale(4), Scale(4));
        DeleteObject(font);
        labelDpi_ = Dpi();
    }
    std::vector<float> text;
    for (const POINT& p : label_.pixels) centre(p.x, p.y, text);
    lines_->DrawPoints(text, toClip, theme::kViewportLabel.R(), theme::kViewportLabel.G(), theme::kViewportLabel.B());

    // Tripod: in axis order, so later axes cover earlier ones as in R3.
    double axes[3][2];
    ScreenAxes(axes);
    std::vector<float> pts[3];
    for (const view::TripodPixel& p : view::TripodPixels(axes, h)) centre(p.x, p.y, pts[p.axis]);
    const theme::Rgb colours[3] = {theme::kTripodX, theme::kTripodY, theme::kTripodZ};
    for (int a = 0; a < 3; ++a)
        lines_->DrawPoints(pts[a], toClip, colours[a].R(), colours[a].G(), colours[a].B());
}

void Viewport::PaintFallback(HDC dc) {
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    FillSolid(dc, rc, theme::kViewportBg.Ref());
    const std::wstring msg = L"OpenGL 3.3 is not available";
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, theme::kViewportLabel.Ref());
    HGDIOBJ old = SelectObject(dc, GetStockObject(DEFAULT_GUI_FONT));
    DrawTextW(dc, msg.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, old);
}

LRESULT Viewport::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        // Every window that will show OpenGL needs the context's pixel format.
        if (context_->IsValid()) context_->PrepareWindow(Hwnd());
        return 0;
    case WM_ERASEBKGND:
        return 1; // OpenGL fills every pixel
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(Hwnd(), &ps);
        if (context_->IsValid()) Render(dc);
        else PaintFallback(dc);
        EndPaint(Hwnd(), &ps);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        if (onActivate) onActivate();
        SetFocus(Hwnd()); // so the mouse wheel comes here
        const NavMode mode = navMode ? navMode() : NavMode::None;
        if (mode != NavMode::None) BeginDrag(mode, GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        else if (creationActive && creationActive()) {
            creatingDrag_ = true;
            SetCapture(Hwnd());
            CreateEvent(scene::Creator::Event::Press, lp);
        } else if (gizmoEditing_.on && gizmoEditing_.level >= 0 && transformTool_ && onMoveBegin && scene_) {
            // A deformer's gizmo or centre (10.3): a press anywhere drags it (NOTE: in R3 the
            // press may need to be on the gizmo; unverified). The drag plane passes through the
            // centre.
            const scene::Node* node = scene_->FindById(gizmoEditing_.node);
            moving_ = true;
            moveStart_ = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            movePivot_ = node ? math::TransformPoint(node->ObjectMatrix(), gizmoEditing_.mod->CenterPoint()) : math::Vec3{0, 0, 0};
            SetCapture(Hwnd());
            onMoveBegin();
        } else if (vertexEditing_.on && (vertexEditing_.tool == 1 || vertexEditing_.tool == 2) && onToolClick) {
            // Create / Cut (10.9): the click goes to the tool, not to selection.
            const EditedMeshView v = EditedView();
            const double px = GET_X_LPARAM(lp) + 0.5, py = GET_Y_LPARAM(lp) + 0.5;
            ToolClick c;
            c.towards = OutOfScreen();
            scene::ConstructionPlane plane;
            double u = 0, w = 0, upp = 0;
            if (PointOnPlane(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), plane, u, w, upp)) { c.onPlane = true; c.point = plane.ToWorld(u, w); }
            const int hv = view::PickNearestPoint(v.pixels, px, py, 4.0);
            if (hv >= 0) c.vertex = static_cast<int>(v.vertexMap[static_cast<size_t>(hv)]);
            const int he = view::PickNearestSegment(v.pixels, v.segs, px, py, 4.0);
            if (he >= 0) {
                const view::Point2 &a = v.pixels[v.segs[static_cast<size_t>(he)].a], &b = v.pixels[v.segs[static_cast<size_t>(he)].b];
                const double dx = b.x - a.x, dy = b.y - a.y, len2 = dx * dx + dy * dy;
                c.edge = v.segEdge[static_cast<size_t>(he)];
                c.t = len2 > 0 ? std::min(1.0, std::max(0.0, ((px - a.x) * dx + (py - a.y) * dy) / len2)) : 0.5;
            }
            onToolClick(c);
        } else if (vertexEditing_.on && vertexEditing_.tool == 3 && transformTool_ && onMoveBegin && scene_) {
            // Slice Plane (10.9): a transform tool drags the plane, from anywhere.
            const scene::Node* node = scene_->FindById(vertexEditing_.node);
            moving_ = true;
            moveStart_ = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            movePivot_ = node ? math::TransformPoint(node->ObjectMatrix(), vertexEditing_.planePoint) : math::Vec3{0, 0, 0};
            SetCapture(Hwnd());
            onMoveBegin();
        } else if (vertexEditing_.on && vertexEditing_.weldTarget && vertexEditing_.level == 0 && onWeldTarget) {
            // Weld > Target: press on a vertex; where it is released decides the target.
            const EditedMeshView v = EditedView();
            const std::vector<uint32_t> hits = SubObjectsAt(v, GET_X_LPARAM(lp) + 0.5, GET_Y_LPARAM(lp) + 0.5, nullptr);
            if (!hits.empty()) {
                weldDragging_ = true;
                weldFrom_ = hits[0];
                SetCapture(Hwnd());
            }
        } else if (vertexEditing_.on && onVertexSelect) {
            // Vertex or Face level: with a transform tool, pressing on a vertex (a face)
            // drags the selection (selecting it first); otherwise a click or region selects.
            // Selections are reported as Edit Mesh's input indices.
            const EditedMeshView v = EditedView();
            const double px = GET_X_LPARAM(lp) + 0.5, py = GET_Y_LPARAM(lp) + 0.5;
            math::Vec3 pivot{0, 0, 0};
            const std::vector<uint32_t> hits = SubObjectsAt(v, px, py, &pivot);
            if (transformTool_ && !hits.empty()) {
                const auto& sel = SubObjectSelection();
                bool all = true;
                for (uint32_t h : hits) all = all && std::binary_search(sel.begin(), sel.end(), h);
                if (!all) onVertexSelect(hits, false, (wp & MK_CONTROL) != 0, false);
                moving_ = true;
                moveStart_ = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
                movePivot_ = pivot;
                SetCapture(Hwnd());
                if (onMoveBegin) onMoveBegin();
            } else {
                selecting_ = true;
                dragStart_ = dragLast_ = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
                regionVisible_ = false;
                SetCapture(Hwnd());
            }
        } else if (onSelect && transformTool_ && hoverMask_ != 0 && !scene_->Selection().empty()) {
            // On the Move gizmo (12.2): drag the selection, constrained to the part pressed.
            dragMask_ = hoverMask_;
            moving_ = true;
            moveStart_ = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            movePivot_ = TransformCenter();
            SetCapture(Hwnd());
            if (onMoveBegin) onMoveBegin();
        } else if (onSelect && transformTool_ && PickAt(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)) != 0) {
            // Select and Move / Rotate / Scale on an object: select it if needed, then drag
            // the selection.
            const scene::Node::Id id = PickAt(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
            if (!scene_->IsSelected(id)) onSelect({id}, false, (wp & MK_CONTROL) != 0, false);
            dragMask_ = 0;                       // the toolbar constraint
            moving_ = true;
            moveStart_ = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            movePivot_ = TransformCenter();      // the drag plane through the transform centre (9.9)
            SetCapture(Hwnd());
            if (onMoveBegin) onMoveBegin();
        } else if (onSelect) {
            // Selecting: a click, or a region once the mouse moves.
            selecting_ = true;
            dragStart_ = dragLast_ = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            regionVisible_ = false;
            SetCapture(Hwnd());
        }
        return 0;
    }
    case WM_MBUTTONDOWN: {
        if (onActivate) onActivate();
        SetFocus(Hwnd());
        // Middle drag pans; with Alt it orbits, with Ctrl+Alt it zooms.
        // NOTE: R3.1's exact middle-button combinations are not verified.
        const bool alt = GetKeyState(VK_MENU) < 0, ctrl = (wp & MK_CONTROL) != 0;
        BeginDrag(alt ? (ctrl ? NavMode::Zoom : NavMode::ArcRotate) : NavMode::Pan,
                  GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        return 0;
    }
    case WM_MOUSEMOVE:
        if (moving_) {
            if (onMoveDrag) onMoveDrag(MoveDelta(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)), moveStart_.y - GET_Y_LPARAM(lp));
            return 0;
        }
        if (selecting_) {
            dragLast_ = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            const bool moved = std::abs(dragLast_.x - dragStart_.x) > 2 || std::abs(dragLast_.y - dragStart_.y) > 2;
            if (moved || regionVisible_) { regionVisible_ = true; InvalidateRect(Hwnd(), nullptr, FALSE); }
            return 0;
        }
        // The Move gizmo's part under the mouse (12.2): highlighted, and taken by a drag.
        if (drag_ == NavMode::None && !creatingDrag_ && !(creationActive && creationActive())) {
            int part = 0;
            double ax[3][2];
            int ox = 0, oy = 0;
            if (transformTool_ && GizmoScreen(ax, ox, oy)) part = view::GizmoPartAt(ax, ox, oy, GET_X_LPARAM(lp) + 0.5, GET_Y_LPARAM(lp) + 0.5);
            if (part != hoverMask_) { hoverMask_ = part; InvalidateRect(Hwnd(), nullptr, FALSE); }
            if (!trackingLeave_) {
                TRACKMOUSEEVENT t{sizeof(t), TME_LEAVE, Hwnd(), 0};
                trackingLeave_ = TrackMouseEvent(&t) != FALSE;
            }
        }
        // The status bar's X/Y/Z follow the cursor when nothing is being dragged.
        if (onCursorWorld && drag_ == NavMode::None && !selecting_ && !creatingDrag_) {
            scene::ConstructionPlane plane;
            double u = 0, v = 0, upp = 1;
            // Only positions inside this view count: windows can receive mouse messages
            // for points outside them (at startup, or while another view has captured
            // the mouse), and those must not show coordinates.
            RECT rc;
            GetClientRect(Hwnd(), &rc);
            const int mx = GET_X_LPARAM(lp), my = GET_Y_LPARAM(lp);
            const bool inside = mx >= 0 && my >= 0 && mx < rc.right && my < rc.bottom;
            const bool ok = inside && PointOnPlane(mx, my, plane, u, v, upp);
            onCursorWorld(ok, ok ? plane.ToWorld(u, v) : math::Vec3{0, 0, 0});
        }
        // Creation follows the mouse with or without the button (the height steps).
        if (drag_ == NavMode::None && creationActive && creationActive()) CreateEvent(scene::Creator::Event::Move, lp);
        if (drag_ != NavMode::None) {
            const int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp);
            const int dx = x - dragLast_.x, dy = y - dragLast_.y;
            dragLast_ = {x, y};
            switch (drag_) {
            case NavMode::Pan:        Pan(dx, dy); break;
            case NavMode::Zoom:       Zoom(view::ZoomFactorForDrag(dy)); break;
            case NavMode::ZoomAll: {
                const double f = view::ZoomFactorForDrag(dy);
                Zoom(f);
                if (onZoomAll) onZoomAll(f);
                break;
            }
            case NavMode::ArcRotate:  Orbit(dx, dy); break;
            case NavMode::RegionZoom: regionVisible_ = true; InvalidateRect(Hwnd(), nullptr, FALSE); break;
            case NavMode::None:       break;
            }
            UpdateWindow(Hwnd()); // redraw now, so the view follows the mouse
        }
        return 0;
    case WM_LBUTTONUP:
        if (weldDragging_) {
            // Released within the "pixels" threshold of another vertex: weld into it
            // (NOTE: R3 shows the vertex following the cursor while dragging; not here).
            weldDragging_ = false;
            ReleaseCapture();
            EditedMeshView v = EditedView();
            for (size_t i = 0; i < v.pixels.size(); ++i)
                if (v.vertexMap[i] == weldFrom_) v.pixels[i] = {-1e9, -1e9};    // not its own target
            const double tolerance = vertexEditing_.mod->Params().Int(scene::EditMeshModifier::kWeldTargetPixels, 4);
            const int hit = view::PickNearestPoint(v.pixels, GET_X_LPARAM(lp) + 0.5, GET_Y_LPARAM(lp) + 0.5, tolerance);
            if (hit >= 0 && onWeldTarget) onWeldTarget(weldFrom_, v.vertexMap[static_cast<size_t>(hit)]);
            return 0;
        }
        if (moving_) {
            moving_ = false;
            ReleaseCapture();
            if (onMoveEnd) onMoveEnd(true);
            return 0;
        }
        if (selecting_) {
            selecting_ = false;
            const bool region = regionVisible_;
            regionVisible_ = false;
            ReleaseCapture();
            const bool ctrl = (wp & MK_CONTROL) != 0, alt = GetKeyState(VK_MENU) < 0;
            if (vertexEditing_.on && onVertexSelect) {
                const EditedMeshView v = EditedView();
                const std::vector<uint32_t> verts = region       // input indices at the current level
                    ? SubObjectsIn(v, dragStart_.x + 0.5, dragStart_.y + 0.5, dragLast_.x + 0.5, dragLast_.y + 0.5)
                    : SubObjectsAt(v, GET_X_LPARAM(lp) + 0.5, GET_Y_LPARAM(lp) + 0.5, nullptr);
                InvalidateRect(Hwnd(), nullptr, FALSE);
                onVertexSelect(verts, region, ctrl, alt);
                return 0;
            }
            std::vector<scene::Node::Id> ids;
            if (region) {
                ids = PickRect(dragStart_.x, dragStart_.y, dragLast_.x, dragLast_.y);
            } else if (const scene::Node::Id id = PickAt(GET_X_LPARAM(lp), GET_Y_LPARAM(lp))) {
                ids.push_back(id);
            }
            InvalidateRect(Hwnd(), nullptr, FALSE);
            if (onSelect) onSelect(ids, region, ctrl, alt);
            return 0;
        }
        if (creatingDrag_) {
            creatingDrag_ = false;
            ReleaseCapture();
            CreateEvent(scene::Creator::Event::Release, lp);
            return 0;
        }
        [[fallthrough]];
    case WM_MBUTTONUP:
        if (drag_ != NavMode::None) {
            EndDrag(true);
            ReleaseCapture();
        }
        return 0;
    case WM_CAPTURECHANGED:
        if (moving_) { moving_ = false; if (onMoveEnd) onMoveEnd(true); }
        creatingDrag_ = false;
        if (selecting_) { selecting_ = false; regionVisible_ = false; InvalidateRect(Hwnd(), nullptr, FALSE); }
        if (drag_ != NavMode::None) EndDrag(false);
        return 0;
    case WM_KEYDOWN:
        if (wp == VK_DELETE && onDeleteKey) { onDeleteKey(); return 0; }
        break;
    case WM_MOUSELEAVE:
        trackingLeave_ = false;
        if (hoverMask_ && !moving_) { hoverMask_ = 0; InvalidateRect(Hwnd(), nullptr, FALSE); }
        return 0;
    case WM_MOUSEWHEEL:
        // One notch = 120 units of wheel delta.
        Zoom(view::ZoomFactorForWheel(GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA));
        return 0;
    case WM_SETCURSOR:
        if (LOWORD(lp) == HTCLIENT) {
            const NavMode mode = drag_ != NavMode::None ? drag_ : (navMode ? navMode() : NavMode::None);
            if (mode != NavMode::None) { SetCursor(CursorFor(mode)); return TRUE; }
            // Creating: a cross-hair (NOTE: R3's creation cursor is unverified).
            if (creationActive && creationActive()) { SetCursor(LoadCursorW(nullptr, IDC_CROSS)); return TRUE; }
            // Over the gizmo or an object (12.2): the tool's cursor (NOTE: R3's cursors drawn
            // from memory; Move is the four-way arrow).
            if (scene_ && onSelect) {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(Hwnd(), &pt);
                if (moving_ || hoverMask_ || PickAt(pt.x, pt.y) != 0) { SetCursor(cursors::ForTool(toolKind_)); return TRUE; }
            }
        }
        break;
    case WM_RBUTTONDOWN:
        if (onActivate) onActivate();
        return 0;
    case WM_RBUTTONUP:
        // While moving, right-click cancels the move.
        if (moving_) {
            moving_ = false;
            if (onMoveEnd) onMoveEnd(false);
            ReleaseCapture();
            return 0;
        }
        // While building an object, right-click cancels it.
        if (onCreateCancel && onCreateCancel()) return 0;
        // Otherwise, in creation mode, right-click ends it (12.2; as in R3).
        if (creationActive && creationActive() && onCreateExit) { onCreateExit(); return 0; }
        // In a navigation mode, right-click leaves the mode (as in R3).
        if (navMode && navMode() != NavMode::None) {
            if (onExitMode) onExitMode();
            return 0;
        }
        // The label menu opens on release, as context menus do on Windows.
        if (InLabel(GET_X_LPARAM(lp), GET_Y_LPARAM(lp))) {
            POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            ClientToScreen(Hwnd(), &pt);
            ShowLabelMenu(pt);
        }
        return 0;
    }
    return win::Window::HandleMessage(msg, wp, lp);
}

} // namespace ui
