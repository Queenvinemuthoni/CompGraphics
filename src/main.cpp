#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <limits>
#include <algorithm>

using namespace std;

// --- Data Structures ---
struct Color { int r, g, b; };
struct Vec3 { float x, y, z; };
struct Point2D { int x, y; };

enum BlockType { AIR = 0, GRASS = 1, DIRT = 2, STONE = 3 };

Color getBlockColor(BlockType type) {
    switch (type) {
        case GRASS: return {70, 200, 70};
        case DIRT:  return {139, 69, 19};
        case STONE: return {150, 150, 150};
        default:    return {0, 0, 0};
    }
}

// Struct to group triangles by face and define which direction that face points
struct FaceDef {
    int dirX, dirY, dirZ; // The neighbor coordinate offset in the 3D array
    int t1[3];            // Triangle 1 indices
    int t2[3];            // Triangle 2 indices
};

const float FOV_FACTOR = 400.0f;

Point2D project3DTo2D(Vec3 point, int screenWidth, int screenHeight) {
    Point2D projected;
    float z = (point.z == 0.0f) ? 0.1f : point.z; 
    projected.x = static_cast<int>((point.x * FOV_FACTOR) / z) + (screenWidth / 2);
    projected.y = static_cast<int>((point.y * FOV_FACTOR) / z) + (screenHeight / 2);
    return projected;
}

// 3D Transformations
Vec3 translate3D(Vec3 p, float tx, float ty, float tz) { return {p.x + tx, p.y + ty, p.z + tz}; }
Vec3 rotateY(Vec3 p, float angle) { float s = sin(angle), c = cos(angle); return {p.x * c + p.z * s, p.y, -p.x * s + p.z * c}; }
Vec3 rotateX(Vec3 p, float angle) { float s = sin(angle), c = cos(angle); return {p.x, p.y * c - p.z * s, p.y * s + p.z * c}; }

// Vector Math
Vec3 sub(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 crossProduct(Vec3 a, Vec3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
float dotProduct(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec3 normalize(Vec3 v) { float len = sqrt(v.x * v.x + v.y * v.y + v.z * v.z); return len == 0 ? Vec3{0,0,0} : Vec3{v.x/len, v.y/len, v.z/len}; }

// --- Graphics Engine ---
class Canvas {
private:
    int width, height;
    vector<Color> pixels;
    vector<float> zBuffer;

public:
    Canvas(int w, int h) : width(w), height(h), pixels(w * h, {30, 30, 40}), zBuffer(w * h, numeric_limits<float>::max()) {}

    void setPixel(int x, int y, float z, Color c) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            int index = y * width + x;
            if (z < zBuffer[index]) {
                zBuffer[index] = z;
                pixels[index] = c;
            }
        }
    }

    float edgeFunction(const Point2D& a, const Point2D& b, const Point2D& c) {
        return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
    }

    void drawTriangle(Point2D p0, float z0, Point2D p1, float z1, Point2D p2, float z2, Color c) {
        int minX = max(0, min({p0.x, p1.x, p2.x}));
        int maxX = min(width - 1, max({p0.x, p1.x, p2.x}));
        int minY = max(0, min({p0.y, p1.y, p2.y}));
        int maxY = min(height - 1, max({p0.y, p1.y, p2.y}));

        float area = edgeFunction(p0, p1, p2);
        if (area == 0) return;

        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                Point2D p = {x, y};
                float w0 = edgeFunction(p1, p2, p), w1 = edgeFunction(p2, p0, p), w2 = edgeFunction(p0, p1, p);

                if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                    w0 /= area; w1 /= area; w2 /= area;
                    float z = w0 * z0 + w1 * z1 + w2 * z2;
                    setPixel(x, y, z, c);
                }
            }
        }
    }

    void save(const string& filename) {
        ofstream out(filename);
        out << "P3\n" << width << " " << height << "\n255\n";
        for (const auto& p : pixels) out << p.r << " " << p.g << " " << p.b << "\n";
        cout << "Final Render saved to " << filename << endl;
    }
};

