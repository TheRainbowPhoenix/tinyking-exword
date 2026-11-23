#ifndef TK_DIRTY_H
#define TK_DIRTY_H
#include <stdbool.h>

/* Inclusive screen pixel coordinates.  The EX-word LCD receives these bounds
   directly, while gint's software clip uses exclusive right/bottom bounds. */
typedef struct { int x1, y1, x2, y2; } tk_area;
#define TK_DAMAGE_MAX 12
typedef struct { tk_area boxes[TK_DAMAGE_MAX]; int count; } tk_damage;

enum { TK_MAP_X = 104, TK_MAP_W = 320, TK_MAP_H = 320,
       TK_LCD_W = 528, TK_LCD_H = 320 };

tk_area tk_area_union(tk_area a, tk_area b);
bool tk_area_clip(tk_area *a, tk_area bounds);
tk_area tk_sprite_area(int x_q8, int y_q8, int pose, int room, int camera);
void tk_damage_add(tk_damage *damage, tk_area area);
#endif
