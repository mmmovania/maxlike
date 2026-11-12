// Edit Mesh's Face level and Delete (Phase 9.4): deletions by input index with maps back,
// undo of the whole state, and face picking.
#include "scene/MeshTopology.h"
#include "scene/Modifier.h"
#include "scene/Primitives.h"
#include "scene/Undo.h"
#include "view/Picking.h"
#include <cmath>
#include <cstdio>
#include <set>
#include <map>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

// A closed, consistently wound mesh: every directed edge a -> b exactly once, its reverse
// present; and its Euler characteristic V - E + F.
static bool ClosedConsistent(const scene::Mesh& m) {
    std::map<std::pair<uint32_t, uint32_t>, int> directed;
    for (const auto& f : m.faces) for (int k = 0; k < 3; ++k) ++directed[{f.v[k], f.v[(k + 1) % 3]}];
    for (const auto& [e, n] : directed) if (n != 1 || !directed.count({e.second, e.first})) return false;
    return true;
}
static long Euler(const scene::Mesh& m) {
    std::set<std::pair<uint32_t, uint32_t>> edges;
    std::set<uint32_t> used;
    for (const auto& f : m.faces) for (int k = 0; k < 3; ++k) {
        const uint32_t a = f.v[k], b = f.v[(k + 1) % 3];
        edges.insert(a < b ? std::make_pair(a, b) : std::make_pair(b, a));
        used.insert(a);
    }
    return static_cast<long>(used.size()) - static_cast<long>(edges.size()) + static_cast<long>(m.faces.size());
}

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
    // Weld Target (9.7): vertex 1 welded into vertex 0, which stays put; twice is once.
    {
        scene::EditMeshModifier em(8, {});
        em.SetSelectedVertices({1});
        const auto s = em.AfterWeldingTarget(1, 0);
        CHECK(s.welds.size() == 1 && s.welds[0] == std::make_pair(1u, 0u) && s.selectedVertices.empty());
        em.SetState(s);
        const scene::Mesh out = em.Edited(box);
        CHECK(out.verts.size() == 7 && out.verts[0].x == box.verts[0].x && out.verts[0].y == box.verts[0].y);
        CHECK(em.AfterWeldingTarget(1, 0).welds.size() == 1 && em.AfterWeldingTarget(2, 2).welds.size() == 1);
    }
    // Detach (9.7): the top's two faces as a compact mesh of 4 vertices, in place.
    {
        scene::EditMeshModifier em(8, {});
        const scene::Mesh part = em.Extract(box, {2, 3});
        CHECK(part.FaceCount() == 2 && part.verts.size() == 4);
        for (const auto& v : part.verts) CHECK(v.z == box.verts[4].z);         // the top's height
        for (const auto& f : part.faces) for (uint32_t v : f.v) CHECK(v < 4);
        CHECK(part.faces[0].smoothing == box.faces[2].smoothing && part.faces[0].materialId == box.faces[2].materialId);
        // Faces already deleted cannot be detached.
        em.SetSelectedFaces({2});
        em.SetState(em.AfterDeletingFaces(box, false));
        CHECK(em.Extract(box, {2, 3}).FaceCount() == 1);
    }
    // Per-vertex offsets (10.5): each vertex gets its own, merged with existing moves.
    {
        const std::vector<scene::EditMeshModifier::Move> before = {{2, {1, 0, 0}}};
        const auto after = scene::EditMeshModifier::AddToMoves(before, {2, 5}, std::vector<math::Vec3>{{0, 1, 0}, {0, 0, 3}});
        scene::EditMeshModifier em(8, after);
        const scene::Mesh out = em.Edited(box);
        CHECK(out.verts[2].x == box.verts[2].x + 1 && out.verts[2].y == box.verts[2].y + 1 && out.verts[5].z == box.verts[5].z + 3);
        CHECK(out.verts[3].x == box.verts[3].x);
    }
    // Added geometry and the simple Edit Geometry tools (10.7). The box: bottom z 0, top z 20.
    {
        const std::vector<uint32_t> top = {4, 5, 6, 7};
        for (uint32_t v : top) CHECK(box.verts[v].z == 20);
        // An added vertex and face (vertex 8, face 12) behave like any other.
        scene::EditMeshModifier em(8, {});
        auto st = em.GetState();
        st.addedVertices = {{0, 0, 50}};
        scene::Face f{};
        f.v[0] = 4; f.v[1] = 5; f.v[2] = 8; f.edgeVisible = 7;
        st.addedFaces = {f};
        st.moves = {{8, {0, 0, 5}}};
        em.SetState(st);
        scene::Mesh out = em.Edited(box);
        CHECK(out.verts.size() == 9 && out.FaceCount() == 13 && out.verts[8].z == 55);
        em.SetSelectedFaces({12});
        em.SetState(em.AfterDeletingFaces(box, true));
        out = em.Edited(box);
        CHECK(out.FaceCount() == 12 && out.verts.size() == 8);           // its own vertex went with it (isolated)
        // Remove Isolated Vertices: an unused added vertex goes, the box's corners stay.
        scene::EditMeshModifier iso(8, {});
        auto si = iso.GetState();
        si.addedVertices = {{100, 0, 0}};
        iso.SetState(si);
        iso.SetState(iso.AfterRemovingIsolated(box));
        CHECK(iso.Edited(box).verts.size() == 8 && iso.DeletedVertices() == std::vector<uint32_t>{8});
        // Select Open Edges: with the top's two faces deleted, exactly its 4 rim edges.
        scene::EditMeshModifier open(8, {});
        open.SetSelectedFaces({2, 3});
        open.SetState(open.AfterDeletingFaces(box, false));
        open.SetState(open.AfterSelectingOpenEdges(box));
        CHECK(open.SelectedEdges().size() == 4);
        for (uint32_t e : open.SelectedEdges()) {
            uint32_t a, b;
            CHECK(scene::EdgeVertices(box, e, a, b) && box.verts[a].z == 20 && box.verts[b].z == 20);
        }
        // Collapse the top's corners: one vertex at their centre.
        scene::EditMeshModifier col(8, {});
        col.SetState(col.AfterCollapsing(box, top));
        out = col.Edited(box);
        CHECK(out.verts.size() == 5);
        bool centred = false;
        for (const auto& v : out.verts) centred = centred || (std::fabs(v.x) < 1e-9 && std::fabs(v.y) < 1e-9 && v.z == 20);
        CHECK(centred);
        // Make Planar: a corner lifted by 6 comes back into the top's plane (normal +Z,
        // through the centroid at z 21.5): every top corner ends at the same height.
        scene::EditMeshModifier pl(8, {{4, {0, 0, 6}}});
        pl.SetState(pl.AfterMakingPlanar(box, top));
        out = pl.Edited(box);
        for (uint32_t v : top) CHECK(std::fabs(out.verts[v].z - 21.5) < 1e-9);
        // View Align (normal Z through the centroid) and Grid Align (the plane z = 0).
        scene::EditMeshModifier va(8, {{4, {0, 0, 6}}});
        va.SetState(va.AfterAligning(box, top, {0, 0, 1}, nullptr));
        out = va.Edited(box);
        for (uint32_t v : top) CHECK(std::fabs(out.verts[v].z - 21.5) < 1e-9);
        const math::Vec3 origin{0, 0, 0};
        scene::EditMeshModifier ga(8, {});
        ga.SetState(ga.AfterAligning(box, top, {0, 0, 1}, &origin));
        out = ga.Edited(box);
        for (uint32_t v : top) CHECK(std::fabs(out.verts[v].z) < 1e-9);
    }
    // Topology tools (10.8), checked by structure: closed, consistently wound, Euler 2.
    {
        CHECK(ClosedConsistent(box) && Euler(box) == 2);
        // Turn the top's hidden diagonal (shared by faces 2 and 3).
        uint32_t diag = UINT32_MAX;
        for (int k = 0; k < 3; ++k)
            if (!(box.faces[2].edgeVisible & (1u << k))) diag = scene::EdgeId(2, k);
        CHECK(diag != UINT32_MAX);
        scene::EditMeshModifier tu(8, {});
        tu.SetSelectedEdges({diag});
        tu.SetState(tu.AfterTurning(box));
        scene::Mesh out = tu.Edited(box);
        CHECK(out.FaceCount() == 12 && ClosedConsistent(out) && Euler(out) == 2);
        uint32_t a0, b0;
        scene::EdgeVertices(box, diag, a0, b0);
        bool oldDiagonal = false;
        for (const auto& f : out.faces) {
            int has = 0;
            for (uint32_t v : f.v) has += (v == a0 || v == b0);
            oldDiagonal = oldDiagonal || has == 2;
            if (has == 2) for (uint32_t v : f.v) if (v != a0 && v != b0) oldDiagonal = oldDiagonal && box.verts[v].z != 20;
        }
        CHECK(!oldDiagonal);                                  // no top face holds the old diagonal any more
        // Extrude the top by 10: closed, Euler 2, the top now at 30.
        scene::EditMeshModifier ex(8, {});
        ex.SetSelectedFaces({2, 3});
        ex.SetState(ex.AfterExtruding(box, 10));
        out = ex.Edited(box);
        CHECK(out.verts.size() == 12 && out.FaceCount() == 20 && ClosedConsistent(out) && Euler(out) == 2);
        double topZ = 0;
        for (const auto& v : out.verts) topZ = std::max(topZ, v.z);
        CHECK(std::fabs(topZ - 30) < 1e-9 && ex.SelectedFaces().size() == 2);
        // Chamfer a top corner: still closed and consistent, Euler 2.
        scene::EditMeshModifier ch(8, {});
        ch.SetSelectedVertices({4});
        ch.SetState(ch.AfterChamfering(box, 3));
        out = ch.Edited(box);
        CHECK(out.verts.size() > 8 && ClosedConsistent(out) && Euler(out) == 2);
        // Chamfering two corners sharing faces: still closed (each sees the other's faces).
        scene::EditMeshModifier ch2(8, {});
        ch2.SetSelectedVertices({4, 5});
        ch2.SetState(ch2.AfterChamfering(box, 3));
        out = ch2.Edited(box);
        CHECK(ClosedConsistent(out) && Euler(out) == 2);
        // Tessellate the top: by edge 4 faces each (shared midpoints), by centre 3 each.
        scene::EditMeshModifier te(8, {});
        te.SetSelectedFaces({2, 3});
        te.SetState(te.AfterTessellating(box, false));
        out = te.Edited(box);
        CHECK(out.FaceCount() == 18 && out.verts.size() == 13 && te.SelectedFaces().size() == 8);
        scene::EditMeshModifier tc(8, {});
        tc.SetSelectedFaces({2, 3});
        tc.SetState(tc.AfterTessellating(box, true));
        out = tc.Edited(box);
        CHECK(out.FaceCount() == 16 && out.verts.size() == 10);
        // Break a corner: one copy for each face but the first using it.
        size_t using4 = 0;
        for (const auto& f : box.faces) for (uint32_t v : f.v) using4 += v == 4;
        scene::EditMeshModifier br(8, {});
        br.SetSelectedVertices({4});
        br.SetState(br.AfterBreaking(box));
        out = br.Edited(box);
        CHECK(out.verts.size() == 8 + using4 - 1 && out.FaceCount() == 12 && !ClosedConsistent(out));
        // Explode at 1 degree: the box's six sides; to elements, nothing shared: 24 vertices.
        scene::EditMeshModifier xp(8, {});
        std::vector<uint32_t> all;
        for (uint32_t f = 0; f < 12; ++f) all.push_back(f);
        xp.SetSelectedFaces(all);
        CHECK(xp.ExplodeGroups(box, 1).size() == 6);
        xp.SetState(xp.AfterExplodingToElements(box, 1));
        out = xp.Edited(box);
        CHECK(out.verts.size() == 24 && out.FaceCount() == 12);
        // Attach a second box: appended.
        scene::Mesh other = box;
        for (auto& v : other.verts) v.x += 50;
        scene::EditMeshModifier at(8, {});
        at.SetState(at.AfterAttaching(box, other));
        out = at.Edited(box);
        CHECK(out.verts.size() == 16 && out.FaceCount() == 24 && out.verts[8].x == box.verts[0].x + 50);
    }
    // Divide and Bevel (11.1).
    {
        // Divide one top edge: one new vertex, its two faces become four, still closed.
        scene::EditMeshModifier dv(8, {});
        dv.SetSelectedEdges({scene::EdgeId(2, 0)});
        dv.SetState(dv.AfterDividingEdges(box));
        scene::Mesh out = dv.Edited(box);
        CHECK(out.verts.size() == 9 && out.FaceCount() == 14 && ClosedConsistent(out) && Euler(out) == 2);
        // Bevel the top: height 10, outline 5; closed, top at 30, each corner 5 further out
        // from the top's centre than a plain Extrude puts it.
        scene::EditMeshModifier bv(8, {}), ex(8, {});
        bv.SetSelectedFaces({2, 3});
        ex.SetSelectedFaces({2, 3});
        bv.SetState(bv.AfterBeveling(box, 10, 5));
        ex.SetState(ex.AfterExtruding(box, 10));
        const scene::Mesh b = bv.Edited(box), e = ex.Edited(box);
        CHECK(ClosedConsistent(b) && Euler(b) == 2 && b.verts.size() == 12);
        for (size_t i = 8; i < 12; ++i) {
            CHECK(std::fabs(b.verts[i].z - 30) < 1e-9);
            const double rb = std::hypot(b.verts[i].x, b.verts[i].y), re = std::hypot(e.verts[i].x, e.verts[i].y);
            CHECK(std::fabs(rb - re - 5) < 1e-9);
        }
    }
    // Interactive tools (10.9).
    {
        // Slice the closed box at z = 10: still closed, Euler 2, 8 new vertices (4 vertical
        // edges, 4 side diagonals), 28 faces; nothing straddles the plane.
        scene::EditMeshModifier sl(8, {});
        sl.SetState(sl.AfterSlicing(box, {0, 0, 10}, {0, 0, 1}, false, false));
        scene::Mesh out = sl.Edited(box);
        CHECK(out.verts.size() == 16 && out.FaceCount() == 28 && ClosedConsistent(out) && Euler(out) == 2);
        for (const auto& f : out.faces) {
            bool above = false, below = false;
            for (uint32_t v : f.v) { above = above || out.verts[v].z > 10 + 1e-9; below = below || out.verts[v].z < 10 - 1e-9; }
            CHECK(!(above && below));
        }
        // Split: the upper side gets its own copies; the halves come apart.
        scene::EditMeshModifier sp(8, {});
        sp.SetState(sp.AfterSlicing(box, {0, 0, 10}, {0, 0, 1}, true, false));
        out = sp.Edited(box);
        CHECK(out.verts.size() == 24 && out.FaceCount() == 28 && !ClosedConsistent(out));
        // Cut across face 0 from its edge 0 to its edge 1: the face becomes 3, its two
        // neighbours across those edges 2 each; still closed.
        scene::EditMeshModifier cu(8, {});
        cu.SetState(cu.AfterCutting(box, scene::EdgeId(0, 0), 0.5, scene::EdgeId(0, 1), 0.5));
        out = cu.Edited(box);
        CHECK(out.verts.size() == 10 && out.FaceCount() == 16 && ClosedConsistent(out) && Euler(out) == 2);
        CHECK(cu.AfterCutting(box, scene::EdgeId(0, 0), 0.5, scene::EdgeId(1, 0), 0.5).addedVertices.size() == cu.AddedVertices().size());   // different faces: refused, nothing new
        // Create: a vertex (selected), and a face wound towards the viewer whatever the order.
        scene::EditMeshModifier cr(8, {});
        cr.SetState(cr.AfterCreatingVertex(box, {0, 0, 40}));
        CHECK(cr.Edited(box).verts.size() == 9 && cr.SelectedVertices() == std::vector<uint32_t>{8});
        for (const auto& order : {std::vector<uint32_t>{4, 5, 8}, std::vector<uint32_t>{5, 4, 8}}) {
            scene::EditMeshModifier c2 = cr;
            c2.SetState(c2.AfterCreatingFace(box, order[0], order[1], order[2], {0, -1, 0}));
            const scene::Mesh m2 = c2.Edited(box);
            const auto& f = m2.faces.back();
            const math::Vec3 nrm = math::Cross(m2.verts[f.v[1]] - m2.verts[f.v[0]], m2.verts[f.v[2]] - m2.verts[f.v[0]]);
            CHECK(m2.FaceCount() == 13 && math::Dot(nrm, {0, -1, 0}) >= 0);
        }
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
