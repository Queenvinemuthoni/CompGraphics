#include "engine/HudOverlay.h"

namespace {

// A full-screen triangle generated from gl_VertexID; no vertex buffer needed.
const char* VS = R"(
    #version 330 core
    out vec2 uv;
    void main() {
        vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
        uv = vec2(p.x, 1.0 - p.y);   // screen (-1..1) -> texture (0..1); canvas row 0 is the top
        gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
    }
)";

const char* FS = R"(
    #version 330 core
    in vec2 uv;
    out vec4 color;
    uniform sampler2D hud;
    void main() {
        color = texture(hud, uv);
        if (color.a < 0.01) discard;
    }
)";

}  // namespace

HudOverlay::HudOverlay(int width, int height)
    : canvas_(width, height, KEY), rgba_(static_cast<size_t>(width) * height * 4) {
    program_ = compileProgram(VS, FS);
    glGenVertexArrays(1, &vao_);
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}

HudOverlay::~HudOverlay() {
    glDeleteTextures(1, &texture_);
    glDeleteVertexArrays(1, &vao_);
    glDeleteProgram(program_);
}

void HudOverlay::upload() {
    size_t i = 0;
    for (int y = 0; y < canvas_.height(); ++y)
        for (int x = 0; x < canvas_.width(); ++x, i += 4) {
            Color c = canvas_.getPixel(x, y);
            rgba_[i] = c.r; rgba_[i + 1] = c.g; rgba_[i + 2] = c.b;
            rgba_[i + 3] = c == KEY ? 0 : 255;
        }
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, canvas_.width(), canvas_.height(), GL_RGBA, GL_UNSIGNED_BYTE, rgba_.data());
}

void HudOverlay::draw() const {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(program_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glUniform1i(glGetUniformLocation(program_, "hud"), 0);
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
