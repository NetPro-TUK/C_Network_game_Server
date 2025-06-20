#include "net_client.h"
#include "protocol.h"
#include "net_utils.h"
#include <stdio.h>
#include <stdlib.h>

// 서버에 TCP 연결을 시도하는 함수
SOCKET connect_to_server(const char* ip, int port) {
    WSADATA wsaData;
    SOCKET sock;
    SOCKADDR_IN servAdr;

    // 윈속 초기화 (필수)
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fputs("WSAStartup() error!\n", stderr);
        return INVALID_SOCKET;
    }

    // TCP 소켓 생성
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        fputs("socket() error\n", stderr);
        return INVALID_SOCKET;
    }

    // 서버 주소 설정
    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_addr.s_addr = inet_addr(ip);    // 문자열 IP → 정수 변환
	servAdr.sin_port = htons(port);              // 포트 → 네트워크 바이트 순서

    // 서버에 연결 요청
    if (connect(sock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR) {
        fputs("connect() error\n", stderr);
        closesocket(sock);
        WSACleanup();
        return INVALID_SOCKET;
    }

    return sock; // 연결된 소켓 핸들 반환
}

// 클라이언트 역할을 서버에 전송하고, 서버가 부여한 entity ID를 수신
uint32_t send_join_and_get_id(SOCKET sock, int role) {
    PayloadJoin joinPayload = { .role = role };
    MsgHeader joinHeader = {
        .type = MSG_JOIN,
        .length = htonl(sizeof(joinPayload))    // 페이로드 크기 전송
    };

    // 메시지 전송 (헤더 → 페이로드)
    send_full(sock, &joinHeader, sizeof(joinHeader));
    send_full(sock, &joinPayload, sizeof(joinPayload));

    // 서버로부터 응답 메시지 수신
    MsgHeader ackHeader;
    recv_full(sock, &ackHeader, sizeof(ackHeader));
    if (ackHeader.type != MSG_JOIN_ACK || ntohl(ackHeader.length) != sizeof(PayloadJoinAck)) {
        printf("Client> 잘못된 MSG_JOIN_ACK 수신\n");
        closesocket(sock);
        WSACleanup();
        exit(1);
    }

    // 실제 PayloadJoinAck 수신
    PayloadJoinAck ackPayload;
    recv_full(sock, &ackPayload, sizeof(ackPayload));

    // 네트워크 바이트 순서 → 호스트 바이트 순서 변환
    return ntohl(ackPayload.entityId);
}

void send_state_update(SOCKET sock, uint32_t id, int x, int y) {
    // 상태 정보 페이로드 생성
    PayloadStateUpdate statePayload = {
        .entityId = htonl(id),  // 서버와 통신 시 ID는 네트워크 바이트 순서로
        .x = x,
        .y = y
    };
    // 헤더 생성
    MsgHeader stateHeader = {
        .type = MSG_STATE_UPDATE,
        .length = htonl(sizeof(statePayload))
    };
    // 전송 (헤더 → 페이로드 순서)
    send_full(sock, &stateHeader, sizeof(stateHeader));
    send_full(sock, &statePayload, sizeof(statePayload));
}

// 게임 이벤트 메세지
void handle_server_message(MsgHeader* header, void* payload) {
    if (header->type == MSG_GAME_EVENT) {
        PayloadGameEvent* gamePayload = (PayloadGameEvent*)payload;

        switch (gamePayload->event_type) {
        case GAME_OVER:
            printf("게임 종료!\n");
            break;
        case GAME_WIN:
            printf("게임 승리!\n");
            break;
        case PLAYER_REJECTED:
            printf("[알림] 방어자는 이미 존재합니다. 게임에 참여할 수 없습니다.\n");
            exit(0);  // 또는 return; 등 적절한 조치
        default:
            printf("알 수 없는 게임 이벤트 수신: %d\n", gamePayload->event_type);
            break;
        }
    }
}