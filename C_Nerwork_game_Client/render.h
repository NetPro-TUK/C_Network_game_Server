#ifndef RENDER_H
#define RENDER_H

#include <windows.h>

#define FIELD_WIDTH 80
#define FIELD_HEIGHT 25
#define DEFENDER_CHAR '<'
#define ATTACKER_CHAR '@'
#define BULLET_CHAR '-'


// 콘솔 동기화 초기화/정리
void init_console_sync(void);
void cleanup_console_sync(void);

// 상태 메시지 그리기 (동기화 포함)
void draw_status(const char* role_name);

// 콘솔 커서 위치 이동 및 가시성 제어
void gotoxy(int x, int y);
void hide_cursor();
void show_cursor();

// 엔티티 렌더링
void draw_defender(int x, int y);
void erase_defender(int x, int y);
void draw_attacker(int x, int y);
void erase_attacker(int x, int y);
void draw_bullet(int x, int y);
void erase_bullet(int x, int y);

// 게임 화면 테두리 그리기
void draw_border();

#endif // RENDER_H
