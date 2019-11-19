#include <list.h>
#include "vm/page.h"
#include "threads/palloc.h"
#include "threads/thread.h"
// #include "threads/vaddr.h"
// #include "userprog/process.h"
#include "vm/frame.h"

void
lru_list_init(void) {
    list_init(&lru_list);
    lock_init(&lru_lock);
    lru_clock = NULL;
}

void
add_page_to_lru_list (struct page* page) {
    list_push_back(&lru_list, &page->lru_elem);
}

void
delete_page_to_lru_list (struct page* page) {
    list_remove(&page->lru_elem);
}

struct
page* alloc_page(enum palloc_flags flags, struct vm_entry *vme) {

    void *kpage = palloc_get_page (flags);
    struct page *page;
    lock_acquire(&lru_lock);
    page = malloc(sizeof(struct page));
    page->thread = thread_current();
    page->kaddr = kpage;
    page->vme = vme;

    if (kpage) {
        add_page_to_lru_list(page);
    }

    lock_release(&lru_lock);
    return page;
}

void
free_page(void *kaddr) {
    struct list_elem *e;
    struct page *page = NULL;
    lock_acquire(&lru_lock);
    for (e = list_begin (&lru_list); e != list_end (&lru_list);
       e = list_next (e))
    {
        page = list_entry(e, struct page, lru_elem);
        if (page->kaddr == kaddr) {
            break;
        }
    }
    list_remove(&page->lru_elem);
    palloc_free_page (page->kaddr);
    lock_release(&lru_lock);
}

static struct
list_elem* get_next_lru_clock() {
    lock_acquire(&lru_lock);
    if (lru_clock == NULL) {
        lru_clock = list_begin(&lru_list);
    }
    else if (lru_clock == list_end(&lru_list)) {
        lcok_release(&lru_lock);
        return NULL;
    }
    else {
        lru_clock = list_next(lru_clock);
    }
    lock_release(&lru_lock);
    return lru_clock;
}
