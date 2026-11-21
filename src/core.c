/* Direct integer/Q8 translation of tinyking_py/core.py (30 Hz). */
#include "core.h"
#include <string.h>
static int imin(int a,int b){return a<b?a:b;}
static int imax(int a,int b){return a>b?a:b;}
static int iabs(int a){return a<0?-a:a;}
static int clamp(int n,int lo,int hi){return imax(lo,imin(hi,n));}
/* CPython's // rounds toward -infinity; C's / truncates toward zero. */
static int floordiv(int n,int d){int q=n/d,r=n%d;return q-(r<0);}
void tk_player_init(tk_player *p,int x,int y)
{
    memset(p,0,sizeof *p);
    p->x=x*TK_Q; p->y=y*TK_Q; p->facing=1; p->crash=-1;
}
void tk_step(tk_player *p,int keys,const tk_box *boxes,int count,int width,int wind)
{
    int dx=((keys&TK_RIGHT)!=0)-((keys&TK_LEFT)!=0);
    p->landed=p->bounced=false;
    bool down=p->crash>=0 && p->grounded;
    bool on_ice=p->ice && p->grounded;
    if(down){
        p->vy=p->charge=0;
        if(!on_ice) p->vx=0;
        if(p->crash>0) p->crash--;
        else if((keys&TK_RECOVER) && !(p->previous&TK_RECOVER)) p->crash=-1;
        if(!on_ice){p->previous=keys;return;}
    }
    if(p->grounded && !down){
        if(!on_ice) p->vx=dx*TK_WALK;
        if(dx) p->facing=dx;
        if((keys&TK_JUMP) && (p->charge || !(p->previous&TK_JUMP))){
            p->charge++;
            p->direction=dx;
            if(dx) p->facing=dx;
        }
        if(p->charge){
            if(!on_ice) p->vx=0;
            if(!(keys&TK_JUMP) || p->charge>=TK_CHARGE){
                p->vy=-imax(342,TK_JUMP_SPEED*p->charge/TK_CHARGE);
                p->vx=on_ice?clamp(p->vx+p->direction*TK_AIR,-TK_AIR,TK_AIR)
                             :p->direction*TK_AIR;
                p->charge=p->direction=0;
                p->grounded=false;
            }
        }
    }else p->charge=0;
    p->previous=keys;

    int x=p->x,y=p->y,target_x;
    if(on_ice && p->grounded){
        int total=0;
        int target=(p->charge || down)?0:dx*TK_WALK;
        int first_wind=floordiv(wind,2);
        for(int half=0;half<2;half++){
            if(target && (((target>0)!=(p->vx>0)) || iabs(target)>iabs(p->vx)))
                p->vx=p->vx<target?imin(target,p->vx+TK_ICE_PUSH):imax(target,p->vx-TK_ICE_PUSH);
            p->vx+=half==0?first_wind:wind-first_wind;
            total+=p->vx;
            p->vx=p->vx>0?imax(0,p->vx-TK_ICE_FRICTION):imin(0,p->vx+TK_ICE_FRICTION);
        }
        int movement=total>=0?total/2:-((-total)/2);
        target_x=x+movement;
    }else{
        p->vx+=wind;
        int correction=wind>=0?(wind+2)/4:-((-wind+2)/4);
        target_x=x+p->vx-correction;
    }
    int nx=target_x;
    for(int i=0;i<count;i++){
        const tk_box *b=&boxes[i];
        if(b->kind==1) continue;
        int bx=b->x*TK_Q,by=b->y*TK_Q;
        int br=bx+b->w*TK_Q,bb=by+b->h*TK_Q;
        if(y<bb && y+TK_PH*TK_Q>by){
            int left=bx,right=br;
            if(b->kind==2) left=br-floordiv((imin(bb,y+TK_PH*TK_Q)-by)*b->w,b->h);
            else if(b->kind==3) right=bx+floordiv((imin(bb,y+TK_PH*TK_Q)-by)*b->w,b->h);
            else if(b->kind==4) right=br-floordiv((imax(by,y)-by)*b->w,b->h);
            else if(b->kind==5) left=bx+floordiv((imax(by,y)-by)*b->w,b->h);
            if(target_x>x && x+TK_PW*TK_Q<=left && nx+TK_PW*TK_Q>left)
                nx=left-TK_PW*TK_Q;
            else if(target_x<x && x>=right && nx<right)
                nx=right;
        }
    }
    nx=clamp(nx,0,(width-TK_PW)*TK_Q);
    if(nx!=target_x){
        p->vx=p->grounded?0:-floordiv(p->vx,2);
        p->bounced=true;
    }
    p->x=nx;
    int vy=p->vy?p->vy:TK_GRAVITY;
    int ny=y+vy;
    p->grounded=false;
    int floor_kind=0;
    bool floor_ice=false;
    for(int i=0;i<count;i++){
        const tk_box *b=&boxes[i];
        int bx=b->x*TK_Q,by=b->y*TK_Q;
        int br=bx+b->w*TK_Q,bb=by+b->h*TK_Q;
        if(nx<br && nx+TK_PW*TK_Q>bx){
            int floor=by,ceiling=bb;
            if(b->kind==2) floor=bb-floordiv((imin(br,nx+TK_PW*TK_Q)-bx)*b->h,b->w);
            else if(b->kind==3) floor=by+floordiv((imax(bx,nx)-bx)*b->h,b->w);
            else if(b->kind==4) ceiling=bb-floordiv((imax(bx,nx)-bx)*b->h,b->w);
            else if(b->kind==5) ceiling=by+floordiv((imin(br,nx+TK_PW*TK_Q)-bx)*b->h,b->w);
            if(vy>0 && y+TK_PH*TK_Q<=floor && ny+TK_PH*TK_Q>=floor){
                floor_ice=ny==floor-TK_PH*TK_Q?(floor_ice || b->kind==6):b->kind==6;
                ny=floor-TK_PH*TK_Q;
                p->grounded=true;
                floor_kind=b->kind;
            }else if(vy<0 && b->kind!=1 && y>=ceiling && ny<ceiling)
                ny=ceiling;
        }
    }
    ny=imax(0,ny);
    p->y=ny;
    p->ice=p->grounded && floor_ice;
    if(p->grounded && (floor_kind==2 || floor_kind==3)){
        int tangent=floordiv(floor_kind==2?vy-p->vx:vy+p->vx,2);
        tangent=clamp(tangent,-2560,2560);
        p->vx=floor_kind==2?-tangent:tangent;
        p->vy=imin(TK_MAX_FALL,tangent+TK_GRAVITY);
        p->grounded=p->landed=false;
        p->charge=0;
        return;
    }
    if(p->grounded){
        p->landed=p->vy>0;
        if(p->landed && vy>=TK_MAX_FALL){
            p->falls++;
            p->crash=TK_SPLAT_TICKS;
            p->charge=p->direction=0;
        }
        if(!p->ice) p->vx=0;
        else if(!on_ice)
            p->vx=p->vx>0?imax(0,p->vx-2*TK_ICE_FRICTION):imin(0,p->vx+2*TK_ICE_FRICTION);
        p->vy=0;
    }else if(ny!=y+vy) p->vy=TK_GRAVITY;
    else p->vy=imin(vy+TK_GRAVITY,TK_MAX_FALL);
}
