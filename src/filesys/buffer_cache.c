#include "filesys/buffer_cache.h"
#include "threads/synch.h"

// Size of block cache
#define block_cache_size 64

struct lock 

/* 
  Allocate memory to buffer cache.
  Use array for all block cache.
*/
struct buffer_head buffer_table[block_cache_size]

/*
  Initialize buffer cache.
  Set all flags of is_used to false
*/
void
bc_init(void) {
    int i;

    for (i = 0; i<block_cache_size; i++) {
        buffer_table[i].is_used = false;
    }
}

void
bc_term(void) {

}

struct buffer_head*
bc_select_victim (void) {
    
}

struct buffer_head*
bc_lookup (block_sector_t sector) {
    int i;

    for (i=0; i<block_cache_size; i++) {
        if buffer_table[i].sector == sector
            return buffer_table[i];
    }
    return NULL;
}
