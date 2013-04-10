#include <stdint.h>
#include "util.h"
#include "memmgr_virtual.h"
#include "memmgr_physical.h"

#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))


/*
 * Internal Function Declarations
 */
static void set_frame(memmgr_physical_t *self, uintptr_t frame_addr);
static void clear_frame(memmgr_physical_t *self, uintptr_t frame_addr);
static uint32_t test_frame(memmgr_physical_t *self, uintptr_t frame_addr);
static uint32_t first_frame(memmgr_physical_t *self);


void memmgr_physical_init(memmgr_physical_t *self, uintptr_t highest_addr)
{
    self->n_frames = idivc(highest_addr, PAGE_SIZE);
}

uintptr_t memmgr_physical_size(memmgr_physical_t *self)
{
    return idivc(self->n_frames, sizeof(uint32_t)) * sizeof(uint32_t);
}

void memmgr_physical_set_frames(memmgr_physical_t *self, uint32_t *frames)
{
    self->frames = frames;

    /* Zero the memory */
    for (uintptr_t ii = 0; ii < idivc(self->n_frames, sizeof(uint32_t)); ii++)
    {
        frames[ii] = 0;
    }
}

/* Callback to map from page directory to used frames */
static int set_page_cb(void* data, uintptr_t dir_offset, uintptr_t page_offset, page_t* page)
{
    UNUSED(dir_offset);
    UNUSED(page_offset);
    memmgr_physical_t *self = (memmgr_physical_t*)data;

    uintptr_t frame_addr = page->frame * PAGE_SIZE;
    set_frame(self, frame_addr);
    return 0;
}

void memmgr_set_from_page_directory(memmgr_physical_t *self, page_directory_t* page_directory)
{
    page_directory_walk(page_directory, 0, set_page_cb, self);
}

void memmgr_physical_set_range(memmgr_physical_t *self, uintptr_t start_addr, uintptr_t count)
{
    /* TODO: Optimise this to set 32 bits at a time instead of 1 bit at a time */
    for (uint32_t ii = 0; ii < count; ii++)
    {
        uint32_t frame_addr = start_addr + (ii * PAGE_SIZE);
        set_frame(self, frame_addr);
    }
}


/*
 * Bitset Implementation Taken from: http://www.jamesmolloy.co.uk/tutorial_html/6.-Paging.html
 */

// Static function to set a bit in the frames bitset
static void set_frame(memmgr_physical_t *self, uintptr_t frame_addr)
{
    if (frame_addr > self->n_frames * PAGE_SIZE)
    {
        return; // Past the end of the array, ignored
    }

    uintptr_t frame = frame_addr/PAGE_SIZE;
    uintptr_t idx = INDEX_FROM_BIT(frame);
    uintptr_t off = OFFSET_FROM_BIT(frame);
    self->frames[idx] |= (0x1 << off);
}

// Static function to clear a bit in the frames bitset
static void clear_frame(memmgr_physical_t *self, uintptr_t frame_addr)
{
    if (frame_addr > self->n_frames * PAGE_SIZE)
    {
        return; // Past the end of the array, ignored
    }
    uintptr_t frame = frame_addr/PAGE_SIZE;
    uintptr_t idx = INDEX_FROM_BIT(frame);
    uintptr_t off = OFFSET_FROM_BIT(frame);
    self->frames[idx] &= ~(0x1 << off);
}

// Static function to test if a bit is set.
static uint32_t test_frame(memmgr_physical_t *self, uintptr_t frame_addr)
{
    if (frame_addr > self->n_frames * PAGE_SIZE)
    {
        return 1; // Past the end of the array, assume its used
    }
    uintptr_t frame = frame_addr/PAGE_SIZE;
    uintptr_t idx = INDEX_FROM_BIT(frame);
    uintptr_t off = OFFSET_FROM_BIT(frame);
    return (self->frames[idx] & (0x1 << off));
}

// Static function to find the first free frame.
static uint32_t first_frame(memmgr_physical_t *self)
{
    uintptr_t i, j;
    for (i = 0; i < INDEX_FROM_BIT(self->n_frames); i++)
    {
        if (self->frames[i] != 0xFFFFFFFF) // nothing free, exit early.
        {
            // at least one bit is free here.
            for (j = 0; j < 32; j++)
            {
                uintptr_t toTest = 0x1 << j;
                if ( !(self->frames[i]&toTest) )
                {
                    return i*4*8+j;
                }
            }
        }
    }
    return 0; /* TODO: Don't really know what to do here */
}
