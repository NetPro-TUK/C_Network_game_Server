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
uint32_t send_join_and_get_id(SOCKET sock, int role) {
    PayloadJoin joinPayload = { .role = role };
    MsgHeader joinHeader = {
        .type = MSG_JOIN,
        .length = htonl(sizeof(joinPayload))    // ���̷ε� ũ�� ����
    };

    // �޽��� ���� (��� �� ���̷ε�)
    send_full(sock, &joinHeader, sizeof(joinHeader));
    send_full(sock, &joinPayload, sizeof(joinPayload));

    // �����κ��� ���� �޽��� ����
    MsgHeader ackHeader;
    recv_full(sock, &ackHeader, sizeof(ackHeader));
    if (ackHeader.type != MSG_JOIN_ACK || ntohl(ackHeader.length) != sizeof(PayloadJoinAck)) {
        printf("Client> �߸��� MSG_JOIN_ACK ����\n");
        closesocket(sock);
        WSACleanup();
        exit(1);
    }

    // ���� PayloadJoinAck ����
    PayloadJoinAck ackPayload;
    recv_full(sock, &ackPayload, sizeof(ackPayload));

    // ��Ʈ��ũ ����Ʈ ���� �� ȣ��Ʈ ����Ʈ ���� ��ȯ
    return ntohl(ackPayload.entityId);
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

// ���� �̺�Ʈ �޼���
void handle_server_message(MsgHeader* header, void* payload) {
    if (header->type == MSG_GAME_EVENT) {
        PayloadGameEvent* gamePayload = (PayloadGameEvent*)payload;

        switch (gamePayload->event_type) {
        case GAME_OVER:
            printf("���� ����!\n");
            break;
        case GAME_WIN:
            printf("���� �¸�!\n");
            break;
        case PLAYER_REJECTED:
            printf("[�˸�] ����ڴ� �̹� �����մϴ�. ���ӿ� ������ �� �����ϴ�.\n");
            exit(0);  // �Ǵ� return; �� ������ ��ġ
        default:
            printf("�� �� ���� ���� �̺�Ʈ ����: %d\n", gamePayload->event_type);
            break;
        }
    }
}