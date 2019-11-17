#include <debug.h>
#include <list.h>
#include <stdint.h>
#include <hash.h>
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"


#define VM_BIN 0
#define VM_FILE 1
#define VM_ANON 2

#define MAX_STACK_SIZE (1 << 23)

struct lock *filesys_lock;


struct vm_entry {
    uint8_t type;   /* VM_BIN, VM_FILE, VM_ANON type */
    void *vaddr;    /* virtual page number */
    bool writable;  /* if Ture, can write. else if false can`t */

    bool is_loaded;  /* flag for loading physical memory */
    bool mmap; /* If Ture, vm entry is made in mmap function */
    struct file *file;  /* mapping file */
    struct list_elem mmap_elem;     /* element of mmap list */
    struct list_elem page_entry_elem;

    size_t offset;  /* file offset */
    size_t read_bytes;  /* size of data which is wrote in virtual page */
    size_t zero_bytes;  /* remaining byte of page */

    size_t swap_slot;
};

struct mmap_file {
    int mapid;
    struct file *file;
    struct list_elem elem;
    struct list vme_list;
};

void insert_vme(struct list *page_entry_list, struct vm_entry *vme);
bool delete_vme (struct list *page_entry_list, struct vm_entry *vme);
struct vm_entry *find_vme (struct list *page_entry_list, void *vaddr);
void destroy_vme (struct list *page_entry_list);
bool handle_mm_fault (struct vm_entry *vme);
bool load_file (void *kaddr, struct vm_entry *vme);
static bool install_page (void *upage, void *kpage, bool writable);
void destory_mmap_list (struct list *mmap_list);
void destory_mmap_list (struct list *mmap_list);
// bool delete_vme (struct hash *vm, struct vm_entry *vme);
// struct vm_entry *find_vme (void *vaddr);
// void vm_destroy (struct hash *vm);
// void vm_destroy_func (struct hash_elem *e, void *aux);
// bool handle_mm_fault (struct vm_entry *vme);
// bool load_file (void *kaddr, struct vm_entry *vme);


