#include <stdint.h>
#include <stdalign.h>
#include "multiboot.h"
#include "kernel.h"
#include "memmgr_physical.h"
#include "memmgr_virtual.h"

alignas(0x1000) static page_table_t page_table769;
static page_directory_t page_directory;

static void read_multiboot_info(void);

void kmain(void)
{
    uint32_t flags = _b_multiboot_info.flags;

    if (0 >= (flags & MULTIBOOT_INFO_MEMORY))
    {
        _b_print("Memory info is not valid");
        return;
    }

    memmgr_virtual_bootstrap(&page_directory, &page_table769);
    read_multiboot_info();

    for (;;);
}

static void read_multiboot_info(void)
{
    multiboot_info_t *multiboot_info = &_b_multiboot_info;  /* I just wanted a shorthand */
}
