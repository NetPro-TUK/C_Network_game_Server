#include <time.h>
#include "net_server.h"
#include "game_logic.h"

#define TICK_INTERVAL_MS 50  // 게임 틱 간격: 20 FPS 기준 (50ms 간격)

int main() {
    // 서버 소켓 초기화 및 바인딩
    SOCKET serverSock = init_server_socket(9000);
    if (serverSock < 0) return 1; 

    // 게임 루프용 시간 변수 초기화
    clock_t lastTick = clock();

    while (1) {
        // 이벤트 감지 (최대 numOfClnt개 이벤트)
        int index = WSAWaitForMultipleEvents(numOfClnt, eventArr, FALSE, 1, FALSE);  // 1ms 대기
        int startIdx = index - WSA_WAIT_EVENT_0;

        for (int i = startIdx; i < numOfClnt; ++i) {
            // 개별 클라이언트에 대한 이벤트가 발생했는지 확인
            int sigIdx = WSAWaitForMultipleEvents(1, &eventArr[i], TRUE, 0, FALSE);
            if (sigIdx == WSA_WAIT_FAILED || sigIdx == WSA_WAIT_TIMEOUT) continue;

            WSANETWORKEVENTS netEvents;
			// 이벤트 종류 확인
            WSAEnumNetworkEvents(sockArr[i], eventArr[i], &netEvents);

            // 클라이언트 접속 요청이 들어온 경우
            if (netEvents.lNetworkEvents & FD_ACCEPT) {
				accept_new_client(serverSock);  // 새로운 클라이언트 연결 처리
            }
			// 클라이언트로부터 데이터 수신 이벤트가 발생한 경우
            else if (netEvents.lNetworkEvents & FD_READ) {
				if (recv_and_dispatch(i) < 0) {     // 수신 오류 발생
                    closesocket(sockArr[i]);
                    WSACloseEvent(eventArr[i]);
                    sockArr[i] = sockArr[numOfClnt - 1];
                    eventArr[i] = eventArr[numOfClnt - 1];
                    --numOfClnt; --i;
                    printf("Server> client 종료\n");
                }
            }
            // 클라이언트가 정상 종료된 경우
            else if (netEvents.lNetworkEvents & FD_CLOSE) {
                closesocket(sockArr[i]);
                WSACloseEvent(eventArr[i]);
                // 배열에서 제거
                sockArr[i] = sockArr[numOfClnt - 1];
                eventArr[i] = eventArr[numOfClnt - 1];
                --numOfClnt; --i;
                printf("Server> client 종료\n");
            }
        }

        // 게임 틱 처리: 일정 시간 간격으로 게임 상태 갱신
        clock_t now = clock();
        double elapsed_ms = (double)(now - lastTick) * 1000 / CLOCKS_PER_SEC;
        if (elapsed_ms >= TICK_INTERVAL_MS) {
			game_tick();            // 엔터티 상태 업데이트, 충돌 검사 등
			send_state_update();    // 모든 클라이언트에게 상태 전송
			lastTick = now;         // 다음 틱 시간 갱신
        }
    }

    // 종료 시 리소스 해제
    closesocket(serverSock);
    WSACleanup();
    return 0;
}
