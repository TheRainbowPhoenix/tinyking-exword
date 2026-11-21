/* Ordinary gint application: no edl_main() or custom drawing/input API. */
#include <gint/display.h>
#include <gint/image.h>
#include <gint/keyboard.h>
#include <gint/timer.h>
#include <gint/rtc.h>
#include <gint/cpu.h>
#include <math.h>
#include <stdlib.h>
#include "edl.h"
#include "ebrew_assets.h"

static const image_t wallpaper = {.format = IMAGE_RGB565,
                                  .flags = IMAGE_FLAGS_DATA_RO,
                                  .width = 528,
                                  .height = 160,
                                  .stride = 528 * 2,
                                  .data = (void *)edl_wallpaper};
static volatile int ready;
int main(void)
{
    unsigned char *scratch = malloc(8192);
    if (!scratch)
        return -1;
    for (unsigned i = 0; i < 8192; i++)
        scratch[i] = (unsigned char)i;
    int timer = timer_configure(TIMER_ANY, 16667, GINT_CALL_SET(&ready));
    if (timer >= 0)
        timer_start(timer);
    unsigned frame = 0;
    int x = DWIDTH / 2;
    uint32_t last = rtc_ticks();
    for (;;)
    {
        key_event_t event = pollevent();
        if (event.type == KEYEV_DOWN && (event.key == KEY_EXIT || event.key == KEY_ACON))
            break;
        if (event.type == KEYEV_DOWN && event.key == KEY_EXE)
            exit(0);
        uint32_t now = rtc_ticks();
        if (!ready && now == last)
        {
            sleep();
            continue;
        }
        ready = 0;
        last = now;
        if (keydown(KEY_LEFT) && x > 16)
            x -= 3;
        if (keydown(KEY_RIGHT) && x < DWIDTH - 16)
            x += 3;
        int y = 90 + (int)(45 * sin(frame * 0.08));
        dclear(C_WHITE);
        dimage(0, 20, &wallpaper);
        dcircle(x, y, 12, 0x3d9b, C_NONE);
        drect(0, 0, DWIDTH - 1, 19, 0x3d9b);
        dtext(12, 5, C_WHITE, "Standard gint inside Edelweiss");
        dprint(20, 200, C_BLACK, "Frame %u - %dx%d - heap check %u", frame, DWIDTH, DHEIGHT,
               scratch[255]);
        dtext(20, 224, C_BLACK, "dimage / dcircle / dprint / timer / malloc / sin");
        dtext(20, 248, C_BLACK, "Left/right: move. EXIT: return. EXE: exit(0).");
        dtext(20, 278, C_BLACK, "The wallpaper alone is 168,960 loaded bytes.");
        dupdate();
        frame++;
    }
    if (timer >= 0)
        timer_stop(timer);
    free(scratch);
    return 0;
}
EDL_METADATA const struct edl_metadata edl_meta = {
    .magic = EDL_ABI_MAGIC,
    .version = 1,
    .title = "gint ELF example",
    .description = "Standard gint renderer, input, timers and heap",
    .icon = {EDL_DEMO_ICON}};
