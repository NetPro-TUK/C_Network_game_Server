#ifndef RENDER_H
#define RENDER_H

#define FIELD_WIDTH 80
#define FIELD_HEIGHT 25
#define PLAYER_CHAR 'A'

void gotoxy(int x, int y);
void hide_cursor();
void show_cursor();
void draw_player(int x, int y);
void erase_player(int x, int y);

#endif

