#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <winsock2.h>
#include <time.h>
#include "protocol.h"

// ȭ�� ��� (�ܼ� �������� �����Ӱ� ����)
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

extern uint32_t defender_owner_id;
#define COLLISION_RADIUS 1  // 1�̸� �����¿� 1ĭ���� �浹 ó��

// ���� ���� �̺�Ʈ ó�� �Լ�
void handle_join(SOCKET client_fd, PayloadJoin* payload);

// �Ѿ� �߻� �̺�Ʈ ó�� �Լ�
void handle_shooting_event(SOCKET client_fd, PayloadShootingEvent* payload);

// ���� �ð��� �и��� ������ ��ȯ�ϴ� �Լ�
uint64_t current_time_ms();

// ������ �̺�Ʈ ó�� �Լ�
void handle_reload_request();

// ���� ƽ ó�� �Լ�
void game_tick();

// ������ �ڵ� �̵�
void auto_move_attackers(void);

// ��ƼƼ ���� ������Ʈ �Լ�
void send_state_update();

// �浹 üũ �Լ�
void check_collision();

// ���� ���� üũ �Լ�
void check_game_over();

#endif
