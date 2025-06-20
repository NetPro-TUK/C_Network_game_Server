#include <winsock2.h>
#include <conio.h>
#include <stdio.h>
#include <windows.h>

#include "protocol.h"
#include "net_client.h"
#include "render.h"

int main(void)
{
    // 1. 서버 연결
    SOCKET hSocket = connect_to_server("127.0.0.1", 9000);
    if (hSocket == INVALID_SOCKET) return 1;

    // 2. JOIN 메시지 전송 및 내 ID 수신
    int role = 1;  // 방어자 고정
    uint32_t myId = send_join_and_get_id(hSocket, role);

    // 3. 초기 플레이어 위치
    int player_x = 10;
    int player_y = FIELD_HEIGHT / 2;

    // 4. 콘솔 초기화
    hide_cursor();
    system("cls");
    draw_player(player_x, player_y);
    gotoxy(0, FIELD_HEIGHT);
    printf("↑ ↓ 방향키로 이동. ESC로 종료\n");

    // 5. 입력 처리 루프
    while (1) {
        if (_kbhit()) {
            int key = _getch();
            if (key == 27) break;  // ESC = 종료

            if (key == 224) {
                key = _getch();  // 방향키 실제 코드

                // 이동 처리
                erase_player(player_x, player_y);
                if (key == 72 && player_y > 0) player_y--;                         // ↑
                else if (key == 80 && player_y < FIELD_HEIGHT - 1) player_y++;   // ↓
                draw_player(player_x, player_y);

                // 상태 업데이트 서버 전송
                send_state_update(hSocket, myId, player_x, player_y);
            }
        }

        Sleep(50);  // 프레임 간격 (20 FPS)
    }

    // 6. 종료 처리
    show_cursor();
    closesocket(hSocket);
    WSACleanup();
    printf("\nClient> 종료되었습니다.\n");

    return 0;
}
