#include "engine/Player.h"

#include <algorithm>
#include <cmath>

Vec3 Player::forward() const {
    return normalize({std::cos(radians(yaw)) * std::cos(radians(pitch)), std::sin(radians(pitch)),
                      std::sin(radians(yaw)) * std::cos(radians(pitch))});
}

void Player::look(float dYaw, float dPitch) {
    yaw += dYaw;
    pitch = std::clamp(pitch + dPitch, -89.0f, 89.0f);
}

bool Player::collides(const VoxelWorld& world, Vec3 p) const {
    int x0 = int(std::floor(p.x - HALF_WIDTH)), x1 = int(std::floor(p.x + HALF_WIDTH));
    int y0 = int(std::floor(p.y)), y1 = int(std::floor(p.y + HEIGHT));
    int z0 = int(std::floor(p.z - HALF_WIDTH)), z1 = int(std::floor(p.z + HALF_WIDTH));
    for (int x = x0; x <= x1; ++x)
        for (int y = y0; y <= y1; ++y)
            for (int z = z0; z <= z1; ++z)
                if (isOpaque(world.get(x, y, z))) return true;
    return false;
}

bool Player::occupies(int x, int y, int z) const {
    return x + 1 > position.x - HALF_WIDTH && x < position.x + HALF_WIDTH &&
           y + 1 > position.y && y < position.y + HEIGHT &&
           z + 1 > position.z - HALF_WIDTH && z < position.z + HALF_WIDTH;
}

void Player::update(const VoxelWorld& world, const PlayerInput& in, float dt) {
    dt = std::min(dt, 0.05f);   // avoid tunnelling after a long frame
    // Horizontal movement follows the view direction projected onto the ground.
    Vec3 f = normalize({std::cos(radians(yaw)), 0, std::sin(radians(yaw))});
    Vec3 r = normalize(cross(f, {0, 1, 0}));
    Vec3 wish = f * in.forward + r * in.right;
    if (length(wish) > 1) wish = normalize(wish);
    const float speed = flying ? 9.0f : 4.5f;
    velocity_.x = wish.x * speed;
    velocity_.z = wish.z * speed;

    if (flying) {
        velocity_.y = (in.jump ? speed : 0.0f) - (in.down ? speed : 0.0f);
    } else {
        velocity_.y -= 22.0f * dt;                       // gravity
        if (in.jump && onGround_) velocity_.y = 7.5f;    // jump
        velocity_.y = std::max(velocity_.y, -40.0f);
    }

    // Move one axis at a time; if that axis collides, undo it and stop on that axis.
    onGround_ = false;
    Vec3 p = position;
    p.x += velocity_.x * dt;
    if (collides(world, p)) { p.x = position.x; velocity_.x = 0; }
    p.z += velocity_.z * dt;
    if (collides(world, p)) { p.z = position.z; velocity_.z = 0; }
    float oldY = p.y;
    p.y += velocity_.y * dt;
    if (collides(world, p)) {
        if (velocity_.y < 0) onGround_ = true;
        p.y = velocity_.y < 0 ? std::floor(oldY + 1e-3f) : oldY;   // land on the block surface
        if (collides(world, p)) p.y = oldY;
        velocity_.y = 0;
    }
    position = p;

    if (position.y < -30) {   // fell off the island: respawn above the middle
        position = {world.sizeX() / 2.0f + 0.5f, float(world.sizeY()), world.sizeZ() / 2.0f + 0.5f};
        velocity_ = {};
    }
}
