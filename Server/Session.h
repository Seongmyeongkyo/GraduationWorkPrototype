#pragma once
#include <array>
#include <cstdint>
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

    SESSION();
    ~SESSION();

    void do_recv();
    void do_send(int num_bytes, char* mess);
    void send_avatar_info();
    void send_move_packet(int mover);
    void send_add_player(int player_id);
    void send_login_success();
    void send_remove_player(int player_id);
    void process_packet(unsigned char* p);
};

extern std::array<SESSION, MAX_PLAYERS> clients;
