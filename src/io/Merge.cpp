#include "io/Merge.h"
#include "io/NativeFile.h"
#include <algorithm>
#include <cctype>

namespace io {

bool MergeScene(const std::vector<uint8_t>& bytes, scene::Scene& scene, ImportReport& report,
                std::vector<scene::Node::Id>& added, std::string& error) {
    added.clear();
    const bool native = IsNativeFile(bytes);
    // Into a scratch scene first: a file that cannot be read leaves this one untouched.
    {
        scene::Scene scratch;
        if (native ? !LoadNative(bytes, scratch, error) : !ImportMax(bytes, scratch, report, error)) return false;
    }
    const size_t before = scene.Nodes().size();
    std::vector<std::string> existing;
    for (const auto& n : scene.Nodes()) existing.push_back(n->Name());
    report = {};
    if (native ? !LoadNative(bytes, scene, error) : !ImportMax(bytes, scene, report, error)) return false;
    std::vector<scene::Node*> nodes;
    for (size_t i = before; i < scene.Nodes().size(); ++i) nodes.push_back(scene.Nodes()[i].get());
    for (scene::Node* n : nodes) {
        if (std::find(existing.begin(), existing.end(), n->Name()) == existing.end()) { existing.push_back(n->Name()); continue; }
        std::string base = n->Name();
        while (!base.empty() && std::isdigit(static_cast<unsigned char>(base.back()))) base.pop_back();
        const std::string renamed = scene.UniqueName(base.empty() ? "Object" : base);
        report.notes.push_back(n->Name() + " is already in the scene: merged as " + renamed);
        n->SetName(renamed);
        existing.push_back(renamed);
    }
    auto depth = [](const scene::Node* n) { int d = 0; for (const scene::Node* p = n->Parent(); p; p = p->Parent()) ++d; return d; };
    std::stable_sort(nodes.begin(), nodes.end(), [&](const scene::Node* a, const scene::Node* b) { return depth(a) < depth(b); });
    for (const scene::Node* n : nodes) added.push_back(n->GetId());
    report.nodes = nodes.size();
    return true;
}

} // namespace io
