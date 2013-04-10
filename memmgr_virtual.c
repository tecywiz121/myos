#include <stdint.h>
#include <stdalign.h>
#include <stdbool.h>
#include "util.h"
#include "memmgr_physical.h"
#include "memmgr_virtual.h"

/*
 * Externs
 */
extern uint32_t _b_page_directory;

/*
 * Internal Function
 */

void memmgr_virtual_bootstrap(page_directory_t *page_directory, page_table_t *page_table769)
{
    page_directory->tablesPhysical = &_b_page_directory;
    page_directory->physicalAddr = (uintptr_t)&_b_page_directory;

    /* Remap the structures created by the bootstrap after the kernel */
    int tableIdx = 0;
    memmgr_virtual_map_page(&page_table769->pages[tableIdx++],
                            (uintptr_t)&_b_page_directory,
                            true, true);                    /* Map the page directory */
    for (int ii = 0; ii < 1024; ii++)                       /* Map all present pages */
    {
        uintptr_t addrPhy = page_directory->tablesPhysical[ii];
        if ((addrPhy & 1) > 0)                              /* Check if present bit is set */
        {
            addrPhy &= 0xFFFFF000;                          /* Convert entry to physical address */

            uintptr_t addrVirt = (769u*1024u*PAGE_SIZE);    /* Address of first byte in table 769 */
            addrVirt += PAGE_SIZE * tableIdx;               /* Add the offset of the current page */

            memmgr_virtual_map_page(&page_table769->pages[tableIdx++], addrPhy, true, true);

            page_directory->tables[ii] = (page_table_t*)addrVirt;
        }
    }

    /* Save the virtual address into the page_directory structure */
    page_directory->tables[769] = page_table769;

    /* Save the directory entry into the page directory */
    uintptr_t directoryEntry = (uintptr_t)page_table769;    /* The virtual address of the new table */
    directoryEntry -= (uintptr_t)&KERNEL_BASE;              /* Convert it to a physical address */
    directoryEntry &= 0xFFFFF000;                           /* 20 most significant bits are used */
    directoryEntry |= 0x1;                                  /* Bit-0: The present bit */
    page_directory->tablesPhysical[769] = directoryEntry;

    /* Update the tlb */
    memmgr_virtual_flush_tlb();

    /* Update the page directory pointer */
    page_directory->tablesPhysical = (uint32_t*)(769u * 1024u * PAGE_SIZE);
}

/* Clears the mapping for a virtual address */
void memmgr_virtual_unmap(page_directory_t* page_directory, void* addr)
{
    uintptr_t page = (uintptr_t)addr / PAGE_SIZE;           /* Convert address to page number */
    uintptr_t o_dir = page / 1024;                          /* Offset into page directory */
    uintptr_t o_tbl = page % 1024;                          /* Offset into page table */

    if (!(page_directory->tablesPhysical[o_dir] & 1))       /* Check the present bit on page table */
    {
        return;                                             /* No page table, so address can't be mapped */
    }

    page_table_t *table = page_directory->tables[o_dir];    /* Get the page table */
    table->pages[o_tbl].present = 0;                        /* Clear the present bit */
    memmgr_virtual_flush_addr(addr);                        /* Update the TLB */
}


/* Walk a page directory calling table_cb for each present table, and page_cb for each present page */
void page_directory_walk(page_directory_t* page_directory, pg_dir_table_cb* table_cb, pg_dir_page_cb* page_cb, void *data)
{
    for (int ii = 0; ii < 1024; ii++)
    {
        if ((page_directory->tablesPhysical[ii] & 1) > 0)           /* Check the present bit */
        {
            if (table_cb && table_cb(data, ii, page_directory->tables[ii]))
            {
                return;
            }

            if (page_cb)
            {
                for (int jj = 0; jj < 1024; jj++)
                {
                    page_t *page = &page_directory->tables[ii]->pages[jj];
                    if (page->present && page_cb(data, ii, jj, page))
                    {
                        return;
                    }
                }
            }
        }
    }
}

/* Callback for finding a virtual address for a given physical address */
static int phy_to_virt_page_cb(void* data, uintptr_t dir_offset, uintptr_t page_offset, page_t* page)
{
    uintptr_t **args = (uintptr_t**)data;
    uintptr_t addr = *args[0];
    uintptr_t *result = args[1];

    if (page->present && page->frame == addr)
    {
        *result = dir_offset*1024u*PAGE_SIZE + page_offset*PAGE_SIZE;
        return 1;                                                   /* Break early */
    }

    return 0;                                                       /* Continue executing */
}

/*
 * Returns the lowest virtual address that corresponds to the physical
 * address, or ~0x0 if no mapping exists.
 */
void *memmgr_virtual_phy_to_virt(page_directory_t* page_directory, uintptr_t addr)
{
    uintptr_t offset = addr % PAGE_SIZE;                            /* Offset from page start */
    addr /= PAGE_SIZE;                                              /* Only need 20 most sig. bits */

    uintptr_t result = ~0;                                          /* Storage for the callback */
    uintptr_t *args[] = {&addr, &result};                           /* Arguments for the callback */
    page_directory_walk(page_directory, 0, &phy_to_virt_page_cb, &args);

    result += offset;
    return (void*)result;
}

#if (0)
static uint32_t memmgr_virtual_map_region(page_table_t* page_table, int table_idx, uintptr_t phy_start, uintptr_t phy_end, bool is_kernel, bool is_writable)
{
    uintptr_t count = idivc(phy_end - phy_start, PAGE_SIZE);
    for (uintptr_t ii = 0; ii < count; ii++)
    {
        memmgr_virtual_map_page(&page_table->pages[ii + table_idx],
                                phy_start + PAGE_SIZE*ii,
                                is_kernel, is_writable);
    }

    return (phy_start & PAGE_SIZE) + PAGE_SIZE*table_idx;       /* Return the offset into the memory described by the page table where the mapped region exists */
}
#endif

void memmgr_virtual_map_page(page_t *page, uintptr_t frame, bool is_kernel, bool is_writable)
{
    page->present = 1;
    page->rw = (is_writable) ? 1 : 0;
    page->user = (is_kernel) ? 0 : 1;
    page->frame = frame / PAGE_SIZE;
}

void memmgr_virtual_flush_tlb(void)
{
    __asm__ volatile (
        "mov eax, cr3;"
        "mov cr3, eax;"
    );
}

void memmgr_virtual_flush_addr(void* addr)
{
    __asm__ volatile (
        "invlpg [%0]"
        : /* No output values */
        : "r" ((uintptr_t)addr)
        : "memory"
    );
}
