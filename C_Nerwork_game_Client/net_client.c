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
    MsgHeader joinHeader;
    PayloadJoin joinPayload;

    // 헤더 구성
    joinHeader.type = MSG_JOIN;
    joinHeader.length = htonl(sizeof(PayloadJoin));

    // 페이로드 구성
    joinPayload.role = role;

    // 전송
    send_full(sock, &joinHeader, sizeof(joinHeader));
    send_full(sock, &joinPayload, sizeof(joinPayload));
}

// 게임 준비 신호 전송
void send_ready(SOCKET sock, uint32_t my_entity_id) {
    MsgHeader h = {
        .type = MSG_READY,
        .length = htonl(0)
    };
    send_full(sock, &h, sizeof(h));
}

// 엔티티 상태 업데이트 전송
void send_state_update(SOCKET sock, uint32_t id, int x, int y) {
    MsgHeader stateHeader;
    PayloadStateUpdate statePayload;

    // 헤더 구성
    stateHeader.type = MSG_STATE_UPDATE;
    stateHeader.length = htonl(sizeof(PayloadStateUpdate));

    // 페이로드 구성
    statePayload.entityId = htonl(id);
    statePayload.x = x;
    statePayload.y = y;

    // 전송
    send_full(sock, &stateHeader, sizeof(stateHeader));
    send_full(sock, &statePayload, sizeof(statePayload));
}

// 총알 발사 이벤트 전송
void send_shooting_event(SOCKET sock, uint32_t shooter_id, uint32_t bullet_id, int dirX, int dirY) {
    MsgHeader shootingHeader;
    PayloadShootingEvent shootingPayload;

    // 헤더 구성
    shootingHeader.type = MSG_SHOOTING_EVENT;
    shootingHeader.length = htonl(sizeof(PayloadShootingEvent));

    // 페이로드 구성
    shootingPayload.shooterId = htonl(shooter_id);
    shootingPayload.bulletId = htonl(bullet_id);
    shootingPayload.dirX = dirX;
    shootingPayload.dirY = dirY;

    // 전송
    send_full(sock, &shootingHeader, sizeof(shootingHeader));
    send_full(sock, &shootingPayload, sizeof(shootingPayload));
}

// 재장전 요청 전송
void send_reload_request(SOCKET sock, uint32_t entity_id) {
    MsgHeader reloadHeader;
    PayloadGameEvent reloadPayload;

    // 헤더 구성
    reloadHeader.type = MSG_GAME_EVENT;
    reloadHeader.length = htonl(sizeof(PayloadGameEvent));

    // 페이로드 구성
    reloadPayload.event_type = RELOAD_REQUEST;
    reloadPayload.entityId = htonl(entity_id);

    // 전송
    send_full(sock, &reloadHeader, sizeof(reloadHeader));
    send_full(sock, &reloadPayload, sizeof(reloadPayload));
}

// 리스폰 요청 전송
void send_respawn_request(SOCKET sock, uint32_t entity_id) {
    MsgHeader respawnHeader;
    PayloadGameEvent respawnPayload;

    // 헤더 구성
    respawnHeader.type = MSG_GAME_EVENT;
    respawnHeader.length = htonl(sizeof(PayloadGameEvent));

    // 페이로드 구성
    respawnPayload.event_type = RESPAWN_REQUEST;
    respawnPayload.entityId = htonl(entity_id);

    // 전송
    send_full(sock, &respawnHeader, sizeof(respawnHeader));
    send_full(sock, &respawnPayload, sizeof(respawnPayload));
}
