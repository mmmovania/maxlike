#pragma once
#include "scene/Creation.h"
#include "scene/Undo.h"
#include <memory>
#include <string>

// Interactive creation in the scene: while the mouse builds an object, a live node
// shows it; when it is finished, the live node is replaced by one undoable "Create"
// step (so Undo removes it in one go); when cancelled, nothing remains.
// No Windows dependency (unit-tested in tests/test_creation_tool.cpp).
namespace scene {

class CreationTool {
public:
    CreationTool(Scene& scene, UndoStack& undo) : scene_(scene), undo_(undo) {}

    // The primitive to create (-1 = none), in the Object Type order (Box, Cone, Sphere,
    // GeoSphere, Cylinder, Tube, Torus, Pyramid, Teapot, Plane). Changing it cancels
    // a creation in progress.
    void SetType(int type);
    int  Type() const { return type_; }

    // The colour of the next object (0xRRGGBB): Name and Color's swatch.
    void SetColor(uint32_t rgb) { color_ = rgb; }
    uint32_t Color() const { return color_; }

    // Feed a mouse event on construction plane `plane` at (u, v), screen row `screenY`.
    // `owner` identifies the viewport; while an object is being built, events from other
    // viewports are ignored. Returns true if the scene changed (redraw).
    bool Feed(Creator::Event e, int owner, const ConstructionPlane& plane, double u, double v,
              int screenY, double unitsPerPixel);
    bool Cancel();                                       // right-click / Esc; true if something was removed

    // Keyboard Entry's Create button: the current type with typed parameters (Max
    // indices), its pivot at world position `position`, oriented like the view's
    // construction plane. One undo step, like a dragged object. Returns the new node's
    // name ("" if no type is selected).
    std::string CreateDirect(const std::vector<std::pair<size_t, double>>& params,
                             const ConstructionPlane& plane, const math::Vec3& position);

    bool Busy() const { return creator_ != nullptr; }    // an object is being built
    int  Owner() const { return owner_; }
    int  Step() const { return creator_ ? creator_->Step() : 0; }
    const std::string& LastCreatedName() const { return lastName_; }

    // R3 gives every new object a random colour; a fixed generator keeps tests stable.
    // NOTE: R3's palette for these colours is unverified.
    static uint32_t NextRandomColor(uint32_t previous);

private:
    void Apply();          // copy the creator's parameters and pivot to the live node
    void Finish();
    void RemoveLive();

    Scene& scene_;
    UndoStack& undo_;
    int type_ = -1;
    uint32_t color_ = 0x8D073A;
    std::unique_ptr<Creator> creator_;
    std::shared_ptr<Object> object_;
    ConstructionPlane plane_;
    Node::Id liveId_ = 0;
    std::string name_, lastName_;
    int owner_ = -1;
};

// A new object of the given type, in the Object Type order (nullptr for others).
std::shared_ptr<Object> MakePrimitive(int type);
const char* PrimitiveBaseName(int type);   // "Box", "Cone", ...

} // namespace scene
