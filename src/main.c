/* Tinyking for EX-word / Edelweiss. Logical display: 528x320.
   The original 320-pixel-wide map occupies the central square; the otherwise
   unused margins carry the HUD. No Python/MicroPython runtime is involved. */
#include "core.h"
#include "pak.h"
#include "world.h"
#include "weather.h"
#include "scene.h"
#include "touch.h"
#include "icon.h"
#include "dirty.h"
#include "camera.h"
#include "edl.h"
#include <gint/display.h>
#include <gint/drivers/r61523.h>
#include <gint/image.h>
#include <gint/keyboard.h>
#include <gint/timer.h>
#include <gint/cpu.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { MAP_X=TK_MAP_X, MAP_W=TK_MAP_W, MAP_H=TK_MAP_H };
enum { LEFT_BG=0x1947, RIGHT_BG=0x1105, FRAME=0xd5ac,
       TEXT=0xffff, DIM=0xb5b6, CHARGE=0xf4c5, READY=0x6d28, DOWN=0xd986 };
static tk_pak pack;
static tk_world world;
static tk_weather weather;
static tk_scene scene;
static tk_speech speech;
static tk_player player;
static tk_touch touch;
static uint8_t king_pixels[TK_PW*4*TK_PH/2],splat_pixels[TK_CRASH_W*2*TK_CRASH_H/2];
static uint16_t king_palette[16],splat_palette[16];
static image_t king,splat;
static volatile int frame_ready;
static int old_room=-1,old_camera=-1,old_x=-999,old_y=-999,old_pose=-1;
static int old_mask=-1,old_page=-1,old_speech=-1;
static int hud_charge=-1,hud_falls=-1,hud_height=-1,hud_status=-1,hud_wind=99;
static bool display_ready;
static int hud_base;

static int imax(int a,int b){return a>b?a:b;}
static int imin(int a,int b){return a<b?a:b;}
static int clamp(int n,int lo,int hi){return imax(lo,imin(hi,n));}
static unsigned le16(const uint8_t *p){return p[0]|((unsigned)p[1]<<8);}

static void show_error(const char *message)
{
    dclear(C_WHITE);
    dtext(20,25,C_BLACK,"Tinyking EX-word");
    dtext(20,55,C_BLACK,message);
    dtext(20,95,C_BLACK,"Check tower.pak beside main.edl");
    dtext(20,125,C_BLACK,"Press any key to return.");
    dupdate();
    getkey();
}
static int init_game(void)
{
    if(DWIDTH!=528 || DHEIGHT!=320)return -2;
    if(tk_pak_open(&pack,"tower.pak")<0)return -1;
    if(tk_world_init(&world,&pack)<0 || tk_weather_init(&weather,&pack,world.rooms)<0 ||
       tk_speech_init(&speech,&pack)<0)return -3;
    uint8_t meta[2];
    if(tk_pak_read(&pack,"HUD_META",meta,sizeof meta,0)!=2)return -3;
    hud_base=le16(meta);
    if(tk_pak_load_image(&pack,"KING",TK_PW*4,TK_PH,true,
                          &king,king_pixels,sizeof king_pixels,king_palette)<0 ||
       tk_pak_load_image(&pack,"SPLAT",TK_CRASH_W*2,TK_CRASH_H,true,
                          &splat,splat_pixels,sizeof splat_pixels,splat_palette)<0)return -3;
    tk_player_init(&player,world.spawn_x,world.spawn_y);
    if(tk_world_update(&world,player.y)<0)return -3;
    return 0;
}
/* Render P4 background in <=160-line strips. The EX-word blitter truncates
   height >255, and source/target may not have the same stride. */
