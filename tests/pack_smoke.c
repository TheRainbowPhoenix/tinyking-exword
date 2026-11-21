/* Host-only smoke: verify that every shipped data entry is readable. */
#include "pak.h"
#include "world.h"
#include "weather.h"
#include "scene.h"
#include <stdio.h>
#include <stdlib.h>

#define REQUIRE(cond) do{if(!(cond)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#cond);return 1;}}while(0)
int main(int argc,char **argv)
{
    REQUIRE(argc==2);
    tk_pak pack;tk_world *w=calloc(1,sizeof *w);
    tk_weather *weather=calloc(1,sizeof *weather);
    tk_scene *scene=calloc(1,sizeof *scene);
    tk_speech speech;
    REQUIRE(w&&weather&&scene);
    REQUIRE(tk_pak_open(&pack,argv[1])==0);
    REQUIRE(tk_world_init(w,&pack)==0);
    REQUIRE(w->rooms==22 && w->width==320);
    REQUIRE(tk_weather_init(weather,&pack,w->rooms)==0);
    REQUIRE(tk_speech_init(&speech,&pack)==0);
    uint8_t pixels[8192];uint16_t palette[16];image_t img;
    REQUIRE(tk_pak_load_image(&pack,"KING",48,17,true,&img,pixels,sizeof pixels,palette)==0);
    REQUIRE(tk_pak_load_image(&pack,"SPLAT",40,10,true,&img,pixels,sizeof pixels,palette)==0);
    REQUIRE(tk_pak_load_image(&pack,"HUD",320,48,false,&img,pixels,sizeof pixels,palette)==0);
    REQUIRE(tk_pak_load_image(&pack,"DIGITS",100,14,false,&img,pixels,sizeof pixels,palette)==0);
    REQUIRE(tk_pak_load_image(&pack,"FALL_DIGITS",100,16,false,&img,pixels,sizeof pixels,palette)==0);
    for(int i=0;i<speech.count;i++){
        char name[16];snprintf(name,sizeof name,"TALK_%d",i);
        REQUIRE(tk_pak_load_image(&pack,name,256,64,true,&img,pixels,sizeof pixels,palette)==0);
    }
    for(int r=0;r<w->rooms;r++){
        REQUIRE(tk_pak_load_bg(&pack,r)==0);
        REQUIRE(tk_world_update(w,(r*480+200)*256)==0);
        REQUIRE(w->all_count>0 && w->near_count>=0);
        REQUIRE(tk_scene_load(scene,&pack,r)==0);
        tk_player p;tk_player_init(&p,160,r*480+20);
        REQUIRE(tk_weather_tick(weather,&p)==0);
        (void)tk_weather_force(weather,&p);
    }
    /* Every upper/lower weather screen has its own mask. */
    for(int s=0;s<44;s++){
        tk_player p;tk_player_init(&p,160,(43-s)*240);
        REQUIRE(tk_weather_tick(weather,&p)==0);
    }
    printf("GMPK smoke PASS: %d rooms, %d assets, spawn (%d,%d)\n",
           w->rooms,pack.count,w->spawn_x,w->spawn_y);
    tk_pak_close(&pack);
    free(w);free(weather);free(scene);
    return 0;
}
