#pragma once
#include "io/MaxImport.h"
#include <cstdint>
#include <string>
#include <vector>

// File > Merge (Phase 9): another file's objects (.max or .mlk) added to `scene`. Nothing
// changes if the file cannot be read. Names already in the scene get the next free number
// (R3 asks in a dialog; NOTE: not built), with a note. `added` lists the new nodes,
// parents before children (for scene::AddedNodesCommand). No Windows dependency.
namespace io {

bool MergeScene(const std::vector<uint8_t>& fileBytes, scene::Scene& scene, ImportReport& report,
                std::vector<scene::Node::Id>& added, std::string& error);

// Phase 11.2: R3's Merge dialog lists a file's objects and merges the chosen ones.
struct MergeCandidate {
    std::string name;
    int depth = 0;            // links below the top (for "Display Subtree")
    bool groupHead = false;   // listed under "Groups"
    int parent = -1;          // index of its parent in the list (for "Select Subtree")
};
bool ListMergeCandidates(const std::vector<uint8_t>& fileBytes, std::vector<MergeCandidate>& out, std::string& error);
// As MergeScene, merging only the objects whose index in ListMergeCandidates' list is in
// `chosen` (a chosen object whose parent is not chosen comes in at the top, where it was).
bool MergeChosen(const std::vector<uint8_t>& fileBytes, const std::vector<int>& chosen, scene::Scene& scene,
                 ImportReport& report, std::vector<scene::Node::Id>& added, std::string& error);

} // namespace io
