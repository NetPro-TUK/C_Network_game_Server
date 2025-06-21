#include <windows.h>
#include <stdio.h>
#include "render.h"

// ����ȭ
static CRITICAL_SECTION g_console_cs;

// �ܼ� ����ȭ �ʱ�ȭ/����
void init_console_sync(void) {
    InitializeCriticalSection(&g_console_cs);
}

void cleanup_console_sync(void) {
    DeleteCriticalSection(&g_console_cs);
}

// ���ο�: ����ȭ�� Ŀ�� �̵�
static void locked_gotoxy(int x, int y) {
    COORD pos = { (SHORT)x, (SHORT)y };                                 // COORD�� �ܼ� ��ǥ�� ����ü
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);     // �ܼ� �ڵ鿡 ��ǥ ����
}

// ���� �޽��� �׸��� (����ȭ ����)
void draw_status(const char* role_name) {
    EnterCriticalSection(&g_console_cs);
    locked_gotoxy(0, FIELD_HEIGHT + 1);
    printf("%s ���� ��. ESC Ű�� �����մϴ�.", role_name);
    LeaveCriticalSection(&g_console_cs);
}

// �ܼ� Ŀ���� ���� ��ǥ (x, y)�� �̵���Ű�� �Լ�
void gotoxy(int x, int y) {
    EnterCriticalSection(&g_console_cs);
    locked_gotoxy(x, y);
    LeaveCriticalSection(&g_console_cs);
}

// �ܼ� Ŀ���� ����� �Լ� (���� ȭ�� ����ϰ� ����� ���� ���)
void hide_cursor(void) {
    CONSOLE_CURSOR_INFO ci = { 0 };
    ci.bVisible = FALSE;             // Ŀ�� �� ���̰� ����
    ci.dwSize = 1;                   // Ŀ�� ũ�� (�ʼ� ����)
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
}

// �ܼ� Ŀ���� �ٽ� ���̰� ����� �Լ� (���� ���� �� Ŀ�� ������)
void show_cursor(void) {
	CONSOLE_CURSOR_INFO ci = { 0 }; // Ŀ�� �ٽ� ���̰� ����    
    ci.bVisible = TRUE;
    ci.dwSize = 1;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
}

// ����� ĳ���͸� (x, y) ��ġ�� ����ϴ� �Լ�
void draw_defender(int x, int y) {
    if (x < 0 || x >= FIELD_WIDTH || y < 0 || y >= FIELD_HEIGHT) return;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    EnterCriticalSection(&g_console_cs);
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    locked_gotoxy(x, y);            // ���� ��ǥ�� �̵�
    putchar(DEFENDER_CHAR);           // �÷��̾� ���� ��� (�⺻��: 'A')
    SetConsoleTextAttribute(hConsole, 7);
    LeaveCriticalSection(&g_console_cs);
}

// ����� ĳ���͸� ����� �Լ� (�������� �����)
void erase_defender(int x, int y) {
    if (x < 0 || x >= FIELD_WIDTH || y < 0 || y >= FIELD_HEIGHT) return;
    EnterCriticalSection(&g_console_cs);
    locked_gotoxy(x, y);
    putchar(' ');                   // ���� ��ġ�� �������� ����ؼ� ����
    LeaveCriticalSection(&g_console_cs);
}

// ������ ĳ���͸� (x, y) ��ġ�� ����ϴ� �Լ�
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

// ������ ĳ���͸� ����� �Լ� (�������� �����)
void erase_attacker(int x, int y) {
    if (x < 0 || x >= FIELD_WIDTH || y < 0 || y >= FIELD_HEIGHT) return;
    EnterCriticalSection(&g_console_cs);
    locked_gotoxy(x, y);
    putchar(' ');
    LeaveCriticalSection(&g_console_cs);
}

// �Ѿ��� (x, y) ��ġ�� ����ϴ� �Լ�
void draw_bullet(int x, int y) {
	if (x < 0 || x >= FIELD_WIDTH || y < 0 || y >= FIELD_HEIGHT) return;
	EnterCriticalSection(&g_console_cs);
	locked_gotoxy(x, y);
	putchar(BULLET_CHAR);
	LeaveCriticalSection(&g_console_cs);
}

// �Ѿ��� ����� �Լ� (�������� �����)
void erase_bullet(int x, int y) {
	if (x < 0 || x >= FIELD_WIDTH || y < 0 || y >= FIELD_HEIGHT) return;
	EnterCriticalSection(&g_console_cs);
	locked_gotoxy(x, y);
	putchar(' ');
	LeaveCriticalSection(&g_console_cs);
}

// �׵θ��� ��Ʈ������ �׸��� �Լ�
void draw_border(void) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    EnterCriticalSection(&g_console_cs);
    SetConsoleTextAttribute(hConsole, 11);  // ���� û��
    // ��/�� ���
    for (int x = 0; x < FIELD_WIDTH; x++) {
        locked_gotoxy(x, 0);
        putchar('-');
        locked_gotoxy(x, FIELD_HEIGHT - 1);
        putchar('-');
    }
    // ��/�� ���
    for (int y = 0; y < FIELD_HEIGHT; y++) {
        locked_gotoxy(0, y);
        putchar('|');
        locked_gotoxy(FIELD_WIDTH - 1, y);
        putchar('|');
    }
    SetConsoleTextAttribute(hConsole, 7);   // �⺻ ���� ����
    LeaveCriticalSection(&g_console_cs);
}
