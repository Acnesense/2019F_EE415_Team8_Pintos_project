#include "vm/page.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "userprog/syscall.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static unsigned 
vm_hash_func(const struct hash_elem *e, void *aux)
{
	return hash_int((int) hash_entry(e, struct vm_entry, elem)->vaddr);
}

static bool 
vm_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux)
{
	return (hash_entry(a, struct vm_entry, elem)->vaddr < 
		hash_entry(b, struct vm_entry, elem)->vaddr);
}

static void 
vm_destroy_func(struct hash_elem *e, void *aux)
{
	struct vm_entry * vme=hash_entry(e, struct vm_entry, elem);
	free(vme);
}

void 
vm_init(struct hash *vm)
{
	hash_init(vm,vm_hash_func, vm_less_func, NULL);
}

void 
vm_destroy(struct hash *vm)
{
	hash_destroy(vm,vm_destroy_func);
}

struct vm_entry *
find_vme(void * vaddr)
{
	struct hash_elem *hash_dum;
	struct vm_entry dum;
	dum.vaddr=pg_round_down(vaddr);  
	hash_dum=hash_find(&thread_current()->vm, &dum.elem);
	if(hash_dum) return hash_entry(hash_dum, struct vm_entry, elem);
	else return NULL;
}

bool 
insert_vme(struct hash *vm, struct vm_entry *vme)
{
	return !(hash_insert(vm,&vme->elem));
}

bool 
delete_vme(struct hash *vm, struct vm_entry *vme)
{
	return hash_delete(vm,&vme->elem);
}

void 
check_valid_buffer(void *buffer, unsigned size, void *esp, bool to_write)
{
	int i;
	for(i=0;i<size;i++) 
	{
		struct vm_entry * vme=check_addr(buffer, esp);
		if(to_write&&!vme->writable) syscall_exit(-1);
	}
}

void 
check_valid_string(const void *str, void *esp)
{
	struct vm_entry * vme=check_addr(str, esp);
	if(vme==NULL) syscall_exit(-1);
}

/* A function to load segments of ELF format files properly in process virtual address space
	call when executrble file is loaded*/

bool
load_file(void *kaddr, struct vm_entry *vme)
{	
/* Using file_read_at()*/
/* Write physical memory as much as read_bytes by file_read_at*/
/* Return file_read_at status*/ 	
/* Pad 0 as much as zero_bytes*/
/* if file is loaded to memory, return true */
	if(file_read_at(vme->file,kaddr,vme->read_bytes,vme->offset)!= (int) vme->read_bytes)
		return false;
	memset (kaddr + vme->read_bytes, 0, vme->zero_bytes);
	vme->is_loaded=true;
	return true;
}
