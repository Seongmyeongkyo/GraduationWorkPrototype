#include "Session.h"
#include "Logger.h"
#include <cstring>

using namespace std;

std::array<SESSION, MAX_PLAYERS> clients;
std::mutex g_clients_mutex;

SESSION::SESSION() {
    m_is_connected = false;
    m_id = 999;
    m_client = INVALID_SOCKET;
    m_recv_over.m_iotype = IO_RECV;
    m_x = 0.f; m_y = 0.f; m_z = 0.f;
    m_hp = 100.f; m_mp = 100.f; m_exp = 0;
    m_prev_recv = 0;
}

SESSION::~SESSION() {
    if (m_is_connected) closesocket(m_client);
}

void SESSION::do_recv() {
    m_recv_flag = 0;
    memset(&m_recv_over.m_over, 0, sizeof(m_recv_over.m_over));
    // A partial packet left over from the last reassembly pass already sits at
    // m_buff[0..m_prev_recv-1] (put there by GameServer::Run's IO_RECV handler).
    // Receive new bytes right after it, not on top of it - otherwise a packet
    // split across two WSARecv completions gets its leftover half overwritten
    // and the reassembled "packet" is corrupted.
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

void SESSION::send_health_result(float hp) {
    S2C_HealthResult packet;
    packet.size = sizeof(S2C_HealthResult);
    packet.type = S2C_HEALTH_RESULT;
    packet.currentHealth = hp;
    do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void SESSION::send_mana_result(float mp) {
    S2C_ManaResult packet;
    packet.size = sizeof(S2C_ManaResult);
    packet.type = S2C_MANA_RESULT;
    packet.currentMana = mp;
    do_send(packet.size, reinterpret_cast<char*>(&packet));
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

        for (auto& other : clients) {
            if (!other.m_is_connected || other.m_id == m_id) continue;
            other.send_add_player(m_id);
            send_add_player(other.m_id);
        }
        break;
    }
    case C2S_MOVE: {
        C2S_Move* packet = reinterpret_cast<C2S_Move*>(p);
        m_x = packet->x;
        m_y = packet->y;
        m_z = packet->z;
        Logger::Log("[MOVE] id=" + to_string(m_id) + " pos=(" + to_string(m_x) + ", " + to_string(m_y) + ", " + to_string(m_z) + ")");
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
        Logger::Log("[SKILL] id=" + to_string(m_id) + " skill=" + to_string(static_cast<int>(packet->skillIndex)) + " dir=(" + to_string(packet->dirX) + ", " + to_string(packet->dirY) + ")");
        for (auto& cl : clients)
            if (cl.m_is_connected) cl.send_skill(m_id, packet->skillIndex, packet->dirX, packet->dirY);
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
        m_exp += packet->amount; // now tracked on the session; echo below still sends the delta, not the running total
        Logger::Log("[EXP] id=" + to_string(m_id) + " amount=" + to_string(packet->amount) + " total=" + to_string(m_exp));
        send_exp_result(packet->amount); // personal - echoed back to the sender only
        break;
    }
    case C2S_GET_ITEM: {
        C2S_GetItem* packet = reinterpret_cast<C2S_GetItem*>(p);
        Logger::Log("[ITEM] id=" + to_string(m_id) + " itemId=" + to_string(packet->itemId));
        send_item_result(packet->itemId); // personal - echoed back to the sender only
        break;
    }
    case C2S_UPDATE_HEALTH: {
        C2S_UpdateHealth* packet = reinterpret_cast<C2S_UpdateHealth*>(p);
        m_hp = packet->currentHealth;
        Logger::Log("[HEALTH] id=" + to_string(m_id) + " hp=" + to_string(m_hp));
        send_health_result(m_hp); // personal - echoed back to the sender only
        break;
    }
    case C2S_UPDATE_MANA: {
        C2S_UpdateMana* packet = reinterpret_cast<C2S_UpdateMana*>(p);
        m_mp = packet->currentMana;
        Logger::Log("[MANA] id=" + to_string(m_id) + " mp=" + to_string(m_mp));
        send_mana_result(m_mp); // personal - echoed back to the sender only
        break;
    }
    default: break;
    }
}
