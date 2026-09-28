#include "GameServer.h"
#include "Logger.h"
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace std;

namespace {
    // 로그용 IP 조회. 실패해도 접속에는 영향 없음.
    std::string GetPeerIp(SOCKET s) {
        sockaddr_in addr{};
        int addr_len = sizeof(addr);
        if (getpeername(s, reinterpret_cast<sockaddr*>(&addr), &addr_len) != 0) {
            return "unknown";
        }
        char ip_str[INET_ADDRSTRLEN] = {};
        if (!inet_ntop(AF_INET, &addr.sin_addr, ip_str, sizeof(ip_str))) {
            return "unknown";
        }
        return ip_str;
    }
}

bool GameServer::Init(unsigned short Port) {
    Logger::Init(Port);

    WSADATA WSAData;
    WSAStartup(MAKEWORD(2, 2), &WSAData);
    m_server = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    SOCKADDR_IN server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(Port);
    server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
    bind(m_server, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
    listen(m_server, SOMAXCONN);

    m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    CreateIoCompletionPort((HANDLE)m_server, m_iocp, -1, 0);

    PostAccept();

    Logger::Log("Server is running on port " + std::to_string(Port) + "...");
    return true;
}

void GameServer::PostAccept() {
    for (;;) {
        m_client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        ZeroMemory(&m_accept_over.m_over, sizeof(m_accept_over.m_over));
        if (AcceptEx(m_server, m_client_socket, &m_accept_over.m_buff, 0,
            sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, &m_accept_over.m_over)) {
            return; // 동기 완료여도 IOCP 완료 통지는 따로 온다
        }
        const int err = WSAGetLastError();
        if (err == ERROR_IO_PENDING) return;

        closesocket(m_client_socket);
        m_client_socket = INVALID_SOCKET;
        // 수락 전에 상대가 연결을 끊은 경우 - 그 연결만의 문제이므로 다시 시도
        if (err == WSAECONNRESET) continue;

        Logger::Log("[ERROR] AcceptEx failed, error=" + std::to_string(err) + " - no longer accepting new connections.");
        return;
    }
}

void GameServer::Disconnect(int p_id) {
    if (p_id < 0 || p_id >= MAX_PLAYERS || !clients[p_id].m_is_connected) return;

    Logger::Log("[DISCONNECT] id=" + std::to_string(p_id) + " ip=" + clients[p_id].m_ip);
    clients[p_id].m_is_connected = false;
    for (auto& cl : clients) {
        if (cl.m_is_connected) cl.send_remove_player(p_id);
    }
    closesocket(clients[p_id].m_client);
    clients[p_id].reset();
}

void GameServer::SendLoginFail(SOCKET client, const char* message) {
    S2C_LoginResult packet;
    packet.size = sizeof(S2C_LoginResult);
    packet.type = S2C_LOGIN_RESULT;
    packet.success = false;
    strncpy_s(packet.message, message, sizeof(packet.message));
    WSABUF wsa_buf{ packet.size, reinterpret_cast<char*>(&packet) };
    WSASend(client, &wsa_buf, 1, 0, 0, nullptr, nullptr);
}

void GameServer::Run() {
    // 워커 3개 + Logger::Init에서 띄운 로그 스레드 1개 = 총 4개.
    // 워커는 여전히 g_clients_mutex로 직렬화되므로 늘려도 처리량은 늘지 않는다.
    // 핵심은 콘솔/파일 출력이 락 밖(로그 스레드)으로 빠졌다는 점이다.
    constexpr unsigned int WORKER_THREAD_COUNT = 3;
    Logger::Log("Starting " + std::to_string(WORKER_THREAD_COUNT) + " IOCP worker threads (+1 logger thread).");

    std::vector<std::thread> workers;
    workers.reserve(WORKER_THREAD_COUNT);
    for (unsigned int i = 0; i < WORKER_THREAD_COUNT; ++i) {
        workers.emplace_back(&GameServer::WorkerLoop, this);
    }
    for (auto& t : workers) {
        t.join(); // WorkerLoop는 끝나지 않으므로 여기서 영원히 대기
    }

    closesocket(m_server);
    WSACleanup();
}

void GameServer::WorkerLoop() {
    for (;;) {
        DWORD num_bytes;
        ULONG_PTR key;
        LPOVERLAPPED over;
        // 여기서 무기한 대기하므로 락을 잡고 있으면 안 된다 (다른 워커가 전부 멈춤).
        BOOL ret = GetQueuedCompletionStatus(m_iocp, &num_bytes, &key, &over, INFINITE);
        const DWORD io_error = (ret == FALSE) ? GetLastError() : 0;
        EXP_OVER* exp_over = reinterpret_cast<EXP_OVER*>(over);

        std::lock_guard<std::mutex> lock(g_clients_mutex);

        if (ret == FALSE && exp_over && exp_over->m_iotype == IO_ACCEPT) {
            // 이 연결 하나만 실패한 것 (수락 전 상대가 리셋 등). 다시 걸지 않으면 이후 접속을 영영 못 받는다.
            Logger::Log("[ACCEPT FAIL] error=" + std::to_string(io_error));
            closesocket(m_client_socket);
            PostAccept();
            continue;
        }

        if (ret == FALSE || (num_bytes == 0 && exp_over && exp_over->m_iotype == IO_RECV)) {
            Disconnect(static_cast<int>(key));
            if (exp_over && exp_over->m_iotype == IO_SEND) delete exp_over;
            continue;
        }

        switch (exp_over->m_iotype) {
        case IO_ACCEPT: {
            int player_index = -1;
            for (int i = 0; i < MAX_PLAYERS; ++i) {
                if (!clients[i].m_is_connected) { player_index = i; break; }
            }
            // AcceptEx로 받은 소켓은 이 설정 전에는 getpeername()이 실패한다 (IP 로그용).
            setsockopt(m_client_socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
                reinterpret_cast<char*>(&m_server), sizeof(m_server));
            if (player_index == -1) {
                Logger::Log("[REJECT] server full, ip=" + GetPeerIp(m_client_socket));
                SendLoginFail(m_client_socket, "Server is full.");
                closesocket(m_client_socket);
            }
            else {
                CreateIoCompletionPort((HANDLE)m_client_socket, m_iocp, player_index, 0);
                clients[player_index].m_is_connected = true;
                clients[player_index].m_client = m_client_socket;
                clients[player_index].m_id = player_index;
                clients[player_index].m_ip = GetPeerIp(m_client_socket);
                // 나머지 필드는 이미 초기화 상태 (생성자 또는 Disconnect()의 reset())
                Logger::Log("[CONNECT] id=" + std::to_string(player_index) + " ip=" + clients[player_index].m_ip);
                clients[player_index].send_login_success();
                clients[player_index].do_recv();
            }
            PostAccept();
            break;
        }
        case IO_RECV: {
            int p_id = static_cast<int>(key);
            SESSION& cl = clients[p_id];
            unsigned char* p = reinterpret_cast<unsigned char*>(cl.m_recv_over.m_buff);
            int data_size = num_bytes + cl.m_prev_recv;
            bool malformed = false;
            while (data_size > 0) {
                int packet_size = p[0];
                // 0이면 p가 전진하지 않아 락을 잡은 채 무한 루프 → 서버 전체 정지. 1이면 타입 바이트조차 없다.
                if (packet_size < 2) { malformed = true; break; }
                if (data_size < 2) break; // 타입 바이트가 아직 안 옴
                if (packet_size != ExpectedC2SPacketSize(p[1])) { malformed = true; break; }
                if (packet_size > data_size) break;
                cl.process_packet(p);
                p += packet_size;
                data_size -= packet_size;
            }
            if (malformed) {
                // 길이가 한 번 틀어진 TCP 스트림은 복구할 수 없으므로 연결을 끊는다.
                std::string detail = "size=" + std::to_string(p[0]);
                if (data_size >= 2) detail += " type=" + std::to_string(p[1]);
                Logger::Log("[BAD PACKET] id=" + std::to_string(p_id) + " ip=" + cl.m_ip + " " + detail);
                Disconnect(p_id);
                break;
            }
            if (data_size > 0) memmove(cl.m_recv_over.m_buff, p, data_size);
            cl.m_prev_recv = data_size;
            cl.do_recv();
            break;
        }
        case IO_SEND:
            delete reinterpret_cast<EXP_OVER*>(over);
            break;
        }
    }
}
