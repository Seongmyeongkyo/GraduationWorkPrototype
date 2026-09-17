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
    default: break;
    }
}
