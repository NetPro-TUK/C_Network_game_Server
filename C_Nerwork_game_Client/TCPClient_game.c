#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <conio.h>
#include <stdio.h>
#include <windows.h>

#include "protocol.h"
#include "net_client.h"
#include "render.h"

// ������ �ڵ� �̵� ó��
void auto_move_attacker(SOCKET sock, uint32_t id, int* x, int* y) {
    erase_attacker(*x, *y);

    // x��ǥ ���� �� ��� ó��
    (*x)++;
    if (*x > 78) *x = 1;

    // y�� ������ �̵�
    int dir = rand() % 2 == 0 ? -1 : 1;
    *y += dir;
    if (*y < 1) *y = 1;
    if (*y >= FIELD_HEIGHT - 1) *y = FIELD_HEIGHT - 2;

    draw_attacker(*x, *y);
    send_state_update(sock, id, *x, *y);
}


int main(void) {
    srand((unsigned int)time(NULL));

    // 1. ���� ����
    int role = 0;
    while (role != 1 && role != 2) {
        printf("������ �����ϼ���: [1] ����� (DEFENDER), [2] ������ (ATTACKER): ");
        scanf("%d", &role);
    }

    // 2. ���� ����
    SOCKET hSocket = connect_to_server("127.0.0.1", 9000);
    if (hSocket == INVALID_SOCKET) return 1;

    // 3. JOIN �� �� ID ����
    uint32_t myId = send_join_and_get_id(hSocket, role);

    // 4. �ܼ� �ʱ�ȭ
    hide_cursor();
    system("cls");
    draw_border();

    // 5. �ʱ� ��ġ ����
    int x = (role == 1) ? 70 : 1;
    int y = FIELD_HEIGHT / 2;

    if (role == 1) draw_defender(x, y);
    else draw_attacker(x, y);

    gotoxy(0, FIELD_HEIGHT);
    printf("%s ���� ��. ESC �Ǵ� ���� Ű�� �����ϴ�.\n", role == 1 ? "�����" : "������");

    // 6. ���� ���� (���ҿ� ���� �б�)
    if (role == 1) {
        // �����: ����Ű �� ��
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
        // ������: �ڵ� �̵� + ESC ���� ����
        while (1) {
            if (_kbhit()) {
                int key = _getch();
                if (key == 27) break;  // ESC to exit
            }
            auto_move_attacker(hSocket, myId, &x, &y);
            Sleep(300);  // ������ �̵� ����
        }
    }

    // 7. ���� ó��
    show_cursor();
    closesocket(hSocket);
    WSACleanup();
    system("cls");
    printf("\nClient> ����Ǿ����ϴ�.\n");
    return 0;
}
