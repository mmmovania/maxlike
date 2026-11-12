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

namespace {
bool LoadAny(const std::vector<uint8_t>& bytes, scene::Scene& s, ImportReport& report, std::string& error) {
    return IsNativeFile(bytes) ? LoadNative(bytes, s, error) : ImportMax(bytes, s, report, error);
}
} // namespace

bool ListMergeCandidates(const std::vector<uint8_t>& bytes, std::vector<MergeCandidate>& out, std::string& error) {
    out.clear();
    scene::Scene s;
    ImportReport report;
    if (!LoadAny(bytes, s, report, error)) return false;
    for (const auto& n : s.Nodes()) {
        MergeCandidate c;
        c.name = n->Name();
        c.groupHead = n->IsGroupHead();
        for (const scene::Node* p = n->Parent(); p; p = p->Parent()) ++c.depth;
        for (size_t i = 0; i < s.Nodes().size(); ++i)
            if (s.Nodes()[i].get() == n->Parent()) c.parent = static_cast<int>(i);
        out.push_back(c);
    }
    return true;
}

bool MergeChosen(const std::vector<uint8_t>& bytes, const std::vector<int>& chosen, scene::Scene& scene,
                 ImportReport& report, std::vector<scene::Node::Id>& added, std::string& error) {
    if (!MergeScene(bytes, scene, report, added, error)) return false;
    // The merged nodes in file order are the last ones created; drop the unchosen ones,
    // children first (a chosen child of a dropped parent keeps its world placement).
    std::vector<scene::Node*> merged;
    for (scene::Node::Id id : added) merged.push_back(scene.FindById(id));
    std::vector<scene::Node*> inFileOrder = merged;
    std::sort(inFileOrder.begin(), inFileOrder.end(), [](const scene::Node* a, const scene::Node* b) { return a->GetId() < b->GetId(); });
    std::vector<scene::Node::Id> keep;
    for (size_t i = inFileOrder.size(); i-- > 0;) {
        scene::Node* n = inFileOrder[i];
        if (std::find(chosen.begin(), chosen.end(), static_cast<int>(i)) != chosen.end()) { keep.push_back(n->GetId()); continue; }
        for (scene::Node* c : std::vector<scene::Node*>(n->Children())) scene.Link(c, n->Parent());
        scene.DeleteNode(n);
    }
    std::vector<scene::Node::Id> ordered;
    for (scene::Node::Id id : added)
        if (std::find(keep.begin(), keep.end(), id) != keep.end()) ordered.push_back(id);
    added = ordered;
    report.nodes = added.size();
    return true;
}

} // namespace io
