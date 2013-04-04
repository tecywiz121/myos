#include <stdint.h>
#include "multiboot.h"

extern multiboot_info_t multiboot_info;
extern void bootstrap_print(char * str);

void kmain(void)
{
    uint32_t flags = multiboot_info.flags;

    if (0 < flags & MULTIBOOT_INFO_MEMORY)
    {
        bootstrap_print("Memory info is valid");
    }
    else
    {
        bootstrap_print("Memory info is not vaild");
    }

    for (;;);
}
