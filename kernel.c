#include <stdint.h>
#include "multiboot.h"
#include "kernel.h"
#include "memmgr_physical.h"
#include "memmgr_virtual.h"

void kmain(void)
{
    uint32_t flags = _b_multiboot_info.flags;

    if (0 >= (flags & MULTIBOOT_INFO_MEMORY))
    {
        _b_print("Memory info is not valid");
        return;
    }

    memmgr_physical_init();
    memmgr_virtual_init();

    for (;;);
}
