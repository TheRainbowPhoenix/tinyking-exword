#include "dirty.h"
#include "core.h"

static int min(int a, int b) { return a < b ? a : b; }
static int max(int a, int b) { return a > b ? a : b; }

tk_area tk_area_union(tk_area a, tk_area b)
{
    return (tk_area){min(a.x1,b.x1), min(a.y1,b.y1),
                     max(a.x2,b.x2), max(a.y2,b.y2)};
}

bool tk_area_clip(tk_area *a, tk_area bounds)
{
    a->x1 = max(a->x1, bounds.x1); a->y1 = max(a->y1, bounds.y1);
    a->x2 = min(a->x2, bounds.x2); a->y2 = min(a->y2, bounds.y2);
    return a->x1 <= a->x2 && a->y1 <= a->y2;
}

tk_area tk_sprite_area(int x_q8, int y_q8, int pose, int room, int camera)
{
    int x = x_q8 / TK_Q + TK_MAP_X;
    int y = y_q8 / TK_Q - room * 480 - camera;
    int w = TK_PW, h = TK_PH;
    if(pose >= 4) {
        x += (TK_PW-TK_CRASH_W)/2;
        y += TK_PH-TK_CRASH_H;
        w = TK_CRASH_W;
        h = TK_CRASH_H;
    }
    return (tk_area){x, y, x+w-1, y+h-1};
}

void tk_damage_add(tk_damage *damage, tk_area area)
{
    if(!tk_area_clip(&area, (tk_area){0,0,TK_LCD_W-1,TK_LCD_H-1}))
        return;
    /* Merge overlaps, but NOT adjacent tiles. Otherwise the HUD and central
       map merge into a full-screen transfer every time the king moves. */
    for(int i=0; i<damage->count;) {
        tk_area b=damage->boxes[i];
        if(area.x1 <= b.x2 && b.x1 <= area.x2 &&
           area.y1 <= b.y2 && b.y1 <= area.y2) {
            area=tk_area_union(area,b);
            damage->boxes[i]=damage->boxes[--damage->count];
            i=0;
        } else i++;
    }
    if(damage->count == TK_DAMAGE_MAX) {
        damage->boxes[0]=(tk_area){0,0,TK_LCD_W-1,TK_LCD_H-1};
        damage->count=1;
    } else damage->boxes[damage->count++]=area;
}
