#include "engine/GlUtil.h"

#include <cstddef>
#include <iostream>

namespace {

GLuint compileShader(GLenum type, const char* source) {
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &source, nullptr);
    glCompileShader(id);
    GLint ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(id, sizeof log, nullptr, log);
        std::cerr << "Shader compile error:\n" << log << std::endl;
    }
    return id;
}

}  // namespace

GLuint compileProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(program, sizeof log, nullptr, log);
        std::cerr << "Program link error:\n" << log << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

void setUniform(GLuint p, const char* name, const Mat4& m) {
    glUniformMatrix4fv(glGetUniformLocation(p, name), 1, GL_TRUE, m.data());
}
void setUniform(GLuint p, const char* name, Vec3 v) { glUniform3f(glGetUniformLocation(p, name), v.x, v.y, v.z); }
void setUniform(GLuint p, const char* name, Color c) {
    glUniform3f(glGetUniformLocation(p, name), c.r / 255.0f, c.g / 255.0f, c.b / 255.0f);
}
void setUniform(GLuint p, const char* name, float v) { glUniform1f(glGetUniformLocation(p, name), v); }
void setUniform(GLuint p, const char* name, int v) { glUniform1i(glGetUniformLocation(p, name), v); }

Mesh::~Mesh() {
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
}

void Mesh::upload(const std::vector<Vertex>& vertices) {
    if (!vao_) {
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        const GLsizei stride = sizeof(Vertex);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, px));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, nx));
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, r));
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, u));
        for (int i = 0; i < 4; ++i) glEnableVertexAttribArray(i);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
    count_ = static_cast<int>(vertices.size());
}

void Mesh::draw(GLenum mode) const {
    if (!count_) return;
    glBindVertexArray(vao_);
    glDrawArrays(mode, 0, count_);
}

void appendCubeFace(std::vector<Vertex>& out, float x, float y, float z, int face, Color c, float h) {
    // Corners of each face in counter-clockwise order seen from outside, with UVs.
    static const float corners[6][4][3] = {
        {{1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1}},   // +x
        {{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}},   // -x
        {{0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0}},   // +y
        {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}},   // -y
        {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}},   // +z
        {{1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}},   // -z
    };
    static const float normals[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    static const float uvs[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    static const int order[6] = {0, 1, 2, 0, 2, 3};
    const float r = c.r / 255.0f, g = c.g / 255.0f, b = c.b / 255.0f;
    for (int k : order) {
        const float* p = corners[face][k];
        out.push_back({x + p[0], y + p[1] * h, z + p[2], normals[face][0], normals[face][1], normals[face][2],
                       r, g, b, uvs[k][0], uvs[k][1]});
    }
}

std::vector<Vertex> unitCubeVertices() {
    std::vector<Vertex> v;
    for (int f = 0; f < 6; ++f) appendCubeFace(v, -0.5f, -0.5f, -0.5f, f, {255, 255, 255});
    return v;
}
