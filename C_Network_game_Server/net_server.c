
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

SOCKET sockArr[MAX_CLIENT];         
WSAEVENT eventArr[MAX_CLIENT];     
int numOfClnt = 0;                 
static uint32_t client_id = 1;
uint32_t current_score = 0;

static int	attacker_ready_count = 0; // 공격자 최소 3명 이상 확인


// 서버 소켓 초기화 및 리슨 설정
int init_server_socket(int port) {
    WSADATA wsaData;
    SOCKET hServSock;
    SOCKADDR_IN servAdr;

    // Winsock 초기화
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fputs("WSAStartup() error!\n", stderr);
        return -1;
    }
    // TCP 소켓 생성
    hServSock = socket(PF_INET, SOCK_STREAM, 0);
    if (hServSock == INVALID_SOCKET) {
        fputs("socket() error\n", stderr);
        return -1;
    }
    // 주소 설정: localhost:port
    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servAdr.sin_port = htons(port);
    // 소켓 바인드
    if (bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR) {
        fputs("bind() error\n", stderr);
        return -1;
    }
    // 수신 대기
    listen(hServSock, MAX_CLIENT);

    // 서버 소켓 이벤트 등록: FD_ACCEPT
    eventArr[0] = WSACreateEvent();
    sockArr[0] = hServSock;
    WSAEventSelect(hServSock, eventArr[0], FD_ACCEPT);
    numOfClnt = 1; // 서버 소켓 포함

    return hServSock;
}

// 새로운 클라이언트 연결 수락 및 등록
void accept_new_client(SOCKET serverSock) {
    SOCKET hClntSock;
    SOCKADDR_IN clntAdr;
    int clntAdrSize = sizeof(clntAdr);

    // accept 호출 (blocking 아님)
    hClntSock = accept(serverSock, (SOCKADDR*)&clntAdr, &clntAdrSize);
    if (hClntSock == INVALID_SOCKET) return;

    // 배열에 소켓과 이벤트 추가
    sockArr[numOfClnt] = hClntSock;
    eventArr[numOfClnt] = WSACreateEvent();
    WSAEventSelect(hClntSock, eventArr[numOfClnt], FD_READ | FD_CLOSE);

    printf("Server> client connected: %s:%d\n", inet_ntoa(clntAdr.sin_addr), ntohs(clntAdr.sin_port));
    numOfClnt++;
}

// 인덱스 i의 클라이언트로부터 메시지 수신 및 처리
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

    // 메시지 타입 및 길이 추출
    MsgType type = header.type;
    uint32_t payload_len = ntohl(header.length);

    // 타입별 분기 처리
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
    // 2. 상태 업데이트 메시지 처리 
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
    // 3. 준비 완료 메시지 처리
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
            attacker_ready_count++;
            printf("Server> ATTACKER ready\n");
        }

        // 둘 다 준비됐을 때만 게임 시작 (공격자 3명 이상, 방어자 하나)
        if (defender_ready && attacker_ready_count >= 3) {
            // GAME_START 이벤트 브로드캐스트
            MsgHeader h = { .type = MSG_GAME_EVENT, .length = htonl(sizeof(PayloadGameEvent)) };
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
        if (defender_ready && attacker_ready_count >= 3) {
        }
        return 0;
    }
    // 4. 슈팅 이벤트 메시지 처리
    else if (type == MSG_SHOOTING_EVENT && payload_len == sizeof(PayloadShootingEvent)) {
        PayloadShootingEvent actionPayload;
        ret = recv_full(sockArr[i], &actionPayload, sizeof(actionPayload));
        if (ret <= 0) {
            printf("[DEBUG] PayloadActionEvent 수신 실패 (ret=%d, err=%d)\n",
                ret, WSAGetLastError());
            return 0;
        }
        handle_shooting_event(sockArr[i], &actionPayload);
    }
    // 5. 게임 이벤트 메시지 처리
    else if (type == MSG_GAME_EVENT && payload_len == sizeof(PayloadGameEvent)) {
        PayloadGameEvent ev;
        recv_full(sockArr[i], &ev, sizeof(ev));

        GameEventType type = ev.event_type;
        uint32_t entityId = ntohl(ev.entityId);

        // 1) 리스폰 요청 이벤트 처리
        if (type == RESPAWN_REQUEST) {
            LOG_INFO("리스폰 요청 수신: entityId = %u", entityId);

            // - 1 먼저 해당 ID가 이미 살아있는지 확인
            Entity* existing = get_entity_by_id(entityId);
            if (existing && existing->alive) {
                LOG_WARN("리스폰 요청 거부: entityId=%u 이미 활성 상태", entityId);
                return 0;
            }

            // - 2 새로운 엔티티 생성
            Entity* e = create_entity(ENTITY_ATTACKER, entityId, sockArr[i]);
            if (e) {
                e->entity_id = entityId;
                e->x = 0;
                e->y = rand() % SCREEN_HEIGHT; // 랜덤 Y 위치

                // 클라이언트에 상태 전송
                PayloadStateUpdate update = { .entityId = htonl(entityId), .x = e->x,  .y = e->y,  .role = ENTITY_ATTACKER };
                MsgHeader hdr = { .type = MSG_STATE_UPDATE, .length = htonl(sizeof(update)) };
                broadcast_all(&hdr, sizeof(hdr));
                broadcast_all(&update, sizeof(update));
            }
        }
        // 2) 재장전 요청 이벤트 처리
        else if (type == RELOAD_REQUEST) {
            LOG_INFO("재장전 요청 수신: entityId = %u", entityId);
            handle_reload_request();
        }
        else {
            printf("[DEBUG] 알 수 없는 게임 이벤트 타입: %d\n", type);
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

// 클라이언트 연결 끊김 시 정리 및 배열 압축
void remove_client_at(int index) {
    if (index < 0 || index >= numOfClnt) return;

    closesocket(sockArr[index]);
    WSACloseEvent(eventArr[index]);

    // 마지막 요소로 덮어쓰기
    sockArr[index] = sockArr[numOfClnt - 1];
    eventArr[index] = eventArr[numOfClnt - 1];
    numOfClnt--;
}

// 방어자 소켓 종료 시 ID 초기화 및 엔티티 비활성화
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

// 모든 클라이언트에 메시지 브로드캐스트
void broadcast_all(const void* buf, int len) {
     for (int i = 1; i < numOfClnt; ++i) {      // 0번은 서버 소켓이므로 제외
         send_full(sockArr[i], buf, len);
     }
 }

// 클라이언트 ID 생성 함수: 내부 카운터 사용
uint32_t generate_client_id(void) {
    return client_id++;
}