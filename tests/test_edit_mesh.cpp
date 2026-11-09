// Edit Mesh's Face level and Delete (Phase 9.4): deletions by input index with maps back,
// undo of the whole state, and face picking.
#include "scene/MeshTopology.h"
#include "scene/Modifier.h"
#include "scene/Primitives.h"
#include "scene/Undo.h"
#include "view/Picking.h"
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

int main() {
    scene::BoxObject boxObject(20, 20, 20);
    const scene::Mesh box = boxObject.Evaluate();            // 8 vertices, 12 faces: bottom 0-1, top 2-3, ...
    CHECK(box.verts.size() == 8 && box.FaceCount() == 12);
    // Deleting the top's two faces: 10 faces left, the maps skip 2 and 3; vertices stay.
    {
        scene::EditMeshModifier em(8, {});
        em.SetSelectedFaces({3, 2});
        em.SetState(em.AfterDeletingFaces(box, false));
        std::vector<uint32_t> vmap, fmap;
        const scene::Mesh out = em.Edited(box, &vmap, &fmap);
        CHECK(out.FaceCount() == 10 && out.verts.size() == 8 && fmap.size() == 10);
        CHECK(fmap.size() == 10 && fmap[0] == 0 && fmap[1] == 1 && fmap[2] == 4);
        CHECK(em.SelectedFaces().empty() && em.DeletedFaces() == (std::vector<uint32_t>{2, 3}));
        scene::Mesh applied = box;
        CHECK(em.Apply(applied) && applied.FaceCount() == 10);
    }
    // Deleting the top's faces with isolated vertices: none are isolated on a box (the
    // top's corners still belong to the sides) ...
    {
        scene::EditMeshModifier em(8, {});
        em.SetSelectedFaces({2, 3});
        CHECK(em.AfterDeletingFaces(box, true).deletedVertices.empty());
        // ... but after deleting every face touching vertex 7, it is.
        std::vector<uint32_t> touching;
        for (uint32_t f = 0; f < box.faces.size(); ++f)
            for (uint32_t v : box.faces[f].v) if (v == 7) touching.push_back(f);
        em.SetSelectedFaces(touching);
        const auto s = em.AfterDeletingFaces(box, true);
        CHECK(s.deletedVertices == std::vector<uint32_t>{7});
    }
    // Deleting a vertex takes its faces; the output is renumbered, the maps lead back, and
    // a move recorded by input index still lands on the right vertex.
    {
        scene::EditMeshModifier em(8, {{6, {0, 0, 5}}});
        em.SetSelectedVertices({0});
        em.SetState(em.AfterDeletingVertices());
        std::vector<uint32_t> vmap, fmap;
        const scene::Mesh out = em.Edited(box, &vmap, &fmap);
        CHECK(out.verts.size() == 7 && vmap[0] == 1 && vmap[6] == 7);
        size_t touching0 = 0;
        for (const auto& f : box.faces) touching0 += (f.v[0] == 0 || f.v[1] == 0 || f.v[2] == 0);
        CHECK(out.FaceCount() == 12 - touching0);
        CHECK(out.verts[5].z == box.verts[6].z + 5);           // input vertex 6 is output 5
        for (const auto& f : out.faces) for (uint32_t v : f.v) CHECK(v < out.verts.size());
        CHECK(em.SelectedVertices().empty());
    }
    // One undo step for any change: select, then delete, both undone exactly.
    {
        scene::Scene s;
        scene::UndoStack undo;
        auto em = std::make_shared<scene::EditMeshModifier>(8, std::vector<scene::EditMeshModifier::Move>{});
        auto st = em->GetState();
        st.selectedFaces = {2, 3};
        undo.Execute(std::make_unique<scene::EditMeshStateCommand>("Select", em, st), s);
        undo.Execute(std::make_unique<scene::EditMeshStateCommand>("Delete", em, em->AfterDeletingFaces(box, false)), s);
        CHECK(em->DeletedFaces().size() == 2 && em->SelectedFaces().empty() && undo.UndoName() == "Delete");
        CHECK(undo.Undo(s) && em->DeletedFaces().empty() && em->SelectedFaces() == (std::vector<uint32_t>{2, 3}));
        CHECK(undo.Undo(s) && em->SelectedFaces().empty());
        CHECK(undo.Redo(s) && undo.Redo(s) && em->DeletedFaces().size() == 2);
    }
    // Picking a face: two overlapping triangles, the nearer one wins; skipped faces are
    // ignored; a rectangle takes faces with a vertex inside.
    {
        const std::vector<view::Point2> p = {{0, 0}, {10, 0}, {0, 10}, {0, 0}, {10, 0}, {0, 10}, {20, 20}};
        const std::vector<double> depth = {5, 5, 5, 2, 2, 2, 1};
        const std::vector<view::Tri> faces = {{0, 1, 2}, {3, 4, 5}};
        CHECK(view::PickFace(p, depth, faces, nullptr, 2, 2) == 1);
        const std::vector<bool> skip = {false, true};
        CHECK(view::PickFace(p, depth, faces, &skip, 2, 2) == 0);
        CHECK(view::PickFace(p, depth, faces, nullptr, 9, 9) == -1);
        CHECK(view::PickFacesInRect(p, faces, nullptr, -1, -1, 1, 1).size() == 2);
        CHECK(view::PickFacesInRect(p, faces, &skip, 9, -1, 11, 1) == std::vector<int>{0});
    }
    // Polygons, elements and edges (9.5): a box side is a polygon of its two triangles,
    // the box one element; two boxes in one mesh are two elements; an edge has two faces.
    {
        for (uint32_t f = 0; f < 12; ++f) {
            const auto p = scene::PolygonOf(box, f);
            CHECK(p.size() == 2 && (p[0] / 2 == f / 2) && (p[1] / 2 == f / 2));
            CHECK(scene::ElementOf(box, f).size() == 12);
        }
        CHECK(scene::ExpandToPolygons(box, {0, 5}) == (std::vector<uint32_t>{0, 1, 4, 5}));
        scene::Mesh two = box;
        const uint32_t n = static_cast<uint32_t>(two.verts.size());
        for (const auto& v : box.verts) two.verts.push_back({v.x + 50, v.y, v.z});
        for (auto f : box.faces) { for (auto& v : f.v) v += n; two.faces.push_back(f); }
        CHECK(scene::ElementOf(two, 0).size() == 12 && scene::ElementOf(two, 12).size() == 12 && scene::ElementOf(two, 12)[0] == 12);
        CHECK(scene::ExpandToElements(two, {3}).size() == 12);
        CHECK(scene::FacesOnEdge(box, scene::EdgeId(0, 0)).size() == 2);
        // Deleting an edge deletes the faces on both sides.
        scene::EditMeshModifier em(8, {});
        em.SetSelectedEdges({scene::EdgeId(2, 0)});
        const auto s = em.AfterDeletingEdges(box, false);
        CHECK(s.deletedFaces.size() == 2 && s.selectedEdges.empty());
        uint32_t a, b;
        CHECK(scene::EdgeVertices(box, scene::EdgeId(2, 0), a, b));
        for (uint32_t f : s.deletedFaces) {
            int has = 0;
            for (uint32_t v : box.faces[f].v) has += (v == a || v == b);
            CHECK(has == 2);
        }
    }
    // Hide / Unhide All (9.6): hidden faces leave the output and come back; hidden vertices
    // keep the mesh as it is.
    {
        scene::EditMeshModifier em(8, {});
        em.SetSelectedFaces({2, 3});
        em.SetState(em.AfterHiding(true));
        CHECK(em.Edited(box).FaceCount() == 10 && em.SelectedFaces().empty() && em.HiddenFaces().size() == 2);
        em.SetState(em.AfterUnhidingAll(true));
        CHECK(em.Edited(box).FaceCount() == 12 && em.HiddenFaces().empty());
        em.SetSelectedVertices({0, 1});
        em.SetState(em.AfterHiding(false));
        CHECK(em.HiddenVertices() == (std::vector<uint32_t>{0, 1}) && em.Edited(box).verts.size() == 8 && em.SelectedVertices().empty());
    }
    // Weld Selected (9.6): vertex 1 moved onto vertex 0, both selected; within 0.1 they
    // merge, faces using both collapse and go, the rest stay valid; out of reach nothing.
    {
        const math::Vec3 gap = box.verts[0] - box.verts[1];
        scene::EditMeshModifier em(8, {{1, gap + math::Vec3{0.05, 0, 0}}});
        em.SetSelectedVertices({0, 1});
        size_t merged = 0;
        const auto none = em.AfterWelding(box, 0.01, &merged);
        CHECK(merged == 0 && none.welds.empty());
        const auto s = em.AfterWelding(box, 0.1, &merged);
        CHECK(merged == 1 && s.welds.size() == 1 && s.welds[0] == std::make_pair(1u, 0u) && s.selectedVertices == std::vector<uint32_t>{0});
        size_t both = 0;
        for (const auto& f : box.faces) {
            int has = 0;
            for (uint32_t v : f.v) has += (v == 0 || v == 1);
            both += has == 2;
        }
        scene::Scene sc;
        scene::UndoStack undo;
        auto shared = std::make_shared<scene::EditMeshModifier>(em);
        undo.Execute(std::make_unique<scene::EditMeshStateCommand>("Weld", shared, s), sc);
        const scene::Mesh out = shared->Edited(box);
        CHECK(out.verts.size() == 7 && out.FaceCount() == 12 - both && both == 2);
        for (const auto& f : out.faces) {
            CHECK(f.v[0] != f.v[1] && f.v[1] != f.v[2] && f.v[2] != f.v[0]);
            for (uint32_t v : f.v) CHECK(v < out.verts.size());
        }
        // The merged vertex sits at the average of the two (vertex 0 and its near neighbour).
        CHECK(math::Length(out.verts[0] - (box.verts[0] + math::Vec3{0.025, 0, 0})) < 1e-9);
        CHECK(undo.Undo(sc) && shared->Welds().empty() && shared->Edited(box).verts.size() == 8);
    }
    // Picking edges: the nearest segment within the tolerance; rectangles take segments
    // with an end inside.
    {
        const std::vector<view::Point2> p = {{0, 0}, {100, 0}, {0, 50}, {100, 50}};
        const std::vector<view::Seg> segs = {{0, 1}, {2, 3}, {0, 2}};
        CHECK(view::PickNearestSegment(p, segs, 50, 2, 4) == 0);
        CHECK(view::PickNearestSegment(p, segs, 50, 48, 4) == 1);
        CHECK(view::PickNearestSegment(p, segs, 50, 25, 4) == -1);
        CHECK(view::PickNearestSegment(p, segs, 1, 25, 4) == 2);
        CHECK(view::PickSegmentsInRect(p, segs, 90, -5, 110, 5) == std::vector<int>{0});
    }
    if (failures == 0) std::printf("edit mesh: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
