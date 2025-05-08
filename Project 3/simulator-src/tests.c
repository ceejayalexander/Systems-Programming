#include "tests.h"


// TODO: remove after fixing warnings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"

void run_tests() {
    test_get_vaddr_vpn(); 
    test_get_vaddr_offset();
    test_get_page_table();
    test_get_page_table_entry(); 
    test_get_physical_address();
    test_compute_stats();
    test_daemon_update();
    test_select_victim_frame_approx_lru();
    test_select_victim_frame_fifo(); 

    printf("All tests passed!\n");

}

void test_get_vaddr_vpn() {
    assert(get_vaddr_vpn((vaddr_t) ((0xFF<<OFFSET_LEN) + 0x2032)) == 0xFF);
    printf("Passed address_splitting.h/get_vaddr_vpn() test!\n");
}

void test_get_vaddr_offset() {
    assert(get_vaddr_offset((vaddr_t) ((0xFF<<OFFSET_LEN) + 0x2032)) == 0x2032);
    printf("Passed address_splitting.h/get_vaddr_offset() test!\n");
}

void test_get_page_table() {
    pte_t* page_table = get_page_table(0x3, 0x00);
    assert(page_table == (pte_t*) 0xc000);
    printf("Passed address_splitting.h/get_page_table() test!\n");
}

void test_get_page_table_entry() {
    pte_t* page_table_entry = get_page_table_entry(0x1, 0x3, 0x00);
    assert(page_table_entry == (pte_t*)0xc010);
    printf("Passed address_splitting.h/get_page_table_entry() test!\n");
}

void test_get_physical_address() {
   assert(get_physical_address(0x3, 0x2032) == 0xe032);
   printf("Passed address_splitting.h/get_physical_address() test!\n");
}

void test_compute_stats() {
    stats.accesses = 21;
    stats.writebacks = 5;
    stats.page_faults = 3;

    compute_stats();

    // casting to int to avoid floating point comparison, should be fine for this case
    assert((int)stats.amat == (int)81152);
    printf("Passed stats.c/compute_stats() test!\n");

}

void test_daemon_update() {
    // Make sure your get_page_table_entry() function is working correctly, otherwise this test will fail.

    // Initialization
    mem = calloc(1, MEM_SIZE);
    pcb_t* process = (pcb_t*) malloc(sizeof(pcb_t));
    process->pid = 0;
    process->state = PROC_RUNNING;
    process->saved_ptbr = 0x2; // TODO: verify this is correct


    frame_table = (fte_t*) mem;

    int i = 1;

    frame_table[i].process = process;
    frame_table[i].vpn = 0x10;
    frame_table[i].ref_count = 0;

    // TODO: are these bits being set correctly?
    frame_table[i].mapped = 1;
    frame_table[i].protected = 0;

    pte_t* page_table_entry = get_page_table_entry(frame_table[i].vpn, frame_table[i].process->saved_ptbr, mem);

    // Testing your code.
    page_table_entry->referenced = 1;
    daemon_update();
    assert(frame_table[1].ref_count == 128);
    page_table_entry->referenced = 1;
    daemon_update();
    assert(frame_table[1].ref_count == 192);
    daemon_update();
    assert(frame_table[1].ref_count == 96);


    // cleanup
    free(mem);
    free(process);

    printf("Passed page_replacement.c/daemon_update() test!\n");


}

void test_select_victim_frame_approx_lru() {

    mem = calloc(1, MEM_SIZE);
    frame_table = (fte_t*) mem;


    // make all the pages in memory mapped.
    for (int i = 0; i < NUM_FRAMES; i++) {
        frame_table[i].protected = 0;
        frame_table[i].mapped = 1;
        frame_table[i].ref_count = 0xFF;
    }

    // protect the frame table
    frame_table[0].protected = 1;

    frame_table[1].ref_count = 192;
    frame_table[2].ref_count = 128;
    frame_table[3].ref_count = 245;

    replacement = APPROX_LRU;

    assert(select_victim_frame() == 2);

    free(mem);

    printf("Passed page_replacement.c/select_victim_frame() - approx_lru test!\n");
}

// CHANGE THIS ENTIRE THING -- VERY IMPORTANT
void test_select_victim_frame_fifo() {

    //allocate
    mem = calloc(1, MEM_SIZE);
    frame_table = (fte_t*) mem;

    // Set up frames, make all mapped and unprotected
    for (int i = 0; i < NUM_FRAMES; i++) {
        frame_table[i].protected = 0;
        frame_table[i].mapped = 1; 
    }

    // Protect the first frame (so it's never selected as a victim)
    frame_table[0].protected = 1;

    replacement = FIFO;
    last_evicted = 1;

    // First eviction should return frame 2 if frame 1 was last evicted
    assert(select_victim_frame() == 2);

    // Cleanup memory
    free(mem);

    printf("Passed page_replacement.c/select_victim_frame() - FIFO test!\n");
    
}
// THE END OF THIS ENTIRE THING