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

const float FOV_FACTOR = 400.0f;

Point2D project3DTo2D(Vec3 point, int screenWidth, int screenHeight) {
    Point2D projected;
    float z = (point.z == 0.0f) ? 0.1f : point.z; 
    projected.x = static_cast<int>((point.x * FOV_FACTOR) / z) + (screenWidth / 2);
    projected.y = static_cast<int>((point.y * FOV_FACTOR) / z) + (screenHeight / 2);
    return projected;
}

// 3D Transformations
Vec3 translate3D(Vec3 p, float tx, float ty, float tz) {
    return {p.x + tx, p.y + ty, p.z + tz};
}
Vec3 rotateY(Vec3 p, float angle) {
    float s = sin(angle), c = cos(angle);
    return {p.x * c + p.z * s, p.y, -p.x * s + p.z * c};
}
Vec3 rotateX(Vec3 p, float angle) {
    float s = sin(angle), c = cos(angle);
    return {p.x, p.y * c - p.z * s, p.y * s + p.z * c};
}
// --- Vector Math Utilities ---

// Subtracts two vectors (a - b) to find a direction
Vec3 sub(Vec3 a, Vec3 b) { 
    return {a.x - b.x, a.y - b.y, a.z - b.z}; 
}

// Cross Product: Finds a vector perpendicular to two other vectors (Surface Normal)
Vec3 crossProduct(Vec3 a, Vec3 b) {
    return {
        a.y * b.z - a.z * b.y, 
        a.z * b.x - a.x * b.z, 
        a.x * b.y - a.y * b.x
    };
}

// Dot Product: Finds the angle difference between two vectors (Lighting & Culling)
float dotProduct(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Normalize: Scales a vector to a length of exactly 1.0
Vec3 normalize(Vec3 v) {
    float length = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (length == 0) return {0, 0, 0};
    return {v.x / length, v.y / length, v.z / length};
}

// --- Graphics Engine ---
class Canvas {
private:
    int width, height;
    vector<Color> pixels;
    vector<float> zBuffer; // 1D Array to store depth of every pixel

public:
    Canvas(int w, int h) : width(w), height(h), 
                           pixels(w * h, {30, 30, 40}), 
                           zBuffer(w * h, numeric_limits<float>::max()) {} // Initialize Z-buffer to infinity

    void setPixel(int x, int y, float z, Color c) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            int index = y * width + x;
            // DEPTH TEST: Only draw if this pixel is closer than what's already there
            if (z < zBuffer[index]) {
                zBuffer[index] = z;
                pixels[index] = c;
            }
        }
    }

    // Helper math for Triangle Filling
    float edgeFunction(const Point2D& a, const Point2D& b, const Point2D& c) {
        return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
    }

    // Solid Polygon Filling with Z-Buffering
    void drawTriangle(Point2D p0, float z0, Point2D p1, float z1, Point2D p2, float z2, Color c) {
        // Find the bounding box of the triangle
        int minX = max(0, min({p0.x, p1.x, p2.x}));
        int maxX = min(width - 1, max({p0.x, p1.x, p2.x}));
        int minY = max(0, min({p0.y, p1.y, p2.y}));
        int maxY = min(height - 1, max({p0.y, p1.y, p2.y}));

        float area = edgeFunction(p0, p1, p2);
        if (area == 0) return; // Prevent divide by zero for zero-area triangles

        // Scan every pixel in the bounding box
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                Point2D p = {x, y};
                
                // Calculate Barycentric coordinates
                float w0 = edgeFunction(p1, p2, p);
                float w1 = edgeFunction(p2, p0, p);
                float w2 = edgeFunction(p0, p1, p);

                // If pixel is inside the triangle (all weights are positive or all negative)
                if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                    w0 /= area; w1 /= area; w2 /= area;
                    
                    // Interpolate the Z-depth for this exact pixel
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
        cout << "Solid 3D Image saved to " << filename << endl;
    }
};

// --- Voxel Data Structures ---

enum BlockType {
    AIR = 0,
    GRASS = 1,
    DIRT = 2,
    STONE = 3
};

// Maps a BlockType to its specific RGB base color
Color getBlockColor(BlockType type) {
    switch (type) {
        case GRASS: return {70, 200, 70};   // Vibrant Green
        case DIRT:  return {139, 69, 19};   // Brown
        case STONE: return {150, 150, 150}; // Grey
        default:    return {0, 0, 0};       // Fallback
    }
}

