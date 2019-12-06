#include "filesys/filesys.h"

/* store file info to buffer head structure */
struct buffer_head {
    struct inode* inode;
    bool dirty;
    bool is_used;
    block_sector_t sector;
    void* data;
};

void bc_init(void);
void bc_term(void);
struct buffer_head* bc_select_victim (void);
struct buffer_head* bc_lookup (block_sector_t sector);


