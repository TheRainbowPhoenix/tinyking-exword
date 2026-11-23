#include "camera.h"

int tk_camera_choose(int player_y, int room, int previous_room, int previous_camera)
{
    int local_y = player_y - room * TK_ROOM_HEIGHT;
    /* Room changes or respawns choose the closer page. Hysteresis is used
       only while remaining in the same room. */
    if(room != previous_room ||
       (previous_camera != TK_CAMERA_UPPER && previous_camera != TK_CAMERA_LOWER))
        return local_y >= (TK_CAMERA_SWITCH_DOWN + TK_CAMERA_SWITCH_UP) / 2
            ? TK_CAMERA_LOWER : TK_CAMERA_UPPER;
    if(previous_camera == TK_CAMERA_UPPER && local_y >= TK_CAMERA_SWITCH_DOWN)
        return TK_CAMERA_LOWER;
    if(previous_camera == TK_CAMERA_LOWER && local_y <= TK_CAMERA_SWITCH_UP)
        return TK_CAMERA_UPPER;
    return previous_camera;
}
