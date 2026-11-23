#include "pak.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

uint8_t tk_bg_pixels[TK_BG_BYTES];
uint16_t tk_bg_palette[16];
image_t tk_bg_image={.format=IMAGE_P4_RGB565,.flags=0,.color_count=16,
                     .width=320,.height=480,.stride=160,
                     .data=tk_bg_pixels,.palette=tk_bg_palette};
static unsigned u16le(const uint8_t *p){return p[0]|((unsigned)p[1]<<8);}
static uint32_t u32le(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
/* Native reads cross the EX-word OS boundary. Larger sequential requests
   cut expensive round trips when streaming a 76.8 KiB background. If a
   firmware rejects them, retain the proven 4 KiB request size thereafter. */
static unsigned tk_read_limit=16384;
static int read_exact(int fd,void *v,unsigned size){
    uint8_t *p=v;
    while(size){
        unsigned request=size>tk_read_limit?tk_read_limit:size;
        int n=read(fd,p,request);
        if(n<=0 && request>4096){
            tk_read_limit=4096;
            continue;
        }
        if(n<=0 || (unsigned)n>request)return -1;
        p+=n;size-=n;
    }
    return 0;
}
static int seek_read(tk_pak *pak,uint32_t offset,void *out,unsigned size){
    if(offset>pak->size || size>pak->size-offset || lseek(pak->fd,(off_t)offset,SEEK_SET)<0)return -1;
    return read_exact(pak->fd,out,size);
}
int tk_pak_open(tk_pak *pak,const char *filename){
    memset(pak,0,sizeof *pak);pak->fd=-1;
    pak->fd=open(filename,O_RDONLY);
    if(pak->fd<0)return -1;
    off_t size=lseek(pak->fd,0,SEEK_END);
    if(size<14 || size>0x7fffffffL)goto bad;
    pak->size=(uint32_t)size;
    uint8_t hdr[10];
    if(seek_read(pak,0,hdr,10)<0 || memcmp(hdr,"GMPK",4) || u16le(hdr+4)!=1)goto bad;
    pak->end=u32le(hdr+6);
    if(pak->end<10 || pak->end>pak->size-4)goto bad;
    uint8_t count[4];
    if(seek_read(pak,pak->end,count,4)<0)goto bad;
    uint32_t n=u32le(count);
    if(n>TK_TOC_MAX || pak->end+4+n*25!=pak->size)goto bad;
    pak->count=(int)n;
    /* A 140-entry tower TOC is just three 64-entry reads and one seek. */
    if(lseek(pak->fd,(off_t)(pak->end+4),SEEK_SET)<0)goto bad;
    for(unsigned i=0;i<n;){
        uint8_t rows[64*25];
        unsigned batch=n-i>64?64:n-i;
        if(read_exact(pak->fd,rows,batch*25)<0)goto bad;
        for(unsigned j=0;j<batch;j++){
            const uint8_t *row=rows+j*25;
            tk_entry *e=&pak->toc[i+j];
            memcpy(e->name,row,16);e->type=row[16];
            e->offset=u32le(row+17);e->length=u32le(row+21);
            if(e->offset<10 || e->offset>pak->end ||
               e->length>pak->end-e->offset)goto bad;
        }
        i+=batch;
    }
    return 0;
bad:tk_pak_close(pak);return -1;
}
void tk_pak_close(tk_pak *pak){if(pak->fd>=0)close(pak->fd);pak->fd=-1;}
const tk_entry *tk_pak_find(const tk_pak *pak,const char *name){
    char name16[16]={0};size_t n=strlen(name);
    if(n>16)return NULL;
    memcpy(name16,name,n);
    for(int i=0;i<pak->count;i++)if(!memcmp(name16,pak->toc[i].name,16))return &pak->toc[i];
    return NULL;
}
int tk_pak_read(tk_pak *pak,const char *name,void *buf,unsigned cap,int type){
    const tk_entry *e=tk_pak_find(pak,name);
    if(!e || e->length>cap || (type>=0 && e->type!=type))return -1;
    if(seek_read(pak,e->offset,buf,e->length)<0)return -1;
    return (int)e->length;
}
static int load_img(tk_pak *pak,const char *name,int w,int h,bool alpha,
                    image_t *dst,uint8_t *pixels,unsigned cap,uint16_t pal[16]){
    const tk_entry *e=tk_pak_find(pak,name);
    if(!e || e->type!=4 || e->length!=14+32+(unsigned)w*h/2 || (unsigned)w*h/2>cap)return -1;
    uint8_t header[46];
    if(lseek(pak->fd,(off_t)e->offset,SEEK_SET)<0 ||
       read_exact(pak->fd,header,sizeof header)<0)return -1;
    if(header[0]!=(alpha?6:5) || u16le(header+1)!=(unsigned)w || u16le(header+3)!=(unsigned)h ||
       u16le(header+5)!=(unsigned)w/2 || header[7]!=16 || u16le(header+8)!=32 ||
       u32le(header+10)!=(unsigned)w*h/2)return -1;
    /* SH is big-endian; source palette is native SuperH RGB565. */
    for(int i=0;i<16;i++)pal[i]=(header[14+2*i]<<8)|header[15+2*i];
    /* Pixel data follows the header: no second native seek is needed. */
    if(read_exact(pak->fd,pixels,(unsigned)w*h/2)<0)return -1;
    *dst=(image_t){.format=alpha?IMAGE_P4_RGB565A:IMAGE_P4_RGB565,.flags=0,
                    .color_count=16,.width=w,.height=h,.stride=w/2,
                    .data=pixels,.palette=pal};
    return 0;
}
int tk_pak_load_image(tk_pak *pak,const char *name,int w,int h,bool alpha,
                      image_t *dst,uint8_t *pixels,unsigned cap,uint16_t pal[16]){
    return load_img(pak,name,w,h,alpha,dst,pixels,cap,pal);
}
int tk_pak_load_bg(tk_pak *pak,int room){
    char name[16];snprintf(name,sizeof name,"BG_%03d",room);
    return load_img(pak,name,320,480,false,&tk_bg_image,tk_bg_pixels,sizeof tk_bg_pixels,tk_bg_palette);
}
