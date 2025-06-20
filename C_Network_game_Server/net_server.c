#include "net_server.h"
#include "protocol.h"
#include "net_utils.h"
#include "log.h"
#include "entity.h"
#include "game_logic.h"
#include <stdio.h>
#include <string.h>

// 클라이언트 소켓과 이벤트 핸들 배열
SOCKET sockArr[MAX_CLIENT];     // 클라이언트 소켓 배열
WSAEVENT eventArr[MAX_CLIENT];  // 각 소켓에 대한 이벤트 핸들
int numOfClnt = 0;              // 현재 접속한 클라이언트 수

// 서버 소켓 초기화 및 리슨
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

    // 첫 번째 이벤트 등록 (FD_ACCEPT 감지)
    eventArr[0] = WSACreateEvent();
    sockArr[0] = hServSock;
    WSAEventSelect(hServSock, eventArr[0], FD_ACCEPT);
    numOfClnt = 1; // 서버 소켓 포함

    return hServSock;
}

// ------------------------------
// 클라이언트 접속 수락 및 이벤트 등록
// ------------------------------
// 클라이언트 송수신 소켓 생성
void accept_new_client(SOCKET serverSock) {
    SOCKET hClntSock;
    SOCKADDR_IN clntAdr;
    int clntAdrSize = sizeof(clntAdr);

    // 새 클라이언트 수락
    hClntSock = accept(serverSock, (SOCKADDR*)&clntAdr, &clntAdrSize);
    if (hClntSock == INVALID_SOCKET) return;

    // 소켓 및 이벤트 등록
    sockArr[numOfClnt] = hClntSock;
    eventArr[numOfClnt] = WSACreateEvent();
    WSAEventSelect(hClntSock, eventArr[numOfClnt], FD_READ | FD_CLOSE);      // 읽기/종료 감지

    printf("Server> client connected: %s:%d\n", inet_ntoa(clntAdr.sin_addr), ntohs(clntAdr.sin_port));
    numOfClnt++;
}

// ------------------------------
// 클라이언트로부터 메시지 수신 및 처리 분기
// ------------------------------
// 클라이언트 메세지 수신 및 타입 별 처리
int recv_and_dispatch(int i) {
    MsgHeader header;
    int ret = recv_full(sockArr[i], &header, sizeof(header));
    if (ret <= 0) return -1;

    // 헤더 수신 (메시지 타입 및 길이 확인)
    MsgType type = header.type;
    uint32_t payload_len = ntohl(header.length);
    printf("Server> [RECV] Header: type=%d, length=%d\n", type, payload_len);

    // 메시지 종류에 따라 분기 처리
    if (type == MSG_JOIN && payload_len == sizeof(PayloadJoin)) {
        // 클라이언트 접속 요청 처리
        PayloadJoin joinPayload;
        ret = recv_full(sockArr[i], &joinPayload, sizeof(joinPayload));
        if (ret > 0) {
            handle_join(sockArr[i], &joinPayload);       // 역할 지정 및 Entity 생성
        }
    }
    else if (type == MSG_STATE_UPDATE && payload_len == sizeof(PayloadStateUpdate)) {
        // 위치 상태 업데이트 처리
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
        // 발사 이벤트 처리
		PayloadActionEvent actionPayload;
		ret = recv_full(sockArr[i], &actionPayload, sizeof(actionPayload));
		if (ret > 0) {
			handle_action_event(sockArr[i], &actionPayload);        // 총알 생성 등
		}
	}
    else {
        // 정의되지 않은 메시지 또는 덤프 처리
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
     for (int i = 1; i < numOfClnt; ++i) {      // 0번은 서버 소켓이므로 제외
         send_to_client(sockArr[i], buf, len);
     }
 }