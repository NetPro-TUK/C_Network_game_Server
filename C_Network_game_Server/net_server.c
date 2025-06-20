#include "net_server.h"
#include "protocol.h"
#include "net_utils.h"
#include "log.h"
#include "entity.h"
#include "game_logic.h"
#include <stdio.h>
#include <string.h>

SOCKET sockArr[MAX_CLIENT];
WSAEVENT eventArr[MAX_CLIENT];
int numOfClnt = 0;

// 서버 소켓 초기화 및 바인딩
int init_server_socket(int port) {
    WSADATA wsaData;
    SOCKET hServSock;
    SOCKADDR_IN servAdr;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fputs("WSAStartup() error!\n", stderr);
        return -1;
    }

    hServSock = socket(PF_INET, SOCK_STREAM, 0);
    if (hServSock == INVALID_SOCKET) {
        fputs("socket() error\n", stderr);
        return -1;
    }

    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servAdr.sin_port = htons(port);

    if (bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR) {
        fputs("bind() error\n", stderr);
        return -1;
    }

    listen(hServSock, MAX_CLIENT);

    eventArr[0] = WSACreateEvent();
    sockArr[0] = hServSock;
    WSAEventSelect(hServSock, eventArr[0], FD_ACCEPT);
    numOfClnt = 1;

    return hServSock;
}

// 클라이언트 송수신 소켓 생성
void accept_new_client(SOCKET serverSock) {
    SOCKET hClntSock;
    SOCKADDR_IN clntAdr;
    int clntAdrSize = sizeof(clntAdr);

    hClntSock = accept(serverSock, (SOCKADDR*)&clntAdr, &clntAdrSize);
    if (hClntSock == INVALID_SOCKET) return;

    sockArr[numOfClnt] = hClntSock;
    eventArr[numOfClnt] = WSACreateEvent();
    WSAEventSelect(hClntSock, eventArr[numOfClnt], FD_READ | FD_CLOSE);

    printf("Server> client connected: %s:%d\n", inet_ntoa(clntAdr.sin_addr), ntohs(clntAdr.sin_port));
    numOfClnt++;
}

// 클라이언트 메세지 수신 및 타입 별 처리
int recv_and_dispatch(int i) {
    MsgHeader header;
    int ret = recv_full(sockArr[i], &header, sizeof(header));
    if (ret <= 0) return -1;

    MsgType type = header.type;
    uint32_t payload_len = ntohl(header.length);
    printf("[RECV] Header: type=%d, length=%d\n", type, payload_len);

    if (type == MSG_JOIN && payload_len == sizeof(PayloadJoin)) {
        PayloadJoin joinPayload;
        ret = recv_full(sockArr[i], &joinPayload, sizeof(joinPayload));
        if (ret > 0) {
            handle_join(sockArr[i], &joinPayload);
        }
    }
    else if (type == MSG_STATE_UPDATE && payload_len == sizeof(PayloadStateUpdate)) {
        PayloadStateUpdate payload;
        ret = recv_full(sockArr[i], &payload, sizeof(payload));
        if (ret > 0) {
            uint32_t id = ntohl(payload.entityId);

            Entity* ent = get_entity_by_id(id);
            if (ent) {
                update_entity_state(ent->entity_id, payload.x, payload.y, 0, 0);
            }
            else {
                LOG_WARN("Unknown entity ID in MSG_STATE_UPDATE");
            }
        }
    }
	else if (type == MSG_ACTION_EVENT && payload_len == sizeof(PayloadActionEvent)) {
		PayloadActionEvent actionPayload;
		ret = recv_full(sockArr[i], &actionPayload, sizeof(actionPayload));
		if (ret > 0) {
			handle_action_event(sockArr[i], &actionPayload);
		}
	}
    else {
        char dump[512];
        if (payload_len < sizeof(dump)) recv_full(sockArr[i], dump, payload_len);
        else return -1;
    }

    return 0;
}

// 특정 클라이언트에게 메세지 전송
void send_to_client(SOCKET sock, const void* buf, int len) {
        send(sock, buf, len, 0);
}

// 모든 클라이언트에게 메세지 브로드캐스트
void broadcast_all(const void* buf, int len) {
     for (int i = 1; i < numOfClnt; ++i) {
         send_to_client(sockArr[i], buf, len);
     }
 }