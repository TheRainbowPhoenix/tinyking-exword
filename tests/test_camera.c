#include "camera.h"
#include "core.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    /* All 22 rooms scroll only once on a full down/up traversal. */
    for(int room=0;room<22;room++) {
        int camera=-1,changes=0;
        for(int local=0;local<TK_ROOM_HEIGHT;local++) {
            int next=tk_camera_choose(room*480+local,room,room,camera);
            assert(next==0 || next==160);
            if(camera>=0 && next!=camera)changes++;
            camera=next;
            assert(local>=camera &&
                   (local+TK_PH<=camera+320 || local+TK_PH>TK_ROOM_HEIGHT));
        }
        assert(changes==1);
        for(int local=479;local>=0;local--) {
            int next=tk_camera_choose(room*480+local,room,room,camera);
            assert(next==0 || next==160);
            assert(local>=next &&
                   (local+TK_PH<=next+320 || local+TK_PH>TK_ROOM_HEIGHT));
            camera=next;
        }
        assert(camera==0);
    }
    /* Bouncing in the 104px hysteresis band must not move the camera. */
    int camera=0,changes=0;
    for(int i=0;i<10000;i++) {
        int local=(i&1)?205:265;
        int next=tk_camera_choose(local,0,0,camera);
        changes+=(next!=camera);
        camera=next;
    }
    assert(changes==0);
    assert(tk_camera_choose(288,0,0,0)==160);
    assert(tk_camera_choose(287,0,0,0)==0);
    assert(tk_camera_choose(185,0,0,160)==160);
    assert(tk_camera_choose(184,0,0,160)==0);
    assert(tk_camera_choose(480,1,0,160)==0);
    assert(tk_camera_choose(479,0,1,0)==160);
    assert(tk_camera_choose(10453,21,-1,-1)==160);
    puts("fixed two-page camera and hysteresis: PASS");
    return 0;
}
