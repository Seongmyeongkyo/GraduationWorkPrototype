#include "Session.h"
#include "Logger.h"
#include <cstring>

using namespace std;

// 가장 큰 패킷도 송수신 버퍼(BUF_SIZE)와 1바이트 크기 필드에 들어가야 한다.
static_assert(sizeof(S2C_Chat) <= BUF_SIZE && sizeof(S2C_Chat) <= 255, "S2C_Chat too large");
static_assert(sizeof(C2S_Chat) <= BUF_SIZE && sizeof(C2S_Chat) <= 255, "C2S_Chat too large");

std::array<SESSION, MAX_PLAYERS> clients;
std::mutex g_clients_mutex;

namespace {
    // 채팅 도배 제한: CHAT_LIMIT_WINDOW 동안 CHAT_LIMIT_COUNT개까지
    constexpr int CHAT_LIMIT_COUNT = 5;
    constexpr auto CHAT_LIMIT_WINDOW = std::chrono::seconds(5);

    // 널 종료를 보장하고 제어문자(줄바꿈 등)는 공백으로 바꾼다 (로그 위조, 채팅창 줄 깨짐 방지).
    // 공백뿐이면 false.
    bool sanitize_chat(const char* src, char (&dst)[CHAT_MAX_BYTES]) {
        bool has_text = false;
        int i = 0;
        for (; i < CHAT_MAX_BYTES - 1 && src[i] != '\0'; ++i) {
            unsigned char c = static_cast<unsigned char>(src[i]);
            if (c < 0x20 || c == 0x7F) c = ' ';
            if (c != ' ') has_text = true;
            dst[i] = static_cast<char>(c);
        }
        dst[i] = '\0';
        return has_text;
    }
}

void broadcast_system_chat(const std::string& message) {
    for (auto& cl : clients)
        if (cl.m_is_connected) cl.send_chat(-1, CHAT_SYSTEM, message.c_str());
}

SESSION::SESSION() {
    m_recv_over.m_iotype = IO_RECV;
    reset();
}

void SESSION::reset() {
    m_is_connected = false;
    m_client = INVALID_SOCKET;
    m_id = 999;
    m_prev_recv = 0;
    memset(m_username, 0, sizeof(m_username));
    m_x = 0.f; m_y = 0.f; m_z = 0.f;
    m_hp = 100.f; m_mp = MAX_MANA; m_exp = 0;
    m_team = 0;
    m_ip.clear();
    m_chat_window_start = {};
    m_chat_count = 0;
}

SESSION::~SESSION() {
    if (m_is_connected) closesocket(m_client);
}

void SESSION::do_recv() {
    m_recv_flag = 0;
    memset(&m_recv_over.m_over, 0, sizeof(m_recv_over.m_over));
    // 이전 수신에서 남은 미완성 패킷이 버퍼 앞쪽 m_prev_recv 바이트에 있으므로 그 뒤에 이어서 받는다.
    // (앞에 덮어쓰면 두 번에 나눠 도착한 패킷이 깨진다)
    m_recv_over.m_wsa.buf = m_recv_over.m_buff + m_prev_recv;
    m_recv_over.m_wsa.len = BUF_SIZE - m_prev_recv;
    WSARecv(m_client, &m_recv_over.m_wsa, 1, 0, &m_recv_flag, &m_recv_over.m_over, nullptr);
}

void SESSION::do_send(int num_bytes, char* mess) {
    EXP_OVER* o = new EXP_OVER(IO_SEND);
    o->m_wsa.len = num_bytes;
    memcpy(o->m_buff, mess, num_bytes);
    WSASend(m_client, &o->m_wsa, 1, 0, 0, &o->m_over, nullptr);
}

