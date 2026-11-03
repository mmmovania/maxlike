#include <glad/gl.h>
#include "gl/LineRenderer.h"

namespace gl {

// GLSL 3.30 (OpenGL 3.3). The vertex shader multiplies each point by one matrix,
// giving OpenGL's clip space; the fragment shader paints every covered pixel in
// one colour.
static const char* kVertexShader = R"(#version 330 core
layout(location = 0) in vec3 aPosition;
uniform mat4 uMatrix;
void main() {
    gl_Position = uMatrix * vec4(aPosition, 1.0);
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
    uMatrix_ = glGetUniformLocation(program_, "uMatrix");
    uColor_ = glGetUniformLocation(program_, "uColor");

    // A vertex array object remembers how the buffer's bytes map to `aPosition`.
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glBindVertexArray(0);
    return true;
}

void LineRenderer::Draw(const std::vector<Line>& lines, const math::Mat4& matrix, float r, float g, float b) {
    if (!program_ || lines.empty()) return;
    glUseProgram(program_);
    float m[16];
    matrix.ToFloats(m);
    glUniformMatrix4fv(uMatrix_, 1, GL_FALSE, m); // GL_FALSE: already column-major
    glUniform3f(uColor_, r, g, b);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    // Upload this batch (two points per line). Small and simple: a few hundred lines.
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(lines.size() * sizeof(Line)),
                 lines.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lines.size() * 2));
    glBindVertexArray(0);
}

} // namespace gl
