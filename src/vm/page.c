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
        vme = list_entry (e, struct vm_entry, page_entry_elem);
        list_remove(e);
        free(vme);
    }
}

void
destroy_mmap_list (struct list *mmap_list) {
    struct thread *cur = thread_current ();
    struct list_elem *e;
    struct list_elem *mmap_e;
    struct vm_entry *vme;
    while(!list_empty(mmap_list)) {
        e = list_begin(mmap_list);
        struct mmap_file *mmap_f = list_entry (e, struct mmap_file, elem);

        while(!list_empty(&mmap_f->vme_list)) {
            mmap_e = list_begin(&mmap_f->vme_list);
            vme = list_entry (mmap_e, struct vm_entry, mmap_elem);
            list_remove(mmap_e);
        }
        list_remove(e);
        free(mmap_f);
    }
}


bool
handle_mm_fault (struct vm_entry *vme) {
    if (vme == NULL) {
        return false;
    }
    if(vme->is_loaded) {
        return true;
    }
    uint8_t *kpage = palloc_get_page (PAL_USER);
    if (kpage == NULL)
        return false;
    int type_number = vme->type;

    switch (type_number) {
        case VM_BIN:
        {
            if (!load_file(kpage, vme)) {
                return false;
            }
            if (!install_page (vme->vaddr, kpage, vme->writable)) {
                palloc_free_page (kpage);
                printf("install page");
                return false;
            }
            return true;
        }
        case VM_FILE:
            if (!load_file(kpage, vme)) {
                return false;
            }
            if (!install_page (vme->vaddr, kpage, vme->writable)) {
                palloc_free_page (kpage);
                return false;
            }
            return true;
        case VM_ANON:
            return false;
    }
}

bool
load_file (void *kaddr, struct vm_entry *vme) {
    // printf("%d\n", vme->vaddr);

    if(vme->file == NULL) {
        return false;
    }
    if((int)vme->offset < 0) {
        return false;
    }
    lock_acquire(&filesys_lock);
    file_seek (vme->file, vme->offset);
    int read = file_read(vme->file, kaddr, vme->read_bytes);
    if (read != (int) vme->read_bytes) {
        lock_release(&filesys_lock);

        return false;
    }
    lock_release(&filesys_lock);

    ASSERT(vme->read_bytes + vme->zero_bytes == PGSIZE);
    memset (kaddr + vme->read_bytes, 0, vme->zero_bytes);
    vme->is_loaded = true;
    return true;
}

static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}


bool
expand_stack (void *vaddr) {
    // printf("expand stack\n");

    if ((size_t) (PHYS_BASE - pg_round_down(vaddr)) > MAX_STACK_SIZE) {
        sys_exit(-1);
        goto fail;
    }

    uint8_t *kpage;
    struct vm_entry *vme = malloc(sizeof(struct vm_entry));

    vme->vaddr = pg_round_down(vaddr);
    vme->is_loaded = false;
    vme->type = VM_BIN;
    // printf("%d", vme->vaddr);
    insert_vme(&thread_current()->page_entry_list, vme);

    kpage = palloc_get_page (PAL_USER | PAL_ZERO);
    if (kpage != NULL) 
        {
        if (!(install_page (vme->vaddr, kpage, true))) {
            palloc_free_page (kpage);
            goto fail;
        }
    }
    return true;
fail:
    return false;
}