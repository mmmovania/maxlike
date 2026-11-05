#pragma once
#include "scene/Object.h"
#include <memory>

// Parametric primitives, built the way 3ds Max builds them where that is known
// (max-format.md §7). Parameters use Max's indices.
namespace scene {

// Box (class 0x10). 0 length (Y), 1 width (X), 2 height (Z), 3 width segs, 4 length
// segs, 5 height segs. Pivot at the centre of the base.
class BoxObject : public Object {
public:
    enum { kLength = 0, kWidth, kHeight, kWidthSegs, kLengthSegs, kHeightSegs };
    BoxObject(double length = 25, double width = 25, double height = 25);
    std::string ClassName() const override { return "Box"; }
    Mesh Evaluate() const override;
};

// Plane (PB2). 0 length (Y), 1 width (X), 2 length segs, 3 width segs. Centred on the
// pivot, facing +Z. (Segment ids 2/3: from the JS reader, still ◐ in max-format.md.)
class PlaneObject : public Object {
public:
    enum { kLength = 0, kWidth, kLengthSegs, kWidthSegs };
    PlaneObject(double length = 25, double width = 25);
    std::string ClassName() const override { return "Plane"; }
    Mesh Evaluate() const override;
};

// Pyramid. 0 width (X), 1 depth (Y), 2 height (Z), 3 width segs, 4 depth segs,
// 5 height segs. Base centred on the pivot, apex above it.
class PyramidObject : public Object {
public:
    enum { kWidth = 0, kDepth, kHeight, kWidthSegs, kDepthSegs, kHeightSegs };
    PyramidObject(double width = 20, double depth = 20, double height = 20);
    std::string ClassName() const override { return "Pyramid"; }
    Mesh Evaluate() const override;
};

// Cylinder (class 0x12). 0 radius, 1 height, 2 height segs, 3 cap segs, 4 sides,
// 5 smooth. Pivot at the centre of the base.
class CylinderObject : public Object {
public:
    enum { kRadius = 0, kHeight, kHeightSegs, kCapSegs, kSides, kSmooth };
    CylinderObject(double radius = 15, double height = 25);
    std::string ClassName() const override { return "Cylinder"; }
    Mesh Evaluate() const override;
};

// Cone. 0 radius 1 (base), 1 radius 2 (top), 2 height, 3 height segs, 4 cap segs,
// 5 sides, 6 smooth (7-9 slice: not yet). Negative radii count as 0 (Max clamps them).
class ConeObject : public Object {
public:
    enum { kRadius1 = 0, kRadius2, kHeight, kHeightSegs, kCapSegs, kSides, kSmooth };
    ConeObject(double radius1 = 15, double radius2 = 0, double height = 25);
    std::string ClassName() const override { return "Cone"; }
    Mesh Evaluate() const override;
};

// Tube. 0 radius 1, 1 radius 2, 2 height, 3 sides, 4 cap segs, 5 height segs, 6 smooth
// (note: a different order from Cone, verified). The larger radius is the outside.
class TubeObject : public Object {
public:
    enum { kRadius1 = 0, kRadius2, kHeight, kSides, kCapSegs, kHeightSegs, kSmooth };
    TubeObject(double radius1 = 15, double radius2 = 10, double height = 25);
    std::string ClassName() const override { return "Tube"; }
    Mesh Evaluate() const override;
};

// Torus. 0 radius 1 (ring), 1 radius 2 (tube), 2 rotation, 3 twist (degrees,
// unverified), 4 segments (around the ring), 5 sides (around the tube).
class TorusObject : public Object {
public:
    enum { kRadius1 = 0, kRadius2, kRotation, kTwist, kSegments, kSides };
    TorusObject(double radius1 = 25, double radius2 = 5);
    std::string ClassName() const override { return "Torus"; }
    Mesh Evaluate() const override;
};

} // namespace scene
