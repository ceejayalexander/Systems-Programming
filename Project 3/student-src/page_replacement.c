#include "types.h"
#include "pagesim.h"
#include "mmu.h"
#include "swapops.h"
#include "stats.h"
#include "util.h"

pfn_t select_victim_frame(void);

/**
 * --------------------------------- PROBLEM 7 --------------------------------------
 * Checkout PDF section 7 for this problem
 *
 * Make a free frame for the system to use. You call the select_victim_frame() method
 * to identify an "available" frame in the system (already given). You will need to
 * check to see if this frame is already mapped in, and if it is, you need to evict it.
 *
 * @return victim_pfn: a phycial frame number to a free frame be used by other functions.
 *
 * HINTS:
 *      - When evicting pages, remember what you checked for to trigger page faults
 *      in mem_access
 *      - If the page table entry has been written to before, you will need to use
 *      swap_write() to save the contents to the swap queue.
 * ----------------------------------------------------------------------------------
 */
pfn_t free_frame(void) {
    pfn_t victim_pfn; 
    victim_pfn = select_victim_frame(); // we got a frame that a process wants to begin using

    // TODO: evict any mapped pages.
    if (frame_table[victim_pfn].mapped) { // is a process using the frame already?

        // how do we manage the state of the process 'victim_pfn' is leaving ownership of?
        pcb_t *old_process = frame_table[victim_pfn].process; // get the process that used to own this page
        vpn_t old_process_vpn = frame_table[victim_pfn].vpn; // get old process's VPN using the frame table
        pte_t *old_process_PTE = get_page_table_entry(old_process_vpn, old_process->saved_ptbr, mem); // get old process's PTE

        old_process_PTE->valid = 0; // entry got evicted, no longer valid

        if(old_process_PTE->dirty) { // do we need to save it to disk because of modifications?
            swap_write(old_process_PTE, &mem[old_process_PTE->pfn * PAGE_SIZE]); // save old page data to disk
            stats.writebacks++; // increment number of writebacks
            old_process_PTE->dirty = 0; // clear dirty bit
        }

        frame_table[victim_pfn].mapped = 0;
        frame_table[victim_pfn].process = NULL;
        frame_table[victim_pfn].vpn = 0;
        frame_table[victim_pfn].protected = 0;
        frame_table[victim_pfn].ref_count = 0;
    }

    return victim_pfn;
}

/**
 * --------------------------------- PROBLEM 9 --------------------------------------
 * Checkout PDF section 7, 9, and 11 for this problem
 *
 * Finds a free physical frame. If none are available, uses either a
 * randomized, Approximate LRU, or FIFO algorithm to find a used frame for
 * eviction.
 *
 * @return The physical frame number of a victim frame.
 *
 * HINTS:
 *      - Use the global variables MEM_SIZE and PAGE_SIZE to calculate
 *      the number of entries in the frame table.
 *      - Use the global last_evicted to keep track of the pointer into the frame table
 * ----------------------------------------------------------------------------------
 */
pfn_t select_victim_frame() {
    /* See if there are any free frames first */
    size_t num_entries = MEM_SIZE / PAGE_SIZE;
    for (size_t i = 0; i < num_entries; i++) {
        if (!frame_table[i].protected && !frame_table[i].mapped)
        {
            return i;
        }
    }

    if (replacement == RANDOM) {
        /* Play Russian Roulette to decide which frame to evict */
        pfn_t unprotected_found = NUM_FRAMES;
        for (pfn_t i = 0; i < num_entries; i++) {
            if (!frame_table[i].protected) {
                unprotected_found = i;
                if (prng_rand() % 2) {
                    return i;
                }
            }
        }
        /* If no victim found yet take the last unprotected frame
           seen */
        if (unprotected_found < NUM_FRAMES) {
            return unprotected_found;
        }
    }
    else if (replacement == APPROX_LRU) {
        /* Implement a LRU algorithm here */
        pfn_t victim = NUM_FRAMES; // track the frame...
        uint8_t min_ref_count = UINT8_MAX; // ...with the smallest reference count

        for(size_t i = 0; i < NUM_FRAMES; ++i) {
            if (!frame_table[i].protected && frame_table[i].mapped && frame_table[i].ref_count < min_ref_count) {
                min_ref_count = frame_table[i].ref_count;
                victim = i;
            }
        }

        return victim;
    }
    else if (replacement == FIFO) {
        /* Implement a FIFO algorithm here */
        pfn_t victim = (last_evicted + 1) % num_entries;

        for(size_t i = 1; i < NUM_FRAMES; ++i) {
            if (!frame_table[victim].protected) {
                last_evicted = victim;
                return victim;
            }
            victim = (victim + 1) % num_entries;
        }

    }

    // If every frame is protected, give up. This should never happen on the traces we provide you.
    panic("System ran out of memory\n");
    exit(1);
}
/**
 * --------------------------------- PROBLEM 10.2 --------------------------------------
 * Checkout PDF for this problem
 *
 * Updates the associated variables for the Approximate LRU,
 * called every time the simulator daemon wakes up.
 *
 * ----------------------------------------------------------------------------------
 */
void daemon_update(void)
{
    /** FIX ME */
    for (size_t i = 0; i < NUM_FRAMES; ++i) { // for every page in memory
        if(frame_table[i].mapped && !frame_table[i].protected) { // if it's not protected and is mapped
            vpn_t vpn_i = frame_table[i].vpn; // get the process's vpn
            pfn_t page_frame_i = frame_table[i].process->saved_ptbr; // get the process's ptbr
            pte_t* pte_i = get_page_table_entry(vpn_i, page_frame_i, mem); // get the address of the pte that maps the vpn to the ith pfn


            // So frame_table[i].ref_count >>= 1; will right shift all the bits in the counter by 1 
            // shift the referenced bit left 7 times, so that when LRU #2 executes, the lowest counter will be selected as a victim page
            frame_table[i].ref_count >>= 1;
            frame_table[i].ref_count |= (pte_i->referenced << 7); // Treat ref bit as MSB (8-bit aging)

            pte_i->referenced = 0; 
        }
    }
}
