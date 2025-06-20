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
void draw_player(int x, int y) {
    gotoxy(x, y);           // 지정 좌표로 이동
    putchar(PLAYER_CHAR);   // 플레이어 문자 출력 (기본값: 'A')
}

// 플레이어 캐릭터를 지우는 함수 (공백으로 덮어쓰기)
void erase_player(int x, int y) {
    gotoxy(x, y);
    putchar(' ');    // 기존 위치를 공백으로 출력해서 삭제
}
