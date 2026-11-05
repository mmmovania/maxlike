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

} // namespace scene
