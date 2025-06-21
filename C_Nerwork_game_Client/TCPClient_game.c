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
volatile RoleStatus role_status = ROLE_STATUS_PENDING; // 역할 상태
volatile uint32_t my_entity_id = 0;    // 내 엔티티 ID
volatile int socket_disconnected = 0;  // 소켓 끊김 여부
static bool game_started = false;      // 게임 시작 여부
int role = 0;       // 내 역할 (0: 공격자, 1: 방어자)                   

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

        // 서버의 조인 응답
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
                // 1) 로컬 화면 초기화 (main()에서 system("cls")와 draw_border()는 이미 호출됨)
                for (int i = 0; i < MAX_ENTITIES; ++i) {
                    view_entities[i].active = 0;
                }
                game_started = true;
            }
            else if (p.event_type == PLAYER_REJECTED) {
                role_status = ROLE_STATUS_REJECTED;
            }
        }
        // **게임 시작 신호를 받은 후에만 상태 업데이트 처리**
        else if (game_started && header.type == MSG_STATE_UPDATE && len == sizeof(PayloadStateUpdate)) {
            PayloadStateUpdate payload;
            recv_full(sock, &payload, sizeof(payload));

            uint32_t id = ntohl(payload.entityId);
            int      x = payload.x;
            int      y = payload.y;
            int      entType = payload.role;


            if (!is_valid_position(x, y)) continue;

            // 기존 엔티티 갱신
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
            // 새 엔티티 등록
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
            // 나머지 메시지(또는 game_started==false시 STATE_UPDATE) 무시
            char dummy[512];
            if (len <= sizeof(dummy)) recv_full(sock, dummy, len);
        }
    }
    return 0;
}

int main(void) {
    srand((unsigned int)time(NULL));
    SOCKET hSocket;

	init_console_sync(); // 콘솔 동기화 초기화

    // 1) 역할 선택 & 서버 접속
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

    // 역할 선택 완료 후, 게임 시작 전용 대기 화면
    system("cls");
    printf("====================================\n");
    printf("    역할: %s 로 선택되었습니다.\n",
        role == 1 ? "방어자" : "공격자");
    printf("\n");
    printf("    게임을 시작하려면 ENTER 키를 누르세요.\n");
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

    // 2) 기존 게임 화면 초기화
    hide_cursor();
    system("cls");
    draw_border();

    // 3) 초기 위치 설정
    int x = (role == 1) ? 70 : 1;
    int y = FIELD_HEIGHT / 2;

    // 4) 플레이 루프 시작 전: 내 콘솔 윈도우 핸들 얻기
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);

    INPUT_RECORD rec;
    DWORD        cnt;


    // 4) 플레이 루프
    if (role == 1) { // 방어자
        while (1) {
            // 콘솔 입력 버퍼에서 이벤트가 쌓였으면 하나 가져오기
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
                }
            }
            Sleep(50);
        }
    }
    else { // 공격자
        while (1) {
            // ESC 만 처리 (자동 이동은 계속)
            if (PeekConsoleInput(hStdin, &rec, 1, &cnt) && cnt > 0) {
                ReadConsoleInput(hStdin, &rec, 1, &cnt);
                if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown &&
                    rec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) {
                    break;
                }
            }
            Sleep(120);
        }
    }

    // 5) 종료 처리
    show_cursor();
    cleanup_console_sync();   
    closesocket(hSocket);
    WSACleanup();
    system("cls");
    printf("\nClient> 종료되었습니다.\n");
    return 0;
}