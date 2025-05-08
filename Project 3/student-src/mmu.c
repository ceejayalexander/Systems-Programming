#include "mmu.h"
#include "pagesim.h"
#include "address_splitting.h"
#include "swapops.h"
#include "stats.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/* The frame table pointer. You will set this up in system_init. */
fte_t *frame_table;

/**
 * --------------------------------- PROBLEM 2 --------------------------------------
 * Checkout PDF sections 4 for this problem
 * 
 * In this problem, you will initialize the frame_table pointer. The frame table will
 * be located at physical address 0 in our simulated memory. You should zero out the 
 * entries in the frame table, in case for any reason physical memory is not clean.
 * 
 * HINTS:
 *      - mem: Simulated physical memory already allocated for you.
 *      - PAGE_SIZE: The size of one page
 * ----------------------------------------------------------------------------------
 */
void system_init(void) {
    // TODO: initialize the frame_table pointer.
    frame_table = (fte_t*) mem;

    // Frame Table sits in protected page 0
    frame_table[0].protected = 1; // FTE 0 immune from eviction
    frame_table[0].mapped = 0; // FTE 0 is mapped
    frame_table[0].ref_count = 0; // FTE 0 reference count 0
    frame_table[0].process = NULL;     // FTE 0 has no process
    frame_table[0].vpn = 0;

    // zero out remaining frame table entries
    for(int i = 1; i < NUM_FRAMES; ++i) {
        frame_table[i].protected = 0;
        frame_table[i].mapped = 0;
        frame_table[i].ref_count = 0;
        frame_table[i].process = NULL;
        frame_table[i].vpn = 0;
    }
}

/**
 * --------------------------------- PROBLEM 5 --------------------------------------
 * Checkout PDF section 6 for this problem
 * 
 * Takes an input virtual address and performs a memory operation.
 * 
 * @param addr virtual address to be translated
 * @param access 'r' if the access is a read, 'w' if a write
 * @param data If the access is a write, one byte of data to write to our memory.
 *             Otherwise NULL for read accesses.
 * 
 * HINTS:
 *      - Remember that not all the entry in the process's page table are mapped in. 
 *      Check what in the pte_t struct signals that the entry is mapped in memory.
 * ----------------------------------------------------------------------------------
 */
uint8_t mem_access(vaddr_t address, char access, uint8_t data) {
    // TODO: translate virtual address to physical, then perform the specified operation

    vpn_t vpn = get_vaddr_vpn(address);
    pte_t *PTE = get_page_table_entry(vpn, PTBR, mem); 

    if (PTE->valid == 0) { // doesn't map to valid pfn
        page_fault(address);
        ++stats.page_faults; // increment number of page faults

        PTE = get_page_table_entry(vpn, PTBR, mem);
    }

    stats.accesses++; // increment memory access counter
    PTE->referenced = 1; // Mark as accessed
    
    uint16_t offset = get_vaddr_offset(address);
    paddr_t physical_address = get_physical_address(PTE->pfn, offset);

    // Either read or write the data to the physical address depending on 'rw'
    if (access == 'r') {
        return mem[physical_address]; // read operation
    } else {
        mem[physical_address] = data; // write operation
        PTE->dirty = 1; // page is modified
        return data; // return the written value
    }
}
