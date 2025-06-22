#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include <winsock2.h>
#include <stdint.h> 
#include "protocol.h"


typedef enum {
    ROLE_STATUS_PENDING = 0,   // ���� ���� ���� �� ��
    ROLE_STATUS_APPROVED = 1,  // ���� ���� ���ε�
    ROLE_STATUS_REJECTED = 2   // ���� ���� �źε�
} RoleStatus;

// ���� �غ�
void send_ready(SOCKET sock, uint32_t my_entity_id);

// ������ TCP�� ���� ��û�� ������, ���� �� ����� ������ ��ȯ��
SOCKET connect_to_server(const char* ip, int port);

// Ŭ���̾�Ʈ�� ����(DEFENDER or ATTACKER)�� ������ �����ϰ�,
// ������ �ο��� ���� entity ID�� �޾Ƽ� ��ȯ
void send_join_and_get_id(SOCKET sock, int role);

// �غ� �Ϸ� ��ȣ�� ������ ����
void send_ready(SOCKET sock, uint32_t my_entity_id);

// Ŭ���̾�Ʈ�� ���� ��ġ(x, y) ������ ������ ����
void send_state_update(SOCKET sock, uint32_t id, int x, int y);

// ���� �̺�Ʈ �޼���
void handle_server_message(MsgHeader* header, void* payload);

// Ŭ���̾�Ʈ�� ������ ������ �������� ��, �Ѿ� �߻� �̺�Ʈ�� ������ ����
void send_shooting_event(SOCKET sock, uint32_t shooter_id, uint32_t bullet_id, int dirX, int dirY);

// Ŭ���̾�Ʈ�� ������ ��û�� ������ ����
void send_reload_request(SOCKET sock, uint32_t entity_id);

// Ŭ���̾�Ʈ�� ������ ��û�� ������ ����
void send_respawn_request(SOCKET sock, uint32_t entity_id);

#endif
