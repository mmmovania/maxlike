#include <glad/gl.h>
#include "gl/LineRenderer.h"

namespace gl {

// GLSL 3.30 (OpenGL 3.3). The vertex shader turns pixel coordinates (top-down)
// into OpenGL's clip space (-1..1, bottom-up); the fragment shader paints every
// covered pixel in one colour.
static const char* kVertexShader = R"(#version 330 core
layout(location = 0) in vec2 aPixel;
uniform vec2 uViewport;              // width, height in pixels
void main() {
    vec2 ndc = vec2(aPixel.x / uViewport.x * 2.0 - 1.0,
                    1.0 - aPixel.y / uViewport.y * 2.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
}
)";

static const char* kFragmentShader = R"(#version 330 core
uniform vec3 uColor;
out vec4 fragColor;
void main() {
    fragColor = vec4(uColor, 1.0);
}
)";

// Compile one shader stage; on failure return 0 and put the driver's message in `error`.
static GLuint Compile(GLenum stage, const char* source, std::string& error) {
    GLuint shader = glCreateShader(stage);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024] = {};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        error = std::string(stage == GL_VERTEX_SHADER ? "vertex" : "fragment") + " shader: " + log;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

// OpenGL objects belong to the context: they can only be deleted while it is
// current, so this is an explicit call (ViewPanel does it on WM_DESTROY), not a
// destructor. (Deleting the context would also free them.)
void LineRenderer::Release() {
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (program_) glDeleteProgram(program_);
    vbo_ = vao_ = program_ = 0;
}

bool LineRenderer::Create() {
    GLuint vs = Compile(GL_VERTEX_SHADER, kVertexShader, error_);
    if (!vs) return false;
    GLuint fs = Compile(GL_FRAGMENT_SHADER, kFragmentShader, error_);
    if (!fs) { glDeleteShader(vs); return false; }

    // Link the two stages into one program.
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs); // the program keeps what it needs
    glDeleteShader(fs);
    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024] = {};
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        error_ = std::string("link: ") + log;
        glDeleteProgram(program);
        return false;
    }
    program_ = program;
    uViewport_ = glGetUniformLocation(program_, "uViewport");
    uColor_ = glGetUniformLocation(program_, "uColor");

    // A vertex array object remembers how the buffer's bytes map to `aPixel`.
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glBindVertexArray(0);
    return true;
}

void LineRenderer::Draw(const std::vector<PixelLine>& lines, int w, int h, float r, float g, float b) {
    if (!program_ || lines.empty() || w <= 0 || h <= 0) return;
    glUseProgram(program_);
    glUniform2f(uViewport_, static_cast<float>(w), static_cast<float>(h));
    glUniform3f(uColor_, r, g, b);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    // Upload this batch (two points per line). Small and simple: a few hundred lines.
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(lines.size() * sizeof(PixelLine)),
                 lines.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lines.size() * 2));
    glBindVertexArray(0);
}

} // namespace gl
