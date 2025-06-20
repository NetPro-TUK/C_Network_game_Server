#include <windows.h>
#include <stdio.h>
#include "render.h"

// �ܼ� Ŀ���� ���� ��ǥ (x, y)�� �̵���Ű�� �Լ�
void gotoxy(int x, int y) {
    COORD pos = { (SHORT)x, (SHORT)y };                                 // COORD�� �ܼ� ��ǥ�� ����ü
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);     // �ܼ� �ڵ鿡 ��ǥ ����
}

// �ܼ� Ŀ���� ����� �Լ� (���� ȭ�� ����ϰ� ����� ���� ���)
void hide_cursor() {
    CONSOLE_CURSOR_INFO cursorInfo = { 0, };
    cursorInfo.bVisible = 0;        // Ŀ�� �� ���̰� ����
    cursorInfo.dwSize = 1;          // Ŀ�� ũ�� (�ʼ� ����)
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

// �ܼ� Ŀ���� �ٽ� ���̰� ����� �Լ� (���� ���� �� Ŀ�� ������)
void show_cursor() {
    CONSOLE_CURSOR_INFO cursorInfo = { 0, };    // Ŀ�� �ٽ� ���̰� ����
    cursorInfo.bVisible = 1;
    cursorInfo.dwSize = 1;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

// �÷��̾� ĳ���͸� (x, y) ��ġ�� ����ϴ� �Լ�
void draw_player(int x, int y) {
    gotoxy(x, y);           // ���� ��ǥ�� �̵�
    putchar(PLAYER_CHAR);   // �÷��̾� ���� ��� (�⺻��: 'A')
}

// �÷��̾� ĳ���͸� ����� �Լ� (�������� �����)
void erase_player(int x, int y) {
    gotoxy(x, y);
    putchar(' ');    // ���� ��ġ�� �������� ����ؼ� ����
}
