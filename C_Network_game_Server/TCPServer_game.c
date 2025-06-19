#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "protocol.h"
#include "net_utils.h"
#include "log.h"

#define MAX_CLIENT  64       // 최대 클라이언트 수
#define BUF_SIZE    512      // 임시 버퍼 크기

void ErrorHandling(char *message);
DWORD WINAPI ProcessClient(LPVOID arg);

typedef struct {
    uint32_t entityId;
    MsgType type;
    int x;
    int y;
} Entity;

int main(void)
{
	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAdr, clntAdr;
	int clntAdrSize;

	SOCKET sockArr[MAX_CLIENT];       // 클라이언트 소켓 저장 배열
	WSAEVENT eventArr[MAX_CLIENT];    // 각 소켓에 대한 이벤트 핸들 저장 배열
	int numOfClnt = 0;

	// WinSock 초기화
	if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!"); 

	// 서버 소켓 생성
	hServSock = socket(PF_INET, SOCK_STREAM, 0);  

	if(hServSock == INVALID_SOCKET)
		ErrorHandling("socket() error");

	// 서버 주소 설정
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = inet_addr("127.0.0.1"); // htonl(INADDR_ANY);
	servAdr.sin_port = htons(9000);

	// 바인드
	if(bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
		ErrorHandling("bind() error");
	
	// 리슨
	listen(hServSock, MAX_CLIENT);
	
	// 서버 소켓에 대한 이벤트 생성 및 설정
	eventArr[0] = WSACreateEvent();
	sockArr[0] = hServSock;
	WSAEventSelect(hServSock, eventArr[0], FD_ACCEPT);
	numOfClnt = 1;


    while (1) {
        // 이벤트 발생 대기
        int index = WSAWaitForMultipleEvents(numOfClnt, eventArr, FALSE, WSA_INFINITE, FALSE);
        int startIdx = index - WSA_WAIT_EVENT_0;

        for (int i = startIdx; i < numOfClnt; ++i) {
            // 이벤트 확인
            int sigIdx = WSAWaitForMultipleEvents(1, &eventArr[i], TRUE, 0, FALSE);
            if (sigIdx == WSA_WAIT_FAILED || sigIdx == WSA_WAIT_TIMEOUT)
                continue;

            WSANETWORKEVENTS netEvents;
            WSAEnumNetworkEvents(sockArr[i], eventArr[i], &netEvents);

            // 새로운 클라이언트 접속
            if (netEvents.lNetworkEvents & FD_ACCEPT) {
                clntAdrSize = sizeof(clntAdr);
                hClntSock = accept(sockArr[i], (SOCKADDR*)&clntAdr, &clntAdrSize);
                if (hClntSock == INVALID_SOCKET) continue;

                // 새 소켓과 이벤트 등록
                eventArr[numOfClnt] = WSACreateEvent();
                sockArr[numOfClnt] = hClntSock;
                WSAEventSelect(hClntSock, eventArr[numOfClnt], FD_READ | FD_CLOSE);

                printf("Server> client connected: %s:%d\n", inet_ntoa(clntAdr.sin_addr), ntohs(clntAdr.sin_port));
                ++numOfClnt;
            }
            // 클라이언트 데이터 수신
            else if (netEvents.lNetworkEvents & FD_READ) {
                MsgHeader header;
                int ret = recv_full(sockArr[i], &header, sizeof(header));
                if (ret <= 0) {
                    // 소켓 종료 처리
                    closesocket(sockArr[i]);
                    WSACloseEvent(eventArr[i]);
                    sockArr[i] = sockArr[numOfClnt - 1];
                    eventArr[i] = eventArr[numOfClnt - 1];
                    --numOfClnt; --i;
                    printf("Server> client 종료\n");
                    continue;
                }

                // 메시지 해석
                MsgType type = header.type;
                uint32_t payload_len = ntohl(header.length);

                printf("[RECV] Header: type=%d, length=%d\n", type, payload_len);

                if (type == MSG_STATE_UPDATE && payload_len == sizeof(PayloadStateUpdate)) {
                    PayloadStateUpdate payload;
                    ret = recv_full(sockArr[i], &payload, sizeof(payload));
                    if (ret > 0) {
                        uint32_t id = ntohl(payload.entityId);
                        printf("[STATE_UPDATE] id=%u, pos=(%d, %d)\n",
                            id, payload.x, payload.y);
                    }
                }
                else {
                    // 처리 안 되는 메시지는 스킵
                    char dump[512];
                    if (payload_len < sizeof(dump)) recv_full(sockArr[i], dump, payload_len);
                    else {
                        closesocket(sockArr[i]);
                        WSACloseEvent(eventArr[i]);
                        sockArr[i] = sockArr[numOfClnt - 1];
                        eventArr[i] = eventArr[numOfClnt - 1];
                        --numOfClnt; --i;
                    }
                }
            }
            // 클라이언트 연결 종료
            else if (netEvents.lNetworkEvents & FD_CLOSE) {
                closesocket(sockArr[i]);
                WSACloseEvent(eventArr[i]);
                sockArr[i] = sockArr[numOfClnt - 1];
                eventArr[i] = eventArr[numOfClnt - 1];
                --numOfClnt; --i;
            }
        }
    }
	closesocket(hServSock);
	WSACleanup();
	return 0;
}

void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
