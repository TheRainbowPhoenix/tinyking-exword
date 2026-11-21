#include "scene.h"
#include <string.h>
#include <stdio.h>
static unsigned le16(const uint8_t *p){return p[0]|((unsigned)p[1]<<8);}
static int signed16(const uint8_t *p){int v=le16(p);return v>=32768?v-65536:v;}
int tk_scene_load(tk_scene *s,tk_pak *pak,int room)
{
    uint8_t data[TK_MAX_COVERS*10];
    char name[16];
    snprintf(name,sizeof name,"SEC_%03d",room);
    int len=tk_pak_read(pak,name,data,sizeof data,0);
    if(len<0 || len%10)return -1;
    s->pak=pak;s->room=room;s->count=len/10;
    for(int i=0;i<s->count;i++){
        tk_cover *c=&s->covers[i];
        const uint8_t *e=data+i*10;
        c->x=le16(e);c->y=le16(e+2);c->w=le16(e+4);
        c->h=le16(e+6);c->serial=le16(e+8);
        if(c->x<0 || c->x+c->w>320 || c->y<0 || c->y+c->h>480 ||
           c->w%2 || c->w*c->h/2>TK_COVER_BYTES)return -1;
        snprintf(name,sizeof name,"HIDE_%02d",c->serial);
        if(tk_pak_load_image(pak,name,c->w,c->h,true,&c->image,
                             c->pixels,sizeof c->pixels,c->palette)<0)return -1;
        uint8_t hits[TK_MAX_HITS*8];
        snprintf(name,sizeof name,"HIT_%02d",c->serial);
        int bytes=tk_pak_read(pak,name,hits,sizeof hits,0);
        if(bytes<0 || bytes%8)return -1;
        c->hits_count=bytes/8;
        for(int j=0;j<c->hits_count;j++){
            const uint8_t *h=hits+j*8;
            c->hits[j]=(tk_rect){signed16(h),signed16(h+2),signed16(h+4),signed16(h+6)};
        }
    }
    return 0;
}
bool tk_cover_visible(const tk_cover *c,int px,int py)
{
    for(int i=0;i<c->hits_count;i++){
        tk_rect b=c->hits[i];
        if(px<b.x+b.w && px+TK_PW>b.x && py<b.y+b.h && py+TK_PH>b.y)return false;
    }
    return true;
}
int tk_speech_init(tk_speech *s,tk_pak *pak)
{
    uint8_t data[8];
    if(tk_pak_read(pak,"TALK_META",data,sizeof data,0)!=8)return -1;
    memset(s,0,sizeof *s);
    s->pak=pak;s->npc_x=le16(data);s->npc_y=le16(data+2);
    s->room=le16(data+4);s->count=le16(data+6);s->page=-1;
    return s->count>0 && s->count<=100?0:-1;
}
static int load_page(tk_speech *s,int page)
{
    char name[32];snprintf(name,sizeof name,"TALK_%d",page);
    return tk_pak_load_image(s->pak,name,256,64,true,&s->image,s->pixels,
                              sizeof s->pixels,s->palette);
}
int tk_speech_step(tk_speech *s,const tk_player *p,bool next)
{
    int px=p->x/TK_Q,py=p->y/TK_Q;
    bool near=!p->flying && p->crash<0 && p->grounded &&
              px+TK_PW/2>s->npc_x-100 && px+TK_PW/2<s->npc_x+100 &&
              py+TK_PH>s->npc_y-20 && py+TK_PH<s->npc_y+20;
    if(s->visible && !near){s->visible=false;s->seen=true;}
    else if(s->visible){
        s->timer++;
        if(next || s->timer>=120){
            s->page++;s->timer=0;
            if(s->page>=s->count){s->visible=false;s->seen=true;}
            else if(load_page(s,s->page)<0)return -1;
        }
    }else if(near){
        s->delay++;
        if(next || (!s->seen && s->delay>=30)){
            s->visible=true;s->page=0;s->timer=0;
            if(load_page(s,0)<0)return -1;
        }
    }else s->delay=0;
    return 0;
}
