﻿#include <time.h>
#include <stdint.h>  
#include "entity.h"
#include "net_server.h"
#include "game_logic.h"

#define TICK_INTERVAL_MS 50   // 게임 틱 간격: 20 FPS 기준 (50ms 간격)
#define AUTO_MOVE_MS     120  // 공격자 자동 이동 간격

int main() {
    SOCKET serverSock = init_server_socket(9000);
    if (serverSock < 0) return 1;

    clock_t lastTick = clock();
    clock_t lastAutoMv = lastTick;

    while (1) {
        int index = WSAWaitForMultipleEvents(numOfClnt, eventArr, FALSE, 1, FALSE);
        int startIdx = index - WSA_WAIT_EVENT_0;

        for (int i = startIdx; i < numOfClnt; ++i) {
            int sigIdx = WSAWaitForMultipleEvents(1, &eventArr[i], TRUE, 0, FALSE);
            if (sigIdx == WSA_WAIT_FAILED || sigIdx == WSA_WAIT_TIMEOUT) continue;

            WSANETWORKEVENTS netEvents;
            WSAEnumNetworkEvents(sockArr[i], eventArr[i], &netEvents);

            if (netEvents.lNetworkEvents & FD_ACCEPT) {
                accept_new_client(serverSock);
            }
            else if (netEvents.lNetworkEvents & FD_READ) {
                int r = recv_and_dispatch(i);
                if (r < 0) {
                    int err = WSAGetLastError();
                    if (err == WSAEWOULDBLOCK) continue;
                    reset_defender_if_match(sockArr[i]);
                    remove_client_at(i);
                    --i;
                    printf("Server> client 종료 (FD_READ, err=%d)\n", err);
                }
            }
            else if (netEvents.lNetworkEvents & FD_CLOSE) {
                if (sockArr[i] != INVALID_SOCKET) {
                    reset_defender_if_match(sockArr[i]);
                    remove_client_at(i);
                    --i;
                    printf("Server> client 종료 (FD_CLOSE)\n");
                }
            }
        }

        clock_t now = clock();
        double tick_ms = (double)(now - lastTick) * 1000 / CLOCKS_PER_SEC;
        if (tick_ms >= TICK_INTERVAL_MS) {
            game_tick();
            lastTick = now;

            double auto_ms = (double)(now - lastAutoMv) * 1000 / CLOCKS_PER_SEC;
            if (game_started && auto_ms >= AUTO_MOVE_MS) {
                auto_move_attackers();
                lastAutoMv = now;
            }

            // 게임 오버 처리
            if (server_game_over) {
                printf("Server> GAME OVER");
                break;
            }
        }
    }

    closesocket(serverSock);
    WSACleanup();
    return 0;
}
