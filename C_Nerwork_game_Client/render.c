#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include "render.h"

// ================= 콘솔 동기화 처리 =================
// 콘솔 출력 동기화 전역변수
static CRITICAL_SECTION g_console_cs;

// 콘솔 출력 동기화 초기화
void init_console_sync(void) {
    InitializeCriticalSection(&g_console_cs);
}
// 콘솔 출력 동기화 정리
void cleanup_console_sync(void) {
    DeleteCriticalSection(&g_console_cs);
}

// ================= 내부 유틸 함수 =================
// 필드 범위 체크 함수
static inline int is_within_bounds(int x, int y) { // inline 함수로 최적화
    return (x >= 0 && x < FIELD_WIDTH && y >= 0 && y < FIELD_HEIGHT);
}

// 콘솔 커서 위치 이동
static void locked_gotoxy(int x, int y) {
    COORD pos = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

// 콘솔 커서 위치 이동
void gotoxy(int x, int y) {
    EnterCriticalSection(&g_console_cs);
    locked_gotoxy(x, y);
    LeaveCriticalSection(&g_console_cs);
}

// ================= 커서 설정 함수 =================
// 커서 숨김
void hide_cursor(void) {
    CONSOLE_CURSOR_INFO ci = { 0 };
    ci.bVisible = FALSE;
    ci.dwSize = 1;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
}

// 커서 표시
void show_cursor(void) {
    CONSOLE_CURSOR_INFO ci = { 0 };
    ci.bVisible = TRUE;
    ci.dwSize = 1;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
}

// ================= 엔티티 렌더링 함수 =================
// 방어자 그리기
void draw_defender(int x, int y) {
    if (!is_within_bounds(x, y)) return;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    EnterCriticalSection(&g_console_cs);
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    locked_gotoxy(x, y);
    putchar(DEFENDER_CHAR);
    SetConsoleTextAttribute(hConsole, 7);
    LeaveCriticalSection(&g_console_cs);
}

// 방어자 지우기
void erase_defender(int x, int y) {
    if (!is_within_bounds(x, y)) return;
    EnterCriticalSection(&g_console_cs);
    locked_gotoxy(x, y);
    putchar(' ');
    LeaveCriticalSection(&g_console_cs);
}

// 공격자 그리기
void draw_attacker(int x, int y) {
    if (!is_within_bounds(x, y)) return;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    EnterCriticalSection(&g_console_cs);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
    locked_gotoxy(x, y);
    putchar(ATTACKER_CHAR);
    SetConsoleTextAttribute(hConsole, 7);
    LeaveCriticalSection(&g_console_cs);
}

// 공격자 지우기
void erase_attacker(int x, int y) {
    if (!is_within_bounds(x, y)) return;
    EnterCriticalSection(&g_console_cs);
    locked_gotoxy(x, y);
    putchar(' ');
    LeaveCriticalSection(&g_console_cs);
}

// 총알 그리기
void draw_bullet(int x, int y) {
    if (!is_within_bounds(x, y)) return;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    EnterCriticalSection(&g_console_cs);
    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    locked_gotoxy(x, y);
    putchar(BULLET_CHAR);
    SetConsoleTextAttribute(hConsole, 7);
    LeaveCriticalSection(&g_console_cs);
}

// 총알 지우기
void erase_bullet(int x, int y) {
    if (!is_within_bounds(x, y)) return;
    EnterCriticalSection(&g_console_cs);
    locked_gotoxy(x, y);
    putchar(' ');
    LeaveCriticalSection(&g_console_cs);
}

// ================= 테두리 렌더링 함수 =================
void draw_border(void) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    EnterCriticalSection(&g_console_cs);
    SetConsoleTextAttribute(hConsole, 11);  // 밝은 청록

    for (int x = 0; x < FIELD_WIDTH; x++) {
        locked_gotoxy(x, 0);
        putchar('-');
        locked_gotoxy(x, FIELD_HEIGHT - 1);
        putchar('-');
    }
    for (int y = 0; y < FIELD_HEIGHT; y++) {
        locked_gotoxy(0, y);
        putchar('|');
        locked_gotoxy(FIELD_WIDTH - 1, y);
        putchar('|');
    }

    SetConsoleTextAttribute(hConsole, 7);
    LeaveCriticalSection(&g_console_cs);
}

// ================= 기타 정보 출력 함수 =================
// 조작자 
void draw_role(const char* role_name) {
    EnterCriticalSection(&g_console_cs);
    locked_gotoxy(0, FIELD_HEIGHT + 1);
    printf("%s 조작 중. ESC 키로 종료합니다.", role_name);
    LeaveCriticalSection(&g_console_cs);
}

// 레이블과 값 출력 함수 (점수, 생존시간 출력에 사용) -> 예) 점수: 10
void draw_label_value(const char* label, uint32_t value, int x, int y) {
    EnterCriticalSection(&g_console_cs);
    locked_gotoxy(x, y);
    printf("%s: %u   ", label, value);  // 여백 덮기 포함
    LeaveCriticalSection(&g_console_cs);
}

