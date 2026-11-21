/*
 * Tinyking-specific Edelweiss ELF loader integration test.
 *
 * Based on Edelweiss's tests/check_edl.c. Unlike its >128 KiB large-demo
 * fixture, TKING stores the 1.8 MB tower.gmpak beside (not inside) main.edl.
 * Still exercise the actual Edelweiss parser, zero-fill, symbol relocation,
 * load-at-another-arena behavior, and metadata.
 */
#include "elf.h"
#include "edl.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINK_START 0x8c700000u
#define LINK_END   0x8c7f0000u
#define ARENA_SIZE (LINK_END - LINK_START)
#define OTHER_START 0x8c800000u

static unsigned char image_memory[ARENA_SIZE];
static unsigned char original[ARENA_SIZE];
static uint32_t memory_start = LINK_START;

static int file_read(void *context, uint32_t at, void *out, size_t length)
{
    FILE *file = context;
    return fseek(file, (long)at, SEEK_SET) ||
                   fread(out, 1, length, file) != length
               ? -1
               : 0;
}

static int memory_write(void *context, uint32_t address, const void *data, size_t length)
{
    (void)context;
    assert(address >= memory_start);
    uint32_t at = address - memory_start;
    assert(at < ARENA_SIZE && length <= ARENA_SIZE - at);
    memcpy(image_memory + at, data, length);
    return 0;
}

static int resolve_symbol(void *context, const char *symbol, uint32_t *address)
{
    (void)context;
    uint32_t hash = 0;
    for (; *symbol; symbol++)
        hash = hash * 33 + (unsigned char)*symbol;
    *address = 0x70010000u + (hash & 0xfffu) * 4u;
    return 0;
}

static uint32_t read_be32(const void *value)
{
    /* .edl.meta comes straight from a big-endian SuperH ELF. The host
       runner is little-endian, and edl_elf_metadata() deliberately copies
       the section bytes without translating struct integer fields. */
    const unsigned char *p = value;
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
           (uint32_t)p[2] << 8 | (uint32_t)p[3];
}

static void check_file(const char *name)
{
    FILE *file = fopen(name, "rb");
    assert(file);
    assert(!fseek(file, 0, SEEK_END));
    long size = ftell(file);
    assert(size > 0 && size < 0x1000000L);

    struct edl_image loaded_image;
    memory_start = LINK_START;
    memset(image_memory, 0xa5, sizeof image_memory);
    assert(!edl_elf_load_linked(file_read, file, (uint32_t)size,
                               LINK_START, LINK_END, memory_start, memory_write,
                               NULL, resolve_symbol, NULL, &loaded_image));
    memcpy(original, image_memory, sizeof original);

    memory_start = OTHER_START;
    memset(image_memory, 0xa5, sizeof image_memory);
    assert(!edl_elf_load_linked(file_read, file, (uint32_t)size,
                               LINK_START, LINK_END, memory_start, memory_write,
                               NULL, resolve_symbol, NULL, &loaded_image));
    assert(loaded_image.entry >= memory_start &&
           loaded_image.entry < memory_start + ARENA_SIZE);

    unsigned relocated_pointers = 0;
    for (unsigned i = 0; i < ARENA_SIZE; i += 4)
    {
        uint32_t before = (uint32_t)original[i] << 24 |
                          (uint32_t)original[i + 1] << 16 |
                          (uint32_t)original[i + 2] << 8 | original[i + 3];
        uint32_t after = (uint32_t)image_memory[i] << 24 |
                         (uint32_t)image_memory[i + 1] << 16 |
                         (uint32_t)image_memory[i + 2] << 8 | image_memory[i + 3];
        if (before != after)
        {
            assert(before >= LINK_START && before <= LINK_END);
            assert(after == before + (OTHER_START - LINK_START));
            relocated_pointers++;
        }
    }
    assert(relocated_pointers > 10);

    unsigned loadable_bytes = 0;
    assert(loaded_image.count > 0);
    for (unsigned i = 0; i < loaded_image.count; i++)
    {
        const struct edl_segment *segment = &loaded_image.segments[i];
        loadable_bytes += segment->file_size;
        for (uint32_t j = segment->file_size; j < segment->memory_size; j++)
        {
            uint32_t at = segment->address - memory_start + j;
            assert(at < ARENA_SIZE && image_memory[at] == 0);
        }
    }
    /* The D01 bootstrap has a 128 KiB limit; the external EDL does not
       need to exceed it. The game data is the separate tower.gmpak. */
    assert(loadable_bytes > 0 && loadable_bytes <= ARENA_SIZE);

    struct edl_metadata meta;
    assert(!edl_elf_metadata(file_read, file, (uint32_t)size, &meta, sizeof meta));
    assert(read_be32(&meta.magic) == EDL_ABI_MAGIC &&
           read_be32(&meta.version) == EDL_ABI_VERSION);
    assert(!strcmp(meta.title, "TKING"));

    printf("%s: %u loadable bytes, %u relocated pointers, title %s: PASS\n",
           name, loadable_bytes, relocated_pointers, meta.title);
    assert(!fclose(file));
}

int main(int argc, char **argv)
{
    assert(argc > 1);
    for (int i = 1; i < argc; i++)
        check_file(argv[i]);
    return 0;
}
