// Voxel World Builder - interactive OpenGL prototype (Checkpoint 3).
// Integrates the earlier checkpoints:
//   CP1  the HUD (crosshair, hotbar, minimap, text) is drawn with our own raster
//        algorithms into a Canvas and shown as a texture; same world and colours
//   CP2  our Mat4 transformations drive every OpenGL matrix (no GLM), the
//        day/night cycle drives sky and lighting, and blocks animate with easing
//   CP3  GPU meshes with hidden-face culling, transparent water, fog, sky
//        shader with sun/moon/stars, voxel ray picking, walking + collision
//
// Usage: engine [--time T] [--pos X Y Z] [--look YAW PITCH] [--fly]
//               [--screenshot FILE.ppm] [--frames N]
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "anim/Animation.h"
#include "core/Font.h"
#include "engine/GlUtil.h"
#include "engine/HudOverlay.h"
#include "engine/Player.h"
#include "engine/Raycast.h"
#include "engine/WorldMesh.h"
#include "raster/Lines.h"
#include "scene/Voxel.h"
#include "ui/Hud.h"

using namespace std;

// --- Global World Data ---
VoxelWorld world(15, 13, 15);
bool meshDirty = true;
const char* SAVE_FILE = "world.vxw";

// Blocks that are popping in or shrinking away (CP2 easing).
struct BlockAnim {
    int x, y, z;
    Block block;
    float start;
    bool placing;
};
vector<BlockAnim> blockAnims;
const float PLACE_TIME = 0.25f, REMOVE_TIME = 0.2f;

// --- Global Player / Game State ---
Player player;
int selectedSlot = 0;
float timeOfDay = 0.12f;      // 0 = sunrise, 0.25 = noon, 0.5 = sunset
float daySpeed = 1.0f / 240;  // one full day every 4 minutes
float gameTime = 0.0f;
string toast;
float toastUntil = 0;
bool hudDirty = true;

int windowW = 1280, windowH = 720;
bool firstMouse = true;
double lastX = 640, lastY = 360;

void showToast(const string& text) {
    toast = text;
    toastUntil = gameTime + 2.0f;
    hudDirty = true;
}

bool isAnimating(int x, int y, int z) {
    for (const BlockAnim& a : blockAnims)
        if (a.placing && a.x == x && a.y == y && a.z == z) return true;
    return false;
}

// --- Input Callbacks ---
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = float(xpos - lastX) * 0.1f;
    float yoffset = float(lastY - ypos) * 0.1f;
    lastX = xpos; lastY = ypos;
    player.look(xoffset, yoffset);
    hudDirty = true;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (action != GLFW_PRESS) return;
    RayHit hit = raycastVoxels(world, player.eye(), player.forward(), 7.0f);
    if (!hit.hit) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        Block old = world.get(hit.x, hit.y, hit.z);
        world.set(hit.x, hit.y, hit.z, Block::Air);
        blockAnims.push_back({hit.x, hit.y, hit.z, old, gameTime, false});
        meshDirty = hudDirty = true;
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        int px = hit.x + hit.nx, py = hit.y + hit.ny, pz = hit.z + hit.nz;
        bool inside = px >= 0 && py >= 0 && pz >= 0 && px < world.sizeX() && py < world.sizeY() && pz < world.sizeZ();
        Block there = world.get(px, py, pz);
        Block placing = HOTBAR_BLOCKS[selectedSlot];
        if (!inside || (there != Block::Air && there != Block::Water)) return;
        if (isOpaque(placing) && player.occupies(px, py, pz)) return;   // never place a block inside the player
        world.set(px, py, pz, placing);
        blockAnims.push_back({px, py, pz, placing, gameTime, true});
        meshDirty = hudDirty = true;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    selectedSlot = ((selectedSlot - int(yoffset)) % HOTBAR_SIZE + HOTBAR_SIZE) % HOTBAR_SIZE;
    hudDirty = true;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;
    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_7) { selectedSlot = key - GLFW_KEY_1; hudDirty = true; }
    if (key == GLFW_KEY_F) { player.flying = !player.flying; showToast(player.flying ? "FLY MODE" : "WALK MODE"); }
    if (key == GLFW_KEY_F5) showToast(world.save(SAVE_FILE) ? "WORLD SAVED" : "SAVE FAILED");
    if (key == GLFW_KEY_F9) {
        bool ok = world.load(SAVE_FILE);
        if (ok) { blockAnims.clear(); meshDirty = true; }
        showToast(ok ? "WORLD LOADED" : "NO SAVED WORLD");
    }
    if (key == GLFW_KEY_R) {
        world.clear(); world.generateTerrain(); blockAnims.clear(); meshDirty = true;
        showToast("NEW WORLD");
    }
}

