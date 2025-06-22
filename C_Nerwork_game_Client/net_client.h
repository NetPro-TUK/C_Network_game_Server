#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include <winsock2.h>
#include <stdint.h> 
#include "protocol.h"


typedef enum {
    ROLE_STATUS_PENDING = 0,   // 아직 서버 응답 안 옴
    ROLE_STATUS_APPROVED = 1,  // 역할 선택 승인됨
    ROLE_STATUS_REJECTED = 2   // 역할 선택 거부됨
} RoleStatus;

// 게임 준비
void send_ready(SOCKET sock, uint32_t my_entity_id);

// 서버에 TCP로 연결 요청을 보내고, 성공 시 연결된 소켓을 반환함
SOCKET connect_to_server(const char* ip, int port);

// 클라이언트의 역할(DEFENDER or ATTACKER)을 서버에 전송하고,
// 서버가 부여한 고유 entity ID를 받아서 반환
void send_join_and_get_id(SOCKET sock, int role);

// 준비 완료 신호를 서버로 전송
void send_ready(SOCKET sock, uint32_t my_entity_id);

// 클라이언트의 현재 위치(x, y) 정보를 서버에 전송
void send_state_update(SOCKET sock, uint32_t id, int x, int y);

// 게임 이벤트 메세지
void handle_server_message(MsgHeader* header, void* payload);

// 클라이언트가 공격자 역할을 선택했을 때, 총알 발사 이벤트를 서버로 전송
void send_shooting_event(SOCKET sock, uint32_t shooter_id, uint32_t bullet_id, int dirX, int dirY);

// 클라이언트가 재장전 요청을 서버로 전송
void send_reload_request(SOCKET sock, uint32_t entity_id);

// 클라이언트가 리스폰 요청을 서버로 전송
void send_respawn_request(SOCKET sock, uint32_t entity_id);

#endif
