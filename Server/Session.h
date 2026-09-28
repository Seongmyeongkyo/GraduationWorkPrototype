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
    float m_hp;         // 클라이언트가 마지막으로 보고한 값 (검증 안 함)
    float m_mp;         // 서버가 관리 (스킬 사용 시 차감, 1초마다 회복)
    int32_t m_exp;      // C2S_GetExp로 받은 양의 누적
    std::string m_ip;   // 로그용

    SESSION();
    ~SESSION();

    // 모든 플레이어 데이터를 초기값으로 되돌린다. DB가 없으므로 접속이 끊기면 아무것도 남기지 않는다.
    void reset();

    void do_recv();
    void do_send(int num_bytes, char* mess);
    void send_avatar_info();
    void send_move_packet(int mover);
    void send_add_player(int player_id);
    void send_login_success();
    void send_remove_player(int player_id);

    // 전투/성장 중계 (임시 구조)
    void send_attack(int player_id, float dirX, float dirY);
    void send_skill(int player_id, uint8_t skillIndex, float dirX, float dirY);
    void send_hit(int attacker_id, int target_id, int32_t damage);
    void send_exp_result(int32_t amount);
    void send_item_result(int32_t item_id);

    // 마나
    void send_mana_update();
    void send_skill_fail(uint8_t skillIndex);
    void regen_mana(float amount); // MAX_MANA까지만 회복, 실제로 변했을 때만 본인에게 전송

    void process_packet(unsigned char* p);
};

extern std::array<SESSION, MAX_PLAYERS> clients;

// clients 접근 보호용. 워커 스레드가 완료 처리 1건마다 잡는다.
// SESSION 메서드는 이미 잡힌 상태를 전제하므로 안에서 다시 잡으면 안 된다 (재귀 락이 아니라 데드락).
extern std::mutex g_clients_mutex;
