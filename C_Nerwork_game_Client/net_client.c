#include "net_client.h"
#include "protocol.h"
#include "net_utils.h"
#include <stdio.h>
#include <stdlib.h>


// ������ TCP ������ �õ��ϴ� �Լ�
SOCKET connect_to_server(const char* ip, int port) {
    WSADATA wsaData;
    SOCKET sock;
    SOCKADDR_IN servAdr;

    // ���� �ʱ�ȭ (�ʼ�)
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fputs("WSAStartup() error!\n", stderr);
        return INVALID_SOCKET;
    }

    // TCP ���� ����
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        fputs("socket() error\n", stderr);
        return INVALID_SOCKET;
    }

    // ���� �ּ� ����
    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_addr.s_addr = inet_addr(ip);    // ���ڿ� IP �� ���� ��ȯ
	servAdr.sin_port = htons(port);              // ��Ʈ �� ��Ʈ��ũ ����Ʈ ����

    // ������ ���� ��û
    if (connect(sock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR) {
        fputs("connect() error\n", stderr);
        closesocket(sock);
        WSACleanup();
        return INVALID_SOCKET;
    }

    return sock; // ����� ���� �ڵ� ��ȯ
}

// Ŭ���̾�Ʈ ������ ������ �����ϰ�, ������ �ο��� entity ID�� ����
void send_join_and_get_id(SOCKET sock, int role) {
    MsgHeader joinHeader;
    PayloadJoin joinPayload;

    // ��� ����
    joinHeader.type = MSG_JOIN;
    joinHeader.length = htonl(sizeof(PayloadJoin));

    // ���̷ε� ����
    joinPayload.role = role;

    // ����
    send_full(sock, &joinHeader, sizeof(joinHeader));
    send_full(sock, &joinPayload, sizeof(joinPayload));
}

// ���� �غ� ��ȣ ����
void send_ready(SOCKET sock, uint32_t my_entity_id) {
    MsgHeader h = {
        .type = MSG_READY,
        .length = htonl(0)
    };
    send_full(sock, &h, sizeof(h));
}

// ��ƼƼ ���� ������Ʈ ����
void send_state_update(SOCKET sock, uint32_t id, int x, int y) {
    MsgHeader stateHeader;
    PayloadStateUpdate statePayload;

    // ��� ����
    stateHeader.type = MSG_STATE_UPDATE;
    stateHeader.length = htonl(sizeof(PayloadStateUpdate));

    // ���̷ε� ����
    statePayload.entityId = htonl(id);
    statePayload.x = x;
    statePayload.y = y;

    // ����
    send_full(sock, &stateHeader, sizeof(stateHeader));
    send_full(sock, &statePayload, sizeof(statePayload));
}

// �Ѿ� �߻� �̺�Ʈ ����
void send_shooting_event(SOCKET sock, uint32_t shooter_id, uint32_t bullet_id, int dirX, int dirY) {
    MsgHeader shootingHeader;
    PayloadShootingEvent shootingPayload;

    // ��� ����
    shootingHeader.type = MSG_SHOOTING_EVENT;
    shootingHeader.length = htonl(sizeof(PayloadShootingEvent));

    // ���̷ε� ����
    shootingPayload.shooterId = htonl(shooter_id);
    shootingPayload.bulletId = htonl(bullet_id);
    shootingPayload.dirX = dirX;
    shootingPayload.dirY = dirY;

    // ����
    send_full(sock, &shootingHeader, sizeof(shootingHeader));
    send_full(sock, &shootingPayload, sizeof(shootingPayload));
}

// ������ ��û ����
void send_reload_request(SOCKET sock, uint32_t entity_id) {
    MsgHeader reloadHeader;
    PayloadGameEvent reloadPayload;

    // ��� ����
    reloadHeader.type = MSG_GAME_EVENT;
    reloadHeader.length = htonl(sizeof(PayloadGameEvent));

    // ���̷ε� ����
    reloadPayload.event_type = RELOAD_REQUEST;
    reloadPayload.entityId = htonl(entity_id);

    // ����
    send_full(sock, &reloadHeader, sizeof(reloadHeader));
    send_full(sock, &reloadPayload, sizeof(reloadPayload));
}

// ������ ��û ����
void send_respawn_request(SOCKET sock, uint32_t entity_id) {
    MsgHeader respawnHeader;
    PayloadGameEvent respawnPayload;

    // ��� ����
    respawnHeader.type = MSG_GAME_EVENT;
    respawnHeader.length = htonl(sizeof(PayloadGameEvent));

    // ���̷ε� ����
    respawnPayload.event_type = RESPAWN_REQUEST;
    respawnPayload.entityId = htonl(entity_id);

    // ����
    send_full(sock, &respawnHeader, sizeof(respawnHeader));
    send_full(sock, &respawnPayload, sizeof(respawnPayload));
}
