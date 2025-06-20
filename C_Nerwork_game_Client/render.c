#include <windows.h>
#include <stdio.h>
#include "render.h"

// 콘솔 커서를 지정 좌표 (x, y)로 이동시키는 함수
void gotoxy(int x, int y) {
    COORD pos = { (SHORT)x, (SHORT)y };                                 // COORD는 콘솔 좌표용 구조체
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);     // 콘솔 핸들에 좌표 설정
}

// 콘솔 커서를 숨기는 함수 (게임 화면 깔끔하게 만들기 위해 사용)
void hide_cursor() {
    CONSOLE_CURSOR_INFO cursorInfo = { 0, };
    cursorInfo.bVisible = 0;        // 커서 안 보이게 설정
    cursorInfo.dwSize = 1;          // 커서 크기 (필수 설정)
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

// 콘솔 커서를 다시 보이게 만드는 함수 (게임 종료 시 커서 복원용)
void show_cursor() {
    CONSOLE_CURSOR_INFO cursorInfo = { 0, };    // 커서 다시 보이게 설정
    cursorInfo.bVisible = 1;
    cursorInfo.dwSize = 1;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

// 플레이어 캐릭터를 (x, y) 위치에 출력하는 함수
void draw_defender(int x, int y) {
    gotoxy(x, y);           // 지정 좌표로 이동
    putchar(PLAYER_CHAR);   // 플레이어 문자 출력 (기본값: 'A')
}

// 플레이어 캐릭터를 지우는 함수 (공백으로 덮어쓰기)
void erase_defender(int x, int y) {
    gotoxy(x, y);
    putchar(' ');    // 기존 위치를 공백으로 출력해서 삭제
}

// 공격자 캐릭터를 (x, y) 위치에 출력하는 함수
void draw_attacker(int x, int y) {
    gotoxy(x, y);
    putchar('X');
}

// 공격자 캐릭터를 지우는 함수 (공백으로 덮어쓰기)
void erase_attacker(int x, int y) {
    gotoxy(x, y);
    putchar(' ');
}


// 테두리를 민트색으로 그리는 함수
void draw_border() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // 민트색으로 색상 설정 (밝은 청록 = 11)
    SetConsoleTextAttribute(hConsole, 11);

    // 위쪽 경계선
    for (int x = 0; x < FIELD_WIDTH; x++) {
        gotoxy(x, 0);
        putchar('-');
    }

    // 아래쪽 경계선
    for (int x = 0; x < FIELD_WIDTH; x++) {
        gotoxy(x, FIELD_HEIGHT - 1);
        putchar('-');
    }

    // 왼쪽, 오른쪽 경계선
    for (int y = 0; y < FIELD_HEIGHT; y++) {
        gotoxy(0, y);
        putchar('|');
        gotoxy(FIELD_WIDTH - 1, y);
        putchar('|');
    }

    // 색상 복원 (기본: 회색 = 7)
    SetConsoleTextAttribute(hConsole, 7);
}
