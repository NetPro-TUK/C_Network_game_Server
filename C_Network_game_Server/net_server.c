#include <stdio.h>
#include <string.h>
// ���� ��� ����
#include "protocol.h"   
#include "net_utils.h"
#include "log.h"
#include "entity.h"
// ���� ��� ����
#include "net_server.h"
#include "game_logic.h"


bool defender_ready = false;
bool attacker_ready = false;
bool game_started = false;
bool server_game_over = false;

// Ŭ���̾�Ʈ ���ϰ� �̺�Ʈ �ڵ� �迭
SOCKET sockArr[MAX_CLIENT];     // Ŭ���̾�Ʈ ���� �迭
WSAEVENT eventArr[MAX_CLIENT];  // �� ���Ͽ� ���� �̺�Ʈ �ڵ�
bool    client_ready[MAX_CLIENT];  
int numOfClnt = 0;              // ���� ������ Ŭ���̾�Ʈ ��
static uint32_t client_id = 1;

// game_logic.c �Ǵ� net_server.c
uint32_t attacker_death_time_ms = 0;
bool attacker_pending_respawn = false;


// ���� ���� �ʱ�ȭ �� ����
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

    // ù ��° �̺�Ʈ ��� (FD_ACCEPT ����)
    eventArr[0] = WSACreateEvent();
    sockArr[0] = hServSock;
    WSAEventSelect(hServSock, eventArr[0], FD_ACCEPT);
    numOfClnt = 1; // ���� ���� ����

    return hServSock;
}

// Ŭ���̾�Ʈ �ۼ��� ���� ����
void accept_new_client(SOCKET serverSock) {
    SOCKET hClntSock;
    SOCKADDR_IN clntAdr;
    int clntAdrSize = sizeof(clntAdr);

    // �� Ŭ���̾�Ʈ ����
    hClntSock = accept(serverSock, (SOCKADDR*)&clntAdr, &clntAdrSize);
    if (hClntSock == INVALID_SOCKET) return;

    // ���� �� �̺�Ʈ ���
    sockArr[numOfClnt] = hClntSock;
    eventArr[numOfClnt] = WSACreateEvent();
    WSAEventSelect(hClntSock, eventArr[numOfClnt], FD_READ | FD_CLOSE);      // �б�/���� ����

    // �غ� ���� �ʱ�ȭ
    client_ready[numOfClnt] = false;

    printf("Server> client connected: %s:%d\n", inet_ntoa(clntAdr.sin_addr), ntohs(clntAdr.sin_port));
    numOfClnt++;
}

