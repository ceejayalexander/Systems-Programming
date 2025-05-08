#include "proc.h"
#include "mmu.h"
#include "pagesim.h"
#include "address_splitting.h"
#include "swapops.h"
#include "stats.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/**
 * --------------------------------- PROBLEM 3 --------------------------------------
 * Checkout PDF section 4 for this problem
 * 
 * This function gets called every time a new process is created.
 * You will need to allocate a frame for the process's page table using the
 * free_frame function. Then, you will need update both the frame table and
 * the process's PCB. 
 * 
 * @param proc pointer to process that is being initialized 
 * 
 * HINTS:
 *      - pcb_t: struct defined in pagesim.h that is a process's PCB.
 *      - You are not guaranteed that the memory returned by the free frame allocator
 *      is empty - an existing frame could have been evicted for our new page table.
 * ----------------------------------------------------------------------------------
 */
void proc_init(pcb_t *proc) {
    // TODO: initialize proc's page table.
    pfn_t new_ptbr = free_frame(); // Frame to hold process's page table
    
    // NOTE: There could still be data from an old process in this new page frame so we zero it out
    for(size_t i = 0; i < PAGE_SIZE; ++i) {
        mem[new_ptbr * PAGE_SIZE + i] = 0;
    }

    // Initialize frame table entry
    frame_table[new_ptbr].mapped = 1; // maps pfn (new_ptbr) to vpn = 0 (indicates a page table)
    frame_table[new_ptbr].process = proc; 
    frame_table[new_ptbr].protected = 1; // We do not want to evict the frame containing the page table during page replacement
    frame_table[new_ptbr].ref_count = 0;
    frame_table[new_ptbr].vpn = 0;

    proc->saved_ptbr = new_ptbr; // initialize process's ptbr 

    // There are only 64 pages (0 - 63 pfns) in memory.
    // One of these pages houses the frame table and OS instructions (page 0)
    // That means 63 other pages are free to the user. But really only 63 / 2 = 31 pages are free, because each process requires a page to hold its page table.
}


/**
 * --------------------------------- PROBLEM 4 --------------------------------------
 * Checkout PDF section 5 for this problem
 * 
 * Switches the currently running process to the process referenced by the proc 
 * argument.
 * 
 * Every process has its own page table, as you allocated in proc_init. You will
 * need to tell the processor to use the new process's page table.
 * 
 * @param proc pointer to process to become the currently running process.
 * 
 * HINTS:
 *      - Look at the global variables defined in pagesim.h. You may be interested in
 *      the definition of pcb_t as well.
 * ----------------------------------------------------------------------------------
 */
void context_switch(pcb_t *proc) {
    // TODO: update any global vars and proc's PCB to match the context_switch.
    proc->state = PROC_RUNNING;
    PTBR = proc->saved_ptbr; // set global PTBR (PTBR holds PFN, not a physical address to first PTE)
}

/**
 * --------------------------------- PROBLEM 8 --------------------------------------
 * Checkout PDF section 8 for this problem
 * 
 * When a process exits, you need to free any pages previously occupied by the
 * process.
 * 
 * HINTS:
 *      - If the process has swapped any pages to disk, you must call
 *      swap_free() using the page table entry pointer as a parameter.
 *      - If you free any protected pages, you must also clear their"protected" bits.
 * ----------------------------------------------------------------------------------
 */
void proc_cleanup(pcb_t *proc) {
    // TODO: Iterate the proc's page table and clean up each valid page

    pfn_t page_table = proc->saved_ptbr; // returns a pfn (uint16_t)
    pte_t *pte = get_page_table(page_table, mem); // gets pointer to first pte (vpn 0)

    for (size_t vpn = 0; vpn < NUM_PAGES; ++vpn) { // iterate over all vpns in the page table
        if (!pte[vpn].valid) {
            continue;
        }
        
        if (swap_exists(&pte[vpn])) {
            swap_free(&pte[vpn]);
        }

        pte[vpn].referenced = 0;
        pte[vpn].dirty = 0;
        pte[vpn].valid = 0;
        pte[vpn].sid = 0;

        // Clean up the frame table of the process
        pfn_t pfn = pte[vpn].pfn;
        frame_table[pfn].mapped = 0;
        frame_table[pfn].process = NULL;
        frame_table[pfn].protected = 0;
        frame_table[pfn].ref_count = 0;
        frame_table[pfn].vpn = 0;
        
        // Clean up the page frame that was mapped by the pte 'pfn'
        for(size_t j = 0; j < PAGE_SIZE; ++j) {
            mem[pfn * PAGE_SIZE + j] = 0;
        }

        pte[vpn].pfn = 0;
    }

    // Zero out the page table
    for(size_t i = 0; i < PAGE_SIZE; ++i) {
        mem[page_table * PAGE_SIZE + i] = 0;
    }

    // Clear the process's PTE from the frame table
    frame_table[page_table].mapped = 0;
    frame_table[page_table].process = NULL;
    frame_table[page_table].protected = 0;
    frame_table[page_table].ref_count = 0;
    frame_table[page_table].vpn = 0;
}

#pragma GCC diagnostic pop
