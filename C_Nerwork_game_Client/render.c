#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include "render.h"

// ================= �ܼ� ����ȭ ó�� =================
// �ܼ� ��� ����ȭ ��������
static CRITICAL_SECTION g_console_cs;

// �ܼ� ��� ����ȭ �ʱ�ȭ
void init_console_sync(void) {
    InitializeCriticalSection(&g_console_cs);
}
// �ܼ� ��� ����ȭ ����
void cleanup_console_sync(void) {
    DeleteCriticalSection(&g_console_cs);
}

// ================= ���� ��ƿ �Լ� =================
// �ʵ� ���� üũ �Լ�
static inline int is_within_bounds(int x, int y) { // inline �Լ��� ����ȭ
    return (x >= 0 && x < FIELD_WIDTH && y >= 0 && y < FIELD_HEIGHT);
}

// �ܼ� Ŀ�� ��ġ �̵�
static void locked_gotoxy(int x, int y) {
    COORD pos = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

// �ܼ� Ŀ�� ��ġ �̵�
void gotoxy(int x, int y) {
    EnterCriticalSection(&g_console_cs);
    locked_gotoxy(x, y);
    LeaveCriticalSection(&g_console_cs);
}

// ================= Ŀ�� ���� �Լ� =================
// Ŀ�� ����
void hide_cursor(void) {
    CONSOLE_CURSOR_INFO ci = { 0 };
    ci.bVisible = FALSE;
    ci.dwSize = 1;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
}

// Ŀ�� ǥ��
void show_cursor(void) {
    CONSOLE_CURSOR_INFO ci = { 0 };
    ci.bVisible = TRUE;
    ci.dwSize = 1;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
}

// ================= ��ƼƼ ������ �Լ� =================
// ����� �׸���
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

// ����� �����
void erase_defender(int x, int y) {
    if (!is_within_bounds(x, y)) return;
    EnterCriticalSection(&g_console_cs);
    locked_gotoxy(x, y);
    putchar(' ');
    LeaveCriticalSection(&g_console_cs);
}

// ������ �׸���
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

// ������ �����
void erase_attacker(int x, int y) {
    if (!is_within_bounds(x, y)) return;
    EnterCriticalSection(&g_console_cs);
    locked_gotoxy(x, y);
    putchar(' ');
    LeaveCriticalSection(&g_console_cs);
}

// �Ѿ� �׸���
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

// �Ѿ� �����
void erase_bullet(int x, int y) {
    if (!is_within_bounds(x, y)) return;
    EnterCriticalSection(&g_console_cs);
    locked_gotoxy(x, y);
    putchar(' ');
    LeaveCriticalSection(&g_console_cs);
}

// ================= �׵θ� ������ �Լ� =================
void draw_border(void) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    EnterCriticalSection(&g_console_cs);
    SetConsoleTextAttribute(hConsole, 11);  // ���� û��

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

// ================= ��Ÿ ���� ��� �Լ� =================
// ������ 
void draw_role(const char* role_name) {
    EnterCriticalSection(&g_console_cs);
    locked_gotoxy(0, FIELD_HEIGHT + 1);
    printf("%s ���� ��. ESC Ű�� �����մϴ�.", role_name);
    LeaveCriticalSection(&g_console_cs);
}

// ���̺�� �� ��� �Լ� (����, �����ð� ��¿� ���) -> ��) ����: 10
void draw_label_value(const char* label, uint32_t value, int x, int y) {
    EnterCriticalSection(&g_console_cs);
    locked_gotoxy(x, y);
    printf("%s: %u   ", label, value);  // ���� ���� ����
    LeaveCriticalSection(&g_console_cs);
}

