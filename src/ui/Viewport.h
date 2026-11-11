#pragma once
#include "win/Window.h"
#include <functional>
#include <string>
#include "view/OrthoGrid.h"
#include "view/PerspectiveView.h"
#include "view/Navigation.h"
#include "scene/Creation.h"
#include "scene/Node.h"
#include "view/Picking.h"
#include "view/TransformAxes.h"
#include "scene/Modifier.h"
#include "scene/MoveMath.h"
#include "ui/LabelText.h"

namespace gl { class GLContext; class LineRenderer; class MeshRenderer; }
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
    // Creation (Phase 5): whether a primitive button is checked, and where mouse events
    // go, already converted to this view's construction plane. Return true to redraw.
    std::function<bool()> creationActive;
    std::function<bool(scene::Creator::Event, int owner, const scene::ConstructionPlane&,
                       double u, double v, int screenY, double unitsPerPixel)> onCreateEvent;
    std::function<bool()> onCreateCancel;        // right-click while building
    // Selection (Phase 6): a click (one id or none) or a region (any number), with the
    // Ctrl / Alt keys; and the Delete key.
    std::function<void(const std::vector<scene::Node::Id>& ids, bool region, bool ctrl, bool alt)> onSelect;
    std::function<void()> onDeleteKey;
    // Moving (Select and Move): begin, a world-space movement since the start (preview),
    // end (true: keep, as one undo step; false: cancelled).
    std::function<void()> onMoveBegin;
    // delta: world movement (Move); upPixels: vertical mouse movement (Rotate, Scale).
    std::function<void(const math::Vec3& delta, int upPixels)> onMoveDrag;
    std::function<void(bool commit)> onMoveEnd;
    // The world point under the mouse (view plane in ortho views, the ground in
    // perspective); ok = false when there is none (above the horizon).
    std::function<void(bool ok, const math::Vec3& world)> onCursorWorld;
    void SetIndex(int index) { index_ = index; }

    // `lines` and `meshes` are the panel's shared renderers (created on first paint).
    bool Create(HWND parent, gl::GLContext& context, gl::LineRenderer& lines,
                gl::MeshRenderer& meshes, ViewType type);
    ViewType Type() const { return type_; }
    const wchar_t* Name() const;
    void SetType(ViewType type);
    const view::OrthoView& OrthoState() const { return ortho_; }
    const view::PerspectiveView& PerspectiveState() const { return perspective_; }
    // A view restored from a file (Phase 8): what the ortho and perspective cameras show.
    void SetViewState(const view::OrthoView& ortho, const view::PerspectiveView& perspective) {
        ortho_ = ortho;
        perspective_ = perspective;
        if (Hwnd()) InvalidateRect(Hwnd(), nullptr, FALSE);
    }
    bool ShowGrid() const { return showGrid_; }
    void SetShowGrid(bool show);

    // Navigation (pure maths in view/Navigation.h).
    void Pan(int dx, int dy);
    void Zoom(double factor);
    void Orbit(int dx, int dy);
    void ZoomRegion(int x0, int y0, int x1, int y1);
    void ResetView();   // Zoom Extents with an empty scene: back to the default framing

    // The mouse at window pixel (x, y) on this view's construction plane. False if it
    // misses the plane (perspective view, above the horizon).
    bool PointOnPlane(int x, int y, scene::ConstructionPlane& plane, double& u, double& v,
                      double& unitsPerPixel) const;
    // This view's construction plane (ortho: its right and up; perspective: the ground).
    scene::ConstructionPlane CreationPlane() const;

    // What to draw at the selection's pivot (set by the panel from the toolbar's tool and
    // constraint buttons).
    // Edit Mesh at Vertex level (Phase 7): the node, its stack evaluated up to the Edit
    // Mesh, and the Edit Mesh holding the vertex selection.
    struct VertexEditing {
        bool on = false;
        scene::Node::Id node = 0;
        std::shared_ptr<scene::DerivedObject> stack;
        size_t upTo = 0;
        std::shared_ptr<scene::EditMeshModifier> mod;
        int level = 0;                            // 0 Vertex, 1 Edge, 2 Face, 3 Polygon, 4 Element
        bool weldTarget = false;                  // Weld > Target (9.7): drag a vertex onto another
    };
    void SetVertexEditing(const VertexEditing& v) { vertexEditing_ = v; InvalidateRect(Hwnd(), nullptr, FALSE); }
    std::function<void(const std::vector<uint32_t>& vertices, bool region, bool ctrl, bool alt)> onVertexSelect;
    std::function<void(uint32_t from, uint32_t to)> onWeldTarget;   // input vertex indices

    // A transform tool (Move, Rotate, Scale) is active: pressing on an object drags it.
    // Phase 9.9: the reference coordinate system and the transform centre (0 each pivot,
    // 1 the selection's centre, 2 the system's origin).
    // A deformer's gizmo (Phase 10.3): drawn while the modifier is shown in the Modify
    // panel; at its sub-object levels (0 Gizmo, 1 Center) a transform tool drags it.
    struct GizmoEditing {
        bool on = false;
        scene::Node::Id node = 0;
        std::shared_ptr<scene::DerivedObject> stack;
        size_t index = 0;                          // the modifier's place in the stack
        std::shared_ptr<scene::DeformModifier> mod;
        int level = -1;                            // -1 not at sub-object level
    };
    void SetGizmoEditing(const GizmoEditing& g) { gizmoEditing_ = g; if (Hwnd()) InvalidateRect(Hwnd(), nullptr, FALSE); }
    void SetReferenceSystem(scene::move::RefCoord system, int centre) {
        refCoord_ = system;
        centre_ = centre;
        if (Hwnd()) InvalidateRect(Hwnd(), nullptr, FALSE);
    }
    scene::move::Frame CurrentFrame() const;
    // The direction out of the screen, in world space (towards the viewer).
    math::Vec3 OutOfScreen() const { math::Vec3 r, u; OrthoBasis(r, u); return math::Normalize(math::Cross(r, u)); }
    math::Vec3 TransformCenter() const;
    void SetTransformTool(bool on) { transformTool_ = on; }
    void SetAxesDisplay(view::AxesStyle style, const bool active[3]) {
        axesStyle_ = style;
        for (int i = 0; i < 3; ++i) axesActive_[i] = active[i];
        InvalidateRect(Hwnd(), nullptr, FALSE);
    }

    // Picking (window pixels): the object under the point, 0 if none; the objects a
    // rectangle touches.
    scene::Node::Id PickAt(int x, int y) const;
    std::vector<scene::Node::Id> PickRect(int x0, int y0, int x1, int y1) const;

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
    void DrawShaded(int width, int height, bool facets, bool highlights);
    // The view's right and up directions in world space (ortho views).
    void OrthoBasis(math::Vec3& right, math::Vec3& up) const;
    void DrawRegionRectangle(int width, int height);
    void BeginDrag(NavMode op, int x, int y);
    void EndDrag(bool apply);
    HCURSOR CursorFor(NavMode mode) const;
    void ScreenAxes(double out[3][2]) const; // how X, Y, Z appear in this view

    gl::GLContext* context_ = nullptr;
    gl::LineRenderer* lines_ = nullptr;
    gl::MeshRenderer* meshes_ = nullptr;
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
    bool creatingDrag_ = false;          // left button held during creation
    bool selecting_ = false;             // left button held for a selection click or region
    bool moving_ = false;                // dragging the selection with a transform tool
    bool transformTool_ = false;
    VertexEditing vertexEditing_;
    GizmoEditing gizmoEditing_;
    scene::move::RefCoord refCoord_ = scene::move::RefCoord::View;
    int centre_ = 0;
    bool weldDragging_ = false;
    uint32_t weldFrom_ = 0;
    // Edit Mesh's output as the views see it, with the maps back to its input indices
    // (what Edit Mesh stores), screen positions, depths, and the faces turned away.
    struct EditedMeshView {
        scene::Mesh mesh;
        std::vector<uint32_t> vertexMap, faceMap;
        std::vector<view::Point2> pixels;
        std::vector<double> depth;
        std::vector<math::Vec3> world;
        std::vector<view::Tri> tris;
        std::vector<bool> backFacing;
        std::vector<view::Seg> segs;              // visible edges of the output faces
        std::vector<uint32_t> segEdge, segFace;   // each one's input edge id, output face
    };
    EditedMeshView EditedView() const;
    // Picking at the current level, as Edit Mesh's input indices (vertices, edges or faces;
    // polygons and elements expanded to their faces). `pivot`: where a drag starts.
    std::vector<uint32_t> SubObjectsAt(const EditedMeshView& v, double x, double y, math::Vec3* pivot) const;
    std::vector<uint32_t> SubObjectsIn(const EditedMeshView& v, double x0, double y0, double x1, double y1) const;
    const std::vector<uint32_t>& SubObjectSelection() const;
    std::vector<view::Point2> VertexPixels(std::vector<math::Vec3>* world = nullptr) const;
    void DrawVertices(int width, int height);
    POINT moveStart_{};
    math::Vec3 movePivot_{};
    math::Vec3 SelectionPivot() const;
    bool MouseRay(int x, int y, math::Vec3& origin, math::Vec3& dir) const;
    math::Vec3 MoveDelta(int x, int y) const;   // world movement from moveStart_ to (x, y)
    // World point -> window pixels (and depth, smaller = nearer) in this view; false if
    // behind the camera.
    bool WorldToWindow(const math::Vec3& p, double& x, double& y, double& depth) const;
    void CollectPickData(std::vector<scene::Node::Id>& ids, std::vector<std::vector<view::Segment2>>& outlines,
                         std::vector<std::vector<view::Triangle2>>* surfaces) const;
    void DrawBrackets(int width, int height, const math::Mat4& worldToClip);
    void DrawOpenGroups(int width, int height);     // Phase 10: open groups' outlines
    void DrawGizmo(int width, int height);          // Phase 10.3: a deformer's gizmo and centre
    void DrawTransformAxes(int width, int height);
    view::AxesStyle axesStyle_ = view::AxesStyle::Tripod;
    bool axesActive_[3] = {true, true, false};   // R3's default constraint: XY
    int index_ = 0;                      // which viewport this is (creation owner)
    bool CreateEvent(scene::Creator::Event e, LPARAM lp);
    UINT labelDpi_ = 0;                  // DPI the label was rendered for
};

} // namespace ui
