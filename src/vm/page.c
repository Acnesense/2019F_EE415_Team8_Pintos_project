#include <list.h>
#include "vm/page.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/process.h"


void
insert_vme (struct list *page_entry_list, struct vm_entry *vme) {
    list_push_back(page_entry_list, &vme->page_entry_elem);
}

bool
delete_vme (struct list *page_entry_list, struct vm_entry *vme) {
    struct list_elem *e;
    for (e = list_begin (&page_entry_list); e != list_end (&page_entry_list);
       e = list_next (e))
    {
        struct vm_entry *vme_elem = list_entry(e, struct vm_entry, page_entry_elem);
        if (vme_elem->vaddr == vme->vaddr) {
            list_remove(&vme_elem->page_entry_elem);
            return true;
        }
    }
    return false;
}

struct vm_entry *
find_vme (struct list *page_entry_list, void *vaddr) {
    vaddr = pg_round_down(vaddr);
    struct list_elem *e;
    for (e = list_begin (page_entry_list); e != list_end (page_entry_list);
       e = list_next (e))
    {
        struct vm_entry *vme = list_entry (e, struct vm_entry, page_entry_elem);
        if (vme->vaddr == vaddr) {
            return vme;
        }
    }
    return NULL;
}

void
destroy_vme (struct list *page_entry_list) {
    struct list_elem *e;
    struct vm_entry *vme;
    while(!list_empty(page_entry_list)) {
        e = list_begin(page_entry_list);
        list_remove(e);
        vme = list_entry (e, struct vm_entry, page_entry_elem);
        free(vme);
    }
}