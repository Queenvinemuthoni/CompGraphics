#pragma once
// First-person player: mouse-look camera, walking with gravity and jumping,
// axis-by-axis collision against solid blocks, and a fly mode.
#include "math/Transform.h"
#include "scene/Voxel.h"

struct PlayerInput {
    float forward = 0, right = 0;   // -1..1 from WASD
    bool jump = false;              // space
    bool down = false;              // shift (fly mode)
};

class Player {
public:
    Vec3 position{7.5f, 12.0f, 20.0f};   // feet, world units (1 block = 1 unit)
    float yaw = -90.0f, pitch = -25.0f;  // degrees
    bool flying = false;

    Vec3 eye() const { return position + Vec3{0, EYE_HEIGHT, 0}; }
    Vec3 forward() const;
    void look(float dYaw, float dPitch);
    void update(const VoxelWorld& world, const PlayerInput& input, float dt);
    // True if a block placed at (x, y, z) would overlap the player.
    bool occupies(int x, int y, int z) const;

    static constexpr float EYE_HEIGHT = 1.6f, HEIGHT = 1.8f, HALF_WIDTH = 0.3f;

private:
    bool collides(const VoxelWorld& world, Vec3 feet) const;
    Vec3 velocity_{};
    bool onGround_ = false;
};
