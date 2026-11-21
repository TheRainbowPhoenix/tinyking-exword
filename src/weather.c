#include "weather.h"
#include <stdio.h>
static int clamp(int n,int lo,int hi){return n<lo?lo:n>hi?hi:n;}
static int imin(int a,int b){return a<b?a:b;}
static int imax(int a,int b){return a>b?a:b;}
int tk_weather_init(tk_weather *w,tk_pak *p,int rooms)
{
    w->pak=p;w->rooms=rooms;w->screen=-1;w->phase=0;w->enabled=0;
    if(rooms>50)return -1;
    if(tk_pak_read(p,"WEATHER",w->settings,sizeof w->settings,0)!=rooms*2*4)return -1;
    if(tk_pak_read(p,"GUSTS",w->gusts,sizeof w->gusts,0)!=784)return -1;
    return 0;
}
int tk_weather_tick(tk_weather *w,const tk_player *p)
{
    int screen=clamp(w->rooms*2-1-(p->y/TK_Q)/240,0,w->rooms*2-1);
    if(screen!=w->screen){
        int old=w->screen;
        char name[16];snprintf(name,sizeof name,"AIR_%03d",screen);
        if(tk_pak_read(w->pak,name,w->mask,sizeof w->mask,0)!=676)return -1;
        w->screen=screen;
        if(!w->settings[screen*4])w->enabled=0;
        else if(p->grounded || old>screen)w->enabled=1;
    }else if(p->grounded && w->settings[screen*4])w->enabled=1;
    w->phase=(w->phase+1)%392;
    return 0;
}
static int marked(const tk_weather *w,int x,int y,int offset)
{
    int cx=clamp(((x+1)*3-1)/16,0,59),cy=clamp(((y+1)*3-1)/16,0,44);
    int bit=cy*60+cx;
    return (w->mask[offset+(bit>>3)] & (1<<(bit&7)))!=0;
}
static int wind_velocity(const tk_weather *w,int screen)
{
    int at=screen*4;
    if(!w->settings[at])return 0;
    int strength=w->settings[at+1],direction=w->settings[at+2];
    if(direction)return direction==255?-strength:strength;
    int index=w->phase*2;
    int gust=w->gusts[index]|(w->gusts[index+1]<<8);
    if(gust>=32768)gust-=65536;
    int product=gust*strength;
    return product>=0?(product+128)/256:-((-product+128)/256);
}
int tk_weather_force(const tk_weather *w,const tk_player *p)
{
    if(!w->enabled || p->flying || (p->crash>=0 && !p->ice))return 0;
    int x=p->x/TK_Q,y=(p->y/TK_Q)%240;
    int l=imax(0,((x+1)*3-1)/16),r=imin(59,((x+TK_PW)*3-1)/16);
    int top=imax(0,((y+1)*3-1)/16),bottom=imin(44,((y+TK_PH)*3-1)/16);
    for(int cy=top;cy<=bottom;cy++)for(int cx=l;cx<=r;cx++){
        int bit=cy*60+cx;
        if(w->mask[bit>>3]&(1<<(bit&7)))return 0;
    }
    if(p->grounded && (marked(w,x,imin(239,y+TK_PH),338) ||
                       marked(w,x+TK_PW-1,imin(239,y+TK_PH),338)))return 0;
    return wind_velocity(w,w->screen);
}
