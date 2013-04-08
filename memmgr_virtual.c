#include <stdint.h>
#include <stdalign.h>
#include <stdbool.h>
#include "util.h"
#include "native.h"
#include "memmgr_physical.h"
#include "memmgr_virtual.h"


/*
 * Externs
 */
extern uint32_t _b_page_directory;
extern uint32_t _b_end;
extern uint32_t KERNEL_BASE;

/*
 * Static Variables
 */
static page_directory_t page_directory;
alignas(PAGE_SIZE) static page_table_t page_table769;

/*
 * Internal Function
 */
static void memmgr_virtual_flush_tlb(void);
static void memmgr_virtual_map_page(page_t *page, uintptr_t frame, bool is_kernel, bool is_writable);

void memmgr_virtual_init(void)
{
    page_directory.tablesPhysical = &_b_page_directory;
    page_directory.physicalAddr = (uintptr_t)&_b_page_directory;

    /* Remap the structures created by the bootstrap after the kernel */
    int tableIdx = 0;
    memmgr_virtual_map_page(&page_table769.pages[tableIdx++],
                            &_b_page_directory, true, true);/* Map the page directory */
    for (int ii = 0; ii < 1024; ii++)                       /* Map all present pages */
    {
        uintptr_t addrPhy = page_directory.tablesPhysical[ii];
        if ((addrPhy & 1) > 0)                              /* Check if present bit is set */
        {
            addrPhy &= 0xFFFFF000;                          /* Convert entry to physical address */

            uintptr_t addrVirt = (769u*1024u*PAGE_SIZE);    /* Address of first byte in table 769 */
            addrVirt += 1024u * tableIdx;                   /* Add the offset of the current page */

            memmgr_virtual_map_page(&page_table769.pages[tableIdx++], addrPhy, true, true);

            page_directory.tables[ii] = (page_table_t*)addrVirt;
        }
    }

    /* Save the virtual address into the page_directory structure */
    page_directory.tables[769] = &page_table769;

    /* Save the directory entry into the page directory */
    uintptr_t directoryEntry = (uintptr_t)&page_table769;   /* The virtual address of the new table */
    directoryEntry -= (uintptr_t)&KERNEL_BASE;              /* Convert it to a physical address */
    directoryEntry &= 0xFFFFF000;                           /* 20 most significant bits are used */
    directoryEntry |= 0x1;                                  /* Bit-0: The present bit */
    page_directory.tablesPhysical[769] = directoryEntry;

    /* Update the tlb */
    memmgr_virtual_flush_tlb();

    /* Update the page directory pointer */
    page_directory.tablesPhysical = (uint32_t*)(769u * 1024u * PAGE_SIZE);
}

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

static void memmgr_virtual_map_page(page_t *page, uintptr_t frame, bool is_kernel, bool is_writable)
{
    page->present = 1;
    page->rw = (is_writable) ? 1 : 0;
    page->user = (is_kernel) ? 0 : 1;
    page->frame = frame / PAGE_SIZE;
}

static void memmgr_virtual_flush_tlb(void)
{
    __asm__ volatile (
        "mov eax, cr3;"
        "mov cr3, eax;"
    );
}