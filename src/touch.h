#ifndef TINYKING_TOUCH_H
#define TINYKING_TOUCH_H
#include <stdbool.h>
typedef struct {bool raw,active;int age,quiet,release,x,direction;} tk_touch;
void tk_touch_contact(tk_touch *t,int screen_x,int screen_y);
void tk_touch_lift(tk_touch *t);
int tk_touch_tick(tk_touch *t,int player_center_x);
#endif
