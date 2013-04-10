#ifndef _MEMMGR_PHYSICAL_H_
#define _MEMMGR_PHYSICAL_H_ 1

#include <stdint.h>
#include "memmgr_virtual.h"

#define PAGE_SIZE (0x1000)
#define INITIAL_FRAMES (4096)

struct memmgr_physical
{
    uint32_t *frames;
    uintptr_t n_frames;
};
typedef struct memmgr_physical memmgr_physical_t;


void memmgr_physical_init(memmgr_physical_t *self, uintptr_t highest_addr);

/* Returns the number of bytes required to handle a map upto highest_addr */
uintptr_t memmgr_physical_size(memmgr_physical_t *self);

/* Set the position of frames and initialize it */
void memmgr_physical_set_frames(memmgr_physical_t *self, uint32_t *frames);

/* Scan for and mark the kernel frames as being in use */
void memmgr_set_from_page_directory(memmgr_physical_t *self, page_directory_t* page_directory);
#endif
