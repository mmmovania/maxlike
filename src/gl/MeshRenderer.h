#pragma once
#include "math/Mat4.h"
#include <string>
#include <vector>

namespace gl {

// Draws lit triangles: positions and normals in world space, one colour per call.
// Like LineRenderer: needs the context current; Create once, Release before the
// context goes away.
class MeshRenderer {
public:
    struct Lighting {
        math::Vec3 toLight;     // unit direction from the surface towards the light
        math::Vec3 eye;         // camera position (perspective)
        math::Vec3 viewDir;     // unit direction towards the viewer (ortho)
        bool ortho = false;
        bool highlights = true; // specular highlight on/off (Smooth vs Smooth + Highlights)
    };

    MeshRenderer() = default;
    MeshRenderer(const MeshRenderer&) = delete;
    MeshRenderer& operator=(const MeshRenderer&) = delete;

    bool Create();
    void Release();
    bool IsValid() const { return program_ != 0; }
    const std::string& Error() const { return error_; }

    // `vertices`: x y z nx ny nz per vertex, three vertices per triangle.
    void Draw(const std::vector<float>& vertices, const math::Mat4& worldToClip,
              float r, float g, float b, const Lighting& light);

private:
    unsigned program_ = 0, vao_ = 0, vbo_ = 0;
    int uMatrix_ = -1, uColor_ = -1, uToLight_ = -1, uEye_ = -1, uViewDir_ = -1, uOrtho_ = -1, uHighlights_ = -1;
    std::string error_;
};

} // namespace gl
