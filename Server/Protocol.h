#pragma once
#include <cstdint> // Use standard C++ fixed-width integer types, independent of Unreal

// Default port, used when no port is given on the command line.
// Kept so multiple instances can later run side by side on different ports
// (e.g. Server.exe 4001 / Server.exe 4002) without touching this file.
constexpr short PORT = 3500;
constexpr int MAX_PLAYERS = 10;
constexpr int MAX_NAME_LEN = 20;

// Number of distinct skills a player can use (Q/W/E on the client side today).
// C2S_Skill/S2C_PlayerSkill carry a skillIndex field (0..SKILL_COUNT-1) instead
// of one packet type per skill, so adding a 4th skill later is a client-side
// change only - no new packet type needed.
constexpr int SKILL_COUNT = 3;

// Use standard types (uint8_t, int32_t) that exactly match the size of Unreal's uint8, int32, etc.
enum PACKET_TYPE : uint8_t {
    C2S_LOGIN = 0, C2S_MOVE,
    S2C_LOGIN_RESULT, S2C_AVATAR_INFO, S2C_ADD_PLAYER, S2C_REMOVE_PLAYER, S2C_MOVE_PLAYER,

    // ---- Rough combat/progression packets, added ahead of client support.
    // Field layouts are all placeholders - expect to revise once the client
    // side of each feature exists and we know what it actually needs to send.
    C2S_ATTACK, C2S_SKILL, C2S_HIT, C2S_GET_EXP, C2S_GET_ITEM,
    S2C_PLAYER_ATTACK, S2C_PLAYER_SKILL, S2C_PLAYER_HIT, S2C_EXP_RESULT, S2C_ITEM_RESULT
};

#pragma pack(push, 1) // Prevent padding (client/server byte alignment)
struct C2S_Login {
    uint8_t size;
    PACKET_TYPE type;
    char username[MAX_NAME_LEN];
};

// Destination the player clicked, in Unreal world-space units. The server
// does not simulate movement - it just relays this to every other client.
struct C2S_Move {
    uint8_t size;
    PACKET_TYPE type;
    float x, y, z;
};

struct S2C_LoginResult {
    uint8_t size;
    PACKET_TYPE type;
    bool success;
    char message[50];
};

struct S2C_AvatarInfo {
    uint8_t size;
    PACKET_TYPE type;
    int32_t playerId;
    float x, y, z;
};

struct S2C_AddPlayer {
    uint8_t size;
    PACKET_TYPE type;
    int32_t playerId;
    char username[MAX_NAME_LEN];
    float x, y, z;
};

struct S2C_RemovePlayer {
    uint8_t size;
    PACKET_TYPE type;
    int32_t playerId;
};

struct S2C_MovePlayer {
    uint8_t size;
    PACKET_TYPE type;
    int32_t playerId;
    float x, y, z;
};

// ---- Combat / progression (rough placeholders, see PACKET_TYPE above) ----

// Client announces a basic attack aimed in dirX/dirY. No target/hitbox info -
// the server does not do hit detection, it just relays the action so other
// clients can play the animation/VFX.
struct C2S_Attack {
    uint8_t size;
    PACKET_TYPE type;
    float dirX, dirY;
};

struct C2S_Skill {
    uint8_t size;
    PACKET_TYPE type;
    uint8_t skillIndex; // 0..SKILL_COUNT-1
    float dirX, dirY;
};

// Client-authoritative hit report (same trust model as C2S_Move): the
// attacker's own client decides it landed a hit and how much damage, the
// server just relays it. No server-side validation of range/cooldowns/HP.
struct C2S_Hit {
    uint8_t size;
    PACKET_TYPE type;
    int32_t targetPlayerId;
    int32_t damage;
};

// Amount is whatever the client computed (e.g. from a kill) - server doesn't
// track a running total, just echoes it back for the client to apply.
struct C2S_GetExp {
    uint8_t size;
    PACKET_TYPE type;
    int32_t amount;
};

struct C2S_GetItem {
    uint8_t size;
    PACKET_TYPE type;
    int32_t itemId;
};

struct S2C_PlayerAttack {
    uint8_t size;
    PACKET_TYPE type;
    int32_t playerId;
    float dirX, dirY;
};

struct S2C_PlayerSkill {
    uint8_t size;
    PACKET_TYPE type;
    int32_t playerId;
    uint8_t skillIndex;
    float dirX, dirY;
};

// Broadcast to everyone (not just the target) so both the victim (apply
// damage/HP if targetId == self) and bystanders (hit VFX on the target) can
// react from the same packet.
struct S2C_PlayerHit {
    uint8_t size;
    PACKET_TYPE type;
    int32_t attackerId;
    int32_t targetId;
    int32_t damage;
};

// Sent only back to the player who gained it, not broadcast.
struct S2C_ExpResult {
    uint8_t size;
    PACKET_TYPE type;
    int32_t amount;
};

// Sent only back to the player who picked it up, not broadcast.
struct S2C_ItemResult {
    uint8_t size;
    PACKET_TYPE type;
    int32_t itemId;
};
#pragma pack(pop)
