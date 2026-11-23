/* Regression test for native EX-word OS-world seek/read count. */
#include "pak.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

ssize_t __real_read(int fd, void *dst, size_t size);
off_t __real_lseek(int fd, off_t offset, int whence);
static unsigned reads, seeks, large_errors;
static int reject_large;

ssize_t __wrap_read(int fd, void *dst, size_t size)
{
    reads++;
    if(reject_large && size>4096){
        large_errors++;
        errno=EINVAL;
        return -1;
    }
    return __real_read(fd,dst,size);
}
off_t __wrap_lseek(int fd, off_t offset, int whence)
{
    seeks++;
    return __real_lseek(fd,offset,whence);
}
int main(int argc,char **argv)
{
    assert(argc==2);
    tk_pak pak;
    assert(tk_pak_open(&pak,argv[1])==0);
    assert(pak.count==140);
    assert(reads==5 && seeks==4); /* 2 small reads + 3 TOC batches */
    reads=seeks=0;
    assert(tk_pak_load_bg(&pak,21)==0);
    assert(seeks==1 && reads==6); /* header + five 16 KiB reads */
    reads=seeks=0;
    reject_large=1;
    assert(tk_pak_load_bg(&pak,20)==0);
    assert(large_errors==1 && seeks==1); /* fallback to 4 KiB */
    unsigned old_errors=large_errors;
    reads=seeks=0;
    assert(tk_pak_load_bg(&pak,19)==0);
    assert(large_errors==old_errors && seeks==1);
    tk_pak_close(&pak);
    puts("GMPK startup I/O batching and fallback: PASS");
    return 0;
}