static void draw_background(int cam)
{
    dsubimage(MAP_X,0,&tk_bg_image,0,cam,MAP_W,160,DIMAGE_NONE);
    dsubimage(MAP_X,160,&tk_bg_image,0,cam+160,MAP_W,160,DIMAGE_NONE);
}
static void draw_king(int cam)
{
    int x=player.x/TK_Q,y=player.y/TK_Q-old_room*480;
    int pose=(player.charge?2:0)+(player.facing<0?1:0);
    int w=TK_PW,h=TK_PH,sx=pose*TK_PW;
    image_t *image=&king;
    if(player.crash>=0){
        image=&splat;w=TK_CRASH_W;h=TK_CRASH_H;
        x+=(TK_PW-TK_CRASH_W)/2;
        y+=TK_PH-TK_CRASH_H;
        sx=(player.facing<0)?TK_CRASH_W:0;
    }
    int sy=0;
    if(x<0){sx-=x;w+=x;x=0;}
    if(x+w>320)w=320-x;
    if(y<cam){sy=cam-y;h-=sy;y=cam;}
    if(y+h>cam+MAP_H)h=cam+MAP_H-y;
    if(w>0 && h>0)dsubimage(MAP_X+x,y-cam,image,sx,sy,w,h,DIMAGE_NONE);
}
static int cover_mask(void)
{
    int mask=0,px=player.x/TK_Q,py=player.y/TK_Q-scene.room*480;
    for(int i=0;i<scene.count;i++)
        if(tk_cover_visible(&scene.covers[i],px,py))mask|=1<<i;
    return mask;
}
static void draw_covers(int cam,int mask)
{
    for(int i=0;i<scene.count;i++){
        if(!(mask&(1<<i)))continue;
        const tk_cover *c=&scene.covers[i];
        int top=imax(cam,c->y),bottom=imin(cam+MAP_H,c->y+c->h);
        if(bottom>top)
            dsubimage(MAP_X+c->x,top-cam,&c->image,0,top-c->y,c->w,bottom-top,DIMAGE_NONE);
    }
}
static void draw_speech(int cam)
{
    if(!speech.visible || speech.room!=old_room)return;
    const int top=370;
    int a=imax(cam,top),b=imin(cam+MAP_H,top+64);
    if(b>a)dsubimage(MAP_X+32,a-cam,&speech.image,0,a-top,256,b-a,DIMAGE_NONE);
}
/* Only the fields that change are repainted; the HUD background, labels and
   border stay in VRAM.  Partial LCD sends are much cheaper than dupdate(). */
static void draw_hud_static(void)
{
    drect(0,0,MAP_X-1,319,LEFT_BG);
    drect(MAP_X+MAP_W,0,527,319,RIGHT_BG);
    drect(MAP_X-2,0,MAP_X-1,319,FRAME);
    drect(MAP_X+MAP_W,0,MAP_X+MAP_W+1,319,FRAME);
    dtext(22,12,TEXT,"TINY");
    dtext(22,29,CHARGE,"KING");
    drect(12,52,91,53,FRAME);
    dtext(22,63,DIM,"CHARGE");
    dtext(14,278,DIM,"SHIFT:JUMP");
    dtext(14,294,DIM,"EXIT:QUIT");
    dtext(442,14,TEXT,"HEIGHT");
    dtext(450,75,TEXT,"FALLS");
    drect(436,132,514,133,FRAME);
    dtext(448,145,DIM,"AIM:");
    dtext(435,165,TEXT,"LEFT/RIGHT");
    dtext(448,192,DIM,"WIND");
    drect(436,241,514,242,FRAME);
    dtext(442,258,DIM,"EXE:RISE");
    dtext(438,283,DIM,"30 FPS Q8");
}
static void draw_hud_dynamic(tk_damage *dirty,int elevation,int status,int wind)
{
    if(!display_ready)draw_hud_static();
    if(hud_charge != player.charge){
        int lo=hud_charge<0?0:imin(hud_charge,player.charge);
        int hi=hud_charge<0?TK_CHARGE:imax(hud_charge,player.charge);
        for(int i=lo;i<hi;i++){
            int y=86+(TK_CHARGE-1-i)*8;
            drect(24,y,76,y+5,i<player.charge?CHARGE:0x4269);
        }
        if(lo<hi)tk_damage_add(dirty,(tk_area){24,86+(TK_CHARGE-hi)*8,76,
                                               86+(TK_CHARGE-1-lo)*8+5});
        hud_charge=player.charge;
    }
    if(hud_status!=status){
        drect(12,245,100,265,LEFT_BG);
        dtext(18,247,TEXT,status==2?"CRASHED":status==1?"CHARGING":"READY");
        tk_damage_add(dirty,(tk_area){12,245,100,265});
        hud_status=status;
    }
    if(hud_height!=elevation){
        drect(434,32,522,53,RIGHT_BG);
        dprint(437,35,CHARGE,"%05d",elevation);
        tk_damage_add(dirty,(tk_area){434,32,522,53});
        hud_height=elevation;
    }
    int falls=imin(99999,player.falls);
    if(hud_falls!=falls){
        drect(434,93,522,116,RIGHT_BG);
        dprint(437,96,TEXT,"%05d",falls);
        tk_damage_add(dirty,(tk_area){434,93,522,116});
        hud_falls=falls;
    }
    int direction=wind<0?-1:wind>0?1:0;
    if(hud_wind!=direction){
        drect(434,209,521,231,RIGHT_BG);
        dtext(437,211,TEXT,direction>0?"  > >  ":direction<0?"  < <  ":"  calm");
        tk_damage_add(dirty,(tk_area){434,209,521,231});
        hud_wind=direction;
    }
}