// Ŭ���̾�Ʈ �޼��� ���� �� Ÿ�� �� ó��
int recv_and_dispatch(int i) {
    MsgHeader header;
    int ret = recv_full(sockArr[i], &header, sizeof(header));
    if (ret == 0) {
        printf("[DEBUG] recv_full(): Ŭ���̾�Ʈ�� ������ �����߽��ϴ� (recv=0)\n");
        return -1;
    }
    else if (ret < 0) {
        printf("[DEBUG] recv_full(): ��� ���� ���� (ret=%d), WSAError=%d\n",
            ret, WSAGetLastError());
        return -1;
    }

    MsgType type = header.type;              // protocol.h���� type�� 1����Ʈ(enum)�̸� ntohl ���ʿ�
    uint32_t payload_len = ntohl(header.length); // length �� 4����Ʈ ��Ʈ��ũ ����

    //printf("Server> [RECV] Header: type=%d, length=%u\n", type, payload_len);

    // --- �޽��� ���� �б� ó�� ---
    if (type == MSG_JOIN && payload_len == sizeof(PayloadJoin)) {
        PayloadJoin joinPayload;
        ret = recv_full(sockArr[i], &joinPayload, sizeof(joinPayload));
        if (ret <= 0) {
            printf("[DEBUG] PayloadJoin ���� ���� (ret=%d, err=%d)\n",
                ret, WSAGetLastError());
            return -1;
        }
        handle_join(sockArr[i], &joinPayload);
    }
    else if (type == MSG_STATE_UPDATE && payload_len == sizeof(PayloadStateUpdate)) {
        PayloadStateUpdate payload;
        ret = recv_full(sockArr[i], &payload, sizeof(payload));
        if (ret <= 0) {
            printf("[DEBUG] PayloadStateUpdate ���� ���� (ret=%d, err=%d)\n",
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
    // recv_and_dispatch ������ MSG_READY ó���� ����
    else if (type == MSG_READY && payload_len == 0) {
        Entity* e = find_entity_by_sock(sockArr[i]);
        if (!e) {
            LOG_WARN("�� �� ���� ���Ͽ��� MSG_READY ����");
            return -1;
        }

        // ���Һ��� �غ� ǥ��
        if (e->type == ENTITY_DEFENDER) {
            defender_ready = true;
            printf("Server> DEFENDER ready\n");
        }
        else if (e->type == ENTITY_ATTACKER) {
            attacker_ready = true;
            printf("Server> ATTACKER ready\n");
        }

        // �� �� �غ���� ���� ���� ����
        if (defender_ready && attacker_ready) {
            // GAME_START �̺�Ʈ ��ε�ĳ��Ʈ
            MsgHeader h = {
                .type = MSG_GAME_EVENT,
                .length = htonl(sizeof(PayloadGameEvent))
            };
            PayloadGameEvent ev = { .event_type = GAME_START };
            broadcast_all(&h, sizeof(h));
            broadcast_all(&ev, sizeof(ev));

            // ���� ���� ����
            game_started = true;

            // ��ƼƼ ��ġ ������ ��ε�ĳ��Ʈ
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
            printf("[DEBUG] PayloadActionEvent ���� ���� (ret=%d, err=%d)\n",
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
            LOG_INFO("������ ��û ����: entityId = %u", entityId);

            // 1. ���� �ش� ID�� �̹� ����ִ��� Ȯ��
            Entity* existing = get_entity_by_id(entityId);
            if (existing && existing->alive) {
                LOG_WARN("������ ��û �ź�: entityId=%u �̹� Ȱ�� ����", entityId);
                return 0;
            }

            // 2. ���� ���� ���� (�ߺ� ���� �����)
            Entity* e = create_entity(ENTITY_ATTACKER, 1, rand() % SCREEN_HEIGHT);
            if (e) {
                e->entity_id = entityId;

                // Ŭ���̾�Ʈ�� ���� ����
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
        // ���ǵ��� ���� �޽��� �� ���� Ȯ���ؼ� ����
        if (payload_len > 512) {
            printf("[DEBUG] payload_len (%u) too large �� Ŭ���̾�Ʈ ������ ó��\n",
                payload_len);
            return -1;
        }
        char dump[512];
        ret = recv_full(sockArr[i], dump, payload_len);
        if (ret <= 0) {
            printf("[DEBUG] dump ó�� �� ���� ���� (ret=%d, err=%d)\n",
                ret, WSAGetLastError());
            return -1;
        }
        printf("[DEBUG] Unknown message type=%d, payload_len=%u. Dump ó����\n",
            type, payload_len);
    }

    return 0;
}

// Ŭ���̾�Ʈ ���� ���� �� ���� ����
void remove_client_at(int index) {
    if (index < 0 || index >= numOfClnt) return;

    closesocket(sockArr[index]);
    WSACloseEvent(eventArr[index]);

    // �迭 ����
    sockArr[index] = sockArr[numOfClnt - 1];
    eventArr[index] = eventArr[numOfClnt - 1];
    client_ready[index] = client_ready[numOfClnt - 1];

    numOfClnt--;
    index--;
}

// Ŭ���̾�Ʈ ���� ��: ��������� Ȯ���ϰ� ����
void reset_defender_if_match(SOCKET closingSock) {
    for (int i = 0; i < entityCount; ++i) {
        if (!entityArr[i].alive) continue;

        if (entityArr[i].sock == closingSock) {
            if (entityArr[i].owner_client_id == defender_owner_id) {
                defender_owner_id = 0;
                printf("Server> ����� ���� �� defender_owner_id �ʱ�ȭ��\n");
            }
            entityArr[i].alive = 0;
            break;
        }
    }
}

// Ư�� Ŭ���̾�Ʈ���� �޼��� ����
void send_to_client(SOCKET sock, const void* buf, int len) {
        send(sock, buf, len, 0);
}

// ��� Ŭ���̾�Ʈ���� �޼��� ��ε�ĳ��Ʈ
void broadcast_all(const void* buf, int len) {
     for (int i = 1; i < numOfClnt; ++i) {      // 0���� ���� �����̹Ƿ� ����
         send_to_client(sockArr[i], buf, len);
     }
 }

uint32_t generate_client_id(void) {
    return client_id++;
}