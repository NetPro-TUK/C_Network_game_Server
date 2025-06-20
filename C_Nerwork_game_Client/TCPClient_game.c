#include <winsock2.h>
#include <conio.h>
#include <stdio.h>
#include <windows.h>

#include "protocol.h"
#include "net_client.h"
#include "render.h"

int main(void)
{
    // 1. ���� ����
    SOCKET hSocket = connect_to_server("127.0.0.1", 9000);
    if (hSocket == INVALID_SOCKET) return 1;

    // 2. JOIN �޽��� ���� �� �� ID ����
    int role = 1;  // ����� ����
    uint32_t myId = send_join_and_get_id(hSocket, role);

    // 3. �ʱ� �÷��̾� ��ġ
    int player_x = 10;
    int player_y = FIELD_HEIGHT / 2;

    // 4. �ܼ� �ʱ�ȭ
    hide_cursor();
    system("cls");
    draw_player(player_x, player_y);
    gotoxy(0, FIELD_HEIGHT);
    printf("�� �� ����Ű�� �̵�. ESC�� ����\n");

    // 5. �Է� ó�� ����
    while (1) {
        if (_kbhit()) {
            int key = _getch();
            if (key == 27) break;  // ESC = ����

            if (key == 224) {
                key = _getch();  // ����Ű ���� �ڵ�

                // �̵� ó��
                erase_player(player_x, player_y);
                if (key == 72 && player_y > 0) player_y--;                         // ��
                else if (key == 80 && player_y < FIELD_HEIGHT - 1) player_y++;   // ��
                draw_player(player_x, player_y);

                // ���� ������Ʈ ���� ����
                send_state_update(hSocket, myId, player_x, player_y);
            }
        }

        Sleep(50);  // ������ ���� (20 FPS)
    }

    // 6. ���� ó��
    show_cursor();
    closesocket(hSocket);
    WSACleanup();
    printf("\nClient> ����Ǿ����ϴ�.\n");

    return 0;
}
