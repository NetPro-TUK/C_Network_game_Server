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
void send_join_and_get_id(SOCKET sock, int role) {
    PayloadJoin joinPayload = { .role = role };
    MsgHeader joinHeader = {
        .type = MSG_JOIN,
        .length = htonl(sizeof(joinPayload))
    };
    send_full(sock, &joinHeader, sizeof(joinHeader));
    send_full(sock, &joinPayload, sizeof(joinPayload));
}

// 게임 준비
void send_ready(SOCKET sock, uint32_t my_entity_id) {
    MsgHeader h = {
        .type = MSG_READY,
        .length = htonl(0)
    };
    send_full(sock, &h, sizeof(h));
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

void send_shooting_event(SOCKET sock, uint32_t shooter_id, uint32_t bullet_id, int dirX, int dirY) {
    MsgHeader header;
    PayloadShootingEvent payload;

    // 헤더 구성
    header.length = htonl(sizeof(PayloadShootingEvent));  // 네트워크 바이트 순서로 변환
    header.type = MSG_SHOOTING_EVENT;                   // enum 값 그대로 전송

    // 페이로드 구성
    payload.shooterId = htonl(shooter_id);
    payload.bulletId = htonl(bullet_id);
    payload.dirX = dirX;
    payload.dirY = dirY;

    // 전송
    send(sock, (char*)&header, sizeof(MsgHeader), 0);
    send(sock, (char*)&payload, sizeof(PayloadShootingEvent), 0);
}

void send_reload_request(SOCKET sock, uint32_t entity_id) {
    MsgHeader header;
    PayloadGameEvent payload;

    header.type = MSG_GAME_EVENT;
    header.length = htonl(sizeof(PayloadGameEvent));

    payload.event_type = RELOAD_REQUEST;
    payload.entityId = htonl(entity_id);

    send(sock, (char*)&header, sizeof(header), 0);
    send(sock, (char*)&payload, sizeof(payload), 0);
}