#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct InputState {
    bool w = false;
    bool a = false;
    bool s = false;
    bool d = false;
    bool sprint = false;
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
        velocity = MoveToward(velocity, targetVelocity, acceleration * deltaTime);

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

struct LobbyPlayer {
    int id = -1;
    std::string name;
    bool ready = false;
};

class Lobby {
public:
    Lobby(std::size_t minPlayersToStart, std::size_t maxPlayers)
        : minPlayersToStart(minPlayersToStart), maxPlayers(maxPlayers) {}

    bool Join(int id, const std::string& name) {
        if (players.size() >= maxPlayers || FindPlayer(id) != players.end()) {
            return false;
        }
        players.push_back({id, name, false});
        return true;
    }

    bool Leave(int id) {
        auto it = FindPlayer(id);
        if (it == players.end()) {
            return false;
        }
        players.erase(it);
        return true;
    }

    bool SetReady(int id, bool ready) {
        auto it = FindPlayer(id);
        if (it == players.end()) {
            return false;
        }
        it->ready = ready;
        return true;
    }

    bool CanStartMatch() const {
        if (players.size() < minPlayersToStart) {
            return false;
        }
        return std::all_of(players.begin(), players.end(), [](const LobbyPlayer& p) { return p.ready; });
    }

    void PrintStatus() const {
        std::cout << "\n=== Lobby Status ===\n";
        std::cout << "Players: " << players.size() << "/" << maxPlayers << "\n";
        for (const auto& player : players) {
            std::cout << "- [" << (player.ready ? "Ready" : "Not Ready") << "] " << player.name << " (id " << player.id
                      << ")\n";
        }
        std::cout << "Match start: " << (CanStartMatch() ? "AVAILABLE" : "WAITING") << "\n";
    }

private:
    std::vector<LobbyPlayer>::iterator FindPlayer(int id) {
        return std::find_if(players.begin(), players.end(), [id](const LobbyPlayer& p) { return p.id == id; });
    }

    std::vector<LobbyPlayer> players;
    std::size_t minPlayersToStart;
    std::size_t maxPlayers;
};

struct RangedWeapon {
    std::string name;
    int killsRequired;
    float damage;
    float fireRateSeconds;
    int magazineSize;
    float reloadTimeSeconds;
    float headshotMultiplier;
    int spread;
    int extraHits;
    int range;
};

struct MeleeWeapon {
    std::string name;
    int killsRequired;
    float damage;
    float healthOnHit;
    float delaySeconds;
    float cooldownSeconds;
    int knockback;
    std::optional<float> throwingDamage;
};

struct Utility {
    std::string name;
    int killsRequired;
    std::string description;
};

struct PlayerProfile {
    int kills = 0;
    float health = 100.0f;
    float maxHealth = 100.0f;
    float walkSpeed = 24.0f;
    float runSpeed = 26.0f;
    float maxStamina = 100.0f;
    float meleeDelayMultiplier = 1.0f;
    float meleeCooldownMultiplier = 1.0f;
    float bulletSpreadMultiplier = 1.0f;
};

