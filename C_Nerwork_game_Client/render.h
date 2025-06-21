#ifndef RENDER_H
#define RENDER_H

#include <windows.h>

#define FIELD_WIDTH 80
#define FIELD_HEIGHT 25
#define PLAYER_CHAR 'A'

// �ܼ� ����ȭ �ʱ�ȭ/����
void init_console_sync(void);
void cleanup_console_sync(void);

// ���� �޽��� �׸��� (����ȭ ����)
void draw_status(const char* role_name);

// �ܼ� Ŀ�� ��ġ �̵� �� ���ü� ����
void gotoxy(int x, int y);
void hide_cursor();
void show_cursor();

// ��ƼƼ ������
void draw_defender(int x, int y);
void erase_defender(int x, int y);
void draw_attacker(int x, int y);
void erase_attacker(int x, int y);

// ���� ȭ�� �׵θ� �׸���
void draw_border();

#endif // RENDER_H
