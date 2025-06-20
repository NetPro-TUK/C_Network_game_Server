#ifndef NET_SERVER_H
#define NET_SERVER_H

#include <winsock2.h>

#define MAX_CLIENT  64

extern SOCKET sockArr[MAX_CLIENT];
extern WSAEVENT eventArr[MAX_CLIENT];
extern int numOfClnt;	// ������ Ŭ���̾�Ʈ ����

// ���� �ʱ�ȭ �� ��Ʈ ���ε�
int		init_server_socket(int port);

// ���ο� Ŭ���̾�Ʈ ���� �� �̺�Ʈ �迭 ���
void	accept_new_client(SOCKET serverSock);

// Ŭ���̾�Ʈ�κ��� �޽��� ���� �� Ÿ�Ժ� ó��
int		recv_and_dispatch(int clientIndex);

// Ư�� Ŭ���̾�Ʈ���� ������ ����
void	send_to_client(SOCKET sock, const void* buf, int len);

// ��� Ŭ���̾�Ʈ���� ������ ��ε�ĳ��Ʈ
void	broadcast_all(const void* buf, int len);

// Ŭ���̾�Ʈ ���� ó��
void remove_client_at(int index);
#endif
