#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <conio.h>
#include <stdio.h>
#include <windows.h>
#include <stdbool.h>

#include "protocol.h"
#include "net_client.h"
#include "net_utils.h"
#include "render.h"
#include "entity.h"

#define MAX_ENTITIES 256

typedef struct {
    uint32_t entity_id;
    int x, y;
    int active;
    EntityType type;
} EntityView;

EntityView view_entities[MAX_ENTITIES];

// 상태 변수
volatile RoleStatus role_status = ROLE_STATUS_PENDING; // 역할 상태
volatile uint32_t my_entity_id = 0;    // 내 엔티티 ID
volatile int socket_disconnected = 0;  // 소켓 끊김 여부
int role = 0;                          // 내 역할 (0: 공격자, 1: 방어자)

// 유효 좌표 여부 확인
bool is_valid_position(int x, int y) {
    return x > 0 && x < FIELD_WIDTH - 1 && y > 0 && y < FIELD_HEIGHT - 1;
}

void draw_entity(EntityView* ent) {
    if (!is_valid_position(ent->x, ent->y)) return;
    if (ent->type == ENTITY_DEFENDER) draw_defender(ent->x, ent->y);
    else if (ent->type == ENTITY_ATTACKER) draw_attacker(ent->x, ent->y);
}

void erase_entity(EntityView* ent) {
    if (!is_valid_position(ent->x, ent->y)) return;
    if (ent->type == ENTITY_DEFENDER) erase_defender(ent->x, ent->y);
    else if (ent->type == ENTITY_ATTACKER) erase_attacker(ent->x, ent->y);
}

// 수신 스레드
DWORD WINAPI recv_server_thread(LPVOID arg) {
    SOCKET sock = *(SOCKET*)arg;

    while (1) {
        MsgHeader header;
        int ret = recv_full(sock, &header, sizeof(header));
        if (ret <= 0) {
            socket_disconnected = 1;
            break;
        }

        uint32_t len = ntohl(header.length);

        if (header.type == MSG_JOIN_ACK && len == sizeof(PayloadJoinAck)) {
            PayloadJoinAck payload;
            recv_full(sock, &payload, sizeof(payload));
            my_entity_id = ntohl(payload.entityId);
            role_status = ROLE_STATUS_APPROVED;
        }
        else if (header.type == MSG_GAME_EVENT && len == sizeof(PayloadGameEvent)) {
            PayloadGameEvent payload;
            recv_full(sock, &payload, sizeof(payload));
            if (payload.event_type == PLAYER_REJECTED) {
                role_status = ROLE_STATUS_REJECTED;
            }
        }
        else if (header.type == MSG_STATE_UPDATE && len == sizeof(PayloadStateUpdate)) {
            PayloadStateUpdate payload;
            recv_full(sock, &payload, sizeof(payload));
            uint32_t id = ntohl(payload.entityId);
            int x = payload.x, y = payload.y, type = payload.role;

            if (!is_valid_position(x, y)) continue;

            for (int i = 0; i < MAX_ENTITIES; ++i) {
                if (view_entities[i].active && view_entities[i].entity_id == id) {
                    erase_entity(&view_entities[i]);
                    view_entities[i].x = x;
                    view_entities[i].y = y;
                    view_entities[i].type = type;
                    draw_entity(&view_entities[i]);
                    goto CONTINUE;
                }
            }

            for (int i = 0; i < MAX_ENTITIES; ++i) {
                if (!view_entities[i].active) {
                    view_entities[i].entity_id = id;
                    view_entities[i].x = x;
                    view_entities[i].y = y;
                    view_entities[i].active = 1;
                    view_entities[i].type =
                        (id == my_entity_id)
                        ? (role == 1 ? ENTITY_DEFENDER : ENTITY_ATTACKER)
                        : ENTITY_ATTACKER;
                    draw_entity(&view_entities[i]);
                    break;
                }
            }
        CONTINUE:
            continue;
        }
        else {
            char dummy[512];
            if (len <= sizeof(dummy)) recv_full(sock, dummy, len);
        }
    }

    return 0;
}

// 공격자 자동 이동
void auto_move_attacker(SOCKET sock, uint32_t id, int* x, int* y) {
    (*x)++;
    if (*x >= FIELD_WIDTH - 1) *x = 1;

    int dir = (rand() % 3) - 1;  // -1, 0, 1 중 하나
    *y += dir;
    if (*y < 1) *y = 1;
    if (*y >= FIELD_HEIGHT - 1) *y = FIELD_HEIGHT - 2;

    send_state_update(sock, id, *x, *y);
}

int main(void) {
    srand((unsigned int)time(NULL));
    SOCKET hSocket;

    while (1) {
        printf("역할을 선택하세요: [1] 방어자 (DEFENDER), [2] 공격자 (ATTACKER): ");
        scanf("%d", &role);
        if (role != 1 && role != 2) continue;

        hSocket = connect_to_server("127.0.0.1", 9000);
        if (hSocket == INVALID_SOCKET) {
            printf("서버 연결 실패.\n");
            return 1;
        }

        role_status = ROLE_STATUS_PENDING;
        socket_disconnected = 0;
        CreateThread(NULL, 0, recv_server_thread, &hSocket, 0, NULL);

        send_join_and_get_id(hSocket, role);

        while (role_status == ROLE_STATUS_PENDING && !socket_disconnected) {
            Sleep(10);
        }

        if (role_status == ROLE_STATUS_REJECTED) {
            printf("[알림] 방어자가 이미 존재합니다.\n");
            closesocket(hSocket);
            WSACleanup();
            continue;
        }

        if (socket_disconnected) {
            printf("서버와 연결이 끊겼습니다.\n");
            closesocket(hSocket);
            WSACleanup();
            continue;
        }

        break;
    }

    hide_cursor();
    system("cls");
    draw_border();

    int x = (role == 1) ? 70 : 1;
    int y = FIELD_HEIGHT / 2;

    send_state_update(hSocket, my_entity_id, x, y);

    gotoxy(0, FIELD_HEIGHT);
    printf("%s 조작 중. ESC 키로 종료합니다.\n", role == 1 ? "방어자" : "공격자");

    if (role == 1) {
        while (1) {
            if (_kbhit()) {
                int key = _getch();
                if (key == 27) break;
                if (key == 224) {
                    key = _getch();
                    if (key == 72 && y > 1) y--;
                    else if (key == 80 && y < FIELD_HEIGHT - 2) y++;
                    send_state_update(hSocket, my_entity_id, x, y);
                }
            }
            Sleep(50);
        }
    }
    else {
        while (1) {
            if (_kbhit()) {
                int key = _getch();
                if (key == 27) break;
            }
            auto_move_attacker(hSocket, my_entity_id, &x, &y);
            Sleep(180);
        }
    }

    show_cursor();
    closesocket(hSocket);
    WSACleanup();
    system("cls");
    printf("\nClient> 종료되었습니다.\n");
    return 0;
}