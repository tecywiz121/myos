#ifndef _MEMMGR_PHYSICAL_H_
#define _MEMMGR_PHYSICAL_H_ 1

#include <stdint.h>

#define PAGE_SIZE (0x1000)
#define INITIAL_FRAMES (4096)

struct memmgr_physical
{
    uint32_t total_memory;
    uint32_t available_memory;

    uint32_t *frames;
    uint32_t n_frames;
};
typedef struct memmgr_physical memmgr_physical_t;


void memmgr_physical_init(void);

#endif
