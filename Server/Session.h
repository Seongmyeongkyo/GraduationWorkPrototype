#pragma once
#include <array>
#include <cstdint>
#include <mutex>
#include <string>
#include "Protocol.h"
#include "OverlappedEx.h"

class SESSION {
public:
    SOCKET m_client;
    int32_t m_id;
    bool m_is_connected;
    EXP_OVER m_recv_over;
    int m_prev_recv;
    DWORD m_recv_flag;
    char m_username[MAX_NAME_LEN];
    float m_x, m_y, m_z;
    float m_hp, m_mp;   // last value reported by C2S_UpdateHealth/Mana; not validated, see Protocol.h
    int32_t m_exp;      // running total accumulated from C2S_GetExp amounts
    std::string m_ip; // set by GameServer right after accept; used for logging only

    SESSION();
    ~SESSION();

    void do_recv();
    void do_send(int num_bytes, char* mess);
    void send_avatar_info();
    void send_move_packet(int mover);
    void send_add_player(int player_id);
    void send_login_success();
    void send_remove_player(int player_id);

    // Combat/progression relays (rough placeholders - see Protocol.h)
    void send_attack(int player_id, float dirX, float dirY);
    void send_skill(int player_id, uint8_t skillIndex, float dirX, float dirY);
    void send_hit(int attacker_id, int target_id, int32_t damage);
    void send_exp_result(int32_t amount);
    void send_item_result(int32_t item_id);
    void send_health_result(float hp);
    void send_mana_result(float mp);

    void process_packet(unsigned char* p);
};

extern std::array<SESSION, MAX_PLAYERS> clients;

// Guards all reads/writes of `clients` (including calling SESSION methods that
// touch other sessions, e.g. the login/move broadcast loops). GameServer::Run's
// worker threads take this once per completion before touching any session;
// SESSION methods themselves assume it is already held and must not lock it
// again (std::mutex is non-recursive - a nested lock on the same thread deadlocks).
extern std::mutex g_clients_mutex;
