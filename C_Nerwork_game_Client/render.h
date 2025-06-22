#ifndef RENDER_H
#define RENDER_H

#include <windows.h>
#include <stdint.h>

// ================= 필드 크기 정의 =================
#define FIELD_WIDTH 80
#define FIELD_HEIGHT 25

// ================= 엔티티 문자 정의 =================
#define DEFENDER_CHAR '<'
#define ATTACKER_CHAR '@'
#define BULLET_CHAR '-'

// ================= 콘솔 동기화 처리 =================
// 콘솔 출력 동기화 초기화
void init_console_sync(void);
// 콘솔 출력 동기화 정리
void cleanup_console_sync(void);

// ================= 커서 설정 함수 =================
// 콘솔 커서 위치 이동 (동기화 포함)
void gotoxy(int x, int y);
// 커서 숨김
void hide_cursor(void);
// 커서 표시
void show_cursor(void);

// ================= 엔티티 렌더링 함수 (동기화 포함) =================
// 방어자 그리기
void draw_defender(int x, int y);
// 방어자 지우기
void erase_defender(int x, int y);
// 공격자 그리기
void draw_attacker(int x, int y);
// 공격자 지우기
void erase_attacker(int x, int y);
// 총알 그리기
void draw_bullet(int x, int y);
// 총알 지우기
void erase_bullet(int x, int y);

// ================= 테두리 렌더링 함수 (동기화 포함) =================
// 게임 화면 테두리 그리기 
void draw_border(void);

// ================= 기타 정보 출력 함수 (동기화 포함) =================
// 조작자 역할 출력
void draw_role(const char* role_name);
// 레이블과 값 출력 (예: 점수, 생존시간 등)
void draw_label_value(const char* label, uint32_t value, int x, int y);

#endif // RENDER_H
