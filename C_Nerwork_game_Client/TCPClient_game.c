#include <stdio.h>
#include <stdlib.h>

#include "protocol.h"
#include "net_utils.h"
#include "log.h"

void ErrorHandling(char *message);
#define MAX_PACKET_SIZE  120

int main(void)
{
	WSADATA wsaData;
	SOCKET hSocket;
	SOCKADDR_IN servAdr;
	
	if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!"); 

	hSocket = socket(PF_INET, SOCK_STREAM, 0);   
	if(hSocket == INVALID_SOCKET)
		ErrorHandling("socket() error");
	
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servAdr.sin_port = htons(9000);
	 
	// TCP ���� ��û...
	int ret;
	ret = connect(hSocket, (SOCKADDR*)&servAdr, sizeof(servAdr));
	if (ret == SOCKET_ERROR) {
		printf("<ERROR> Client. connect() ���� ����.\n");
		closesocket(hSocket);
		printf("Client> close socket...\n");
		WSACleanup();
		return 0;
	}
	else {
		printf("Client> connection established...\n");
	}
	
	// -----------------------------
	// MsgHeader + Payload ����
	// -----------------------------
	MsgHeader header;
	PayloadStateUpdate payload;

	// ����Ƽ ���� ���� (float�� �״�� ���)
	payload.entityId = htonl(1);  // ID�� ����Ʈ ���� ó��
	payload.x = 10.5f;
	payload.y = 20.0f;
	payload.vx = 1.0f;
	payload.vy = 0.0f;

	header.length = htonl(sizeof(PayloadStateUpdate));
	header.type = MSG_STATE_UPDATE;

	// ����
	if (send_full(hSocket, &header, sizeof(header)) <= 0) {
		LOG_ERROR("��� ���� ����");
	}
	else {
		LOG_INFO("��� ���� �Ϸ�");
	}

	if (send_full(hSocket, &payload, sizeof(payload)) <= 0) {
		LOG_ERROR("���̷ε� ���� ����");
	}
	else {
		LOG_INFO("���̷ε� ���� �Ϸ�");
	}

	closesocket(hSocket);
	printf("Client> close socket...\n");
	WSACleanup();
	return 0;
}

void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}