#ifndef VM_PAGE_H
#define VM_PAGE_H
#include "filesys/file.h"
#include "lib/kernel/hash.h"

#define VM_BIN 0
#define VM_FILE 1
#define VM_ANON 2

struct vm_entry{
	uint8_t type; /* VM_BIN, VM_FILE, VM_ANON */
	void *vaddr; /* virtual page address */
	bool writable; /* whether write is enable */
	bool is_loaded; /* whether it is loaded */
	struct file* file; /* file mapped */
	struct list_elem mmap_elem; //list element for mmap_file
	size_t offset; /* offset in file */
	size_t read_bytes; /* size of reading data in page */
	size_t zero_bytes; /* bytes of zero padding */
	size_t swap_slot;
	struct hash_elem elem; /* hash element */
};

struct mmap_file {
	int mapid; // id of map
	struct file* file;	//file object of mapped file
	struct list_elem elem;	//list element for mmap in thread
	struct list vme_list;	//vme in mmap
};

struct page {
	void *kaddr; // physical address of page
	struct vm_entry *vme; // vm entry of vaddr that physical page is mapped
	struct thread *thread; // thread using this page
	struct list_elem lru; //field for list
};

void vm_init(struct hash *);
void vm_destroy(struct hash *);
struct vm_entry *find_vme(void *);
bool insert_vme(struct hash *, struct vm_entry*);
bool delete_vme(struct hash *, struct vm_entry*);
void check_valid_buffer(void *, unsigned, void *, bool);
void check_valid_string(const void *, void *);
bool load_file(void *, struct vm_entry *);
#endif /* vm/page.h */