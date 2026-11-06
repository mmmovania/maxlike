#include <glad/gl.h>
#include <windowsx.h>
#include <algorithm>
#include <cmath>
#include "ui/Viewport.h"
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
        lines_->Draw(batch[k], toClip, kGridColours[k].R(), kGridColours[k].G(), kGridColours[k].B());
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

        std::vector<Line> lines;
        for (const auto& [a, b] : mesh.VisibleEdges()) {
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
        // Selected objects are white (measured, R3.1 screenshots).
        const uint32_t c = scene_->IsSelected(node->GetId()) ? 0xFFFFFF : node->WireColor();
        lines_->Draw(lines, toClip, ((c >> 16) & 0xFF) / 255.0f, ((c >> 8) & 0xFF) / 255.0f, (c & 0xFF) / 255.0f);
    }
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
    for (int i : view::PickRegion(outlines, x0 + 0.5, y0 + 0.5, x1 + 0.5, y1 + 0.5)) out.push_back(ids[static_cast<size_t>(i)]);
    return out;
}

// Selected objects in shaded views: white brackets at the corners of the object's
// bounding box, each a quarter of the box's edge (measured roughly from R3.1's Box
// screenshot). Drawn over the shaded objects.
void Viewport::DrawBrackets(int /*w*/, int /*h*/, const math::Mat4& worldToClip) {
    using Line = gl::LineRenderer::Line;
    std::vector<Line> lines;
    for (const auto& node : scene_->Nodes()) {
        if (!node->GetObject() || !scene_->IsSelected(node->GetId())) continue;
        const scene::Bounds b = node->GetObject()->Evaluate().GetBounds();
        if (b.Empty()) continue;
        const math::Mat4 m = node->ObjectMatrix();
        const double lo[3] = {b.min.x, b.min.y, b.min.z}, hi[3] = {b.max.x, b.max.y, b.max.z};
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
            for (int k = 0; k < 3; ++k) {
                const math::Vec3 p = math::TransformPoint(m, mesh.verts[mesh.faces[i].v[k]]);
                const math::Vec3 n = normalToWorld(facets ? fn : corner[i * 3 + static_cast<size_t>(k)]);
                v.insert(v.end(), {static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z),
                                   static_cast<float>(n.x), static_cast<float>(n.y), static_cast<float>(n.z)});
            }
        }
        const uint32_t c = node->WireColor();
        meshes_->Draw(v, toClip, ((c >> 16) & 0xFF) / 255.0f, ((c >> 8) & 0xFF) / 255.0f, (c & 0xFF) / 255.0f, light);
    }
    glDisable(GL_DEPTH_TEST);
    DrawBrackets(w, h, toClip);
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
        if (selecting_) {
            dragLast_ = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            const bool moved = std::abs(dragLast_.x - dragStart_.x) > 2 || std::abs(dragLast_.y - dragStart_.y) > 2;
            if (moved || regionVisible_) { regionVisible_ = true; InvalidateRect(Hwnd(), nullptr, FALSE); }
            return 0;
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
        if (selecting_) {
            selecting_ = false;
            const bool region = regionVisible_;
            regionVisible_ = false;
            ReleaseCapture();
            const bool ctrl = (wp & MK_CONTROL) != 0, alt = GetKeyState(VK_MENU) < 0;
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
        creatingDrag_ = false;
        if (selecting_) { selecting_ = false; regionVisible_ = false; InvalidateRect(Hwnd(), nullptr, FALSE); }
        if (drag_ != NavMode::None) EndDrag(false);
        return 0;
    case WM_KEYDOWN:
        if (wp == VK_DELETE && onDeleteKey) { onDeleteKey(); return 0; }
        break;
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
        }
        break;
    case WM_RBUTTONDOWN:
        if (onActivate) onActivate();
        return 0;
    case WM_RBUTTONUP:
        // While building an object, right-click cancels it.
        if (onCreateCancel && onCreateCancel()) return 0;
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