int main() {
    int screenWidth = 800, screenHeight = 600;
    Canvas canvas(screenWidth, screenHeight);
    
    vector<Vec3> baseVoxel = {
        {-1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f}, {-1.0f,  1.0f, -1.0f},
        {-1.0f, -1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f} 
    };

    // Grouping the triangles into faces mapped to their grid directions
    vector<FaceDef> voxelFaces = {
        { 0,  0, -1, {0, 2, 1}, {0, 3, 2} }, // Front Face (Points to Z-1)
        { 1,  0,  0, {1, 6, 5}, {1, 2, 6} }, // Right Face (Points to X+1)
        { 0,  0,  1, {5, 7, 4}, {5, 6, 7} }, // Back Face (Points to Z+1)
        {-1,  0,  0, {4, 3, 0}, {4, 7, 3} }, // Left Face (Points to X-1)
        { 0,  1,  0, {3, 6, 2}, {3, 7, 6} }, // Top Face (Points to Y+1)
        { 0, -1,  0, {4, 0, 1}, {4, 1, 5} }  // Bottom Face (Points to Y-1)
    };

    // Y is now correctly the first index for vertical stacking
    BlockType world[3][3][3] = {
        // y=0 (Bottom)
        { {STONE, STONE, STONE}, {STONE, STONE, STONE}, {STONE, STONE, STONE} },
        // y=1 (Middle)
        { {DIRT, DIRT, DIRT}, {DIRT, AIR, DIRT}, {DIRT, DIRT, DIRT} },
        // y=2 (Top)
        { {GRASS, AIR, GRASS}, {AIR, AIR, AIR}, {GRASS, AIR, GRASS} }
    };

    Vec3 lightDir = normalize({-1.0f, -1.0f, 1.0f}); 
    float cameraAngleY = 0.5f, cameraAngleX = 0.4f; 

    // Loop through Y (height) first, then X and Z
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            for (int z = 0; z < 3; z++) {
                
                if (world[y][x][z] == AIR) continue;

                Color baseColor = getBlockColor(world[y][x][z]);
                float offsetX = (x - 1) * 2.0f;
                float offsetY = -(y - 1) * 2.0f; 
                float offsetZ = z * 2.0f + 7.0f;

                // Loop through the 6 Faces of the cube
                for (const auto& face : voxelFaces) {
                    
                    // OCCLUSION CULLING: Check the grid neighbor for this specific face
                    int nx = x + face.dirX;
                    int ny = y + face.dirY;
                    int nz = z + face.dirZ;

                    // Note the array lookup matches world[y][x][z]
                    bool neighborIsSolid = (nx >= 0 && nx < 3 && ny >= 0 && ny < 3 && nz >= 0 && nz < 3) 
                                           && (world[ny][nx][nz] != AIR);
                    
                    if (neighborIsSolid) continue; // Skip rendering this hidden face!

                    // If the face is exposed, draw its two triangles
                    const int* faceTriangles[2] = { face.t1, face.t2 };

                    for (int i = 0; i < 2; i++) {
                        Vec3 v0 = baseVoxel[faceTriangles[i][0]];
                        Vec3 v1 = baseVoxel[faceTriangles[i][1]];
                        Vec3 v2 = baseVoxel[faceTriangles[i][2]];

                        v0 = translate3D(v0, offsetX, offsetY, 0.0f);
                        v1 = translate3D(v1, offsetX, offsetY, 0.0f);
                        v2 = translate3D(v2, offsetX, offsetY, 0.0f);

                        v0 = rotateX(rotateY(v0, cameraAngleY), cameraAngleX);
                        v1 = rotateX(rotateY(v1, cameraAngleY), cameraAngleX);
                        v2 = rotateX(rotateY(v2, cameraAngleY), cameraAngleX);

                        v0.z += offsetZ; v1.z += offsetZ; v2.z += offsetZ;

                        Vec3 edge1 = sub(v1, v0);
                        Vec3 edge2 = sub(v2, v0);
                        Vec3 normal = normalize(crossProduct(edge1, edge2));

                        Vec3 viewDir = sub({0.0f, 0.0f, 0.0f}, v0); 
                        if (dotProduct(normal, viewDir) <= 0.0f) continue; // Backface culling

                        Vec3 toLight = {-lightDir.x, -lightDir.y, -lightDir.z};
                        float diffuse = max(0.0f, dotProduct(normal, toLight));
                        float intensity = min(1.0f, 0.3f + diffuse);

                        Color litColor = {
                            static_cast<int>(baseColor.r * intensity),
                            static_cast<int>(baseColor.g * intensity),
                            static_cast<int>(baseColor.b * intensity)
                        };

                        Point2D p0 = project3DTo2D(v0, screenWidth, screenHeight);
                        Point2D p1 = project3DTo2D(v1, screenWidth, screenHeight);
                        Point2D p2 = project3DTo2D(v2, screenWidth, screenHeight);

                        canvas.drawTriangle(p0, v0.z, p1, v1.z, p2, v2.z, litColor);
                    }
                }
            }
        }
    }

    canvas.save("../outputs/phase6_final_culled.ppm");
    return 0;
}