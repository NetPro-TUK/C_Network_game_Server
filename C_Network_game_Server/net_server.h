#ifndef NET_SERVER_H
#define NET_SERVER_H

#include <winsock2.h>
#include <stdint.h>
#include <stdbool.h>

// ����̾�Ʈ ���� �ִ� ��
#define MAX_CLIENT  20


extern bool game_started;			// ���� ����						
extern bool server_game_over;		// ���� ����
extern bool defender_ready;			// ����� �غ� (�����, ������ �ּ� �Ѹ� �̻��� �� ����)
extern bool attacker_ready;			// ������ �غ�

extern SOCKET sockArr[MAX_CLIENT];		// Ŭ�� �ۼ��� ����
extern WSAEVENT eventArr[MAX_CLIENT];	// �� �̺�Ʈ ��ü
extern int numOfClnt;					// ������ Ŭ���̾�Ʈ ����
extern uint32_t current_score;			// ����� ���� ����
extern uint32_t defender_owner_id;		// ����� �ߺ� ���� ����

// ���� �ʱ�ȭ �� ��Ʈ ���ε�
int init_server_socket(int port);

// ���ο� Ŭ���̾�Ʈ ���� �� �̺�Ʈ �迭 ���
void accept_new_client(SOCKET serverSock);

// ���� ������ Ŭ���̾�Ʈ ID �ο�
uint32_t generate_client_id();

// recv �޽��� ���� �� Ÿ�Ժ� ó��
int recv_and_dispatch(int clientIndex);

// ��� Ŭ���̾�Ʈ���� ������ ��ε�ĳ��Ʈ
void broadcast_all(const void* buf, int len);

// Ŭ���̾�Ʈ ���� ó��
void remove_client_at(int index);

// Ŭ���̾�Ʈ ���� �� ����� ���� �ʱ�ȭ
void reset_defender_if_match(SOCKET closingSock);

#endif
