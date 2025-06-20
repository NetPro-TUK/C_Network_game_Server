#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#include "protocol.h"
#include "net_utils.h"
#include "log.h"

void ErrorHandling(char* message);
#define MAX_PACKET_SIZE  120

int main(void)
{
    WSADATA wsaData;
    SOCKET hSocket;
    SOCKADDR_IN servAdr;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        ErrorHandling("WSAStartup() error!");

    hSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (hSocket == INVALID_SOCKET)
        ErrorHandling("socket() error");

    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servAdr.sin_port = htons(9000);

    int ret = connect(hSocket, (SOCKADDR*)&servAdr, sizeof(servAdr));
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
    // 1. 역할 선택 및 MSG_JOIN 전송
    // -----------------------------
    int role;
    printf("역할 선택: (1) 방어자, (2) 공격자 > ");
    scanf("%d", &role);
    if (role != 1 && role != 2) {
        printf("잘못된 입력. 기본값: 방어자(1)\n");
        role = 1;
    }

    PayloadJoin joinPayload = { .role = role };
    MsgHeader joinHeader = {
        .type = MSG_JOIN,
        .length = htonl(sizeof(joinPayload))
    };

    send_full(hSocket, &joinHeader, sizeof(joinHeader));
    send_full(hSocket, &joinPayload, sizeof(joinPayload));
    printf("Client> 역할 선택 전송 완료 (%s)\n", role == 1 ? "방어자" : "공격자");

    // -----------------------------
    // 2. 테스트용 STATE_UPDATE 전송
    // -----------------------------
    PayloadStateUpdate payload;
    payload.entityId = htonl(1); // 테스트용 ID
    payload.x = 10;
    payload.y = 20;

    MsgHeader stateHeader = {
        .type = MSG_STATE_UPDATE,
        .length = htonl(sizeof(payload))
    };

    send_full(hSocket, &stateHeader, sizeof(stateHeader));
    send_full(hSocket, &payload, sizeof(payload));
    printf("Client> 상태 전송 완료\n");

    // (옵션) 서버 응답 수신
    recv_full(hSocket, &stateHeader, sizeof(stateHeader));
    printf("Client> 헤더 수신 완료: type = %d, length = %d\n", stateHeader.type, ntohl(stateHeader.length));

    closesocket(hSocket);
    printf("Client> close socket...\n");
    WSACleanup();
    return 0;
}

void ErrorHandling(char* message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
