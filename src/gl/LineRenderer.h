#pragma once
#include <string>
#include <vector>

namespace gl {

// Draws 1-px lines given in window pixels (top-down, like GDI), in one colour per
// call. Owns a small shader program and a vertex buffer. All methods must be called
// with the OpenGL context current; Create once, then Draw any number of times.
class LineRenderer {
public:
    struct PixelLine { float x0, y0, x1, y1; }; // pixel coordinates; centre of pixel i is i + 0.5

    LineRenderer() = default;
    LineRenderer(const LineRenderer&) = delete;            // owns GPU objects:
    LineRenderer& operator=(const LineRenderer&) = delete; // never copy them

    bool Create();                     // compile shaders, make buffers
    void Release();                    // delete the GPU objects (context must be current)
    bool IsValid() const { return program_ != 0; }
    const std::string& Error() const { return error_; }

    // Draw `lines` in a viewport of `width` x `height` pixels with an RGB colour.
    void Draw(const std::vector<PixelLine>& lines, int width, int height, float r, float g, float b);

private:
    unsigned program_ = 0;  // GLuint
    unsigned vao_ = 0;
    unsigned vbo_ = 0;
    int uViewport_ = -1;    // uniform locations
    int uColor_ = -1;
    std::string error_;
};

} // namespace gl