void SESSION::send_avatar_info() {
    S2C_AvatarInfo packet;
    packet.size = sizeof(S2C_AvatarInfo);
    packet.type = S2C_AVATAR_INFO;
    packet.playerId = m_id;
    packet.x = m_x;
    packet.y = m_y;
    packet.z = m_z;
    do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void SESSION::send_login_success() {
    S2C_LoginResult packet;
    packet.size = sizeof(S2C_LoginResult);
    packet.type = S2C_LOGIN_RESULT;
    packet.success = true;
    strncpy_s(packet.message, "Login successful.", sizeof(packet.message));
    do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void SESSION::send_remove_player(int player_id) {
    S2C_RemovePlayer packet;
    packet.size = sizeof(S2C_RemovePlayer);
    packet.type = S2C_REMOVE_PLAYER;
    packet.playerId = player_id;
    do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void SESSION::send_add_player(int player_id) {
    S2C_AddPlayer packet;
    packet.size = sizeof(S2C_AddPlayer);
    packet.type = S2C_ADD_PLAYER;
    packet.playerId = player_id;
    SESSION& pl = clients[player_id];
    memcpy(packet.username, pl.m_username, sizeof(packet.username));
    packet.x = pl.m_x;
    packet.y = pl.m_y;
    packet.z = pl.m_z;
    do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void SESSION::send_move_packet(int mover) {
    S2C_MovePlayer packet;
    packet.size = sizeof(S2C_MovePlayer);
    packet.type = S2C_MOVE_PLAYER;
    packet.playerId = mover;
    packet.x = clients[mover].m_x;
    packet.y = clients[mover].m_y;
    packet.z = clients[mover].m_z;
    do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void SESSION::send_attack(int player_id, float dirX, float dirY) {
    S2C_PlayerAttack packet;
    packet.size = sizeof(S2C_PlayerAttack);
    packet.type = S2C_PLAYER_ATTACK;
    packet.playerId = player_id;
    packet.dirX = dirX;
    packet.dirY = dirY;
    do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void SESSION::send_skill(int player_id, uint8_t skillIndex, float dirX, float dirY) {
    S2C_PlayerSkill packet;
    packet.size = sizeof(S2C_PlayerSkill);
    packet.type = S2C_PLAYER_SKILL;
    packet.playerId = player_id;
    packet.skillIndex = skillIndex;
    packet.dirX = dirX;
    packet.dirY = dirY;
    do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void SESSION::send_hit(int attacker_id, int target_id, int32_t damage) {
    S2C_PlayerHit packet;
    packet.size = sizeof(S2C_PlayerHit);
    packet.type = S2C_PLAYER_HIT;
    packet.attackerId = attacker_id;
    packet.targetId = target_id;
    packet.damage = damage;
    do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void SESSION::send_exp_result(int32_t amount) {
    S2C_ExpResult packet;
    packet.size = sizeof(S2C_ExpResult);
    packet.type = S2C_EXP_RESULT;
    packet.amount = amount;
    do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void SESSION::send_item_result(int32_t item_id) {
    S2C_ItemResult packet;
    packet.size = sizeof(S2C_ItemResult);
    packet.type = S2C_ITEM_RESULT;
    packet.itemId = item_id;
    do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void SESSION::send_mana_update() {
    S2C_ManaUpdate packet;
    packet.size = sizeof(S2C_ManaUpdate);
    packet.type = S2C_MANA_UPDATE;
    packet.currentMana = m_mp;
    packet.maxMana = MAX_MANA;
    do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void SESSION::send_skill_fail(uint8_t skillIndex) {
    S2C_SkillFail packet;
    packet.size = sizeof(S2C_SkillFail);
    packet.type = S2C_SKILL_FAIL;
    packet.skillIndex = skillIndex;
    do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void SESSION::regen_mana(float amount) {
    if (m_mp >= MAX_MANA) return;
    m_mp += amount;
    if (m_mp > MAX_MANA) m_mp = MAX_MANA; // std::min은 windows.h의 min 매크로와 충돌해서 쓰지 않음
    send_mana_update();
}

void SESSION::send_chat(int sender_id, uint8_t channel, const char* message) {
    S2C_Chat packet{};
    packet.size = sizeof(S2C_Chat);
    packet.type = S2C_CHAT;
    packet.senderId = sender_id;
    packet.channel = channel;
    if (sender_id >= 0 && sender_id < MAX_PLAYERS) {
        const SESSION& sender = clients[sender_id];
        memcpy(packet.senderName, sender.m_username, sizeof(packet.senderName));
        packet.isTeammate = (sender.m_team == m_team) ? 1 : 0;
    }
    strncpy_s(packet.message, message, _TRUNCATE);
    do_send(packet.size, reinterpret_cast<char*>(&packet));
}

bool SESSION::allow_chat() {
    const auto now = std::chrono::steady_clock::now();
    if (m_chat_count == 0 || now - m_chat_window_start >= CHAT_LIMIT_WINDOW) {
        m_chat_window_start = now;
        m_chat_count = 0;
    }
    if (m_chat_count >= CHAT_LIMIT_COUNT) return false;
    ++m_chat_count;
    return true;
}

void SESSION::process_packet(unsigned char* p) {
    PACKET_TYPE type = static_cast<PACKET_TYPE>(p[1]);
    switch (type) {
    case C2S_LOGIN: {
        C2S_Login* packet = reinterpret_cast<C2S_Login*>(p);
        memset(m_username, 0, MAX_NAME_LEN);
        strncpy_s(m_username, packet->username, MAX_NAME_LEN - 1);
        Logger::Log("[LOGIN] id=" + to_string(m_id) + " ip=" + m_ip + " username=" + m_username);
        send_avatar_info();
        send_mana_update();

        for (auto& other : clients) {
            if (!other.m_is_connected || other.m_id == m_id) continue;
            other.send_add_player(m_id);
            send_add_player(other.m_id);
        }
        broadcast_system_chat(std::string(m_username) + "님이 접속했습니다.");
        break;
    }
    case C2S_MOVE: {
        C2S_Move* packet = reinterpret_cast<C2S_Move*>(p);
        m_x = packet->x;
        m_y = packet->y;
        m_z = packet->z;
        for (auto& cl : clients)
            if (cl.m_is_connected) cl.send_move_packet(m_id);
        break;
    }
    case C2S_ATTACK: {
        C2S_Attack* packet = reinterpret_cast<C2S_Attack*>(p);
        Logger::Log("[ATTACK] id=" + to_string(m_id) + " dir=(" + to_string(packet->dirX) + ", " + to_string(packet->dirY) + ")");
        for (auto& cl : clients)
            if (cl.m_is_connected) cl.send_attack(m_id, packet->dirX, packet->dirY);
        break;
    }
    case C2S_SKILL: {
        C2S_Skill* packet = reinterpret_cast<C2S_Skill*>(p);
        const std::string skill = to_string(static_cast<int>(packet->skillIndex));
        if (m_mp < SKILL_MANA_COST) {
            Logger::Log("[SKILL FAIL] id=" + to_string(m_id) + " skill=" + skill + " mp=" + to_string(m_mp));
            send_skill_fail(packet->skillIndex);
            break;
        }
        m_mp -= SKILL_MANA_COST;
        Logger::Log("[SKILL] id=" + to_string(m_id) + " skill=" + skill + " mp=" + to_string(m_mp));
        for (auto& cl : clients)
            if (cl.m_is_connected) cl.send_skill(m_id, packet->skillIndex, packet->dirX, packet->dirY);
        send_mana_update();
        break;
    }
    case C2S_HIT: {
        C2S_Hit* packet = reinterpret_cast<C2S_Hit*>(p);
        Logger::Log("[HIT] attacker=" + to_string(m_id) + " target=" + to_string(packet->targetPlayerId) + " damage=" + to_string(packet->damage));
        for (auto& cl : clients)
            if (cl.m_is_connected) cl.send_hit(m_id, packet->targetPlayerId, packet->damage);
        break;
    }
    case C2S_GET_EXP: {
        C2S_GetExp* packet = reinterpret_cast<C2S_GetExp*>(p);
        m_exp += packet->amount;
        Logger::Log("[EXP] id=" + to_string(m_id) + " amount=" + to_string(packet->amount) + " total=" + to_string(m_exp));
        send_exp_result(packet->amount); // 누적값이 아니라 이번에 받은 양을 본인에게만 전송
        break;
    }
    case C2S_GET_ITEM: {
        C2S_GetItem* packet = reinterpret_cast<C2S_GetItem*>(p);
        Logger::Log("[ITEM] id=" + to_string(m_id) + " itemId=" + to_string(packet->itemId));
        send_item_result(packet->itemId); // 본인에게만 전송
        break;
    }
    case C2S_UPDATE_HEALTH: {
        C2S_UpdateHealth* packet = reinterpret_cast<C2S_UpdateHealth*>(p);
        m_hp = packet->currentHealth;
        Logger::Log("[HEALTH] id=" + to_string(m_id) + " hp=" + to_string(m_hp));
        break;
    }
    case C2S_CHAT: {
        C2S_Chat* packet = reinterpret_cast<C2S_Chat*>(p);
        // 로그인 전이거나, 클라이언트가 보낼 수 없는 채널(System 등)이면 무시
        if (m_username[0] == '\0') break;
        if (packet->channel != CHAT_ALL && packet->channel != CHAT_TEAM) {
            Logger::Log("[CHAT REJECT] id=" + to_string(m_id) + " reason=bad channel " + to_string(static_cast<int>(packet->channel)));
            break;
        }

        char message[CHAT_MAX_BYTES];
        if (!sanitize_chat(packet->message, message)) break; // 빈 메시지

        if (!allow_chat()) {
            Logger::Log("[CHAT REJECT] id=" + to_string(m_id) + " reason=rate limit");
            send_chat(-1, CHAT_SYSTEM, "메시지를 너무 빠르게 보내고 있습니다.");
            break;
        }

        const bool team_only = (packet->channel == CHAT_TEAM);
        Logger::Log("[CHAT] id=" + to_string(m_id) + " channel=" + (team_only ? "team" : "all") + " msg=" + message);
        for (auto& cl : clients) {
            if (!cl.m_is_connected) continue;
            if (team_only && cl.m_team != m_team) continue;
            cl.send_chat(m_id, packet->channel, message); // 보낸 본인도 받는다 (모든 클라이언트의 채팅 순서를 같게)
        }
        break;
    }
    default: break;
    }
}
