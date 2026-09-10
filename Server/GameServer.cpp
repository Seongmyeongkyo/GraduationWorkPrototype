#include "GameServer.h"
#include "Logger.h"
#include <cstring>
#include <string>

using namespace std;

namespace {
    // Best-effort peer IP lookup for logging only; never fails the connection.
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

    m_client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    AcceptEx(m_server, m_client_socket, &m_accept_over.m_buff, 0,
        sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, &m_accept_over.m_over);

    Logger::Log("Server is running on port " + std::to_string(Port) + "...");
    return true;
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
    for (int player_index = 0;;) {
        DWORD num_bytes;
        ULONG_PTR key;
        LPOVERLAPPED over;
        BOOL ret = GetQueuedCompletionStatus(m_iocp, &num_bytes, &key, &over, INFINITE);
        EXP_OVER* exp_over = reinterpret_cast<EXP_OVER*>(over);

        if (ret == FALSE || (num_bytes == 0 && exp_over && exp_over->m_iotype == IO_RECV)) {
            int p_id = static_cast<int>(key);
            if (p_id >= 0 && p_id < MAX_PLAYERS && clients[p_id].m_is_connected) {
                Logger::Log("[DISCONNECT] id=" + std::to_string(p_id) + " ip=" + clients[p_id].m_ip);
                clients[p_id].m_is_connected = false;
                for (auto& cl : clients) {
                    if (cl.m_is_connected) cl.send_remove_player(p_id);
                }
                closesocket(clients[p_id].m_client);
                clients[p_id].m_client = INVALID_SOCKET;
            }
            if (exp_over && exp_over->m_iotype == IO_SEND) delete exp_over;
            continue;
        }

        switch (exp_over->m_iotype) {
        case IO_ACCEPT:
            player_index = -1;
            for (int i = 0; i < MAX_PLAYERS; ++i) {
                if (!clients[i].m_is_connected) { player_index = i; break; }
            }
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
                clients[player_index].m_x = 0.f; clients[player_index].m_y = 0.f; clients[player_index].m_z = 0.f;
                clients[player_index].m_prev_recv = 0;
                Logger::Log("[CONNECT] id=" + std::to_string(player_index) + " ip=" + clients[player_index].m_ip);
                clients[player_index].send_login_success();
                clients[player_index].do_recv();
            }
            m_client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
            ZeroMemory(&m_accept_over.m_over, sizeof(m_accept_over.m_over));
            AcceptEx(m_server, m_client_socket, &m_accept_over.m_buff, 0,
                sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, &m_accept_over.m_over);
            break;
        case IO_RECV: {
            int p_id = static_cast<int>(key);
            SESSION& cl = clients[p_id];
            unsigned char* p = reinterpret_cast<unsigned char*>(cl.m_recv_over.m_buff);
            int data_size = num_bytes + cl.m_prev_recv;
            while (data_size > 0) {
                int packet_size = p[0];
                if (packet_size > data_size) break;
                cl.process_packet(p);
                p += packet_size;
                data_size -= packet_size;
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
    closesocket(m_server);
    WSACleanup();
}
