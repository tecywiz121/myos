#include <stdint.h>
#include <stdalign.h>
#include "util.h"
#include "multiboot.h"
#include "kernel.h"
#include "memmgr_physical.h"
#include "memmgr_virtual.h"
#include "memmgr_dumb.h"

typedef void (mmap_callback_t)(multiboot_memory_map_t*);

/* Page Table that maps the bootstrap stuff above KERNEL_BASE */
alignas(0x1000) static page_table_t page_table769;

/* Structure for referencing the page directory created by the bootstrap */
static page_directory_t page_directory;

/* The highest physical address reported by the bootloader */
static uintptr_t max_physical_address = 0;

/* A very, very basic memory allocator. */
static memmgr_dumb_t memmgr_dumb;

/* The physical memory manager */
static memmgr_physical_t memmgr_phy;

static void die(char *msg);
static void multiboot_walk_mmap(mmap_callback_t* cb);
static void update_max_phy_addr(multiboot_memory_map_t *mmap);
static void apply_mmap_to_memmgr(multiboot_memory_map_t *mmap);
static void unmap_bootstrap(void);

void kmain(void)
{
    uint32_t flags = _b_multiboot_info.flags;                   /* Get the multiboot flags */

    if (0 >= (flags & MULTIBOOT_INFO_MEM_MAP))                  /* Ensure that the memory map is valid */
    {
        die("Memory info is not valid");
    }

    memmgr_virtual_bootstrap(&page_directory, &page_table769);  /* Take over the page directory the bootstrap created */
    dumb_init(&memmgr_dumb, &page_directory);                   /* Initialize the dumb allocator */

    multiboot_walk_mmap(&update_max_phy_addr);                  /* Find the highest available address to determine how big of a bitmap we need */

    memmgr_physical_init(&memmgr_phy, max_physical_address);    /* Initialize memmgr_phy */

    uintptr_t size = memmgr_physical_size(&memmgr_phy);
    void *frame_bitmap = dumb_alloc(&memmgr_dumb, size);        /* Allocate memory for memmgr_physical */
    memmgr_physical_set_frames(&memmgr_phy, (uint32_t *)frame_bitmap);

    multiboot_walk_mmap(&apply_mmap_to_memmgr);                 /* Walk the mmap again and apply it to the memmgr */

    unmap_bootstrap();

    //memmgr_set_from_page_directory(&memmgr_phy, &page_directory);

    die("boot complete!");
}

/* Callback that finds the upper limit to physical memory */
static void update_max_phy_addr(multiboot_memory_map_t *mmap)
{
    if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE && mmap->addr + mmap->len > max_physical_address)
    {
        max_physical_address = mmap->addr + mmap->len;
    }
}

/* Callback that applies the multiboot mmap to memmgr_phy */
static void apply_mmap_to_memmgr(multiboot_memory_map_t *mmap)
{
    if (mmap->type != MULTIBOOT_MEMORY_AVAILABLE)
    {
        memmgr_physical_set_range(&memmgr_phy, mmap->addr, idivc(mmap->len, PAGE_SIZE));
    }
}

/* Calls cb for every entry in the multiboot memory map */
static void multiboot_walk_mmap(mmap_callback_t* cb)
{
    multiboot_info_t *mbt = &_b_multiboot_info;     /* I just wanted a shorthand */

    multiboot_memory_map_t *mmap_phy = (multiboot_memory_map_t *)(uintptr_t)mbt->mmap_addr;
    while ((uintptr_t)mmap_phy < mbt->mmap_length + mbt->mmap_addr)
    {
        multiboot_memory_map_t *mmap =
            (multiboot_memory_map_t*)memmgr_virtual_phy_to_virt(&page_directory, (uintptr_t)mmap_phy);

        if (mmap == (multiboot_memory_map_t *)(~0))
        {
            /* TODO: Handle paging in missing frames */
            die("Memory map not accessable");
        }

        if (mmap->addr > UINTPTR_MAX || mmap->addr + mmap->len > UINTPTR_MAX)
        {
            /* TODO: Handle > 4GB of memory with PAE or somesuch */
            break;
        }

        cb(mmap);

        mmap_phy = (multiboot_memory_map_t*)((uintptr_t)mmap_phy + mmap->size + sizeof(uint32_t));
    }
}

static void unmap_bootstrap(void)
{
    uintptr_t start = (uintptr_t)&_b_start;
    uintptr_t end = (uintptr_t)&_b_end;

    for (uintptr_t ii = start; ii < end; ii+=PAGE_SIZE)
    {
        memmgr_virtual_unmap(&page_directory, (void*)ii);
    }
}

static void die(char *msg)
{
    volatile uint8_t *video = (volatile uint8_t*)0xB8000;
    while (*msg != 0)
    {
        *video++ = *msg++;
        *video++ = 0x07;
    }

    for (;;)
    {
        __asm__ ("hlt");
    }
}
