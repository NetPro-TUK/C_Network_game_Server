#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <winsock2.h>
#include "protocol.h"

// 화면 경계 (콘솔 기준으로 자유롭게 조정)
#define SCREEN_WIDTH 100
#define SCREEN_HEIGHT 150

extern uint32_t defender_owner_id;

// 게임 시작 이벤트 처리 함수
void handle_join(SOCKET client_fd, PayloadJoin* payload);

// 총알 발사 이벤트 처리 함수
void handle_action_event(SOCKET client_fd, PayloadActionEvent* payload);

// 게임 틱 처리 함수
void game_tick();

// 엔티티 상태 업데이트 함수
void send_state_update();

// 충돌 체크 함수
void check_collision();

// 게임 오버 체크 함수
void check_game_over();

#endif
