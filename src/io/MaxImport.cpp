#include "io/MaxImport.h"
#include <cmath>
#include <algorithm>
#include "io/MaxNodes.h"
#include "io/MaxObjects.h"
#include <set>

namespace io {

bool ImportMax(const std::vector<uint8_t>& bytes, scene::Scene& scene, ImportReport& report, std::string& error) {
    report = {};
    MaxScene file;
    if (!file.Open(bytes, error)) return false;
    const std::vector<MaxNode> nodes = ReadNodes(file);
    ObjectReader reader(file);
    std::vector<scene::Node*> created;
    std::set<const scene::Object*> distinct;
    for (const MaxNode& n : nodes) {
        // R3 names are unique within a file; keep them as they are.
        scene::Node* node = scene.CreateNode(n.name.empty() ? scene.UniqueName("Object") : n.name);
        if (n.hasWireColor) node->SetWireColor((uint32_t{n.wire[0]} << 16) | (uint32_t{n.wire[1]} << 8) | n.wire[2]);
        for (const std::string& note : n.notes) report.notes.push_back(node->Name() + ": " + note);
        node->SetBackfaceCull((n.flags & 0x00800000u) != 0);
        if (n.material >= 0) {
            math::Vec3 diffuse;
            std::vector<std::string> notes;
            if (ReadStandardDiffuse(file, static_cast<size_t>(n.material), diffuse, notes)) {
                auto byte = [](double v) { return static_cast<uint32_t>(std::lround(std::max(0.0, std::min(1.0, v)) * 255)); };
                node->SetMaterialColor(true, (byte(diffuse.x) << 16) | (byte(diffuse.y) << 8) | byte(diffuse.z));
            }
            for (const std::string& note : notes) report.notes.push_back(node->Name() + ": " + note);
        }
        // A group head (0x40): an object-less node (its Dummy is R3's own; maxlike draws a
        // group's bracket box itself), its members linked under it.
        if (n.flags & 0x40u) {
            node->SetGroup(true, (n.flags & 0x80u) != 0);
            created.push_back(node);
            continue;
        }
        if (n.object >= 0) {
            const size_t before = reader.Notes().size();
            if (auto obj = reader.Read(static_cast<size_t>(n.object))) {
                node->SetObject(obj);
                distinct.insert(obj.get());
            }
            for (size_t i = before; i < reader.Notes().size(); ++i) report.notes.push_back(node->Name() + ": " + reader.Notes()[i]);
        }
        created.push_back(node);
    }
    // Links once every node exists (a child may come before its parent in the file), while
    // every transform is still identity: Scene::Link keeps a child's world position by
    // recomputing its local transform, but the file's transforms are already relative to
    // the parent, so they are set only after linking.
    for (size_t i = 0; i < nodes.size(); ++i)
        if (nodes[i].parent >= 0 && !scene.Link(created[i], created[static_cast<size_t>(nodes[i].parent)]))
            report.notes.push_back(created[i]->Name() + ": its link to " + created[static_cast<size_t>(nodes[i].parent)]->Name() + " was refused");
    for (size_t i = 0; i < nodes.size(); ++i) {
        const MaxNode& n = nodes[i];
        created[i]->Transform() = {n.transform.position, n.transform.rotation, n.transform.scale};
        created[i]->ObjectOffset() = {n.objectOffset.position, n.objectOffset.rotation, n.objectOffset.scale};
    }
    report.nodes = created.size();
    report.objects = distinct.size();
    return true;
}

} // namespace io
