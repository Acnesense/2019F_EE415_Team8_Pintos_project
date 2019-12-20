#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

// #include "filesys/filesys.h"
#include "devices/block.h"
#include <debug.h>
#include <string.h>
#include <stdbool.h>
#include "filesys/filesys.h"


/* store file info to buffer head structure */
struct buffer_head {
    struct inode* inode;
    bool dirty;
    bool is_used;
    bool accessed;
    block_sector_t sector;
    // void* data;
    uint8_t data[BLOCK_SECTOR_SIZE];
};

void bc_init(void);
void bc_term(void);
struct buffer_head* bc_select_victim (struct block *fs_device);
struct buffer_head* bc_lookup (block_sector_t sector);
void bc_flush_entry (struct buffer_head *flush_entry, struct block *fs_device);
void bc_flush_all_entries (struct block *fs_device);
void bc_read (block_sector_t sector_idx, void *buffer, int chunk_size, struct block *fs_device);

#endif