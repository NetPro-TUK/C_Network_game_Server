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
uint64_t client_last_survival_display = 0;

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
// 통합 렌더링
void redraw_full_screen() {
    system("cls");
    draw_border();
    draw_status(role == 1 ? "방어자" : "공격자");
    for (int i = 0; i < MAX_ENTITIES; ++i) {
        if (view_entities[i].active) {
            draw_entity(&view_entities[i]);
        }
    }
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
                for (int i = 0; i < MAX_ENTITIES; ++i) view_entities[i].active = 0;
                game_started = true;
                client_game_start_time = GetTickCount64();
                client_last_survival_display = client_game_start_time;
                redraw_full_screen();
            }
            else if (p.event_type == PLAYER_REJECTED) {
                role_status = ROLE_STATUS_REJECTED;
            }
            else if (p.event_type == OUT_OF_AMMO) {
                uint32_t id = ntohl(p.entityId);
                if (id == my_entity_id) {
                    gotoxy(0, FIELD_HEIGHT + 2);
                    printf("총알이 다 떨어졌습니다. 장전하시겠습니까? (R 키) ");
                }
            }
            else if (p.event_type == RELOAD_COMPLETE) {
                uint32_t id = ntohl(p.entityId);
                if (id == my_entity_id) {
                    redraw_full_screen();
                    gotoxy(0, FIELD_HEIGHT + 2);
                    printf("재장전이 완료되었습니다.");
                }
            }
            else if (p.event_type == ENTITY_REMOVE) {
                uint32_t id = ntohl(p.entityId);
                for (int k = 0; k < MAX_ENTITIES; ++k) {
                    if (view_entities[k].active && view_entities[k].entity_id == id) {
                        view_entities[k].active = 0;
                        erase_entity(&view_entities[k]);
                        if (id == my_entity_id && view_entities[k].type == ENTITY_ATTACKER) {
                            gotoxy(0, FIELD_HEIGHT + 2);
                            printf("리스폰하려면 (Y 키)를 누르세요. (Q 키)로 게임 종료. ");
                            wants_respawn = 1;
                        }
                        break;
                    }
                }
            }
            else if (p.event_type == SCORE_UPDATE) {
                uint32_t score = ntohl(p.entityId);
                gotoxy(FIELD_WIDTH - 24, FIELD_HEIGHT + 1);
                printf("점수: %u ", score);
            }
            else if (p.event_type == GAME_OVER) {
                system("cls");
                draw_border();

                uint64_t now = GetTickCount64();
                uint32_t survival_time = (uint32_t)((now - client_game_start_time) / 1000);
                uint32_t score = survival_time;  // 생존 시간이 점수가 되는 경우

                gotoxy((FIELD_WIDTH - 24) / 2, FIELD_HEIGHT / 2);
                printf("===> GAME OVER <===\n");

                gotoxy((FIELD_WIDTH - 30) / 2, FIELD_HEIGHT / 2 + 2);
                printf("총 점수: %u 점\n", score);

                gotoxy((FIELD_WIDTH - 30) / 2, FIELD_HEIGHT / 2 + 3);
                printf("생존 시간: %u 초\n", survival_time);

                gotoxy((FIELD_WIDTH - 30) / 2, FIELD_HEIGHT / 2 + 5);
                printf("Press 'Q' to quit...\n");

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

            if (id == my_entity_id && wants_respawn && entType == ENTITY_ATTACKER) {
                wants_respawn = 0;
                redraw_full_screen();
                gotoxy(0, FIELD_HEIGHT + 2);
                printf("리스폰 완료!\n");
            }

        CONTINUE:
            // 생존 시간 출력 (매 1초 간격)
            uint64_t now = GetTickCount64();
            if (now - client_last_survival_display >= 1000) {
                client_last_survival_display = now;
                uint32_t elapsed = (uint32_t)((now - client_game_start_time) / 1000);
                gotoxy(FIELD_WIDTH - 24, FIELD_HEIGHT + 2);
                printf("생존 시간: %us   ", elapsed);
            }

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
                    else if (vk == 'R') {
                        send_reload_request(hSocket, my_entity_id);
                    }
                }
            }
            Sleep(50);
        }
    }
    else { // 공격자
        while (1) {
            if (socket_disconnected) break;

            // 콘솔 키 이벤트 처리
            if (PeekConsoleInput(hStdin, &rec, 1, &cnt) && cnt > 0) {
                ReadConsoleInput(hStdin, &rec, 1, &cnt);

                if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
                    WORD vk = rec.Event.KeyEvent.wVirtualKeyCode;
                    if (vk == VK_ESCAPE) break;

                    // 리스폰 여부 판단
                    if (wants_respawn) {
                        if (vk == 0x59) {
                            // 리스폰 요청
                            MsgHeader hdr = {
                                .type = MSG_GAME_EVENT,
                                .length = htonl(sizeof(PayloadGameEvent))
                            };
                            PayloadGameEvent ev = {
                                .event_type = RESPAWN_REQUEST,
                                .entityId = htonl(my_entity_id)
                            };
                            send(hSocket, (char*)&hdr, sizeof(hdr), 0);
                            send(hSocket, (char*)&ev, sizeof(ev), 0);
                        }
						else if (vk == 0x51) { // Q 키
							printf("게임에서 퇴장합니다.\n");
							break;
						}
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