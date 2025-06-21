#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <winsock2.h>
#include <time.h>
#include "protocol.h"

// 화면 경계 (콘솔 기준으로 자유롭게 조정)
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

extern uint32_t defender_owner_id;
#define COLLISION_RADIUS 1  // 1이면 상하좌우 1칸까지 충돌 처리

// 게임 시작 이벤트 처리 함수
void handle_join(SOCKET client_fd, PayloadJoin* payload);

// 총알 발사 이벤트 처리 함수
void handle_shooting_event(SOCKET client_fd, PayloadShootingEvent* payload);

// 현재 시간을 밀리초 단위로 반환하는 함수
uint64_t current_time_ms();

// 재장전 이벤트 처리 함수
void handle_reload_request();

// 게임 틱 처리 함수
void game_tick();

// 공격자 자동 이동
void auto_move_attackers(void);

// 엔티티 상태 업데이트 함수
void send_state_update();

// 충돌 체크 함수
void check_collision();

// 게임 오버 체크 함수
void check_game_over();

#endif
