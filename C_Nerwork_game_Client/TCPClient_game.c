#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <conio.h>
#include <stdio.h>
#include <windows.h>

#include "protocol.h"
#include "net_client.h"
#include "net_utils.h"
#include "render.h"

// 전역 상태 변수 (수신 스레드와 공유)
volatile int join_result_ready = 0;
volatile uint32_t my_entity_id = 0;
volatile int player_rejected = 0;
volatile int socket_disconnected = 0;

// 수신 스레드
DWORD WINAPI recv_server_thread(LPVOID arg) {
    SOCKET sock = *(SOCKET*)arg;

    while (1) {
        MsgHeader header;
        int ret = recv_full(sock, &header, sizeof(header));
        if (ret <= 0) {
            printf("[Thread] 서버 연결 종료 감지 (recv = %d), 스레드 종료\n", ret);
            socket_disconnected = 1;
            break;
        }

        uint32_t len = ntohl(header.length);

        if (header.type == MSG_JOIN_ACK && len == sizeof(PayloadJoinAck)) {
            PayloadJoinAck payload;
            recv_full(sock, &payload, sizeof(payload));
            my_entity_id = ntohl(payload.entityId);
            join_result_ready = 1;
        }
        else if (header.type == MSG_GAME_EVENT && len == sizeof(PayloadGameEvent)) {
            PayloadGameEvent payload;
            recv_full(sock, &payload, sizeof(payload));
            if (payload.event_type == PLAYER_REJECTED) {
                player_rejected = 1;
                join_result_ready = 1;
            }
        }
        else {
            // 기타 메시지 무시
            char dummy[512];
            if (len <= sizeof(dummy)) recv_full(sock, dummy, len);
        }
    }

    return 0;
}

// 공격자 자동 이동
void auto_move_attacker(SOCKET sock, uint32_t id, int* x, int* y) {
    erase_attacker(*x, *y);
    (*x)++;
    if (*x > 78) *x = 1;

    int dir = rand() % 2 == 0 ? -1 : 1;
    *y += dir;
    if (*y < 1) *y = 1;
    if (*y >= FIELD_HEIGHT - 1) *y = FIELD_HEIGHT - 2;

    draw_attacker(*x, *y);
    send_state_update(sock, id, *x, *y);
}

int main(void) {
    srand((unsigned int)time(NULL));

    int role = 0;
    uint32_t myId = 0;
    SOCKET hSocket;

    while (1) {
        // 1. 역할 선택
        printf("역할을 선택하세요: [1] 방어자 (DEFENDER), [2] 공격자 (ATTACKER): ");
        scanf("%d", &role);
        if (role != 1 && role != 2) continue;

        // 2. 서버 연결 (매번 새로 연결)
        hSocket = connect_to_server("127.0.0.1", 9000);
        if (hSocket == INVALID_SOCKET) {
            printf("서버 연결 실패. 종료합니다.\n");
            return 1;
        }

        // 3. 수신 스레드 시작
        join_result_ready = 0;
        player_rejected = 0;
        socket_disconnected = 0;
        CreateThread(NULL, 0, recv_server_thread, &hSocket, 0, NULL);

        // 4. JOIN 요청
        send_join_and_get_id(hSocket, role);

        // 5. 결과 대기
        while (!join_result_ready && !socket_disconnected) {
            Sleep(10);
        }

        if (socket_disconnected) {
            printf("서버와 연결이 끊겼습니다. 다시 시도하세요.\n");
            closesocket(hSocket);
            WSACleanup();
            continue;
        }

        if (player_rejected) {
            printf("[알림] 방어자가 이미 존재합니다. 다시 선택하세요.\n");
            closesocket(hSocket);
            WSACleanup();
            continue;
        }

        myId = my_entity_id;
        break;  // 성공적으로 join
    }

    // 6. 콘솔 초기화
    hide_cursor();
    system("cls");
    draw_border();

    int x = (role == 1) ? 70 : 1;
    int y = FIELD_HEIGHT / 2;

    if (role == 1) draw_defender(x, y);
    else draw_attacker(x, y);

    gotoxy(0, FIELD_HEIGHT);
    printf("%s 조작 중. ESC 또는 종료 키로 끝냅니다.\n", role == 1 ? "방어자" : "공격자");

    // 7. 게임 루프
    if (role == 1) {
        while (1) {
            if (_kbhit()) {
                int key = _getch();
                if (key == 27) break;
                if (key == 224) {
                    key = _getch();
                    erase_defender(x, y);
                    if (key == 72 && y > 1) y--;
                    else if (key == 80 && y < FIELD_HEIGHT - 2) y++;
                    draw_defender(x, y);
                    send_state_update(hSocket, myId, x, y);
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
            auto_move_attacker(hSocket, myId, &x, &y);
            Sleep(300);
        }
    }

    // 8. 종료
    show_cursor();
    closesocket(hSocket);
    WSACleanup();
    system("cls");
    printf("\nClient> 종료되었습니다.\n");
    return 0;
}