void framebuffer_size_callback(GLFWwindow* window, int w, int h) {
    windowW = w; windowH = h;
    glViewport(0, 0, w, h);
}

void processInput(GLFWwindow *window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    PlayerInput in;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) in.forward += 1;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) in.forward -= 1;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) in.right += 1;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) in.right -= 1;
    in.jump = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    in.down = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    bool fastTime = glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS;
    timeOfDay += dt * daySpeed * (fastTime ? 30.0f : 1.0f);
    player.update(world, in, dt);
}

// --- 3D Shader Code ---
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec3 aColor;
    layout (location = 3) in vec2 aTexCoord;

    out vec3 FragPos;
    out vec3 Normal;
    out vec3 BaseColor;
    out vec2 TexCoord;

    uniform mat4 model;
    uniform mat4 viewProj;

    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;
        BaseColor = aColor;
        TexCoord = aTexCoord;
        gl_Position = viewProj * vec4(FragPos, 1.0);
    }
)";

// The texture supplies surface detail; the block colour (same palette as the
// pure C++ renderers) supplies the hue. Light colour and ambient come from the
// day/night cycle.
const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    in vec3 FragPos;
    in vec3 Normal;
    in vec3 BaseColor;
    in vec2 TexCoord;

    uniform sampler2D texture1;
    uniform vec3 toLight;
    uniform vec3 lightColor;
    uniform float ambient;
    uniform vec3 tint;
    uniform float alpha;
    uniform vec3 cameraPos;
    uniform vec3 fogColor;
    uniform float fogStart;
    uniform float fogEnd;

    void main() {
        vec3 norm = normalize(Normal);
        float diff = max(dot(norm, normalize(toLight)), 0.0);
        // Fixed per-face shading keeps faces distinguishable at night, like the isometric renderer.
        float faceShade = norm.y > 0.5 ? 1.0 : (norm.y < -0.5 ? 0.55 : (abs(norm.x) > 0.5 ? 0.8 : 0.68));
        float detail = dot(texture(texture1, TexCoord).rgb, vec3(0.299, 0.587, 0.114)) * 2.4;
        vec3 lit = BaseColor * tint * detail * faceShade * lightColor * (ambient + (1.0 - ambient) * diff);
        float fog = clamp((length(FragPos - cameraPos) - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
        FragColor = vec4(mix(lit, fogColor, fog), alpha);
    }
)";

// --- Sky Shader: gradient, sun, moon and stars from the view direction ---
const char* skyVertexShaderSource = R"(
    #version 330 core
    out vec2 ndc;
    void main() {
        vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2) * 2.0 - 1.0;
        ndc = p;
        gl_Position = vec4(p, 0.999, 1.0);
    }
)";

