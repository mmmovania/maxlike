#pragma once
#include "scene/Undo.h"
#include <memory>
#include <string>
#include <vector>

// Groups (Phase 10): a group head is an object-less node (Node::IsGroupHead) its members
// are linked under. While a group is closed, picking any member selects the group; an
// open group lets members be picked one by one. NOTE: R3 draws an open group as a pink
// box and places the head's pivot at the group's centre; both are from memory.
namespace scene::groups {

// What clicking `node` selects: the outermost closed group above it, else the node itself.
Node* SelectableFor(Node* node);
// `ids` mapped by SelectableFor, each once, in order.
std::vector<Node::Id> Selectable(Scene& scene, const std::vector<Node::Id>& ids);
// Everything a selection stands for: the ids plus every descendant of a group head among
// them (for Delete, Clone, highlighting).
std::vector<Node::Id> WithMembers(const Scene& scene, const std::vector<Node::Id>& ids);
// Drawn as selected: selected itself, or inside a selected group.
bool ShownSelected(const Scene& scene, const Node& node);
// Nodes Select All takes: everything not inside a closed group.
std::vector<Node::Id> AllSelectable(Scene& scene);
// The world bounding box of a group's members (false if none has geometry).
bool Bounds(const Node& head, math::Vec3& lo, math::Vec3& hi);

// The commands, built from existing undo steps. Group creates the head (named `name`, at
// the members' bounding-box centre) right away; the returned command records it.
std::unique_ptr<MacroCommand> MakeGroup(Scene& scene, const std::vector<Node::Id>& members, const std::string& name);
// Ungroup: the heads' members back to each head's parent, the heads removed; Explode does
// the same for every group nested inside too. Members end up selected.
std::unique_ptr<MacroCommand> MakeUngroup(Scene& scene, const std::vector<Node::Id>& heads, bool explode);
// Detach: members out of their (open) group, to the group's parent. Attach: nodes into `head`.
std::unique_ptr<MacroCommand> MakeDetach(Scene& scene, const std::vector<Node::Id>& members);
std::unique_ptr<MacroCommand> MakeAttach(Scene& scene, const std::vector<Node::Id>& nodes, Node::Id head);

} // namespace scene::groups
