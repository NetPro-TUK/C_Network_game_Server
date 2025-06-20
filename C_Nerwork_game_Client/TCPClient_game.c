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

// 수신 스레드: 서버에서 오는 메시지를 처리
DWORD WINAPI recv_server_thread(LPVOID arg) {
    SOCKET sock = *(SOCKET*)arg;

    while (1) {
        MsgHeader header;
        int result = recv_full(sock, &header, sizeof(header));
        if (result <= 0) {
            printf("[Thread] 서버 연결 종료 감지 (recv = %d), 스레드 종료\n", result);
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
            char dummy[512];
            if (len <= sizeof(dummy)) recv_full(sock, dummy, len);
        }
    }

    return 0; // 소켓은 main에서만 닫기
}


// 공격자 자동 이동 처리 함수
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

    // 1. 서버 연결
    SOCKET hSocket = connect_to_server("127.0.0.1", 9000);
    if (hSocket == INVALID_SOCKET) return 1;

    // 2. 수신 스레드 시작
    CreateThread(NULL, 0, recv_server_thread, &hSocket, 0, NULL);

    // 3. 역할 선택 및 JOIN 처리
    int role = 0;
    uint32_t myId = 0;

    while (1) {
        printf("역할을 선택하세요: [1] 방어자 (DEFENDER), [2] 공격자 (ATTACKER): ");
        scanf("%d", &role);

        if (role != 1 && role != 2) continue;

        // 기존 소켓 닫고 재연결
        closesocket(hSocket);
        hSocket = connect_to_server("127.0.0.1", 9000);
        if (hSocket == INVALID_SOCKET) return 1;

        // 수신 스레드 다시 시작
        CreateThread(NULL, 0, recv_server_thread, &hSocket, 0, NULL);

        // 요청 전송
        send_join_and_get_id(hSocket, role);

        // 응답 대기
        join_result_ready = 0;
        while (!join_result_ready) Sleep(10);

        if (player_rejected) {
            player_rejected = 0;
            continue;
        }

        myId = my_entity_id;
        break;
    }


    // 4. 콘솔 초기화
    hide_cursor();
    system("cls");
    draw_border();

    // 5. 초기 위치 설정
    int x = (role == 1) ? 70 : 1;
    int y = FIELD_HEIGHT / 2;

    if (role == 1) draw_defender(x, y);
    else draw_attacker(x, y);

    gotoxy(0, FIELD_HEIGHT);
    printf("%s 조작 중. ESC 또는 종료 키로 끝냅니다.\n", role == 1 ? "방어자" : "공격자");

    // 6. 게임 루프 (역할에 따라 분기)
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

    // 7. 종료 처리
    show_cursor();
    closesocket(hSocket);
    WSACleanup();
    system("cls");
    printf("\nClient> 종료되었습니다.\n");
    return 0;
}
