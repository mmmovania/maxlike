#pragma once
#include "scene/Mesh.h"
#include "scene/ParamBlock.h"
#include <memory>
#include <string>

// What a node shows: a geometric object that evaluates to a mesh. Several nodes may
// share one object (Max's instances), so objects are held by shared_ptr.
namespace scene {

class Object {
public:
    virtual ~Object() = default;
    virtual std::string ClassName() const = 0;     // "Box", "Sphere", "Editable Mesh", ...
    virtual Mesh Evaluate() const = 0;             // the mesh in object space
    ParamBlock& Params() { return params_; }
    const ParamBlock& Params() const { return params_; }

protected:
    ParamBlock params_;
};

// A mesh edited directly (Max's Editable Mesh): it simply holds its mesh.
class MeshObject : public Object {
public:
    explicit MeshObject(Mesh mesh) : mesh_(std::move(mesh)) {}
    std::string ClassName() const override { return "Editable Mesh"; }
    Mesh Evaluate() const override { return mesh_; }
    Mesh& EditMesh() { return mesh_; }

private:
    Mesh mesh_;
};

} // namespace scene
