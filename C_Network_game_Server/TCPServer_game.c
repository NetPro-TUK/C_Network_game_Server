#include <time.h>
#include "net_server.h"
#include "game_logic.h"

#define TICK_INTERVAL_MS 50  // 20 FPS

int main() {
    SOCKET serverSock = init_server_socket(9000);
    if (serverSock < 0) return 1;

    clock_t lastTick = clock();

    while (1) {
        int index = WSAWaitForMultipleEvents(numOfClnt, eventArr, FALSE, 1, FALSE);  // 1ms timeout
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
                if (recv_and_dispatch(i) < 0) {
                    closesocket(sockArr[i]);
                    WSACloseEvent(eventArr[i]);
                    sockArr[i] = sockArr[numOfClnt - 1];
                    eventArr[i] = eventArr[numOfClnt - 1];
                    --numOfClnt; --i;
                    printf("Server> client 종료\n");
                }
            }
            else if (netEvents.lNetworkEvents & FD_CLOSE) {
                closesocket(sockArr[i]);
                WSACloseEvent(eventArr[i]);
                sockArr[i] = sockArr[numOfClnt - 1];
                eventArr[i] = eventArr[numOfClnt - 1];
                --numOfClnt; --i;
            }
        }

        // 게임 틱 타이밍 제어
        clock_t now = clock();
        double elapsed_ms = (double)(now - lastTick) * 1000 / CLOCKS_PER_SEC;
        if (elapsed_ms >= TICK_INTERVAL_MS) {
            game_tick();
            send_state_update();
            lastTick = now;
        }
    }

    closesocket(serverSock);
    WSACleanup();
    return 0;
}
