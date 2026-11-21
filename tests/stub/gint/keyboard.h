#ifndef TEST_KEYBOARD_H
#define TEST_KEYBOARD_H
#include <stdint.h>
enum {KEYEV_NONE,KEYEV_DOWN,KEYEV_UP,KEYEV_HOLD,KEYEV_OSMENU,KEYEV_TOUCH_DOWN,KEYEV_TOUCH_DRAG,KEYEV_TOUCH_UP};
enum {KEY_EXIT,KEY_ACON,KEY_EXE,KEY_LEFT,KEY_RIGHT,KEY_7,KEY_9,KEY_SHIFT};
typedef struct {int type,key,x,y;} key_event_t;
key_event_t pollevent(void);
int keydown(int key);
key_event_t getkey(void);
#endif