class WeaponSystem {
public:
    WeaponSystem() {
        rangedWeapons = {
            {"M1911 (Default)", 0, 18, 0.15f, 7, 1.6f, 1.5f, 1, 0, 200},
            {"MP41", 50, 8, 0.12f, 32, 2.2f, 1.5f, 3, 0, 140},
            {"M3", 75, 12, 0.6f, 7, 0.4f, 1.25f, 10, 0, 80},
            {"APS", 100, 10, 0.1f, 21, 2.0f, 1.5f, 3, 0, 160},
            {"Vector CP11", 125, 16, 0.18f, 10, 1.8f, 1.5f, 2, 0, 100},
            {"M1", 200, 20, 0.35f, 8, 0.24f, 1.5f, 1, 1, 300},
            {"MP5", 225, 7, 0.09f, 30, 1.8f, 1.5f, 5, 0, 100},
            {"Double Barrel", 250, 12, 0.4f, 1, 1.8f, 1.25f, 15, 1, 80},
            {"Saw-Off", 300, 26, 0.5f, 6, 2.7f, 1.75f, 2, 1, 120},
            {"M24 (0 to Scope)", 350, 35, 0.9f, 5, 3.0f, 2.0f, 0, 3, 400},
            {"M16", 400, 8, 0.1f, 40, 2.4f, 1.5f, 2, 0, 140},
            {"Desert Eagle", 500, 30, 0.4f, 7, 2.2f, 1.5f, 2, 0, 150},
            {"AK-47", 600, 12, 0.15f, 30, 2.1f, 1.5f, 1, 0, 140},
            {"Bekas-M-Auto", 625, 10, 0.85f, 4, 0.5f, 1.25f, 7, 0, 120},
            {"M9", 650, 15, 0.12f, 15, 2.6f, 1.5f, 2, 0, 240},
            {"SCAR-L", 700, 8, 0.08f, 30, 2.9f, 1.5f, 2, 0, 100},
            {"Judge", 750, 24, 0.3f, 5, 0.4f, 1.5f, 4, 1, 80},
            {"AUG (0 to Scope)", 775, 8.5f, 0.12f, 30, 3.0f, 1.5f, 2, 0, 180},
            {"Uzi", 800, 6, 0.07f, 50, 2.0f, 1.5f, 8, 0, 90},
            {"Flare Gun", 825, 40, 0.5f, 1, 0.85f, 1.0f, 1, 0, 150},
            {"SKS (0 to Scope)", 850, 20, 0.375f, 10, 2.8f, 1.5f, 1, 0, 250},
            {"FAMAS", 875, 9, 0.07f, 25, 2.6f, 1.5f, 4, 0, 100},
            {"S&W-500", 925, 32, 0.8f, 5, 2.3f, 1.75f, 1, 0, 160},
            {"SPAS-12", 950, 8.5f, 0.35f, 8, 0.4f, 1.25f, 12, 0, 100},
            {"AKS-74U", 975, 9, 0.11f, 20, 1.9f, 1.5f, 5, 0, 100},
            {"M2", 1000, 7, 0.1f, 100, 4.6f, 1.0f, 2, 4, 20},
        };

        meleeWeapons = {
            {"Bat", 25, 20, 10, 0.1f, 0.2f, 25, std::nullopt},
            {"Fire Axe", 150, 40, 13, 0.175f, 0.3f, 16, std::nullopt},
            {"Knife", 325, 9, 6, 0.055f, 0.065f, 0, std::nullopt},
            {"Golf Club", 450, 17.5f, 9, 0.1f, 0.15f, 20, std::nullopt},
            {"Baton", 550, 30, 11, 0.15f, 0.25f, 32, std::nullopt},
            {"Hatchet", 725, 18, 10, 0.08f, 0.18f, 10, 60.0f},
            {"Katana", 900, 14, 7, 0.08f, 0.08f, 10, std::nullopt},
        };

        utilities = {
            {"First Aid Kit", 25, "Restores up to 70% health."},
            {"Light Armor", 50, "HP to 120, 15 walkspeed, 26 runspeed."},
            {"Barbed Wire", 75, "Zombies take 20 damage upon contact. Lasts 75 uses."},
            {"Bloxiade", 100, "24 walk speed, 32 run speed, double max stamina for 15 seconds."},
            {"Grenade", 125, "400 damage in a radius of 10 studs."},
            {"Heavy Armor", 150, "HP to 150, 14 walkspeed, 24 runspeed."},
            {"Gloves", 175, "-25% melee delay and cooldown."},
            {"Defibrilator", 200, "Revives a dead teammate. If not playing, spawns an AI teammate."},
            {"Sneakers", 225, "You take 25% less stamina from running."},
            {"Radio", 250, "Spawns an AI teammate. Up to four AI teammates in one game."},
            {"Bandolier", 300, "Magazine +30%, reload time -15%. Movement slowed like Heavy Armor."},
            {"Bandage Basket", 350, "Spawns crate with 6 bandages. Each heals 40% of a player's health."},
            {"Molotov", 400, "24x24 studs fire area, 25 damage every 0.3 seconds for 15 seconds."},
            {"Experimental Tonic", 450, "Zombie damage reduced by 70% for 15 seconds."},
            {"Super Heavy Armor", 500, "HP to 190, 13 walk speed, 22 run speed, melee speed -30%."},
            {"Shotgun Trap", 550, "Ten pellets, each 18 damage, laser-triggered. Lasts 30 uses."},
            {"C4", 600, "600 damage in a 12.5 stud radius. Manual or 2-second auto detonation."},
            {"Flashbang", 650, "Stuns all zombies in a 30 stud radius for 10 seconds."},
            {"Marksman's Arm", 750, "-30% less bullet spread at the cost of -30 max health."},
            {"Bait Bot", 900, "Builds a lure robot. Explodes after 18 seconds or on destroy. 9-stud radius, 300 damage."},
        };
    }

    bool CanUse(const std::string& name, int kills) const {
        auto req = killsRequiredByName.find(name);
        if (req == killsRequiredByName.end()) {
            return false;
        }
        return kills >= req->second;
    }

