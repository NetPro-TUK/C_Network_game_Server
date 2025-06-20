#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>

#include "protocol.h"
#include "net_utils.h"
#include "log.h"

#pragma comment(lib, "Ws2_32.lib")

#define FIELD_WIDTH 80
#define FIELD_HEIGHT 25
#define PLAYER_CHAR 'A'

void gotoxy(int x, int y);
void hide_cursor();
void draw_player(int x, int y);
void erase_player(int x, int y);
void ErrorHandling(char* message);

int main(void)
{
    WSADATA wsaData;
    SOCKET hSocket;
    SOCKADDR_IN servAdr;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        ErrorHandling("WSAStartup() error!");

    hSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (hSocket == INVALID_SOCKET)
        ErrorHandling("socket() error");

    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servAdr.sin_port = htons(9000);

    if (connect(hSocket, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR) {
        printf("<ERROR> Client. connect() 실행 오류.\n");
        closesocket(hSocket);
        WSACleanup();
        return 0;
    }

    int role = 1; // 방어자 고정
    PayloadJoin joinPayload = { .role = role };
    MsgHeader joinHeader = {
        .type = MSG_JOIN,
        .length = htonl(sizeof(joinPayload))
    };

    send_full(hSocket, &joinHeader, sizeof(joinHeader));
    send_full(hSocket, &joinPayload, sizeof(joinPayload));

    MsgHeader ackHeader;
    recv_full(hSocket, &ackHeader, sizeof(ackHeader));
    if (ackHeader.type != MSG_JOIN_ACK || ntohl(ackHeader.length) != sizeof(PayloadJoinAck)) {
        printf("Client> 잘못된 MSG_JOIN_ACK 수신\n");
        closesocket(hSocket); WSACleanup(); return 1;
    }

    PayloadJoinAck ackPayload;
    recv_full(hSocket, &ackPayload, sizeof(ackPayload));
    uint32_t myId = ntohl(ackPayload.entityId);

    int player_x = 10;
    int player_y = FIELD_HEIGHT / 2;

    hide_cursor();
    system("cls");
    draw_player(player_x, player_y);
    gotoxy(0, FIELD_HEIGHT);
    printf("↑ ↓ 방향키로 이동. ESC로 종료\n");

    while (1) {
        if (_kbhit()) {
            int key = _getch();
            if (key == 27) break;

            if (key == 224) {
                key = _getch();
                erase_player(player_x, player_y);
                if (key == 72 && player_y > 0) player_y--; // ↑
                else if (key == 80 && player_y < FIELD_HEIGHT - 1) player_y++; // ↓
                draw_player(player_x, player_y);

                // 서버로 상태 전송
                PayloadStateUpdate statePayload = {
                    .entityId = htonl(myId),
                    .x = player_x,
                    .y = player_y
                };
                MsgHeader stateHeader = {
                    .type = MSG_STATE_UPDATE,
                    .length = htonl(sizeof(statePayload))
                };
                send_full(hSocket, &stateHeader, sizeof(stateHeader));
                send_full(hSocket, &statePayload, sizeof(statePayload));
            }
        }

        Sleep(50);
    }

    closesocket(hSocket);
    CONSOLE_CURSOR_INFO cursorInfo = { 0, };
    cursorInfo.bVisible = 1;
    cursorInfo.dwSize = 1;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    printf("\nClient> close socket...\n");
    WSACleanup();
    return 0;
}

void gotoxy(int x, int y) {
    COORD pos = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

void hide_cursor() {
    CONSOLE_CURSOR_INFO cursorInfo = { 0, };
    cursorInfo.bVisible = 0;
    cursorInfo.dwSize = 1;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

void draw_player(int x, int y) {
    gotoxy(x, y);
    putchar(PLAYER_CHAR);
}

void erase_player(int x, int y) {
    gotoxy(x, y);
    putchar(' ');
}

void ErrorHandling(char* message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
