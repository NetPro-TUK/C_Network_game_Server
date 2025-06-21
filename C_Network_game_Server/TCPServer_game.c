#include <time.h>
#include "entity.h"
#include "net_server.h"
#include "game_logic.h"

#define TICK_INTERVAL_MS 50  // 게임 틱 간격: 20 FPS 기준 (50ms 간격)
#define AUTO_MOVE_MS     120  // 공격자 자동 이동 간격


int main() {
    // 서버 소켓 초기화 및 바인딩
    SOCKET serverSock = init_server_socket(9000);
    if (serverSock < 0) return 1; 

    // 게임 루프용 시간 변수 초기화
    clock_t lastTick = clock();
    clock_t lastAutoMv = lastTick;        // 자동 이동 전용 타이머


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
                int r = recv_and_dispatch(i);
                if (r < 0) {
                    int err = WSAGetLastError();
                    if (err == WSAEWOULDBLOCK) continue; // 아직 읽을 데이터가 없으니, 소켓 닫지 않고 다음 이벤트 대기
                    // 그 외 오류일 때 소켓 정리
                    reset_defender_if_match(sockArr[i]);
                    remove_client_at(i);
                    --i;
                    printf("Server> client 종료 (FD_READ, err=%d)\n", err);
                }
            }
            // 클라이언트가 정상 종료된 경우
            else if (netEvents.lNetworkEvents & FD_CLOSE) {
                if (sockArr[i] != INVALID_SOCKET) {
                    reset_defender_if_match(sockArr[i]);
                    remove_client_at(i);
                    --i;
                    printf("Server> client 종료 (FD_CLOSE)\n");
                }
            }
        }

        // 게임 틱 처리: 일정 시간 간격으로 게임 상태 갱신
        clock_t now = clock();
        double tick_ms = (double)(now - lastTick) * 1000 / CLOCKS_PER_SEC;
        if (tick_ms >= TICK_INTERVAL_MS) {
            // 1) 기본 게임 틱(총알 이동·충돌 등)
            game_tick();
            lastTick = now;

            // 2) 자동 이동 전용 타이밍 체크 (120 ms)
            double auto_ms = (double)(now - lastAutoMv) * 1000 / CLOCKS_PER_SEC;
            if (auto_ms >= AUTO_MOVE_MS) {
                auto_move_attackers();
                lastAutoMv = now;
            }

            // 3) 상태 전송
            send_state_update();
        }
    }

    // 종료 시 리소스 해제
    closesocket(serverSock);
    WSACleanup();
    return 0;
}