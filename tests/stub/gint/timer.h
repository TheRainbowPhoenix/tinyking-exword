#ifndef TEST_TIMER_H
#define TEST_TIMER_H
#define TIMER_ANY 0
#define GINT_CALL_INC(p) 0
int timer_configure(int timer,int usec,int callback);
void timer_start(int timer);
void timer_stop(int timer);
#endif
