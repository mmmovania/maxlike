#pragma once
#include "math/Mat4.h"
#include <string>
#include <vector>

namespace gl {

// Draws 1-px lines in one colour per call. Every point is multiplied by a matrix in
// the vertex shader, so the same renderer draws lines given in window pixels (matrix
// = math::PixelToClip) and lines in the 3D world (matrix = projection * view).
// All methods need the OpenGL context current; Create once, then Draw many times.
class LineRenderer {
public:
    struct Line { float x0, y0, z0, x1, y1, z1; };

    LineRenderer() = default;
    LineRenderer(const LineRenderer&) = delete;            // owns GPU objects:
    LineRenderer& operator=(const LineRenderer&) = delete; // never copy them

    bool Create();                     // compile shaders, make buffers
    void Release();                    // delete the GPU objects (context must be current)
    bool IsValid() const { return program_ != 0; }
    const std::string& Error() const { return error_; }

    // Draw `lines`, transformed by `matrix`, in an RGB colour.
    void Draw(const std::vector<Line>& lines, const math::Mat4& matrix, float r, float g, float b);

private:
    unsigned program_ = 0;  // GLuint
    unsigned vao_ = 0;
    unsigned vbo_ = 0;
    int uMatrix_ = -1;      // uniform locations
    int uColor_ = -1;
    std::string error_;
};

} // namespace gl
