#include "world.h"
#include <stdio.h>
static unsigned u16le(const uint8_t *p){return p[0]|((unsigned)p[1]<<8);}
static int s16le(const uint8_t *p){int n=u16le(p);return n>=32768?n-65536:n;}
static int clamp(int n,int lo,int hi){return n<lo?lo:n>hi?hi:n;}
int tk_world_init(tk_world *w,tk_pak *pak)
{
    uint8_t data[8];w->pak=pak;
    if(tk_pak_read(pak,"WORLD",data,sizeof data,0)!=8)return -1;
    w->width=u16le(data);w->rooms=u16le(data+2);
    w->spawn_x=u16le(data+4);w->spawn_y=u16le(data+6);
    if(w->width!=320 || w->rooms<1 || w->rooms>100 ||
       w->spawn_x>320-TK_PW || w->spawn_y>=w->rooms*480)return -1;
    w->height=w->rooms*480;
    w->room=w->band=-1;w->all_count=w->near_count=0;
    return 0;
}
int tk_world_update(tk_world *w,int y_q8)
{
    int y=y_q8/TK_Q;
    int room=clamp(y/480,0,w->rooms-1);
    int band=clamp(y/64,0,(w->height-1)/64);
    int room_changed=(room!=w->room);
    if(room_changed){
        /* The 1.8MB asset never lives in the ELF arena. Only nearby collisions. */
        w->all_count=0;
        for(int r=room>0?room-1:0;r<w->rooms && r<=room+1;r++){
            char name[16];uint8_t data[TK_MAX_ENTRY];
            snprintf(name,sizeof name,"COL_%03d",r);
            int bytes=tk_pak_read(w->pak,name,data,sizeof data,5);
            if(bytes<0 || bytes%9)return -1;
            for(int i=0;i<bytes;i+=9){
                int x=s16le(data+i), by=s16le(data+i+2);
                int wid=u16le(data+i+4), hei=u16le(data+i+6),kind=data[i+8];
                if(x<0 || x>=320 || by<0 || by>=480 || wid<1 || wid>320-x ||
                   hei<1 || hei>480-by || kind>6 || w->all_count>=TK_MAX_BOXES)return -1;
                w->all[w->all_count++]=(tk_box){x,by+r*480,wid,hei,kind};
            }
        }
        w->room=room;
    }
    if(room_changed || band!=w->band){
        int low=band*64-32,high=(band+1)*64+48;
        w->near_count=0;
        for(int i=0;i<w->all_count;i++){
            const tk_box b=w->all[i];
            if(b.y<high && b.y+b.h>low)w->near[w->near_count++]=b;
        }
        w->band=band;
    }
    return 0;
}
