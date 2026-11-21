#ifndef TINYKING_WORLD_H
#define TINYKING_WORLD_H
#include "core.h"
#include "pak.h"
#define TK_MAX_BOXES 600
typedef struct {
    tk_pak *pak;
    int width,rooms,spawn_x,spawn_y,height,room,band;
    int all_count,near_count;
    tk_box all[TK_MAX_BOXES],near[TK_MAX_BOXES];
} tk_world;
int tk_world_init(tk_world *w,tk_pak *pak);
int tk_world_update(tk_world *w,int y_q8);
#endif
