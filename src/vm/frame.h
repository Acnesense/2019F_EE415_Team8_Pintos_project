#include <list.h>
#include "threads/vaddr.h"

struct list lru_list;
struct lock *lru_lock;
struct lise_elem *lru_clock;

struct page {
    void *kaddr;
    struct vm_entry *vme;
    struct thread *thread;
    struct list_elem lru_elem;
};

