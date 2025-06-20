#include "net_server.h"

int main() {
	// 윈속 초기화
    SOCKET serverSock = init_server_socket(9000);
    if (serverSock < 0) return 1;

	// 이벤트 배열 초기화
    while (1) {
        int index = WSAWaitForMultipleEvents(numOfClnt, eventArr, FALSE, WSA_INFINITE, FALSE);
        int startIdx = index - WSA_WAIT_EVENT_0;

		// 이벤트가 발생한 클라이언트 소켓을 처리
        for (int i = startIdx; i < numOfClnt; ++i) {
            int sigIdx = WSAWaitForMultipleEvents(1, &eventArr[i], TRUE, 0, FALSE);
            if (sigIdx == WSA_WAIT_FAILED || sigIdx == WSA_WAIT_TIMEOUT) continue;

            WSANETWORKEVENTS netEvents;
            WSAEnumNetworkEvents(sockArr[i], eventArr[i], &netEvents);

			// 새로운 클라이언트 연결 수락
            if (netEvents.lNetworkEvents & FD_ACCEPT) {
                accept_new_client(serverSock);
            }
			// 읽기 또는 종료 이벤트 처리
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
			// 종료 이벤트 처리
            else if (netEvents.lNetworkEvents & FD_CLOSE) {
                closesocket(sockArr[i]);
                WSACloseEvent(eventArr[i]);
                sockArr[i] = sockArr[numOfClnt - 1];
                eventArr[i] = eventArr[numOfClnt - 1];
                --numOfClnt; --i;
            }
        }
    }

    closesocket(serverSock);
    WSACleanup();
    return 0;
}
