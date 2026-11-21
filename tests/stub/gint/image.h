#ifndef TEST_GINT_IMAGE_H
#define TEST_GINT_IMAGE_H
#include <stdint.h>
enum {IMAGE_P4_RGB565=6,IMAGE_P4_RGB565A=3};
typedef struct {
    uint8_t format,flags;
    int16_t color_count;
    uint16_t width,height;
    int stride;
    void *data;
    uint16_t *palette;
} image_t;
#endif