/* Restore just the union of old/new sprites while the camera is fixed. gint
   clips overlays and sprites to that rectangle; only one P4 background sub-
   image is decoded, rather than the entire 320x320 room. */
static void redraw_sprite_area(int cam,int mask,tk_area box)
{
    tk_area bounds={MAP_X,0,MAP_X+MAP_W-1,MAP_H-1};
    if(!tk_area_clip(&box,bounds))return;
    struct dwindow previous=dwindow_set((struct dwindow){
        box.x1,box.y1,box.x2+1,box.y2+1});
    dsubimage(box.x1,box.y1,&tk_bg_image,box.x1-MAP_X,cam+box.y1,
              box.x2-box.x1+1,box.y2-box.y1+1,DIMAGE_NONE);
    draw_king(cam);
    draw_covers(cam,mask);
    draw_speech(cam);
    dwindow_set(previous);
}

static void send_damage(const tk_damage *dirty)
{
    if(!display_ready){
        /* The first frame fills all pixels anyway. One LCD transfer only. */
        dupdate();
        display_ready=true;
        return;
    }
    for(int i=0;i<dirty->count;i++){
        tk_area b=dirty->boxes[i];
        r61523_display_rect(gint_vram,b.x1,b.x2,b.y1,b.y2);
    }
}

