#pragma once
// Small OpenGL helpers: shader programs, uniforms with our own matrix types,
// and simple vertex buffers.
#include <GL/glew.h>

#include <vector>

#include "core/Canvas.h"
#include "math/Transform.h"

GLuint compileProgram(const char* vertexSource, const char* fragmentSource);

void setUniform(GLuint program, const char* name, const Mat4& m);   // uploads row-major with transpose
void setUniform(GLuint program, const char* name, Vec3 v);
void setUniform(GLuint program, const char* name, Color c);         // as 0..1 RGB
void setUniform(GLuint program, const char* name, float v);
void setUniform(GLuint program, const char* name, int v);

// Interleaved vertex layout used by every 3D mesh.
struct Vertex {
    float px, py, pz;   // position
    float nx, ny, nz;   // normal
    float r, g, b;      // colour
    float u, v;         // texture coordinate
};

class Mesh {
public:
    Mesh() = default;
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    ~Mesh();

    void upload(const std::vector<Vertex>& vertices);   // replaces the contents
    void draw(GLenum mode = GL_TRIANGLES) const;
    int count() const { return count_; }

private:
    GLuint vao_ = 0, vbo_ = 0;
    int count_ = 0;
};

// Appends the 6 vertices (2 triangles) of one cube face.
// face: 0 +x, 1 -x, 2 +y, 3 -y, 4 +z, 5 -z. The cube spans [x,x+1] etc.;
// `height` lets water sit slightly lower than a full block.
void appendCubeFace(std::vector<Vertex>& out, float x, float y, float z, int face, Color c, float height = 1.0f);
// A unit cube centred on the origin (for animated blocks).
std::vector<Vertex> unitCubeVertices();