const char* skyFragmentShaderSource = R"(
    #version 330 core
    in vec2 ndc;
    out vec4 FragColor;
    uniform mat4 invViewProj;
    uniform vec3 skyTop;
    uniform vec3 skyBottom;
    uniform vec3 sunDir;
    uniform float starAlpha;
    void main() {
        vec4 nearP = invViewProj * vec4(ndc, -1.0, 1.0);
        vec4 farP = invViewProj * vec4(ndc, 1.0, 1.0);
        vec3 dir = normalize(farP.xyz / farP.w - nearP.xyz / nearP.w);
        vec3 color = mix(skyBottom, skyTop, smoothstep(-0.05, 0.6, dir.y));
        float s = dot(dir, normalize(sunDir));
        color += vec3(1.0, 0.85, 0.5) * pow(max(s, 0.0), 300.0) * 0.6;            // glow
        if (s > 0.9988) color = vec3(1.0, 0.88, 0.45);                            // sun disc
        if (dot(dir, -normalize(sunDir)) > 0.9993) color = vec3(0.92, 0.92, 0.86); // moon
        vec3 cell = floor(dir * 220.0);
        float h = fract(sin(dot(cell, vec3(12.9898, 78.233, 37.719))) * 43758.5453);
        if (h > 0.9975 && dir.y > 0.0) color = mix(color, vec3(1.0), starAlpha);
        FragColor = vec4(color, 1.0);
    }
)";

// --- Line Shader: selection box ---
const char* lineVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 viewProj;
    void main() { gl_Position = viewProj * vec4(aPos, 1.0); }
)";

const char* lineFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    uniform vec3 color;
    void main() { FragColor = vec4(color, 1.0); }
)";

// HUD drawn with the pure C++ raster algorithms.
void drawHud(HudOverlay& hud, float fps) {
    Canvas& c = hud.canvas();
    hud.clear();
    const int W = c.width(), H = c.height();
    drawCrosshair(c, W / 2, H / 2);
    drawHotbar(c, W / 2, H - 70, selectedSlot);
    Vec3 f = player.forward();
    drawMinimap(c, world, W - 160, 20, 9, player.position.x, player.position.z, std::atan2(f.z, f.x), false);

    drawShadowText(c, 16, 16, "VOXEL WORLD BUILDER", {255, 255, 255}, 3);
    drawShadowText(c, 16, 48, clockText(timeOfDay) + "   " + (player.flying ? "FLY" : "WALK") + "   FPS " +
                                  std::to_string(int(fps + 0.5f)), {255, 255, 255});
    RayHit hit = raycastVoxels(world, player.eye(), player.forward(), 7.0f);
    if (hit.hit)
        drawShadowText(c, 16, 70, std::string("LOOKING AT ") + blockName(world.get(hit.x, hit.y, hit.z)), {255, 230, 150});
    drawShadowText(c, 16, 96, "WASD MOVE  SPACE JUMP  F FLY  T TIME  1-7/WHEEL BLOCK  F5 SAVE  F9 LOAD  R RESET",
                   {225, 230, 240}, 1);
    if (gameTime < toastUntil) {
        int w = textWidth(toast, 3);
        drawShadowText(c, (W - w) / 2, H / 2 - 80, toast, {255, 220, 120}, 3);
    }
    hud.upload();
}

bool saveScreenshot(const string& path, int w, int h) {
    vector<unsigned char> px(static_cast<size_t>(w) * h * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());
    Canvas shot(w, h);
    for (int y = 0; y < h; ++y)   // OpenGL rows start at the bottom
        for (int x = 0; x < w; ++x) {
            const unsigned char* p = &px[(static_cast<size_t>(h - 1 - y) * w + x) * 3];
            shot.setPixel(x, y, {p[0], p[1], p[2]});
        }
    return shot.savePPM(path);
}

