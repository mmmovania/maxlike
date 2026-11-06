#include "scene/CreationTool.h"
#include "scene/Primitives.h"

namespace scene {

std::shared_ptr<Object> MakePrimitive(int type) {
    switch (static_cast<Primitive>(type)) {
    case Primitive::Box:       return std::make_shared<BoxObject>();
    case Primitive::Cone:      return std::make_shared<ConeObject>();
    case Primitive::Sphere:    return std::make_shared<SphereObject>();
    case Primitive::GeoSphere: return std::make_shared<GeoSphereObject>();
    case Primitive::Cylinder:  return std::make_shared<CylinderObject>();
    case Primitive::Tube:      return std::make_shared<TubeObject>();
    case Primitive::Torus:     return std::make_shared<TorusObject>();
    case Primitive::Pyramid:   return std::make_shared<PyramidObject>();
    case Primitive::Teapot:    return std::make_shared<TeapotObject>();
    case Primitive::Plane:     return std::make_shared<PlaneObject>();
    }
    return nullptr;
}

const char* PrimitiveBaseName(int type) {
    static const char* kNames[] = {"Box", "Cone", "Sphere", "GeoSphere", "Cylinder", "Tube", "Torus",
                                   "Pyramid", "Teapot", "Plane"};
    return (type >= 0 && type < 10) ? kNames[type] : "";
}

uint32_t CreationTool::NextRandomColor(uint32_t previous) {
    // A small linear congruential step on the previous colour, then lifted so colours
    // stay clearly visible on the grey viewport (each channel 0x20..0xDF).
    uint32_t x = previous * 1103515245u + 12345u;
    const uint32_t r = 0x20 + ((x >> 16) & 0xFF) % 0xC0;
    const uint32_t g = 0x20 + ((x >> 8) & 0xFF) % 0xC0;
    const uint32_t b = 0x20 + (x & 0xFF) % 0xC0;
    return (r << 16) | (g << 8) | b;
}

std::shared_ptr<Object> CreationTool::TargetObject() const {
    if (creator_) return object_;
    const Node* n = scene_.FindById(lastId_);
    return n ? n->GetObject() : nullptr;
}

std::string CreationTool::TargetName() const {
    if (creator_) return name_;
    return LastCreatedName();
}

std::string CreationTool::LastCreatedName() const {
    const Node* n = scene_.FindById(lastId_);
    return n ? n->Name() : std::string();
}

void CreationTool::SetType(int type) {
    if (type == type_) return;
    Cancel();
    type_ = type;
}

void CreationTool::RemoveLive() {
    if (Node* n = scene_.FindById(liveId_)) scene_.DeleteNode(n);
    liveId_ = 0;
}

bool CreationTool::Cancel() {
    if (!creator_) return false;
    RemoveLive();
    creator_.reset();
    object_.reset();
    owner_ = -1;
    return true;
}

void CreationTool::Apply() {
    for (const auto& [index, value] : creator_->Params()) object_->Params().Set(index, value);
    if (Node* n = scene_.FindById(liveId_)) n->Transform() = plane_.NodeTransform(creator_->CenterU(), creator_->CenterV());
}

// The live node goes, and one undoable step creates the same object (same name, same
// shared object, same colour and transform) in its place.
void CreationTool::Finish() {
    const Node* live = scene_.FindById(liveId_);
    const PRS transform = live ? live->Transform() : PRS{};
    RemoveLive();
    auto command = std::make_unique<CreateNodeCommand>(name_, object_, color_, transform);
    CreateNodeCommand* created = command.get();
    undo_.Execute(std::move(command), scene_);
    scene_.SetSelection({created->CreatedId()});   // R3: a new object is selected
    lastId_ = created->CreatedId();
    creator_.reset();
    object_.reset();
    owner_ = -1;
}

bool CreationTool::Feed(Creator::Event e, int owner, const ConstructionPlane& plane, double u, double v,
                        int screenY, double unitsPerPixel) {
    if (type_ < 0) return false;
    if (creator_ && owner != owner_) return false;          // another viewport is building
    if (!creator_) {
        if (e != Creator::Event::Press) return false;
        creator_ = std::make_unique<Creator>(static_cast<Primitive>(type_), unitsPerPixel);
        object_ = MakePrimitive(type_);
        plane_ = plane;
        owner_ = owner;
        name_ = scene_.UniqueName(PrimitiveBaseName(type_));
        if (!colorPending_) color_ = NextRandomColor(color_);   // a new object, a new colour
        colorPending_ = false;
        Node* n = scene_.CreateNode(name_);
        n->SetObject(object_);
        n->SetWireColor(color_);
        liveId_ = n->GetId();
    }
    creator_->Feed(e, u, v, screenY);
    if (creator_->Cancelled()) { Cancel(); return true; }
    Apply();
    if (creator_->Done()) Finish();
    return true;
}

std::string CreationTool::CreateDirect(const std::vector<std::pair<size_t, double>>& params,
                                       const ConstructionPlane& plane, const math::Vec3& position) {
    if (type_ < 0) return "";
    Cancel();
    std::shared_ptr<Object> object = MakePrimitive(type_);
    for (const auto& [index, value] : params) object->Params().Set(index, value);
    PRS transform = plane.NodeTransform(0, 0);
    transform.position = position;
    const std::string name = scene_.UniqueName(PrimitiveBaseName(type_));
    if (!colorPending_) color_ = NextRandomColor(color_);
    colorPending_ = false;
    auto command = std::make_unique<CreateNodeCommand>(name, object, color_, transform);
    CreateNodeCommand* created = command.get();
    undo_.Execute(std::move(command), scene_);
    scene_.SetSelection({created->CreatedId()});
    lastId_ = created->CreatedId();
    return name;
}

} // namespace scene
