#pragma once
#include <cstdint> // Use standard C++ fixed-width integer types, independent of Unreal

// Default port, used when no port is given on the command line.
// Kept so multiple instances can later run side by side on different ports
// (e.g. Server.exe 4001 / Server.exe 4002) without touching this file.
constexpr short PORT = 3500;
constexpr int MAX_PLAYERS = 10;
constexpr int MAX_NAME_LEN = 20;

// Use standard types (uint8_t, int32_t) that exactly match the size of Unreal's uint8, int32, etc.
enum PACKET_TYPE : uint8_t {
    C2S_LOGIN = 0, C2S_MOVE,
    S2C_LOGIN_RESULT, S2C_AVATAR_INFO, S2C_ADD_PLAYER, S2C_REMOVE_PLAYER, S2C_MOVE_PLAYER
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
#pragma pack(pop)
