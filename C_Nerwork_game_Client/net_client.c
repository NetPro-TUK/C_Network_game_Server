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
    PayloadJoin joinPayload = { .role = role };
    MsgHeader joinHeader = {
        .type = MSG_JOIN,
        .length = htonl(sizeof(joinPayload))
    };
    send_full(sock, &joinHeader, sizeof(joinHeader));
    send_full(sock, &joinPayload, sizeof(joinPayload));
}

// ���� �غ�
void send_ready(SOCKET sock, uint32_t my_entity_id) {
    MsgHeader h = {
        .type = MSG_READY,
        .length = htonl(0)
    };
    send_full(sock, &h, sizeof(h));
}

void send_state_update(SOCKET sock, uint32_t id, int x, int y) {
    // ���� ���� ���̷ε� ����
    PayloadStateUpdate statePayload = {
        .entityId = htonl(id),  // ������ ��� �� ID�� ��Ʈ��ũ ����Ʈ ������
        .x = x,
        .y = y
    };
    // ��� ����
    MsgHeader stateHeader = {
        .type = MSG_STATE_UPDATE,
        .length = htonl(sizeof(statePayload))
    };
    // ���� (��� �� ���̷ε� ����)
    send_full(sock, &stateHeader, sizeof(stateHeader));
    send_full(sock, &statePayload, sizeof(statePayload));
}

void send_shooting_event(SOCKET sock, uint32_t shooter_id, uint32_t bullet_id, int dirX, int dirY) {
    MsgHeader header;
    PayloadShootingEvent payload;

    // ��� ����
    header.length = htonl(sizeof(PayloadShootingEvent));  // ��Ʈ��ũ ����Ʈ ������ ��ȯ
    header.type = MSG_SHOOTING_EVENT;                   // enum �� �״�� ����

    // ���̷ε� ����
    payload.shooterId = htonl(shooter_id);
    payload.bulletId = htonl(bullet_id);
    payload.dirX = dirX;
    payload.dirY = dirY;

    // ����
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