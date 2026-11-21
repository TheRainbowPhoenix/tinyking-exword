#include "touch.h"
#include "core.h"
void tk_touch_contact(tk_touch *t,int x,int y)
{
    /* The EX-word logical display is 528x320; map is x=104..423. */
    if(x>=104 && x<424 && y>=0 && y<320){
        t->x=x-104;t->raw=true;t->quiet=t->release=0;
    }
}
void tk_touch_lift(tk_touch *t){t->raw=false;}
int tk_touch_tick(tk_touch *t,int player_x)
{
    t->quiet++;
    if(t->quiet>24)t->raw=false;
    if(t->raw){
        if(t->age<2)t->age++;
        t->release=0;
        if(t->age>=2){
            t->active=true;
            int delta=t->x-player_x;
            if(delta< -20)t->direction=TK_LEFT;
            else if(delta>20)t->direction=TK_RIGHT;
            else if(delta>= -10 && delta<=10)t->direction=0;
        }
    }else{
        t->release++;
        if(!t->active || t->release>=2){
            t->active=false;t->age=0;t->direction=0;
        }
    }
    return t->active?(t->direction|TK_JUMP):0;
}
