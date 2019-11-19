#include "devices/block.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include <bitmap.h>

#define SECTOR_SIZE (PGSIZE / BLOCK_SECTOR_SIZE)

struct lock swap_lock;
struct block *block_swap;
struct bitmap *swap_map;
