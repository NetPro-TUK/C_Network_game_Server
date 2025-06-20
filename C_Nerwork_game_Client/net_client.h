#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include <winsock2.h>
#include <stdint.h>  

// 서버에 TCP로 연결 요청을 보내고, 성공 시 연결된 소켓을 반환함
SOCKET connect_to_server(const char* ip, int port);

// 클라이언트의 역할(PLAYER or ATTACKER)을 서버에 전송하고,
// 서버가 부여한 고유 entity ID를 받아서 반환
uint32_t send_join_and_get_id(SOCKET sock, int role);

// 클라이언트의 현재 위치(x, y) 정보를 서버에 전송
void send_state_update(SOCKET sock, uint32_t id, int x, int y);

#endif
