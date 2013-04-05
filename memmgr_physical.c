#include <stdint.h>
#include "memmgr_physical.h"

#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))

/*
 * Symbols provided by the linker
 */
extern uint8_t _b_start;        /* The start address of the bootstrap, and some of our data */
extern uint8_t _b_end;          /* End address of the bootstrap */
extern uint8_t _start_pa;       /* The physical start address of the kernel */
extern uint8_t _end_pa;         /* The physical end address of the kernel */

/*
 * The one and only instance of the physical memory manager
 */
static uint32_t initial_frames[INITIAL_FRAMES/sizeof(uint32_t)];
static memmgr_physical_t memmgr_p;

/*
 * Internal Function Declarations
 */
static void memmgr_mark_kernel_frames(void);
static void set_frame(uint32_t frame_addr);
static void clear_frame(uint32_t frame_addr);
static uint32_t test_frame(uint32_t frame_addr);
static uint32_t first_frame(void);
static void set_range(uint32_t start_addr, uint32_t count);


void memmgr_physical_init(void)
{
    memmgr_p.frames = initial_frames;
    memmgr_p.n_frames = INITIAL_FRAMES;

    memmgr_mark_kernel_frames();
}

static void memmgr_mark_kernel_frames(void)
{
    /* Mark the first megabyte for now */
    uint32_t count = (1024*1024) / PAGE_SIZE;
    set_range(0x0, count);

    /* Mark the region that contains the bootstrap code */
    uint32_t n_bootstrap = (uint32_t)(&_b_end - &_b_start);
    count = (n_bootstrap + PAGE_SIZE-1) / PAGE_SIZE; /* Int division rounding up */
    set_range((uint32_t)&_b_start, count);

    /* Mark the region that contains the kernel */
    uint32_t n_kernel = (uint32_t)(&_end_pa - &_start_pa);
    count = (n_kernel + PAGE_SIZE-1) / PAGE_SIZE;
    set_range((uint32_t)&_start_pa, count);
}

static void set_range(uint32_t start_addr, uint32_t count)
{
    for (uint32_t ii = 0; ii < count; ii++)
    {
        uint32_t frame_addr = start_addr + (ii * PAGE_SIZE);
        set_frame(frame_addr);
    }
}


/*
 * Bitset Implementation Taken from: http://www.jamesmolloy.co.uk/tutorial_html/6.-Paging.html
 */

// Static function to set a bit in the frames bitset
static void set_frame(uint32_t frame_addr)
{
    uint32_t frame = frame_addr/PAGE_SIZE;
    uint32_t idx = INDEX_FROM_BIT(frame);
    uint32_t off = OFFSET_FROM_BIT(frame);
    memmgr_p.frames[idx] |= (0x1 << off);
}

// Static function to clear a bit in the frames bitset
static void clear_frame(uint32_t frame_addr)
{
    uint32_t frame = frame_addr/PAGE_SIZE;
    uint32_t idx = INDEX_FROM_BIT(frame);
    uint32_t off = OFFSET_FROM_BIT(frame);
    memmgr_p.frames[idx] &= ~(0x1 << off);
}

// Static function to test if a bit is set.
static uint32_t test_frame(uint32_t frame_addr)
{
    uint32_t frame = frame_addr/PAGE_SIZE;
    uint32_t idx = INDEX_FROM_BIT(frame);
    uint32_t off = OFFSET_FROM_BIT(frame);
    return (memmgr_p.frames[idx] & (0x1 << off));
}

// Static function to find the first free frame.
static uint32_t first_frame()
{
    uint32_t i, j;
    for (i = 0; i < INDEX_FROM_BIT(memmgr_p.n_frames); i++)
    {
        if (memmgr_p.frames[i] != 0xFFFFFFFF) // nothing free, exit early.
        {
            // at least one bit is free here.
            for (j = 0; j < 32; j++)
            {
                uint32_t toTest = 0x1 << j;
                if ( !(memmgr_p.frames[i]&toTest) )
                {
                    return i*4*8+j;
                }
            }
        }
    }
    return 0; /* TODO: Don't really know what to do here */
} 
