#include "mmu.h"
#include "pagesim.h"
#include "swapops.h"
#include "stats.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/**
 * --------------------------------- PROBLEM 6 --------------------------------------
 * Checkout PDF section 7 for this problem
 * 
 * Page fault handler.
 * 
 * When the CPU encounters an invalid address mapping in a page table, it invokes the 
 * OS via this handler. Your job is to put a mapping in place so that the translation 
 * can succeed.
 * 
 * @param addr virtual address in the page that needs to be mapped into main memory.
 * 
 * HINTS:
 *      - You will need to use the global variable current_process when
 *      altering the frame table entry.
 *      - Use swap_exists() and swap_read() to update the data in the 
 *      frame as it is mapped in.
 * ----------------------------------------------------------------------------------
 */
void page_fault(vaddr_t address) {
    // TODO: Get a new frame, then correctly update the page table and frame table
    vpn_t vpn = get_vaddr_vpn(address);
    pte_t *PTE = get_page_table_entry(vpn, PTBR, mem);

    pfn_t new_frame = free_frame(); // get a new frame (pfn)
    PTE->pfn = new_frame; // update mapping from VPN to new PFN in current process' page table

    if (swap_exists(PTE)) { // if the faulting page is on disk
        swap_read(PTE, &mem[new_frame * PAGE_SIZE]); // bring it back into memory at the new page
    } else { // faulting page was not saved in disk
        for(paddr_t i = 0; i < PAGE_SIZE; ++i) {
            mem[new_frame * PAGE_SIZE + i] = 0;
        }
    }

    PTE->valid = 1; // it now has a valid mapping
    PTE->referenced = 1; // the entry was recently accessed
    PTE->dirty = 0; // fresh entry
    
    // update frame table
    frame_table[new_frame].mapped = 1; 
    frame_table[new_frame].process = current_process;
    frame_table[new_frame].protected = 0;
    frame_table[new_frame].vpn = vpn; 
    frame_table[new_frame].ref_count = 1;
}


#pragma GCC diagnostic pop
