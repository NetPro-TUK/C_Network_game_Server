#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <conio.h>
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>

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
volatile RoleStatus role_status = ROLE_STATUS_PENDING;
volatile uint32_t my_entity_id = 0;
volatile int socket_disconnected = 0;
static bool game_started = false;
int role = 0;
volatile int wants_respawn = 0;

// 점수 측정을 위한 시작 시간
uint64_t client_game_start_time = 0;

bool is_valid_position(int x, int y) {
    return x > 0 && x < FIELD_WIDTH - 1 && y > 0 && y < FIELD_HEIGHT - 1;
}

void draw_entity(EntityView* ent) {
    if (!is_valid_position(ent->x, ent->y)) return;
    if (ent->type == ENTITY_DEFENDER) draw_defender(ent->x, ent->y);
    else if (ent->type == ENTITY_ATTACKER) draw_attacker(ent->x, ent->y);
    else if (ent->type == ENTITY_BULLET) draw_bullet(ent->x, ent->y);
}

void erase_entity(EntityView* ent) {
    if (!is_valid_position(ent->x, ent->y)) return;
    if (ent->type == ENTITY_DEFENDER) erase_defender(ent->x, ent->y);
    else if (ent->type == ENTITY_ATTACKER) erase_attacker(ent->x, ent->y);
    else if (ent->type == ENTITY_BULLET) erase_bullet(ent->x, ent->y);
}

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
            PayloadGameEvent p;
            recv_full(sock, &p, sizeof(p));
            if (p.event_type == GAME_START) {
                for (int i = 0; i < MAX_ENTITIES; ++i) {
                    view_entities[i].active = 0;
                }
                game_started = true;
                client_game_start_time = GetTickCount64();  // 점수용 타이머 시작
            }
            else if (p.event_type == PLAYER_REJECTED) {
                role_status = ROLE_STATUS_REJECTED;
            }
            else if (p.event_type == ENTITY_REMOVE) {
                uint32_t id = ntohl(p.entityId);
                for (int k = 0; k < MAX_ENTITIES; ++k) {
                    if (view_entities[k].active && view_entities[k].entity_id == id) {
                        view_entities[k].active = 0;
                        erase_entity(&view_entities[k]);
                        if (id == my_entity_id && view_entities[k].type == ENTITY_ATTACKER) {
                            printf("💀 공격자가 사망했습니다. 리스폰 하시겠습니까? (S 키)\n");
                            wants_respawn = 1;
                        }
                        break;
                    }
                }
            }
            else if (p.event_type == GAME_OVER) {
                system("cls");
                draw_border();
                gotoxy((FIELD_WIDTH - 24) / 2, FIELD_HEIGHT / 2);
                printf("===> GAME OVER <===\n");
                gotoxy((FIELD_WIDTH - 24) / 2, FIELD_HEIGHT / 2 + 2);
                printf("Press 'Q' to quit...\n");

                // 점수 출력
                uint64_t now = GetTickCount64();
                uint32_t score = (uint32_t)((now - client_game_start_time) / 1000);
                gotoxy((FIELD_WIDTH - 24) / 2, FIELD_HEIGHT / 2 + 4);
                printf("총 점수: %u 점\n", score);

                int ch;
                do {
                    ch = _getch();
                    ch = toupper(ch);
                } while (ch != 'Q');

                socket_disconnected = 1;
                break;
            }
        }
        else if (game_started && header.type == MSG_STATE_UPDATE && len == sizeof(PayloadStateUpdate)) {
            PayloadStateUpdate payload;
            recv_full(sock, &payload, sizeof(payload));

            uint32_t id = ntohl(payload.entityId);
            int x = payload.x;
            int y = payload.y;
            int entType = payload.role;

            if (!is_valid_position(x, y)) continue;

            for (int i = 0; i < MAX_ENTITIES; ++i) {
                if (view_entities[i].active && view_entities[i].entity_id == id) {
                    erase_entity(&view_entities[i]);
                    view_entities[i].x = x;
                    view_entities[i].y = y;
                    view_entities[i].type = entType;
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
                    view_entities[i].type = (EntityType)payload.role;
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

int main(void) {
    SOCKET hSocket;
    init_console_sync();

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

        while (role_status == ROLE_STATUS_PENDING && !socket_disconnected) Sleep(10);

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

    system("cls");
    printf("====================================\n");
    printf("    역할: %s 로 선택되었습니다.\n", role == 1 ? "방어자" : "공격자");
    printf("\n    게임을 시작하려면 ENTER 키를 누르세요.\n");
    printf("====================================\n");

    while (_getch() != '\r');
    while (_kbhit()) _getch();
    send_ready(hSocket, my_entity_id);
    printf("상대 플레이어 준비 중… 잠시만 기다려 주세요.\n");
    while (!game_started) {
        if (socket_disconnected) {
            puts("서버 연결이 끊어졌습니다.");
            return 1;
        }
        Sleep(50);
    }

    hide_cursor();
    system("cls");
    draw_border();
    draw_status(role == 1 ? "방어자" : "공격자");

    int x = (role == 1) ? 70 : 1;
    int y = FIELD_HEIGHT / 2;

    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD rec;
    DWORD cnt;

    if (role == 1) {
        while (!socket_disconnected) {
            if (PeekConsoleInput(hStdin, &rec, 1, &cnt) && cnt > 0) {
                ReadConsoleInput(hStdin, &rec, 1, &cnt);
                if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
                    WORD vk = rec.Event.KeyEvent.wVirtualKeyCode;
                    if (vk == VK_ESCAPE) break;
                    else if (vk == VK_UP && y > 1) {
                        erase_defender(x, y); y--; draw_defender(x, y);
                        send_state_update(hSocket, my_entity_id, x, y);
                    }
                    else if (vk == VK_DOWN && y < FIELD_HEIGHT - 2) {
                        erase_defender(x, y); y++; draw_defender(x, y);
                        send_state_update(hSocket, my_entity_id, x, y);
                    }
                    else if (vk == VK_SPACE) {
                        static uint32_t bullet_id_seq = 100000;
                        send_action_event(hSocket, my_entity_id, bullet_id_seq++, -1, 0);
                    }
                }
            }
            Sleep(50);
        }
    }
    else {
        while (!socket_disconnected) {
            if (PeekConsoleInput(hStdin, &rec, 1, &cnt) && cnt > 0) {
                ReadConsoleInput(hStdin, &rec, 1, &cnt);
                if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
                    WORD vk = rec.Event.KeyEvent.wVirtualKeyCode;
                    if (vk == VK_ESCAPE) break;
                    else if (wants_respawn && vk == 'S') {
                        PayloadGameEvent ev = { .event_type = RESPAWN_REQUEST, .entityId = htonl(my_entity_id) };
                        MsgHeader hdr = { .type = MSG_GAME_EVENT, .length = htonl(sizeof(ev)) };
                        send(hSocket, (char*)&hdr, sizeof(hdr), 0);
                        send(hSocket, (char*)&ev, sizeof(ev), 0);
                        printf("🔁 리스폰 요청을 보냈습니다!\n");
                        wants_respawn = 0;
                    }
                }
            }
            Sleep(120);
        }
    }

    show_cursor();
    cleanup_console_sync();
    closesocket(hSocket);
    WSACleanup();
    system("cls");
    printf("\nClient> 종료되었습니다.\n");
    return 0;
}
