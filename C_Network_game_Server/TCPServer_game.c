#include "net_server.h"

int main() {
	// ���� �ʱ�ȭ
    SOCKET serverSock = init_server_socket(9000);
    if (serverSock < 0) return 1;

	// �̺�Ʈ �迭 �ʱ�ȭ
    while (1) {
        int index = WSAWaitForMultipleEvents(numOfClnt, eventArr, FALSE, WSA_INFINITE, FALSE);
        int startIdx = index - WSA_WAIT_EVENT_0;

		// �̺�Ʈ�� �߻��� Ŭ���̾�Ʈ ������ ó��
        for (int i = startIdx; i < numOfClnt; ++i) {
            int sigIdx = WSAWaitForMultipleEvents(1, &eventArr[i], TRUE, 0, FALSE);
            if (sigIdx == WSA_WAIT_FAILED || sigIdx == WSA_WAIT_TIMEOUT) continue;

            WSANETWORKEVENTS netEvents;
            WSAEnumNetworkEvents(sockArr[i], eventArr[i], &netEvents);

			// ���ο� Ŭ���̾�Ʈ ���� ����
            if (netEvents.lNetworkEvents & FD_ACCEPT) {
                accept_new_client(serverSock);
            }
			// �б� �Ǵ� ���� �̺�Ʈ ó��
            else if (netEvents.lNetworkEvents & FD_READ) {
                if (recv_and_dispatch(i) < 0) {
                    closesocket(sockArr[i]);
                    WSACloseEvent(eventArr[i]);
                    sockArr[i] = sockArr[numOfClnt - 1];
                    eventArr[i] = eventArr[numOfClnt - 1];
                    --numOfClnt; --i;
                    printf("Server> client ����\n");
                }
            }
			// ���� �̺�Ʈ ó��
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
