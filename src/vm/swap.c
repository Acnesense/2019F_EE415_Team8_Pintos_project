#include "vm/swap.h"

/* initialize swap space */
void
swap_init (void) {

    block_swap = block_get_role(BLOCK_SWAP);
    if (block_swap == NULL) {
        printf("fail to initialize swap space");
        return;
    }
    swap_map = bitmap_create( block_size(block_swap) / SECTOR_SIZE);
    bitmap_set_all(swap_map, false);
    lock_init(&swap_lock);
}

// swap in the file to main memory
void
swap_in(size_t used_index, void* kaddr) {
    lock_acquire(&swap_lock);
    if (bitmap_test(swap_map, used_index) == false) {
        PANIC ("invalid access to unassigned space");
    }
    for (int i = 0; i<SECTOR_SIZE; i++) {
        block_read(swap_map, used_index * SECTOR_SIZE + i,
                        kaddr + i * BLOCK_SECTOR_SIZE);
        
    }
    bitmap_set(swap_map, used_index, false);
    lock_release(&swap_lock);

}

// swap out
size_t
swap_out(void* kaddr) {
    lock_acquire(&swap_lock);
    
    size_t swap_index = bitmap_scan(swap_map, 0, 1, false);

    for (int i = 0; i<SECTOR_SIZE; i++) {
        block_write(swap_map, swap_index * SECTOR_SIZE + i,
                            kaddr + i * BLOCK_SECTOR_SIZE );
    }
    bitmap_set(swap_map, swap_index, true);
    lock_release(&swap_lock);

    return swap_index;
}

