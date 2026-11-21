#ifndef TINYKING_PAK_H
#define TINYKING_PAK_H
#include <stdint.h>
#include <stdbool.h>
#include <gint/image.h>
#define TK_TOC_MAX 512
#define TK_BG_BYTES 76800
#define TK_MAX_ENTRY 4096

typedef struct {char name[16];uint8_t type;uint32_t offset,length;} tk_entry;
typedef struct {int fd,count;uint32_t size,end;tk_entry toc[TK_TOC_MAX];} tk_pak;
extern uint8_t tk_bg_pixels[TK_BG_BYTES];
extern uint16_t tk_bg_palette[16];
extern image_t tk_bg_image;
int tk_pak_open(tk_pak *pak,const char *file);
void tk_pak_close(tk_pak *pak);
const tk_entry *tk_pak_find(const tk_pak *pak,const char *name);
int tk_pak_read(tk_pak *pak,const char *name,void *buf,unsigned cap,int expected_type);
int tk_pak_load_bg(tk_pak *pak,int room);
/* Pack image: raw data and 16-color palette buffers must remain alive. */
int tk_pak_load_image(tk_pak *pak,const char *name,int w,int h,bool alpha,
                      image_t *dst,uint8_t *pixels,unsigned cap,uint16_t pal[16]);
#endif
