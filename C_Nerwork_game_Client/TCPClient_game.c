#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#include "protocol.h"
#include "net_utils.h"
#include "log.h"

void ErrorHandling(char* message);
#define MAX_PACKET_SIZE  120

#define ENTITY_PLAYER 1
#define ENTITY_ATTACKER 2


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
        WSACleanup();
        return 0;
    }

    printf("Client> connection established...\n");

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
    // 2. MSG_JOIN_ACK 응답 수신
    // -----------------------------
    MsgHeader ackHeader;
    recv_full(hSocket, &ackHeader, sizeof(ackHeader));
    if (ackHeader.type != MSG_JOIN_ACK || ntohl(ackHeader.length) != sizeof(PayloadJoinAck)) {
        printf("Client> 잘못된 MSG_JOIN_ACK 수신\n");
        closesocket(hSocket); WSACleanup(); return 1;
    }

    PayloadJoinAck ackPayload;
    recv_full(hSocket, &ackPayload, sizeof(ackPayload));
    uint32_t myId = ntohl(ackPayload.entityId);
    printf("Client> 내 Entity ID = %u, 역할 = %s\n", myId,
        ackPayload.role == ENTITY_PLAYER ? "PLAYER" : "ATTACKER");

    // -----------------------------
    // 3. 테스트용 MSG_STATE_UPDATE 전송
    // -----------------------------
    PayloadStateUpdate statePayload;
    statePayload.entityId = htonl(myId);
    statePayload.x = 10;
    statePayload.y = 20;

    MsgHeader stateHeader = {
        .type = MSG_STATE_UPDATE,
        .length = htonl(sizeof(statePayload))
    };

    send_full(hSocket, &stateHeader, sizeof(stateHeader));
    send_full(hSocket, &statePayload, sizeof(statePayload));
    printf("Client> 상태 전송 완료\n");

    // -----------------------------
    // 4. 테스트용 총알 발사(MSG_ACTION_EVENT) 전송
    // -----------------------------
    PayloadActionEvent actionPayload;
    actionPayload.shooterId = htonl(myId);
    actionPayload.bulletId = htonl(999);  // 테스트용 Bullet ID
    actionPayload.dirX = 1;               // 오른쪽 방향
    actionPayload.dirY = 0;

    MsgHeader actionHeader = {
        .type = MSG_ACTION_EVENT,
        .length = htonl(sizeof(actionPayload))
    };

    send_full(hSocket, &actionHeader, sizeof(actionHeader));
    send_full(hSocket, &actionPayload, sizeof(actionPayload));
    printf("Client> 총알 발사 전송 완료 (bulletId = 999)\n");

    // -----------------------------
    // 5. 종료
    // -----------------------------
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
