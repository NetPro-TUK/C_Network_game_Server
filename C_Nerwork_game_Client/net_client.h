#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include <winsock2.h>
#include <stdint.h>  

// ������ TCP�� ���� ��û�� ������, ���� �� ����� ������ ��ȯ��
SOCKET connect_to_server(const char* ip, int port);

// Ŭ���̾�Ʈ�� ����(PLAYER or ATTACKER)�� ������ �����ϰ�,
// ������ �ο��� ���� entity ID�� �޾Ƽ� ��ȯ
uint32_t send_join_and_get_id(SOCKET sock, int role);

// Ŭ���̾�Ʈ�� ���� ��ġ(x, y) ������ ������ ����
void send_state_update(SOCKET sock, uint32_t id, int x, int y);

#endif
