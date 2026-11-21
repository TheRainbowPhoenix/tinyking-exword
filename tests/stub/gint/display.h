#ifndef TEST_DISPLAY_H
#define TEST_DISPLAY_H
#define DWIDTH 528
#define DHEIGHT 320
#define C_BLACK 0
#define C_WHITE 65535
#include "image.h"
void dclear(int color);
void dtext(int x,int y,int color,const char *text);
void drect(int x1,int y1,int x2,int y2,int color);
void dprint(int x,int y,int color,const char *format,...);
void dsubimage(int x,int y,const image_t *image,int sx,int sy,int sw,int sh);
void dupdate(void);
#endif
