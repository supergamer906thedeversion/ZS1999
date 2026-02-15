#include <cmath>
#include <iostream>

struct InputState {
    bool w = false;
    bool a = false;
    bool s = false;
    bool d = false;

    // Expansion 1: Hold shift to sprint.
    bool sprint = false;

    // Expansion 2: Press space to dash in movement direction.
    bool dashPressed = false;
};

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2 operator+(const Vec2& rhs) const { return {x + rhs.x, y + rhs.y}; }
    Vec2 operator-(const Vec2& rhs) const { return {x - rhs.x, y - rhs.y}; }
    Vec2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
};

class PlayerMovement {
public:
    explicit PlayerMovement(Vec2 spawn = {}) : position(spawn) {}

    void Update(float deltaTime, const InputState& input) {
        Vec2 rawDirection = {
            (input.d ? 1.0f : 0.0f) - (input.a ? 1.0f : 0.0f),
            (input.w ? 1.0f : 0.0f) - (input.s ? 1.0f : 0.0f),
        };

        Vec2 moveDirection = Normalize(rawDirection);

        float targetSpeed = input.sprint ? sprintSpeed : walkSpeed;
        Vec2 targetVelocity = moveDirection * targetSpeed;

        // Expansion 3: Smooth acceleration/deceleration instead of instant velocity changes.
        velocity = MoveToward(velocity, targetVelocity, acceleration * deltaTime);

        // Dash logic (cooldown + instant burst in current movement direction).
        if (dashCooldownTimer > 0.0f) {
            dashCooldownTimer -= deltaTime;
        }

        if (input.dashPressed && dashCooldownTimer <= 0.0f && (moveDirection.x != 0.0f || moveDirection.y != 0.0f)) {
            velocity = velocity + moveDirection * dashBoost;
            dashCooldownTimer = dashCooldown;
        }

        position = position + velocity * deltaTime;
    }

    Vec2 GetPosition() const { return position; }
    Vec2 GetVelocity() const { return velocity; }

private:
    static float Length(const Vec2& v) {
        return std::sqrt((v.x * v.x) + (v.y * v.y));
    }

    static Vec2 Normalize(const Vec2& v) {
        float len = Length(v);
        if (len <= 0.0001f) {
            return {};
        }
        return {v.x / len, v.y / len};
    }

    static Vec2 MoveToward(const Vec2& current, const Vec2& target, float maxDelta) {
        Vec2 delta = target - current;
        float deltaLen = Length(delta);

        if (deltaLen <= maxDelta || deltaLen <= 0.0001f) {
            return target;
        }

        Vec2 step = Normalize(delta) * maxDelta;
        return current + step;
    }

private:
    Vec2 position{};
    Vec2 velocity{};

    float walkSpeed = 4.5f;
    float sprintSpeed = 7.0f;
    float acceleration = 18.0f;

    float dashBoost = 10.0f;
    float dashCooldown = 0.8f;
    float dashCooldownTimer = 0.0f;
};

int main() {
    PlayerMovement player({0.0f, 0.0f});

    // Tiny simulation loop for demonstration purposes.
    // In a real game, replace this with your engine's per-frame input + delta time.
    const float dt = 1.0f / 60.0f;

    for (int frame = 0; frame < 240; ++frame) {
        InputState input;

        // Move forward for 2 seconds, then strafe right while sprinting.
        if (frame < 120) {
            input.w = true;
        } else {
            input.d = true;
            input.sprint = true;
        }

        // Dash once when sprinting starts.
        if (frame == 120) {
            input.dashPressed = true;
        }

        player.Update(dt, input);

        if (frame % 30 == 0) {
            Vec2 pos = player.GetPosition();
            Vec2 vel = player.GetVelocity();
            std::cout << "Frame " << frame
                      << " | Pos(" << pos.x << ", " << pos.y << ")"
                      << " | Vel(" << vel.x << ", " << vel.y << ")\n";
        }
    }

    return 0;
}
