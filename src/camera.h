#ifndef TINYKING_CAMERA_H
#define TINYKING_CAMERA_H
/* A 480px room has two overlapping, fixed 320px viewport pages. */
enum {
    TK_ROOM_HEIGHT = 480,
    TK_CAMERA_UPPER = 0,
    TK_CAMERA_LOWER = 160,
    TK_CAMERA_SWITCH_DOWN = 288,
    TK_CAMERA_SWITCH_UP = 184
};
/* player_y is the king's world-space pixel top; previous_camera=0 or 160. */
int tk_camera_choose(int player_y, int room, int previous_room, int previous_camera);
#endif
