#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <conio.h>
#include <stdio.h>
#include <windows.h>

#include "protocol.h"
#include "net_client.h"
#include "render.h"

// 공격자 자동 이동 처리
void auto_move_attacker(SOCKET sock, uint32_t id, int* x, int* y) {
    erase_attacker(*x, *y);

    // x좌표 증가 및 경계 처리
    (*x)++;
    if (*x > 78) *x = 1;

    // y축 무작위 이동
    int dir = rand() % 2 == 0 ? -1 : 1;
    *y += dir;
    if (*y < 1) *y = 1;
    if (*y >= FIELD_HEIGHT - 1) *y = FIELD_HEIGHT - 2;

    draw_attacker(*x, *y);
    send_state_update(sock, id, *x, *y);
}


int main(void) {
    srand((unsigned int)time(NULL));

    // 1. 역할 선택
    int role = 0;
    while (role != 1 && role != 2) {
        printf("역할을 선택하세요: [1] 방어자 (DEFENDER), [2] 공격자 (ATTACKER): ");
        scanf("%d", &role);
    }

    // 2. 서버 연결
    SOCKET hSocket = connect_to_server("127.0.0.1", 9000);
    if (hSocket == INVALID_SOCKET) return 1;

    // 3. JOIN 및 내 ID 수신
    uint32_t myId = send_join_and_get_id(hSocket, role);

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
        // 방어자: 방향키 ↑ ↓
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
        // 공격자: 자동 이동 + ESC 종료 지원
        while (1) {
            if (_kbhit()) {
                int key = _getch();
                if (key == 27) break;  // ESC to exit
            }
            auto_move_attacker(hSocket, myId, &x, &y);
            Sleep(300);  // 공격자 이동 간격
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
