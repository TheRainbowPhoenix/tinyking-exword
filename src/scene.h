#ifndef TINYKING_SCENE_H
#define TINYKING_SCENE_H
#include "core.h"
#include "pak.h"
#define TK_MAX_COVERS 8
#define TK_COVER_BYTES 7000
#define TK_MAX_HITS 8
typedef struct {int x,y,w,h;} tk_rect;
typedef struct {
    int x,y,w,h,serial,hits_count;
    tk_rect hits[TK_MAX_HITS];
    image_t image;
    uint8_t pixels[TK_COVER_BYTES];
    uint16_t palette[16];
} tk_cover;
typedef struct {tk_pak *pak;int room,count;tk_cover covers[TK_MAX_COVERS];} tk_scene;
int tk_scene_load(tk_scene *s,tk_pak *pak,int room);
bool tk_cover_visible(const tk_cover *c,int px,int py);

typedef struct {
    tk_pak *pak;
    int npc_x,npc_y,room,count,page,timer,delay;
    bool visible,seen;
    image_t image;
    uint8_t pixels[8192];
    uint16_t palette[16];
} tk_speech;
int tk_speech_init(tk_speech *s,tk_pak *pak);
int tk_speech_step(tk_speech *s,const tk_player *p,bool next);
#endif
