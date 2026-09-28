#pragma once
#include <cstdint>

// 실행 인자로 포트를 안 주면 쓰는 기본값 (인스턴스마다 다른 포트로 여러 개 실행 가능)
constexpr short PORT = 3500;
constexpr int MAX_PLAYERS = 10;
constexpr int MAX_NAME_LEN = 20;

// 스킬 개수. 스킬마다 패킷 타입을 따로 두지 않고 skillIndex(0 ~ SKILL_COUNT-1)로 구분한다.
constexpr int SKILL_COUNT = 3;

// 게임 수치 규칙. 서버에 접속하지 않았을 때 클라이언트도 같은 규칙으로 직접 처리하므로
// 값을 바꿀 때는 클라이언트 NetworkProtocol.h도 함께 수정할 것.
constexpr float MAX_MANA = 100.f;
constexpr float SKILL_MANA_COST = 20.f;
constexpr float MANA_REGEN_PER_SECOND = 5.f;

// uint8_t/int32_t는 언리얼의 uint8/int32와 크기가 같다.
enum PACKET_TYPE : uint8_t {
    C2S_LOGIN = 0, C2S_MOVE,
    S2C_LOGIN_RESULT, S2C_AVATAR_INFO, S2C_ADD_PLAYER, S2C_REMOVE_PLAYER, S2C_MOVE_PLAYER,

    // 전투/성장 (임시 구조, 클라이언트 기능이 생기면 수정 예정)
    C2S_ATTACK, C2S_SKILL, C2S_HIT, C2S_GET_EXP, C2S_GET_ITEM,
    S2C_PLAYER_ATTACK, S2C_PLAYER_SKILL, S2C_PLAYER_HIT, S2C_EXP_RESULT, S2C_ITEM_RESULT,

    // 체력 보고 (임시 구조)
    C2S_UPDATE_HEALTH,

    // 마나 (서버가 관리)
    S2C_MANA_UPDATE, S2C_SKILL_FAIL
};

#pragma pack(push, 1) // 패딩 제거 (클라이언트와 바이트 배치 일치)
struct C2S_Login {
    uint8_t size;
    PACKET_TYPE type;
    char username[MAX_NAME_LEN];
};

// 클릭한 목적지(언리얼 월드 좌표). 서버는 이동을 계산하지 않고 중계만 한다.
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

// ---- 전투 / 성장 ----
// 스킬의 마나만 서버가 확인/차감하고, 나머지는 클라이언트가 판단한 값을 서버가 검증 없이 중계/저장한다.

// 기본 공격 방향. 서버는 판정하지 않고 다른 클라이언트의 애니메이션/이펙트용으로 중계만 한다.
struct C2S_Attack {
    uint8_t size;
    PACKET_TYPE type;
    float dirX, dirY;
};

// 스킬 사용 요청. 서버가 마나를 확인해서 충분하면 차감 후 S2C_PlayerSkill을 전원에게,
// 부족하면 S2C_SkillFail을 요청한 본인에게 보낸다.
struct C2S_Skill {
    uint8_t size;
    PACKET_TYPE type;
    uint8_t skillIndex; // 0 ~ SKILL_COUNT-1
    float dirX, dirY;
};

// 공격자 클라이언트가 명중 여부와 데미지를 직접 판단해서 보고한다 (사거리/쿨다운/HP 검증 없음).
struct C2S_Hit {
    uint8_t size;
    PACKET_TYPE type;
    int32_t targetPlayerId;
    int32_t damage;
};

// 서버는 세션에 누적(m_exp)하고, 받은 양을 보낸 본인에게 되돌려 준다.
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

// 맞은 사람(데미지 적용)과 주변 사람(피격 이펙트)이 같은 패킷으로 반응하도록 전원에게 보낸다.
struct S2C_PlayerHit {
    uint8_t size;
    PACKET_TYPE type;
    int32_t attackerId;
    int32_t targetId;
    int32_t damage;
};

// 보낸 본인에게만 전송
struct S2C_ExpResult {
    uint8_t size;
    PACKET_TYPE type;
    int32_t amount;
};

// 보낸 본인에게만 전송
struct S2C_ItemResult {
    uint8_t size;
    PACKET_TYPE type;
    int32_t itemId;
};

// ---- 체력 ----
// 클라이언트가 계산한 현재 값을 보고만 한다. 서버는 세션(m_hp)에 저장만 하고 응답하지 않는다.
struct C2S_UpdateHealth {
    uint8_t size;
    PACKET_TYPE type;
    float currentHealth;
};

// ---- 마나 (서버가 관리) ----

// 현재/최대 마나. 로그인 직후, 스킬 사용 후, 회복될 때마다 본인에게만 전송.
struct S2C_ManaUpdate {
    uint8_t size;
    PACKET_TYPE type;
    float currentMana;
    float maxMana;
};

// 스킬 사용 실패 (현재 사유는 마나 부족뿐). 요청한 본인에게만 전송.
struct S2C_SkillFail {
    uint8_t size;
    PACKET_TYPE type;
    uint8_t skillIndex;
};
#pragma pack(pop)

// 클라이언트가 보낼 수 있는 패킷의 정확한 크기 (C2S 타입이 아니면 0).
// 수신한 패킷 크기가 이 값과 다르면 잘못된 패킷으로 보고 연결을 끊는다.
// C2S 패킷을 추가하면 여기에도 반드시 case를 추가할 것 (빠뜨리면 그 패킷을 보낸 클라이언트가 끊김).
constexpr int ExpectedC2SPacketSize(uint8_t type) {
    switch (type) {
    case C2S_LOGIN:         return sizeof(C2S_Login);
    case C2S_MOVE:          return sizeof(C2S_Move);
    case C2S_ATTACK:        return sizeof(C2S_Attack);
    case C2S_SKILL:         return sizeof(C2S_Skill);
    case C2S_HIT:           return sizeof(C2S_Hit);
    case C2S_GET_EXP:       return sizeof(C2S_GetExp);
    case C2S_GET_ITEM:      return sizeof(C2S_GetItem);
    case C2S_UPDATE_HEALTH: return sizeof(C2S_UpdateHealth);
    default:                return 0;
    }
}
