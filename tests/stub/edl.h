#ifndef TEST_EDL_H
#define TEST_EDL_H
#include <stdint.h>
#define EDL_ABI_MAGIC 0x45444c31u
#define EDL_METADATA __attribute__((used))
struct edl_metadata { uint32_t magic,version;char title[32],description[64];uint16_t icon[1024];};
#endif
