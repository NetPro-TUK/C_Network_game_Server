#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "protocol.h"
#include "net_utils.h"
#include "log.h"

#define MAX_CLIENT  64       // �ִ� Ŭ���̾�Ʈ ��
#define BUF_SIZE    512      // �ӽ� ���� ũ��

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

	SOCKET sockArr[MAX_CLIENT];       // Ŭ���̾�Ʈ ���� ���� �迭
	WSAEVENT eventArr[MAX_CLIENT];    // �� ���Ͽ� ���� �̺�Ʈ �ڵ� ���� �迭
	int numOfClnt = 0;

	// WinSock �ʱ�ȭ
	if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!"); 

	// ���� ���� ����
	hServSock = socket(PF_INET, SOCK_STREAM, 0);  

	if(hServSock == INVALID_SOCKET)
		ErrorHandling("socket() error");

	// ���� �ּ� ����
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = inet_addr("127.0.0.1"); // htonl(INADDR_ANY);
	servAdr.sin_port = htons(9000);

	// ���ε�
	if(bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
		ErrorHandling("bind() error");
	
	// ����
	listen(hServSock, MAX_CLIENT);
	
	// ���� ���Ͽ� ���� �̺�Ʈ ���� �� ����
	eventArr[0] = WSACreateEvent();
	sockArr[0] = hServSock;
	WSAEventSelect(hServSock, eventArr[0], FD_ACCEPT);
	numOfClnt = 1;


    while (1) {
        // �̺�Ʈ �߻� ���
        int index = WSAWaitForMultipleEvents(numOfClnt, eventArr, FALSE, WSA_INFINITE, FALSE);
        int startIdx = index - WSA_WAIT_EVENT_0;

        for (int i = startIdx; i < numOfClnt; ++i) {
            // �̺�Ʈ Ȯ��
            int sigIdx = WSAWaitForMultipleEvents(1, &eventArr[i], TRUE, 0, FALSE);
            if (sigIdx == WSA_WAIT_FAILED || sigIdx == WSA_WAIT_TIMEOUT)
                continue;

            WSANETWORKEVENTS netEvents;
            WSAEnumNetworkEvents(sockArr[i], eventArr[i], &netEvents);

            // ���ο� Ŭ���̾�Ʈ ����
            if (netEvents.lNetworkEvents & FD_ACCEPT) {
                clntAdrSize = sizeof(clntAdr);
                hClntSock = accept(sockArr[i], (SOCKADDR*)&clntAdr, &clntAdrSize);
                if (hClntSock == INVALID_SOCKET) continue;

                // �� ���ϰ� �̺�Ʈ ���
                eventArr[numOfClnt] = WSACreateEvent();
                sockArr[numOfClnt] = hClntSock;
                WSAEventSelect(hClntSock, eventArr[numOfClnt], FD_READ | FD_CLOSE);

                printf("Server> client connected: %s:%d\n", inet_ntoa(clntAdr.sin_addr), ntohs(clntAdr.sin_port));
                ++numOfClnt;
            }
            // Ŭ���̾�Ʈ ������ ����
            else if (netEvents.lNetworkEvents & FD_READ) {
                MsgHeader header;
                int ret = recv_full(sockArr[i], &header, sizeof(header));
                if (ret <= 0) {
                    // ���� ���� ó��
                    closesocket(sockArr[i]);
                    WSACloseEvent(eventArr[i]);
                    sockArr[i] = sockArr[numOfClnt - 1];
                    eventArr[i] = eventArr[numOfClnt - 1];
                    --numOfClnt; --i;
                    printf("Server> client ����\n");
                    continue;
                }

                // �޽��� �ؼ�
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
                    // ó�� �� �Ǵ� �޽����� ��ŵ
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
            // Ŭ���̾�Ʈ ���� ����
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
