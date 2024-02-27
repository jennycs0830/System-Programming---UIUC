/**
 * ideal_indirection
 * CS 341 - Fall 2023
 */
#include "mmu.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

mmu *mmu_create() {
    mmu *my_mmu = calloc(1, sizeof(mmu));
    my_mmu->tlb = tlb_create();
    return my_mmu;
}

void new_page_table( page_directory_entry* pde ){
    page_table* new = (page_table*) ( (uintptr_t) pde->base_addr << NUM_OFFSET_BITS );
    memset( new, 0, sizeof(page_table) );

    for( unsigned long i = 0; i < NUM_ENTRIES; i++ ){
        new->entries[i].read_write = 1;
        new->entries[i].user_supervisor = 1;
    }
    return ;
}

page_table_entry* get_table_entry( mmu* this, addr32 virtual_address ){
    // Try to get from TLB
    page_table_entry* pte = tlb_get_pte( &this->tlb, virtual_address & 0xFFFFF000 );
    if( pte == NULL ){
        mmu_tlb_miss( this );

        // Find from page directory and table
        // page directory
        page_directory* pd = this->page_directories[ this->curr_pid ];
        addr32 top_level = virtual_address >> (VIRTUAL_ADDR_SPACE - NUM_OFFSET_BITS);
        page_directory_entry* pde = &pd->entries[ top_level ];
        // PDE not present
        if( pde->present == 0 ){
            mmu_raise_page_fault( this );
            // update attributes
            pde->base_addr = ( ask_kernel_for_frame ( NULL ) >> NUM_OFFSET_BITS );
            read_page_from_disk( (page_table_entry*) pde );
            pde->present = 1;
            pde->read_write = 1;
            pde->user_supervisor = 1;
        }

        // page table
        page_table* pt = (page_table*) get_system_pointer_from_pde( pde );
        addr32 second_level = ( virtual_address & 0x003FF000 ) >>  NUM_OFFSET_BITS ;
        pte = &(pt->entries[ second_level ]);
    }

    if( pte->present == 0 ){
        mmu_raise_page_fault( this );
        // update attributes
        pte->base_addr = ( ask_kernel_for_frame ( pte ) >> NUM_OFFSET_BITS );
        read_page_from_disk( (page_table_entry*) pte );
        pte->present = 1;
        pte->read_write = 1;
        pte->user_supervisor = 1;
    }
    pte->accessed = 1;

    tlb_add_pte( &this->tlb, (virtual_address & 0xFFFFF000 ), pte );
    return pte;
}

void mmu_read_from_virtual_address(mmu *this, addr32 virtual_address,
                                   size_t pid, void *buffer, size_t num_bytes) {
    assert(this);
    assert(pid < MAX_PROCESS_ID);
    assert(num_bytes + (virtual_address % PAGE_SIZE) <= PAGE_SIZE);
    // TODO: Implement me!
    // check cur process
    if( this->curr_pid != pid ){
        tlb_flush( &this->tlb );
        this->curr_pid = pid;
    }
    
    // Segmentation check
    if( !address_in_segmentations( this->segmentations[ pid ], virtual_address ) || !virtual_address ){
        mmu_raise_segmentation_fault( this );
        return ;
    }

    // Permission to read
    vm_segmentation* seg = find_segment( this->segmentations[ pid ], virtual_address );
    if( !( seg->permissions && READ ) ){
        mmu_raise_segmentation_fault( this );
        return ;
    }

    // Get page table entry
    page_table_entry* entry = get_table_entry( this, virtual_address );
    
    // Address translation
    if( entry != NULL ){
        void* frame = (void*) get_system_pointer_from_pte( entry );
        memcpy( buffer, frame + (virtual_address & 0xFFF), num_bytes );
    }
    return ;
}

void mmu_write_to_virtual_address(mmu *this, addr32 virtual_address, size_t pid,
                                  const void *buffer, size_t num_bytes) {
    assert(this);
    assert(pid < MAX_PROCESS_ID);
    assert(num_bytes + (virtual_address % PAGE_SIZE) <= PAGE_SIZE);
    // TODO: Implement me!
    if( this->curr_pid != pid ){
        tlb_flush( &this->tlb );
        this->curr_pid = pid;
    }

    // Segmentation check
    if( !address_in_segmentations( this->segmentations[ pid ], virtual_address ) || !virtual_address ){
        mmu_raise_segmentation_fault( this );
        return ;
    }
    
    vm_segmentations* segs = this->segmentations[ pid ];
    vm_segmentation* seg = find_segment( segs, virtual_address );
    if( seg == NULL ){
        mmu_raise_segmentation_fault( this );
        return ;
    }
    else if( (seg->permissions & WRITE) == 0 ){
        mmu_raise_segmentation_fault( this );
        return ;
    }

    page_table_entry* entry = get_table_entry( this, virtual_address );

    if( entry != NULL ){
        entry->dirty = 1;
        void* frame = (void*) get_system_pointer_from_pte( entry );
        memcpy( frame + (virtual_address & 0xFFF), buffer, num_bytes );
    }
    return ;
}

