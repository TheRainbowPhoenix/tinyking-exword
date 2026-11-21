#ifndef TINYKING_CORE_H
#define TINYKING_CORE_H
#include <stdbool.h>

enum { TK_LEFT=1, TK_RIGHT=2, TK_JUMP=4, TK_EXIT=8, TK_RECOVER=16 };
enum { TK_Q=256, TK_PW=12, TK_PH=17, TK_CRASH_W=20, TK_CRASH_H=10,
       TK_SPLAT_TICKS=15, TK_AIR=1195, TK_GRAVITY=171, TK_MAX_FALL=3413,
       TK_WALK=512, TK_CHARGE=18, TK_JUMP_SPEED=2901,
       TK_ICE_FRICTION=34, TK_ICE_PUSH=68 };
typedef struct { int x,y,w,h,kind; } tk_box;
typedef struct {
    int x,y,vx,vy,charge,direction,previous,facing,crash,falls;
    bool grounded,landed,bounced,flying,ice;
} tk_player;
void tk_player_init(tk_player *p, int x, int y);
void tk_step(tk_player *p, int keys, const tk_box *boxes, int count, int width, int wind);
#endif