static int draw_game(void)
{
    int y=player.y/TK_Q;
    int room=clamp(y/480,0,world.rooms-1);
    bool room_changed=(room!=old_room);
    int previous_room=old_room;
    if(room_changed){
        if(tk_pak_load_bg(&pack,room)<0 || tk_scene_load(&scene,&pack,room)<0)return -1;
        old_room=room;
    }
    /* Two fixed views per room: a full map repaint only on page transition. */
    int cam=tk_camera_choose(y,room,previous_room,old_camera);
    int pose=player.crash>=0?4+(player.facing<0):
             (player.charge?2:0)+(player.facing<0);
    int mask=cover_mask();
    int status=player.crash>=0?2:(player.charge?1:0);
    int elevation=clamp((hud_base-y-TK_PH)*3/2,0,99999);
    bool camera_dirty=room_changed || cam!=old_camera;
    bool overlay_dirty=old_mask!=mask || old_speech!=speech.visible ||
                       (speech.visible && old_page!=speech.page);
    bool sprite_dirty=old_pose!=pose || old_x/TK_Q!=player.x/TK_Q ||
                      old_y/TK_Q!=player.y/TK_Q;
    tk_damage dirty={0};
    if(camera_dirty || overlay_dirty){
        draw_background(cam);
        draw_king(cam);
        draw_covers(cam,mask);
        draw_speech(cam);
        tk_damage_add(&dirty,(tk_area){MAP_X,0,MAP_X+MAP_W-1,MAP_H-1});
    }else if(sprite_dirty){
        tk_area old_box=tk_sprite_area(old_x,old_y,old_pose,old_room,cam);
        tk_area new_box=tk_sprite_area(player.x,player.y,pose,room,cam);
        tk_area changed=tk_area_union(old_box,new_box);
        tk_area map={MAP_X,0,MAP_X+MAP_W-1,MAP_H-1};
        if(tk_area_clip(&changed,map)){
            redraw_sprite_area(cam,mask,changed);
            tk_damage_add(&dirty,changed);
        }
    }
    old_camera=cam;old_x=player.x;old_y=player.y;
    old_pose=pose;old_mask=mask;old_speech=speech.visible;old_page=speech.page;
    draw_hud_dynamic(&dirty,elevation,status,tk_weather_force(&weather,&player));
    if(dirty.count || !display_ready)send_damage(&dirty);
    return 0;
}
static int game_loop(void)
{
    int timer=timer_configure(TIMER_ANY,33333,GINT_CALL_INC(&frame_ready));
    if(timer<0)return -1;
    timer_start(timer);
    bool exe_was_down=false,suppress_touch=false;
    int rc=0;
    for(;;){
        /* Preserve touch events while synchronizing the held keyboard state. */
        for(int i=0;i<32;i++){
            key_event_t e=pollevent();
            if(e.type==KEYEV_NONE)break;
            if(e.type==KEYEV_TOUCH_DOWN || e.type==KEYEV_TOUCH_DRAG)
                tk_touch_contact(&touch,e.x,e.y);
            else if(e.type==KEYEV_TOUCH_UP)tk_touch_lift(&touch);
            else if(e.type==KEYEV_DOWN && (e.key==KEY_EXIT || e.key==KEY_ACON))
                goto done;
        }
        if(!frame_ready){sleep();continue;}
        int ticks=frame_ready;
        frame_ready=0;
        if(ticks>4)ticks=4; /* Capped catch-up, as in the Python main loop. */
        bool was_touch=touch.active;
        int touch_keys=tk_touch_tick(&touch,player.x/TK_Q+TK_PW/2);
        bool exe=keydown(KEY_EXE);
        bool advance=(touch.active && !was_touch) || (exe && !exe_was_down);
        exe_was_down=exe;
        if(advance && speech.visible && touch.active)suppress_touch=true;
        if(suppress_touch){
            if(touch_keys&TK_JUMP)touch_keys&=~TK_JUMP;
            else suppress_touch=false;
        }
        int keys=touch_keys |
                 ((keydown(KEY_LEFT)||keydown(KEY_7))?TK_LEFT:0) |
                 ((keydown(KEY_RIGHT)||keydown(KEY_9))?TK_RIGHT:0) |
                 (keydown(KEY_SHIFT)?TK_JUMP:0) |
                 (advance?TK_RECOVER:0);
        for(int t=0;t<ticks;t++){
            if(tk_world_update(&world,player.y)<0 || tk_weather_tick(&weather,&player)<0){
                rc=-1;goto done;
            }
            tk_step(&player,keys,world.near,world.near_count,world.width,
                    tk_weather_force(&weather,&player));
            if(player.y>world.height*TK_Q){
                int falls=player.falls+1;
                tk_player_init(&player,world.spawn_x,world.spawn_y);
                player.falls=falls;
            }
        }
        if(tk_speech_step(&speech,&player,advance && player.crash<0)<0 || draw_game()<0){rc=-1;break;}
    }
done:
    timer_stop(timer);
    return rc;
}
int main(void)
{
    int init=init_game();
    if(init<0){
        show_error(init==-1?"Cannot open tower.pak":
                   init==-2?"Unsupported screen dimensions":"Invalid tower data");
        tk_pak_close(&pack);
        return init;
    }
    if(draw_game()<0){show_error("Cannot load first room");tk_pak_close(&pack);return -1;}
    int result=game_loop();
    if(result<0)show_error("Stream or rendering error");
    tk_pak_close(&pack);
    return result;
}
EDL_METADATA const struct edl_metadata edl_meta={
    .magic=EDL_ABI_MAGIC,.version=1,.title="TKING",
    .description="Jump up the tower on EX-word",.icon={TINYKING_ICON_PIXELS}
};
