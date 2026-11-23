#include "dirty.h"
#include "core.h"
#include <assert.h>
#include <stdio.h>
static void eq(tk_area a,int x1,int y1,int x2,int y2)
{ assert(a.x1==x1 && a.y1==y1 && a.x2==x2 && a.y2==y2); }
int main(void)
{
    tk_damage d={0};
    tk_damage_add(&d,(tk_area){104,0,423,319});
    tk_damage_add(&d,(tk_area){0,0,103,319});
    tk_damage_add(&d,(tk_area){424,0,527,319});
    assert(d.count==3); /* adjacent map and HUD must not coalesce */
    tk_damage e={0};
    tk_area old=tk_sprite_area(140*TK_Q,170*TK_Q,0,0,0);
    tk_area new=tk_sprite_area(143*TK_Q,170*TK_Q,0,0,0);
    tk_damage_add(&e,tk_area_union(old,new));
    assert(e.count==1);
    assert((e.boxes[0].x2-e.boxes[0].x1+1)*(e.boxes[0].y2-e.boxes[0].y1+1)<528*320/100);
    eq(e.boxes[0],244,170,244+TK_PW+2,170+TK_PH-1);
    tk_damage_add(&e,(tk_area){0,10,100,20});
    assert(e.count==2);
    tk_damage_add(&e,(tk_area){100,10,120,20});
    assert(e.count==2);
    tk_area a={-20,290,550,350};
    assert(tk_area_clip(&a,(tk_area){104,0,423,319}));
    eq(a,104,290,423,319);
    tk_area crash=tk_sprite_area(120*TK_Q,470*TK_Q,4,0,160);
    eq(crash,224+(TK_PW-TK_CRASH_W)/2,310+TK_PH-TK_CRASH_H,
       224+(TK_PW-TK_CRASH_W)/2+TK_CRASH_W-1,310+TK_PH-1);
    tk_damage blank={0};
    tk_damage_add(&blank,(tk_area){-50,-50,-1,-1});
    assert(blank.count==0);
    puts("dirty rectangles and player bounds: PASS");
    return 0;
}