int main() {
    int screenWidth = 800, screenHeight = 600;
    Canvas canvas(screenWidth, screenHeight);
    
    vector<Vec3> baseVoxel = {
        {-1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f}, {-1.0f,  1.0f, -1.0f},
        {-1.0f, -1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f} 
    };

    vector<vector<int>> triangles = {
        {0, 2, 1}, {0, 3, 2}, // Front
        {1, 6, 5}, {1, 2, 6}, // Right
        {5, 7, 4}, {5, 6, 7}, // Back
        {4, 3, 0}, {4, 7, 3}, // Left
        {3, 6, 2}, {3, 7, 6}, // Top
        {4, 0, 1}, {4, 1, 5}  // Bottom
    };

    Vec3 lightDir = normalize({-1.0f, -1.0f, 1.0f}); 
    float cameraAngleY = 0.5f; 
    float cameraAngleX = 0.4f; 

    // --- NEW: The World Data Structure ---
    // A 3x3x3 grid [x][y][z]. Y is height.
    BlockType world[3][3][3] = {
        // Bottom Layer (y=0): All Stone
        { {STONE, STONE, STONE}, {STONE, STONE, STONE}, {STONE, STONE, STONE} },
        
        // Middle Layer (y=1): Dirt with a hole in the middle
        { {DIRT, DIRT, DIRT}, {DIRT, AIR, DIRT}, {DIRT, DIRT, DIRT} },
        
        // Top Layer (y=2): Some Grass blocks scattered on top
        { {GRASS, AIR, GRASS}, {AIR, AIR, AIR}, {GRASS, AIR, GRASS} }
    };

    // --- NEW: Iterating through the World Data ---
    // Loop through X (width), Y (height), and Z (depth)
    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 3; y++) {
            for (int z = 0; z < 3; z++) {
                
                // Fetch the block type from our array
                BlockType currentBlock = world[x][y][z];
                
                // Skip rendering if there is no block here
                if (currentBlock == AIR) continue;

                // Get the specific color for this block type
                Color baseColor = getBlockColor(currentBlock);

                // Calculate world position offsets based on array indices
                // Multiply by 2.0f because our base voxel is 2 units wide (-1 to 1)
                float offsetX = (x - 1) * 2.0f;       // Center the grid around X=0
                float offsetY = (y - 1) * 2.0f;       // Center the grid around Y=0
                float offsetZ = z * 2.0f + 7.0f;      // Push away from camera into the screen

                for (size_t i = 0; i < triangles.size(); i++) {
                    Vec3 v0 = baseVoxel[triangles[i][0]];
                    Vec3 v1 = baseVoxel[triangles[i][1]];
                    Vec3 v2 = baseVoxel[triangles[i][2]];

                    // 1. Translate (Move block to its array coordinate)
                    v0 = translate3D(v0, offsetX, offsetY, 0.0f);
                    v1 = translate3D(v1, offsetX, offsetY, 0.0f);
                    v2 = translate3D(v2, offsetX, offsetY, 0.0f);

                    // 2. Rotate (Camera view)
                    v0 = rotateX(rotateY(v0, cameraAngleY), cameraAngleX);
                    v1 = rotateX(rotateY(v1, cameraAngleY), cameraAngleX);
                    v2 = rotateX(rotateY(v2, cameraAngleY), cameraAngleX);

                    // 3. Push Depth
                    v0.z += offsetZ; v1.z += offsetZ; v2.z += offsetZ;

                    // Backface Culling & Lighting
                    Vec3 edge1 = sub(v1, v0);
                    Vec3 edge2 = sub(v2, v0);
                    Vec3 normal = normalize(crossProduct(edge1, edge2));

                    Vec3 viewDir = sub({0.0f, 0.0f, 0.0f}, v0); 
                    if (dotProduct(normal, viewDir) <= 0.0f) continue; 

                    Vec3 toLight = {-lightDir.x, -lightDir.y, -lightDir.z};
                    float diffuse = max(0.0f, dotProduct(normal, toLight));
                    float ambient = 0.3f; 
                    float intensity = min(1.0f, ambient + diffuse);

                    Color litColor = {
                        static_cast<int>(baseColor.r * intensity),
                        static_cast<int>(baseColor.g * intensity),
                        static_cast<int>(baseColor.b * intensity)
                    };

                    // 4. Project and Draw
                    Point2D p0 = project3DTo2D(v0, screenWidth, screenHeight);
                    Point2D p1 = project3DTo2D(v1, screenWidth, screenHeight);
                    Point2D p2 = project3DTo2D(v2, screenWidth, screenHeight);

                    canvas.drawTriangle(p0, v0.z, p1, v1.z, p2, v2.z, litColor);
                }
            }
        }
    }

    canvas.save("../outputs/phase5_terrain_render.ppm");
    return 0;
}