void mmu_tlb_miss(mmu *this) {
    this->num_tlb_misses++;
}

void mmu_raise_page_fault(mmu *this) {
    this->num_page_faults++;
}

void mmu_raise_segmentation_fault(mmu *this) {
    this->num_segmentation_faults++;
}

void mmu_add_process(mmu *this, size_t pid) {
    assert(pid < MAX_PROCESS_ID);
    addr32 page_directory_address = ask_kernel_for_frame(NULL);
    this->page_directories[pid] =
        (page_directory *)get_system_pointer_from_address(
            page_directory_address);
    page_directory *pd = this->page_directories[pid];
    this->segmentations[pid] = calloc(1, sizeof(vm_segmentations));
    vm_segmentations *segmentations = this->segmentations[pid];

    // Note you can see this information in a memory map by using
    // cat /proc/self/maps
    segmentations->segments[STACK] =
        (vm_segmentation){.start = 0xBFFFE000,
                          .end = 0xC07FE000, // 8mb stack
                          .permissions = READ | WRITE,
                          .grows_down = true};

    segmentations->segments[MMAP] =
        (vm_segmentation){.start = 0xC07FE000,
                          .end = 0xC07FE000,
                          // making this writeable to simplify the next lab.
                          // todo make this not writeable by default
                          .permissions = READ | EXEC | WRITE,
                          .grows_down = true};

    segmentations->segments[HEAP] =
        (vm_segmentation){.start = 0x08072000,
                          .end = 0x08072000,
                          .permissions = READ | WRITE,
                          .grows_down = false};

    segmentations->segments[BSS] =
        (vm_segmentation){.start = 0x0805A000,
                          .end = 0x08072000,
                          .permissions = READ | WRITE,
                          .grows_down = false};

    segmentations->segments[DATA] =
        (vm_segmentation){.start = 0x08052000,
                          .end = 0x0805A000,
                          .permissions = READ | WRITE,
                          .grows_down = false};

    segmentations->segments[TEXT] =
        (vm_segmentation){.start = 0x08048000,
                          .end = 0x08052000,
                          .permissions = READ | EXEC,
                          .grows_down = false};

    // creating a few mappings so we have something to play with (made up)
    // this segment is made up for testing purposes
    segmentations->segments[TESTING] =
        (vm_segmentation){.start = PAGE_SIZE,
                          .end = 3 * PAGE_SIZE,
                          .permissions = READ | WRITE,
                          .grows_down = false};
    // first 4 mb is bookkept by the first page directory entry
    page_directory_entry *pde = &(pd->entries[0]);
    // assigning it a page table and some basic permissions
    pde->base_addr = (ask_kernel_for_frame(NULL) >> NUM_OFFSET_BITS);
    pde->present = true;
    pde->read_write = true;
    pde->user_supervisor = true;

    // setting entries 1 and 2 (since each entry points to a 4kb page)
    // of the page table to point to our 8kb of testing memory defined earlier
    for (int i = 1; i < 3; i++) {
        page_table *pt = (page_table *)get_system_pointer_from_pde(pde);
        page_table_entry *pte = &(pt->entries[i]);
        pte->base_addr = (ask_kernel_for_frame(pte) >> NUM_OFFSET_BITS);
        pte->present = true;
        pte->read_write = true;
        pte->user_supervisor = true;
    }
}

void mmu_remove_process(mmu *this, size_t pid) {
    assert(pid < MAX_PROCESS_ID);
    // example of how to BFS through page table tree for those to read code.
    page_directory *pd = this->page_directories[pid];
    if (pd) {
        for (size_t vpn1 = 0; vpn1 < NUM_ENTRIES; vpn1++) {
            page_directory_entry *pde = &(pd->entries[vpn1]);
            if (pde->present) {
                page_table *pt = (page_table *)get_system_pointer_from_pde(pde);
                for (size_t vpn2 = 0; vpn2 < NUM_ENTRIES; vpn2++) {
                    page_table_entry *pte = &(pt->entries[vpn2]);
                    if (pte->present) {
                        void *frame = (void *)get_system_pointer_from_pte(pte);
                        return_frame_to_kernel(frame);
                    }
                    remove_swap_file(pte);
                }
                return_frame_to_kernel(pt);
            }
        }
        return_frame_to_kernel(pd);
    }

    this->page_directories[pid] = NULL;
    free(this->segmentations[pid]);
    this->segmentations[pid] = NULL;

    if (this->curr_pid == pid) {
        tlb_flush(&(this->tlb));
    }
}

void mmu_delete(mmu *this) {
    for (size_t pid = 0; pid < MAX_PROCESS_ID; pid++) {
        mmu_remove_process(this, pid);
    }

    tlb_delete(this->tlb);
    free(this);
    remove_swap_files();
}