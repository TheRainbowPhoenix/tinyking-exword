#ifndef TINYKING_WEATHER_H
#define TINYKING_WEATHER_H
#include "pak.h"
#include "world.h"
typedef struct {
    tk_pak *pak;
    int rooms,screen,phase,enabled;
    uint8_t settings[400],gusts[784],mask[676];
} tk_weather;
int tk_weather_init(tk_weather *w,tk_pak *p,int rooms);
int tk_weather_tick(tk_weather *w,const tk_player *p);
int tk_weather_force(const tk_weather *w,const tk_player *p);
#endif
