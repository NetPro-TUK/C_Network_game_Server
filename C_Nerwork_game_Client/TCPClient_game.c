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
	 
	// TCP 연결 요청...
	int ret;
	ret = connect(hSocket, (SOCKADDR*)&servAdr, sizeof(servAdr));
	if (ret == SOCKET_ERROR) {
		printf("<ERROR> Client. connect() 실행 오류.\n");
		closesocket(hSocket);
		printf("Client> close socket...\n");
		WSACleanup();
		return 0;
	}
	else {
		printf("Client> connection established...\n");
	}
	
	// -----------------------------
	// MsgHeader + Payload 전송
	// -----------------------------
	MsgHeader header;
	PayloadStateUpdate payload;

	// 엔터티 정보 설정 (float은 그대로 사용)
	payload.entityId = htonl(1);  // ID만 바이트 오더 처리
	payload.x = 10.5f;
	payload.y = 20.0f;
	payload.vx = 1.0f;
	payload.vy = 0.0f;

	header.length = htonl(sizeof(PayloadStateUpdate));
	header.type = MSG_STATE_UPDATE;

	// 전송
	if (send_full(hSocket, &header, sizeof(header)) <= 0) {
		LOG_ERROR("헤더 전송 실패");
	}
	else {
		LOG_INFO("헤더 전송 완료");
	}

	if (send_full(hSocket, &payload, sizeof(payload)) <= 0) {
		LOG_ERROR("페이로드 전송 실패");
	}
	else {
		LOG_INFO("페이로드 전송 완료");
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