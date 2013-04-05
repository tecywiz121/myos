#include <stdint.h>
#include "multiboot.h"
#include "kernel.h"
#include "memmgr_physical.h"

void kmain(void)
{
    uint32_t flags = multiboot_info.flags;

    if (0 >= (flags & MULTIBOOT_INFO_MEMORY))
    {
        bootstrap_print("Memory info is not valid");
        return;
    }

    memmgr_physical_init();

    for (;;);
}
