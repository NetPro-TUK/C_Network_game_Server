#include <stdio.h>
#include <string.h>
// 공용 헤더 파일
#include "protocol.h"   
#include "net_utils.h"
#include "log.h"
#include "entity.h"
// 서버 헤더 파일
#include "net_server.h"
#include "game_logic.h"


bool defender_ready = false;
bool attacker_ready = false;
bool game_started = false;
bool server_game_over = false;

// 클라이언트 소켓과 이벤트 핸들 배열
SOCKET sockArr[MAX_CLIENT];     // 클라이언트 소켓 배열
WSAEVENT eventArr[MAX_CLIENT];  // 각 소켓에 대한 이벤트 핸들
bool    client_ready[MAX_CLIENT];  
int numOfClnt = 0;              // 현재 접속한 클라이언트 수
static uint32_t client_id = 1;

// game_logic.c 또는 net_server.c
uint32_t attacker_death_time_ms = 0;
bool attacker_pending_respawn = false;


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

    // 준비 상태 초기화
    client_ready[numOfClnt] = false;

    printf("Server> client connected: %s:%d\n", inet_ntoa(clntAdr.sin_addr), ntohs(clntAdr.sin_port));
    numOfClnt++;
}

// 클라이언트 메세지 수신 및 타입 별 처리
int recv_and_dispatch(int i) {
    MsgHeader header;
    int ret = recv_full(sockArr[i], &header, sizeof(header));
    if (ret == 0) {
        printf("[DEBUG] recv_full(): 클라이언트가 연결을 종료했습니다 (recv=0)\n");
        return -1;
    }
    else if (ret < 0) {
        printf("[DEBUG] recv_full(): 헤더 수신 오류 (ret=%d), WSAError=%d\n",
            ret, WSAGetLastError());
        return -1;
    }

    MsgType type = header.type;              // protocol.h에서 type이 1바이트(enum)이면 ntohl 불필요
    uint32_t payload_len = ntohl(header.length); // length 는 4바이트 네트워크 순서

    //printf("Server> [RECV] Header: type=%d, length=%u\n", type, payload_len);

    // --- 메시지 종류 분기 처리 ---
    if (type == MSG_JOIN && payload_len == sizeof(PayloadJoin)) {
        PayloadJoin joinPayload;
        ret = recv_full(sockArr[i], &joinPayload, sizeof(joinPayload));
        if (ret <= 0) {
            printf("[DEBUG] PayloadJoin 수신 실패 (ret=%d, err=%d)\n",
                ret, WSAGetLastError());
            return -1;
        }
        handle_join(sockArr[i], &joinPayload);
    }
    else if (type == MSG_STATE_UPDATE && payload_len == sizeof(PayloadStateUpdate)) {
        PayloadStateUpdate payload;
        ret = recv_full(sockArr[i], &payload, sizeof(payload));
        if (ret <= 0) {
            printf("[DEBUG] PayloadStateUpdate 수신 실패 (ret=%d, err=%d)\n",
                ret, WSAGetLastError());
            return 0;
        }
        uint32_t id = ntohl(payload.entityId);
        printf("Server> [STATE_UPDATE] entityId=%u, x=%d, y=%d, role=%d\n", id, payload.x, payload.y, payload.role);
        Entity* ent = get_entity_by_id(id);
        if (ent) {
            update_entity_state(ent->entity_id,
                payload.x, payload.y, 0, 0);
        }
        else {
            LOG_WARN("Unknown entity ID in MSG_STATE_UPDATE");
        }
    }
    // recv_and_dispatch 내부의 MSG_READY 처리부 수정
    else if (type == MSG_READY && payload_len == 0) {
        Entity* e = find_entity_by_sock(sockArr[i]);
        if (!e) {
            LOG_WARN("알 수 없는 소켓에서 MSG_READY 수신");
            return -1;
        }

        // 역할별로 준비 표시
        if (e->type == ENTITY_DEFENDER) {
            defender_ready = true;
            printf("Server> DEFENDER ready\n");
        }
        else if (e->type == ENTITY_ATTACKER) {
            attacker_ready = true;
            printf("Server> ATTACKER ready\n");
        }

        // 둘 다 준비됐을 때만 게임 시작
        if (defender_ready && attacker_ready) {
            // GAME_START 이벤트 브로드캐스트
            MsgHeader h = {
                .type = MSG_GAME_EVENT,
                .length = htonl(sizeof(PayloadGameEvent))
            };
            PayloadGameEvent ev = { .event_type = GAME_START };
            broadcast_all(&h, sizeof(h));
            broadcast_all(&ev, sizeof(ev));

            // 게임 시작 상태
            game_started = true;

            // 엔티티 위치 스냅샷 브로드캐스트
            for (int eidx = 0; eidx < entityCount; ++eidx) {
                Entity* ent = &entityArr[eidx];
                if (!ent->alive) continue;

                PayloadStateUpdate upd = {
                    .entityId = htonl(ent->entity_id),
                    .x = ent->x,
                    .y = ent->y,
                    .role = ent->type
                };
                MsgHeader uh = {
                    .type = MSG_STATE_UPDATE,
                    .length = htonl(sizeof(upd))
                };
                broadcast_all(&uh, sizeof(uh));
                broadcast_all(&upd, sizeof(upd));
            }
        }

        return 0;
    }
    else if (type == MSG_ACTION_EVENT && payload_len == sizeof(PayloadActionEvent)) {
        PayloadActionEvent actionPayload;
        ret = recv_full(sockArr[i], &actionPayload, sizeof(actionPayload));
        if (ret <= 0) {
            printf("[DEBUG] PayloadActionEvent 수신 실패 (ret=%d, err=%d)\n",
                ret, WSAGetLastError());
            return 0;
        }
        handle_action_event(sockArr[i], &actionPayload);
    }
    else if (type == MSG_GAME_EVENT && payload_len == sizeof(PayloadGameEvent)) {
        PayloadGameEvent ev;
        recv_full(sockArr[i], &ev, sizeof(ev));

        GameEventType type = ev.event_type;
        uint32_t entityId = ntohl(ev.entityId);

        if (type == RESPAWN_REQUEST) {
            LOG_INFO("리스폰 요청 수신: entityId = %u", entityId);

            // 1. 먼저 해당 ID가 이미 살아있는지 확인
            Entity* existing = get_entity_by_id(entityId);
            if (existing && existing->alive) {
                LOG_WARN("리스폰 요청 거부: entityId=%u 이미 활성 상태", entityId);
                return 0;
            }

            // 2. 이제 새로 생성 (중복 없음 보장됨)
            Entity* e = create_entity(ENTITY_ATTACKER, 1, rand() % SCREEN_HEIGHT);
            if (e) {
                e->entity_id = entityId;

                // 클라이언트에 상태 전송
                PayloadStateUpdate update = {
                    .entityId = htonl(entityId),
                    .x = e->x,
                    .y = e->y,
                    .role = ENTITY_ATTACKER
                };
                MsgHeader hdr = {
                    .type = MSG_STATE_UPDATE,
                    .length = htonl(sizeof(update))
                };
                broadcast_all(&hdr, sizeof(hdr));
                broadcast_all(&update, sizeof(update));
            }
        }
    }
    else {
        // 정의되지 않은 메시지 → 길이 확인해서 덤프
        if (payload_len > 512) {
            printf("[DEBUG] payload_len (%u) too large → 클라이언트 비정상 처리\n",
                payload_len);
            return -1;
        }
        char dump[512];
        ret = recv_full(sockArr[i], dump, payload_len);
        if (ret <= 0) {
            printf("[DEBUG] dump 처리 중 수신 실패 (ret=%d, err=%d)\n",
                ret, WSAGetLastError());
            return -1;
        }
        printf("[DEBUG] Unknown message type=%d, payload_len=%u. Dump 처리됨\n",
            type, payload_len);
    }

    return 0;
}

// 클라이언트 연결 종료 및 소켓 제거
void remove_client_at(int index) {
    if (index < 0 || index >= numOfClnt) return;

    closesocket(sockArr[index]);
    WSACloseEvent(eventArr[index]);

    // 배열 압축
    sockArr[index] = sockArr[numOfClnt - 1];
    eventArr[index] = eventArr[numOfClnt - 1];
    client_ready[index] = client_ready[numOfClnt - 1];

    numOfClnt--;
    index--;
}

// 클라이언트 종료 전: 방어자인지 확인하고 리셋
void reset_defender_if_match(SOCKET closingSock) {
    for (int i = 0; i < entityCount; ++i) {
        if (!entityArr[i].alive) continue;

        if (entityArr[i].sock == closingSock) {
            if (entityArr[i].owner_client_id == defender_owner_id) {
                defender_owner_id = 0;
                printf("Server> 방어자 종료 → defender_owner_id 초기화됨\n");
            }
            entityArr[i].alive = 0;
            break;
        }
    }
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

uint32_t generate_client_id(void) {
    return client_id++;
}