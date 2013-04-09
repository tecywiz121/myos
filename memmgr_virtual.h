#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>
#include "registers.h"

struct page
{
    uint32_t present    : 1;   // Page present in memory
    uint32_t rw         : 1;   // Read-only if clear, readwrite if set
    uint32_t user       : 1;   // Supervisor level only if clear
    uint32_t accessed   : 1;   // Has the page been accessed since last refresh?
    uint32_t dirty      : 1;   // Has the page been written to since last refresh?
    uint32_t unused     : 7;   // Amalgamation of unused and reserved bits
    uint32_t frame      : 20;  // Frame address (shifted right 12 bits)
};
typedef struct page page_t;

struct page_table
{
   page_t pages[1024];
};
typedef struct page_table page_table_t;

typedef struct page_directory
{
    /**
       Array of pointers to pagetables.
    **/
    page_table_t *tables[1024];
    /**
       Array of pointers to the pagetables above, but gives their *physical*
       location, for loading into the CR3 register.
    **/
    uint32_t *tablesPhysical;
    /**
       The physical address of tablesPhysical. This comes into play
       when we get our kernel heap allocated and the directory
       may be in a different location in virtual memory.
    **/
    uintptr_t physicalAddr;
} page_directory_t;

/**
  Sets up the environment, page directories etc and
  enables paging.
**/
void memmgr_virtual_bootstrap(page_directory_t *page_directory, page_table_t *page_table769);

/**
 * Returns the lowest virtual address that maps to the specified physical address.
 * If no such mapping can be found, it returns ~0. This function is woefully
 * inefficient, so avoid using it, especially once the page directory gets large.
 */
void *memmgr_virtual_phy_to_virt(page_directory_t* page_directory, uintptr_t addr);

/**
 * Writes the proper values for a page_t
 */
void memmgr_virtual_map_page(page_t *page, uintptr_t frame, bool is_kernel, bool is_writable);

/**
 * Flush the entire tlb
 */
void memmgr_virtual_flush_tlb(void);

/**
  Causes the specified page directory to be loaded into the
  CR3 register.
**/
void switch_page_directory(page_directory_t *new);

/**
  Retrieves a pointer to the page required.
  If make == 1, if the page-table in which this page should
  reside isn't created, create it!
**/
page_t *get_page(uint32_t address, int make, page_directory_t *dir);

/**
  Handler for page faults.
**/
void page_fault(registers_t regs);
#endif
