#pragma once

#include "mmu.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/**
 * --------------------------------- PROBLEM 1 --------------------------------------
 * Checkout PDF Section 3 For this Problem
 *
 * Split the virtual address into its virtual page number and offset.
 * 
 * HINT: 
 *      -Examine the global defines in pagesim.h, which will be necessary in 
 *      implementing these functions.
 * ----------------------------------------------------------------------------------
 */

static inline vpn_t get_vaddr_vpn(vaddr_t addr) {
    return (0x003FF000 & addr) >> OFFSET_LEN;
}

static inline uint16_t get_vaddr_offset(vaddr_t addr) {
    return (0x00003FFF & addr); 
}

static inline pte_t* get_page_table(pfn_t ptbr, uint8_t* memory) {
    return (pte_t*) (memory + ptbr * PAGE_SIZE);
}

static inline pte_t* get_page_table_entry(vpn_t vpn, pfn_t ptbr, uint8_t *memory) {
    pte_t* ptbr_addr = get_page_table(ptbr, memory); // returns addr to struct that is a page table entry
    return (pte_t*) (ptbr_addr + vpn); // index into the page table via adding 'vpn'
}

static inline paddr_t get_physical_address(pfn_t pfn, uint16_t offset) {
    return (paddr_t) (pfn * PAGE_SIZE + offset);
    // return (pfn << OFFSET_LEN) | offset; 
    // NOTE: (pfn << OFFSET_LEN) = pfn * PAGE_SIZE since if pfn = 0,1,2,3 and PAGE_SIZE = 2^14, then it's essentially 0 * 2^14, 1 * 2^14, 2 * 2^14, 3 * 2^14, etc.
}

#pragma GCC diagnostic pop
