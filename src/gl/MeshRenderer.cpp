#include <glad/gl.h>
#include "gl/MeshRenderer.h"

namespace gl {

static const char* kVertexShader = R"(#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
uniform mat4 uMatrix;
out vec3 vPosition;
out vec3 vNormal;
void main() {
    vPosition = aPosition;          // already in world space
    vNormal = aNormal;
    gl_Position = uMatrix * vec4(aPosition, 1.0);
}
)";

// One directional light. Ambient + diffuse in the object's colour, plus a white
// Blinn-Phong highlight. Faces seen from behind are lit as if facing the viewer
// (two-sided), so open meshes such as a Plane shade from both sides.
// NOTE: R3's viewport lighting (light placement, ambient, highlight size) is not
// measured yet; these are reasonable defaults.
static const char* kFragmentShader = R"(#version 330 core
in vec3 vPosition;
in vec3 vNormal;
uniform vec3 uColor;
uniform vec3 uToLight;
uniform vec3 uEye;
uniform vec3 uViewDir;
uniform bool uOrtho;
uniform bool uHighlights;
out vec4 fragColor;
void main() {
    vec3 n = normalize(vNormal);
    if (!gl_FrontFacing) n = -n;
    vec3 v = uOrtho ? uViewDir : normalize(uEye - vPosition);
    float diffuse = max(dot(n, uToLight), 0.0);
    vec3 colour = uColor * (0.25 + 0.75 * diffuse);
    if (uHighlights && diffuse > 0.0) {
        vec3 h = normalize(uToLight + v);
        colour += vec3(0.6) * pow(max(dot(n, h), 0.0), 40.0);
    }
    fragColor = vec4(min(colour, vec3(1.0)), 1.0);
}
)";

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

bool MeshRenderer::Create() {
    GLuint vs = Compile(GL_VERTEX_SHADER, kVertexShader, error_);
    if (!vs) return false;
    GLuint fs = Compile(GL_FRAGMENT_SHADER, kFragmentShader, error_);
    if (!fs) { glDeleteShader(vs); return false; }
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
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
    uToLight_ = glGetUniformLocation(program_, "uToLight");
    uEye_ = glGetUniformLocation(program_, "uEye");
    uViewDir_ = glGetUniformLocation(program_, "uViewDir");
    uOrtho_ = glGetUniformLocation(program_, "uOrtho");
    uHighlights_ = glGetUniformLocation(program_, "uHighlights");

    // Interleaved vertices: position (3 floats) then normal (3 floats).
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glBindVertexArray(0);
    return true;
}

void MeshRenderer::Release() {
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (program_) glDeleteProgram(program_);
    vbo_ = vao_ = program_ = 0;
}

void MeshRenderer::Draw(const std::vector<float>& v, const math::Mat4& worldToClip,
                        float r, float g, float b, const Lighting& light) {
    if (!program_ || v.size() < 18) return;
    glUseProgram(program_);
    float m[16];
    worldToClip.ToFloats(m);
    glUniformMatrix4fv(uMatrix_, 1, GL_FALSE, m);
    glUniform3f(uColor_, r, g, b);
    glUniform3f(uToLight_, static_cast<float>(light.toLight.x), static_cast<float>(light.toLight.y), static_cast<float>(light.toLight.z));
    glUniform3f(uEye_, static_cast<float>(light.eye.x), static_cast<float>(light.eye.y), static_cast<float>(light.eye.z));
    glUniform3f(uViewDir_, static_cast<float>(light.viewDir.x), static_cast<float>(light.viewDir.y), static_cast<float>(light.viewDir.z));
    glUniform1i(uOrtho_, light.ortho ? 1 : 0);
    glUniform1i(uHighlights_, light.highlights ? 1 : 0);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(v.size() * sizeof(float)), v.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(v.size() / 6));
    glBindVertexArray(0);
}

} // namespace gl
