#include <stdint.h>
#include <stdbool.h>
#include "memmgr_physical.h"
#include "memmgr_virtual.h"
#include "memmgr_dumb.h"
#include "util.h"

static uintptr_t advance_free_page(memmgr_dumb_t *memmgr_dumb, uintptr_t n_pages);
static uintptr_t get_frame(memmgr_dumb_t *memmgr_dumb);
static void map_frame_to_page(memmgr_dumb_t *memmgr_dumb, uintptr_t page);

/* Very stupid allocator for allocating structures used in the smarter allocators */

void dumb_init(memmgr_dumb_t *memmgr_dumb, page_directory_t *page_directory)
{
    memmgr_dumb->page_directory = page_directory;

    /* Find the first free page after the kernel */
    memmgr_dumb->next_free_page = (uintptr_t)&KERNEL_BASE / PAGE_SIZE;
    advance_free_page(memmgr_dumb, 1);

    /* Find the first free frame after the end of the kernel */
    memmgr_dumb->next_free_frame = idivc((uintptr_t)&_end_pa, PAGE_SIZE);
}

void *dumb_alloc(memmgr_dumb_t *memmgr_dumb, uintptr_t size)
{
    uintptr_t n_pages = idivc(size, PAGE_SIZE);
    uintptr_t free_page = advance_free_page(memmgr_dumb, n_pages);

    if (free_page == -1u)
    {
        /* Couldn't allocate that much memory! */
        return (void*)0;
    }

    uintptr_t page_num = free_page;
    for (uintptr_t ii = 0; ii < n_pages; ii++)
    {
        map_frame_to_page(memmgr_dumb, page_num);
        page_num += 1;
    }

    /* TODO: Only invalidate the needed entries */
    memmgr_virtual_flush_tlb();

    return (void*)(free_page * PAGE_SIZE);
}

/* Finds a free frame and maps it to the specified page number */
static void map_frame_to_page(memmgr_dumb_t *memmgr_dumb, uintptr_t page)
{
    uintptr_t o_dir = page / 1024;                              /* Offset into page directory */
    uintptr_t o_tbl = page % 1024;                              /* Offset into page table */

    /* Find and take a free frame */
    uintptr_t frame_addr = get_frame(memmgr_dumb);

    /* Create the mapping! */
    page_directory_t *pg_dir = memmgr_dumb->page_directory;
    page_table_t *pg_tbl = pg_dir->tables[o_dir];
    page_t *pg = &pg_tbl->pages[o_tbl];

    memmgr_virtual_map_page(pg, frame_addr, true, true);
}

/* Finds a frame, marks it used, and returns its physical address */
static uintptr_t get_frame(memmgr_dumb_t *memmgr_dumb)
{
    uintptr_t frame_addr = memmgr_dumb->next_free_frame * PAGE_SIZE;
    memmgr_dumb->next_free_frame++;
    memmgr_dumb->allocated_frames++;
    return frame_addr;
}

/* Finds a block of contiguous virtual memory with at least n_pages of free pages */
static uintptr_t advance_free_page(memmgr_dumb_t *memmgr_dumb, uintptr_t n_pages)
{
    uintptr_t position = memmgr_dumb->next_free_page;
    page_directory_t *pg_dir = memmgr_dumb->page_directory;

    /* Unfortunately -1 (0xFFFFFFFF) is a valid frame, so by using it as a null value,
     * we lose 4k of memory somewhere above 4GB. Not that it matters here really. */
    uintptr_t result = -1;                                      
    uintptr_t start = -1;
    uintptr_t count = 0;

    do
    {
        uintptr_t o_dir = position / 1024;                      /* Offset into page directory */
        uintptr_t o_tbl = position % 1024;                      /* Offset into page table */
        uintptr_t advance = 1;                                  /* How much to advance the search */

        if ((pg_dir->tablesPhysical[o_dir]&1) == 0)             /* Test for present bit */
        {
            count = 0;                                          /* Since we don't want to write new */
            start = -1;                                         /* page tables, reset, and */
            advance = 1024;                                     /* skip to the next entry */
        }
        else if (pg_dir->tables[o_dir]->pages[o_tbl].present == 0)
        {
            count += 1;                                         /* Free page! */
        }
        else                                                    /* No free page or directory */
        {
            count = 0;                                          /* Reset all of our hard work :( */
            start = -1;
        }

        if (count > 0 && start == -1u)                          /* Free space but no start? */
        {
            start = position;                                   /* Just found it, so it starts here */
        }

        if (count >= n_pages)                                   /* Do we have enough space? */
        {
            result = start;
            break;
        }

        position += advance;
    }
    while (position > (uintptr_t)&KERNEL_BASE / PAGE_SIZE);     /* uint will wrap according to spec */

    if (result == -1u)
    {
        return -1;                                              /* Couldn't find enough space */
    }
    else
    {
        memmgr_dumb->next_free_page = result;
        return result;
    }
}