    std::vector<std::string> UnlocksForKills(int kills) const {
        std::vector<std::string> unlocked;
        for (const auto& weapon : rangedWeapons) {
            if (kills >= weapon.killsRequired) {
                unlocked.push_back("Ranged: " + weapon.name);
            }
        }
        for (const auto& weapon : meleeWeapons) {
            if (kills >= weapon.killsRequired) {
                unlocked.push_back("Melee: " + weapon.name);
            }
        }
        for (const auto& utility : utilities) {
            if (kills >= utility.killsRequired) {
                unlocked.push_back("Utility: " + utility.name);
            }
        }
        return unlocked;
    }

    void ApplyUtility(const std::string& utilityName, PlayerProfile& profile) const {
        if (utilityName == "Light Armor") {
            profile.maxHealth = 120;
            profile.health = std::min(profile.health, profile.maxHealth);
            profile.walkSpeed = 15;
            profile.runSpeed = 26;
        } else if (utilityName == "Heavy Armor") {
            profile.maxHealth = 150;
            profile.health = std::min(profile.health, profile.maxHealth);
            profile.walkSpeed = 14;
            profile.runSpeed = 24;
        } else if (utilityName == "Gloves") {
            profile.meleeDelayMultiplier *= 0.75f;
            profile.meleeCooldownMultiplier *= 0.75f;
        } else if (utilityName == "Super Heavy Armor") {
            profile.maxHealth = 190;
            profile.health = std::min(profile.health, profile.maxHealth);
            profile.walkSpeed = 13;
            profile.runSpeed = 22;
            profile.meleeDelayMultiplier *= 1.30f;
            profile.meleeCooldownMultiplier *= 1.30f;
        } else if (utilityName == "Marksman's Arm") {
            profile.bulletSpreadMultiplier *= 0.70f;
            profile.maxHealth -= 30;
            profile.health = std::min(profile.health, profile.maxHealth);
        }
    }

    void BuildLookup() {
        killsRequiredByName.clear();
        for (const auto& weapon : rangedWeapons) {
            killsRequiredByName[weapon.name] = weapon.killsRequired;
        }
        for (const auto& weapon : meleeWeapons) {
            killsRequiredByName[weapon.name] = weapon.killsRequired;
        }
        for (const auto& utility : utilities) {
            killsRequiredByName[utility.name] = utility.killsRequired;
        }
    }

private:
    std::vector<RangedWeapon> rangedWeapons;
    std::vector<MeleeWeapon> meleeWeapons;
    std::vector<Utility> utilities;
    std::unordered_map<std::string, int> killsRequiredByName;
};

int main() {
    Lobby lobby(2, 4);
    lobby.Join(1, "Alice");
    lobby.Join(2, "Bob");
    lobby.SetReady(1, true);
    lobby.SetReady(2, true);
    lobby.PrintStatus();

    WeaponSystem weaponSystem;
    weaponSystem.BuildLookup();

    PlayerProfile alice;
    alice.kills = 760;

    std::cout << "\n=== Unlocks for Alice (" << alice.kills << " kills) ===\n";
    auto unlocks = weaponSystem.UnlocksForKills(alice.kills);
    std::cout << "Unlocked item count: " << unlocks.size() << "\n";
    for (std::size_t i = 0; i < std::min<std::size_t>(unlocks.size(), 8); ++i) {
        std::cout << "  * " << unlocks[i] << "\n";
    }

    std::cout << "\nCan Alice use AK-47? " << (weaponSystem.CanUse("AK-47", alice.kills) ? "Yes" : "No") << "\n";
    std::cout << "Can Alice use Bait Bot? " << (weaponSystem.CanUse("Bait Bot", alice.kills) ? "Yes" : "No") << "\n";

    weaponSystem.ApplyUtility("Marksman's Arm", alice);
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\nAfter Marksman's Arm -> maxHealth: " << alice.maxHealth
              << ", spreadMultiplier: " << alice.bulletSpreadMultiplier << "\n";

    PlayerMovement player({0.0f, 0.0f});
    const float dt = 1.0f / 60.0f;

    for (int frame = 0; frame < 180; ++frame) {
        InputState input;
        if (frame < 90) {
            input.w = true;
        } else {
            input.d = true;
            input.sprint = true;
        }
        if (frame == 90) {
            input.dashPressed = true;
        }

        player.Update(dt, input);

        if (frame % 45 == 0) {
            Vec2 pos = player.GetPosition();
            Vec2 vel = player.GetVelocity();
            std::cout << "Frame " << frame << " | Pos(" << pos.x << ", " << pos.y << ")"
                      << " | Vel(" << vel.x << ", " << vel.y << ")\n";
        }
    }

    return 0;
}