int main(int argc, char** argv) {
    string screenshotPath;
    int screenshotFrames = 10;
    for (int i = 1; i < argc; ++i) {
        string a = argv[i];
        if (a == "--time" && i + 1 < argc) timeOfDay = stof(argv[++i]);
        else if (a == "--pos" && i + 3 < argc) { player.position = {stof(argv[i + 1]), stof(argv[i + 2]), stof(argv[i + 3])}; i += 3; }
        else if (a == "--look" && i + 2 < argc) { player.yaw = stof(argv[i + 1]); player.pitch = stof(argv[i + 2]); i += 2; }
        else if (a == "--fly") player.flying = true;
        else if (a == "--slot" && i + 1 < argc) selectedSlot = stoi(argv[++i]) % HOTBAR_SIZE;
        else if (a == "--screenshot" && i + 1 < argc) screenshotPath = argv[++i];
        else if (a == "--frames" && i + 1 < argc) screenshotFrames = stoi(argv[++i]);
    }
    const bool capture = !screenshotPath.empty();

    if (!glfwInit()) { std::cout << "Failed to initialise GLFW" << std::endl; return 1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(windowW, windowH, "Voxel World Builder", NULL, NULL);
    if (!window) { std::cout << "Failed to create window (OpenGL 3.3 required)" << std::endl; glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    if (!capture) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwGetFramebufferSize(window, &windowW, &windowH);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { std::cout << "Failed to initialise GLEW" << std::endl; return 1; }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);   // faces are wound counter-clockwise from outside
    glEnable(GL_MULTISAMPLE);

    // Init world: same generated terrain as the pure C++ renderers.
    world.generateTerrain();
    WorldMesh mesh;

    // Compile Shaders
    unsigned int shaderProgram = compileProgram(vertexShaderSource, fragmentShaderSource);
    unsigned int skyProgram = compileProgram(skyVertexShaderSource, skyFragmentShaderSource);
    unsigned int lineProgram = compileProgram(lineVertexShaderSource, lineFragmentShaderSource);
    glUseProgram(shaderProgram);
    setUniform(shaderProgram, "texture1", 0);

    Mesh cube;   // used for animated blocks
    cube.upload(unitCubeVertices());
    unsigned int skyVAO;
    glGenVertexArrays(1, &skyVAO);

    // Selection box: the 12 edges of a slightly enlarged cube.
    unsigned int lineVAO, lineVBO;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    {
        vector<float> edges;
        const float lo = -0.003f, hi = 1.003f;
        for (int a = 0; a < 8; ++a)
            for (int bit = 0; bit < 3; ++bit) {
                int b = a | (1 << bit);
                if (b == a) continue;
                for (int k : {a, b}) {
                    edges.push_back(k & 1 ? hi : lo);
                    edges.push_back(k & 2 ? hi : lo);
                    edges.push_back(k & 4 ? hi : lo);
                }
            }
        glBufferData(GL_ARRAY_BUFFER, edges.size() * sizeof(float), edges.data(), GL_STATIC_DRAW);
    }
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

   // --- Texture Loading ---
    unsigned int texture1;
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    // Works when run from the project root or from bin/.
    unsigned char *data = stbi_load("assets/texture.png", &width, &height, &nrChannels, 4);
    if (!data) data = stbi_load("../assets/texture.png", &width, &height, &nrChannels, 4);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        // Fall back to a plain light-grey texture so blocks still show their colours.
        std::cout << "Failed to load texture. Reason: " << stbi_failure_reason() << std::endl;
        const unsigned char grey[4] = {106, 106, 106, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, grey);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);

    // --- 2D UI Setup: drawn by our own raster code into a texture ---
    HudOverlay hud(windowW, windowH);
    float hudTimer = 0, fps = 60, lastFrame = float(glfwGetTime());

    // --- Render Loop ---
    for (int frame = 0; !glfwWindowShouldClose(window); ++frame) {
        float currentFrame = float(glfwGetTime());
        float deltaTime = capture ? 1.0f / 60 : currentFrame - lastFrame;
        lastFrame = currentFrame;
        gameTime += deltaTime;
        fps = fps * 0.95f + (deltaTime > 0 ? 1.0f / deltaTime : 60.0f) * 0.05f;

        processInput(window, deltaTime);

        // Finished animations: placed blocks join the mesh again.
        for (size_t i = 0; i < blockAnims.size();) {
            float duration = blockAnims[i].placing ? PLACE_TIME : REMOVE_TIME;
            if (gameTime - blockAnims[i].start >= duration) {
                if (blockAnims[i].placing) meshDirty = true;
                blockAnims.erase(blockAnims.begin() + i);
            } else {
                ++i;
            }
        }
        if (meshDirty) { mesh.rebuild(world, isAnimating); meshDirty = false; }

        DayNight dn = dayNight(timeOfDay);
        Vec3 eye = player.eye();
        Mat4 view = Mat4::lookAt(eye, eye + player.forward(), {0, 1, 0});
        Mat4 projection = Mat4::perspective(radians(70.0f), float(windowW) / std::max(windowH, 1), 0.05f, 200.0f);
        Mat4 viewProj = projection * view;

        glClearColor(dn.skyBottom.r / 255.0f, dn.skyBottom.g / 255.0f, dn.skyBottom.b / 255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- 1. Sky ---
        glDepthMask(GL_FALSE);
        glUseProgram(skyProgram);
        setUniform(skyProgram, "invViewProj", viewProj.inverse());
        setUniform(skyProgram, "skyTop", dn.skyTop);
        setUniform(skyProgram, "skyBottom", dn.skyBottom);
        setUniform(skyProgram, "sunDir", dn.sunDir);
        setUniform(skyProgram, "starAlpha", dn.starAlpha);
        glBindVertexArray(skyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDepthMask(GL_TRUE);

        // --- 2. Draw 3D World ---
        glUseProgram(shaderProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        setUniform(shaderProgram, "viewProj", viewProj);
        setUniform(shaderProgram, "model", Mat4::identity());
        // At night the moon (opposite the sun) lights the world weakly.
        setUniform(shaderProgram, "toLight", dn.sunDir.y > -0.1f ? dn.sunDir : dn.sunDir * -1.0f);
        setUniform(shaderProgram, "lightColor", dn.lightTint);
        setUniform(shaderProgram, "ambient", dn.ambient);
        setUniform(shaderProgram, "cameraPos", eye);
        setUniform(shaderProgram, "fogColor", dn.skyBottom);
        setUniform(shaderProgram, "fogStart", 25.0f);
        setUniform(shaderProgram, "fogEnd", 70.0f);
        setUniform(shaderProgram, "tint", Vec3{1, 1, 1});
        setUniform(shaderProgram, "alpha", 1.0f);
        mesh.drawOpaque();

        // Animated blocks: model = T(centre) * R * S * unit cube (CP2 transformations).
        for (const BlockAnim& a : blockAnims) {
            float p = progress(gameTime, a.start, a.placing ? PLACE_TIME : REMOVE_TIME);
            float s = a.placing ? ease::outBack(p) : 1.0f - ease::inQuad(p);
            float spin = a.placing ? 0.0f : p * PI / 2;
            Mat4 model = Mat4::translate({a.x + 0.5f, a.y + 0.5f, a.z + 0.5f}) * Mat4::rotateY(spin) *
                         Mat4::scale({s, s, s});
            setUniform(shaderProgram, "model", model);
            Color c = blockBaseColor(a.block);
            setUniform(shaderProgram, "tint", Vec3{c.r / 255.0f, c.g / 255.0f, c.b / 255.0f});
            cube.draw();
        }
        setUniform(shaderProgram, "model", Mat4::identity());
        setUniform(shaderProgram, "tint", Vec3{1, 1, 1});

        // Water last, blended, without writing depth.
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        setUniform(shaderProgram, "alpha", 0.65f);
        mesh.drawWater();
        glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        // --- 3. Selection box on the block under the crosshair ---
        RayHit hit = raycastVoxels(world, eye, player.forward(), 7.0f);
        if (hit.hit) {
            glUseProgram(lineProgram);
            setUniform(lineProgram, "viewProj", viewProj * Mat4::translate({float(hit.x), float(hit.y), float(hit.z)}));
            setUniform(lineProgram, "color", Vec3{0.08f, 0.08f, 0.1f});
            glBindVertexArray(lineVAO);
            glDrawArrays(GL_LINES, 0, 24);
        }

        // --- 4. Draw 2D UI (redrawn a few times per second or when something changes) ---
        hudTimer -= deltaTime;
        if (hudDirty || hudTimer <= 0 || capture) {
            drawHud(hud, fps);
            hudDirty = false;
            hudTimer = 0.25f;
        }
        hud.draw();

        if (capture && frame + 1 >= screenshotFrames) {
            bool ok = saveScreenshot(screenshotPath, windowW, windowH);
            std::cout << (ok ? "saved  " : "FAILED ") << screenshotPath << std::endl;
            break;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
