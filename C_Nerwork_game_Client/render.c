#include <windows.h>
#include <stdio.h>
#include "render.h"

// 동기화
static CRITICAL_SECTION g_console_cs;

// 콘솔 동기화 초기화/정리
void init_console_sync(void) {
    InitializeCriticalSection(&g_console_cs);
}

void cleanup_console_sync(void) {
    DeleteCriticalSection(&g_console_cs);
}

// 내부용: 동기화된 커서 이동
static void locked_gotoxy(int x, int y) {
    COORD pos = { (SHORT)x, (SHORT)y };                                 // COORD는 콘솔 좌표용 구조체
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);     // 콘솔 핸들에 좌표 설정
}

// 상태 메시지 그리기 (동기화 포함)
void draw_status(const char* role_name) {
    EnterCriticalSection(&g_console_cs);
    locked_gotoxy(0, FIELD_HEIGHT + 1);
    printf("%s 조작 중. ESC 키로 종료합니다.", role_name);
    LeaveCriticalSection(&g_console_cs);
}

// 콘솔 커서를 지정 좌표 (x, y)로 이동시키는 함수
void gotoxy(int x, int y) {
    EnterCriticalSection(&g_console_cs);
    locked_gotoxy(x, y);
    LeaveCriticalSection(&g_console_cs);
}

// 콘솔 커서를 숨기는 함수 (게임 화면 깔끔하게 만들기 위해 사용)
void hide_cursor(void) {
    CONSOLE_CURSOR_INFO ci = { 0 };
    ci.bVisible = FALSE;             // 커서 안 보이게 설정
    ci.dwSize = 1;                   // 커서 크기 (필수 설정)
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
}

// 콘솔 커서를 다시 보이게 만드는 함수 (게임 종료 시 커서 복원용)
void show_cursor(void) {
	CONSOLE_CURSOR_INFO ci = { 0 }; // 커서 다시 보이게 설정    
    ci.bVisible = TRUE;
    ci.dwSize = 1;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
}

// 방어자 캐릭터를 (x, y) 위치에 출력하는 함수
void draw_defender(int x, int y) {
    if (x < 0 || x >= FIELD_WIDTH || y < 0 || y >= FIELD_HEIGHT) return;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    EnterCriticalSection(&g_console_cs);
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    locked_gotoxy(x, y);            // 지정 좌표로 이동
    putchar(DEFENDER_CHAR);           // 플레이어 문자 출력 (기본값: 'A')
    SetConsoleTextAttribute(hConsole, 7);
    LeaveCriticalSection(&g_console_cs);
}

// 방어자 캐릭터를 지우는 함수 (공백으로 덮어쓰기)
void erase_defender(int x, int y) {
    if (x < 0 || x >= FIELD_WIDTH || y < 0 || y >= FIELD_HEIGHT) return;
    EnterCriticalSection(&g_console_cs);
    locked_gotoxy(x, y);
    putchar(' ');                   // 기존 위치를 공백으로 출력해서 삭제
    LeaveCriticalSection(&g_console_cs);
}

// 공격자 캐릭터를 (x, y) 위치에 출력하는 함수
void draw_attacker(int x, int y) {
    if (x < 0 || x >= FIELD_WIDTH || y < 0 || y >= FIELD_HEIGHT) return;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    EnterCriticalSection(&g_console_cs);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
    locked_gotoxy(x, y);
    putchar(ATTACKER_CHAR);
    SetConsoleTextAttribute(hConsole, 7);
    LeaveCriticalSection(&g_console_cs);
}

// 공격자 캐릭터를 지우는 함수 (공백으로 덮어쓰기)
void erase_attacker(int x, int y) {
    if (x < 0 || x >= FIELD_WIDTH || y < 0 || y >= FIELD_HEIGHT) return;
    EnterCriticalSection(&g_console_cs);
    locked_gotoxy(x, y);
    putchar(' ');
    LeaveCriticalSection(&g_console_cs);
}

// 총알을 (x, y) 위치에 출력하는 함수
void draw_bullet(int x, int y) {
	if (x < 0 || x >= FIELD_WIDTH || y < 0 || y >= FIELD_HEIGHT) return;
	EnterCriticalSection(&g_console_cs);
	locked_gotoxy(x, y);
	putchar(BULLET_CHAR);
	LeaveCriticalSection(&g_console_cs);
}

// 총알을 지우는 함수 (공백으로 덮어쓰기)
void erase_bullet(int x, int y) {
	if (x < 0 || x >= FIELD_WIDTH || y < 0 || y >= FIELD_HEIGHT) return;
	EnterCriticalSection(&g_console_cs);
	locked_gotoxy(x, y);
	putchar(' ');
	LeaveCriticalSection(&g_console_cs);
}

// 테두리를 민트색으로 그리는 함수
void draw_border(void) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    EnterCriticalSection(&g_console_cs);
    SetConsoleTextAttribute(hConsole, 11);  // 밝은 청록
    // 상/하 경계
    for (int x = 0; x < FIELD_WIDTH; x++) {
        locked_gotoxy(x, 0);
        putchar('-');
        locked_gotoxy(x, FIELD_HEIGHT - 1);
        putchar('-');
    }
    // 좌/우 경계
    for (int y = 0; y < FIELD_HEIGHT; y++) {
        locked_gotoxy(0, y);
        putchar('|');
        locked_gotoxy(FIELD_WIDTH - 1, y);
        putchar('|');
    }
    SetConsoleTextAttribute(hConsole, 7);   // 기본 색상 복원
    LeaveCriticalSection(&g_console_cs);
}
