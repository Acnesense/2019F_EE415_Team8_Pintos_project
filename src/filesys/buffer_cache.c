#include <debug.h>
#include <string.h>
#include "filesys/buffer_cache.h"
#include "filesys/filesys.h"
#include "threads/synch.h"
#include "devices/block.h"

// Size of block cache
#define block_cache_size 64

struct lock buffer_table_lock;

/* 
  Allocate memory to buffer cache.
  Use array for all block cache.
*/
struct buffer_head buffer_table[block_cache_size];

/*
  Initialize buffer cache.
  Set all flags of is_used to false
*/
void
bc_init(void) {
    lock_init(&buffer_table_lock);
    int i;

    for (i = 0; i<block_cache_size; i++) {
        buffer_table[i].is_used = false;
        buffer_table[i].dirty = false;
    }
}

/*
    Flush all buffer head to disk.
    Free all buffer cache and buffer cache array
*/

// void
// bc_term(void) {
//     lock_acquire(&buffer_table_lock);
//     int i = 0;
//     bc_flush_all_entries();

//     for (i=0; i<block_cache_size; i++) {
//         free(buffer_table[i]);
//     }
//     free(buffer_table);

//     lock_release(&buffer_table_lock);
// }



/*
    Return buffer_head whose sector number is same to input.
*/

struct buffer_head*
bc_lookup (block_sector_t sector) {
    int i;

    for (i=0; i<block_cache_size; i++) {
        if (buffer_table[i].is_used) {
            continue;
        }
        if (buffer_table[i].sector == sector) {
            return &buffer_table[i];
        }
    }
    return NULL;
}

/*
    Flush buffer cache data to disk.
    Use block write and update dirty flag.
*/

void
bc_flush_entry (struct buffer_head *flush_entry, struct block *fs_device) {
    if (flush_entry->dirty) {
        block_write (fs_device, flush_entry->sector, flush_entry->data);
        flush_entry->dirty = false;
    }
}

/*
    If dirty is true, flush buffer cache to disk.
*/

void
bc_flush_all_entries (struct block *fs_device) {
    int i;
    for(i = 0; i<block_cache_size; i++) {
        if (buffer_table[i].dirty) {
            bc_flush_entry(&buffer_table[i], fs_device);
        }
    }
}

/*
    Select victim buffer_head using clock algorithm.
    If dirty of victim entry is true, flush it to disk.
*/

struct buffer_head*
bc_select_victim (struct block *fs_device) {
    int i = 0;
    struct buffer_head *b_head;
    
    while(1) {
        i = i % block_cache_size;
        b_head = &buffer_table[i];
        if (b_head->is_used == false) {
            return b_head;
        }
        // if (b_head->accessed == true) {
        //     b_head->accessed = false;
        // }
        else {
            break;
        }
        i++;
    }
    if (b_head->dirty) {
        bc_flush_entry(b_head, fs_device);
    }
    // b_head->accessed = true;
    b_head->is_used = false;
    return b_head;
}

/*
    Firstly, search sector index in buffer table.
    If there is not, get buffer head of buffer entry for caching
    and read disk block data to buffer cache.
    Copy disk block data to buffer.
    Set flag of clock bit (accessed) to true.
*/

void
bc_read (block_sector_t sector_idx, void *buffer,
struct block *fs_device) {
    
    lock_acquire(&buffer_table_lock);
    // block_read(fs_device, sector_idx, buffer);

    struct buffer_head *b_head;
    b_head = bc_lookup(sector_idx);
    if (b_head == NULL) {
        b_head = bc_select_victim(fs_device);
        
        b_head -> dirty = false;
        b_head -> is_used = true;
        b_head -> sector = sector_idx;
        block_read(fs_device, sector_idx, b_head->data);

    }

    b_head -> accessed = true;
    memcpy(buffer, b_head->data, BLOCK_SECTOR_SIZE);

    lock_release(&buffer_table_lock);
}

/*
    Firstly, search sector index in buffer table.
    If there is not, get buffer head of buffer entry for caching
    and write disk block data to buffer cache.
    Copy disk block data to buffer.
    Set flag of clock bit (accessed) to true.
*/

void
bc_write (block_sector_t sector_idx, void *buffer,
struct block *fs_device) {
    
    lock_acquire(&buffer_table_lock);
    // block_write(fs_device, sector_idx, buffer);

    struct buffer_head *b_head;
    b_head = bc_lookup(sector_idx);
    if (b_head == NULL) {
        b_head = bc_select_victim(fs_device);
        b_head -> is_used = true;
        b_head -> sector = sector_idx;
    }

    b_head -> dirty = true;
    b_head -> accessed = true;

    memcpy(b_head->data, buffer, BLOCK_SECTOR_SIZE);
    // block_write(fs_device, sector_idx, b_head->data);

    lock_release(&buffer_table_lock);
}
