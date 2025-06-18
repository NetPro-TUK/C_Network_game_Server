#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "protocol.h"
#include "net_utils.h"
#include "log.h"

#define MAX_PACKET_SIZE  120
void ErrorHandling(char *message);
int calculation(int opndCnt, int data[], char op);
DWORD WINAPI ProcessClient(LPVOID arg);

int main(void)
{
	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAdr, clntAdr;
	int clntAdrSize;

	if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!"); 
	
	hServSock = socket(PF_INET, SOCK_STREAM, 0);  

	if(hServSock == INVALID_SOCKET)
		ErrorHandling("socket() error");
	
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = inet_addr("127.0.0.1"); // htonl(INADDR_ANY);
	servAdr.sin_port = htons(9000);

	if(bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
		ErrorHandling("bind() error");
	
	// 클라이언트에게 서비스할 준비가 완료...
	listen(hServSock, 3);
	
	HANDLE hThread;
	DWORD ThreadId;
	while (1) {
		printf("Server> 클라이언트 연결 요청 대기중.\n");
		clntAdrSize = sizeof(clntAdr);
		hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &clntAdrSize);
		if (hClntSock == -1) {
			printf("<ERROR> accept 실행 오류.\n");
		}
		else {
			printf("Server> client(IP:%s, Port:%d) 연결됨.\n",
				inet_ntoa(clntAdr.sin_addr), ntohs(clntAdr.sin_port));
		}
		//--- START
		hThread = //CreateThread(NULL, 0, ProcessClient, (LPVOID)hClntSock, 0, &ThreadId);
				  _beginthreadex(NULL, 0, ProcessClient, (LPVOID)hClntSock, 0, &ThreadId);
		if ( hThread == NULL ) {
			printf("<ERROR> 스레드 생성 실패\n");
		}
		else {
			CloseHandle(hThread);
		}
		//--- END
	}
	closesocket(hServSock);
	WSACleanup();
	return 0;
}

DWORD WINAPI ProcessClient(LPVOID arg) {
	SOCKET hClntSock = (SOCKET)arg;
	MsgHeader header;
	int flag = 1;

	LOG_INFO("THREAD> 클라이언트 핸들러 시작됨.");

	while (flag) {
		// 1. MsgHeader 수신
		int ret = recv_full(hClntSock, &header, sizeof(header));
		if (ret = 0) {
			LOG_ERROR("헤더 수신 실패 또는 연결 종료");
			break;
		}

		// 2. 헤더 정보 해석
		uint32_t payload_len = ntohl(header.length);
		MsgType type = header.type;

		LOG_INFO("헤더 수신 완료: type=%d, length=%d", type, payload_len);

		// 3. 메시지 타입에 따라 분기
		if (type == MSG_STATE_UPDATE && payload_len == sizeof(PayloadStateUpdate)) {
			PayloadStateUpdate payload;
			ret = recv_full(hClntSock, &payload, sizeof(payload));
			if (ret <= 0) {
				LOG_ERROR("PayloadStateUpdate 수신 실패");
				break;
			}

			// 4. 바이트 오더 변환
			uint32_t id = ntohl(payload.entityId);  
			float x = payload.x;
			float y = payload.y;

			printf("[STATE_UPDATE] id=%u, pos=(%.2f, %.2f), \n",
				id, x, y);
		}
		else {
			// 아직 처리 안 하는 타입
			LOG_WARN("알 수 없거나 처리하지 않은 메시지 타입 수신됨");
			// 버퍼 날리기
			char dump[512];
			if (payload_len > 0 && payload_len < sizeof(dump)) {
				recv_full(hClntSock, dump, payload_len);
			}
			else {
				LOG_WARN("payload 길이 초과, 스킵");
				break;
			}
		}
	}

	LOG_INFO("THREAD> 클라이언트 소켓 종료");
	closesocket(hClntSock);
	return 0;
}


void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
