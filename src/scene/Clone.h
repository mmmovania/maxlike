#pragma once
#include "scene/Scene.h"
#include <memory>
#include <string>
#include <vector>

// Edit > Clone (Phase 9): copies and instances of nodes.
namespace scene {

// A deep copy of an object: a primitive with its parameters, an Editable Mesh with its
// mesh, a Derived Object with a copied base and copied modifiers (their parameters, Edit
// Mesh's moves and selection, on / off). Unknown kinds are shared (instanced) instead.
std::shared_ptr<Object> CopyObject(const std::shared_ptr<Object>& object);

// Clones of the nodes `ids` (as R3's Clone: same transforms and colours; Copy gives each
// its own objects, Instance shares them). Names: the base name with the next free number,
// or `name` for a single node when not empty. A clone whose parent is also cloned is linked
// under the parent's clone; otherwise it keeps the original's parent. Returns the new
// nodes, parents before children (for AddedNodesCommand).
std::vector<Node::Id> CloneNodes(Scene& scene, const std::vector<Node::Id>& ids, bool instance, const std::string& name = "");

// The base of a name: "Box01" -> "Box" (trailing digits removed).
std::string NameBase(const std::string& name);

} // namespace scene
