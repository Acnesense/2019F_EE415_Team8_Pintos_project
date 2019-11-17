#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "threads/synch.h"
#include "userprog/process.h"
#include <syscall-nr.h>
#include "filesys/file.h"
#include "vm/page.h"


typedef int pid_t;

void sys_halt (void); // pass
void sys_exit (int status); // pass
pid_t sys_exec (const char *file); // pass
int sys_wait (tid_t); // pass
bool sys_create (const char *file, unsigned initial_size); // pass
bool sys_remove (const char *file);
int sys_open (const char *file);
int sys_filesize (int fd);
int sys_read (int fd, void *buffer, unsigned length);
int sys_write (int fd, const void *buffer, unsigned length);
void sys_seek (int fd, unsigned position);
unsigned sys_tell (int fd);
void sys_close (int fd);
int sys_mmap (int *fd, void *addr);
void sys_munmap(int *mapid);
void do_munmap(struct mmap_file *mmap_f);

struct vm_entry *check_address(void *address);
static void syscall_handler (struct intr_frame *);

struct file 
  {
    struct inode *inode;        /* File's inode. */
    off_t pos;                  /* Current position. */
    bool deny_write;            /* Has file_deny_write() been called? */
  };

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&filesys_lock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int arg[3];
  int *syscall_number;
  syscall_number = f->esp;


  switch (*syscall_number){
    case SYS_HALT:
      sys_halt();
    case SYS_EXIT:
    {
      get_arg(f->esp, &arg, 1);
      sys_exit(arg[0]);
    }

    case SYS_EXEC:
    {
      get_arg(f->esp, &arg, 1);
      f->eax = sys_exec(arg[0]);
      break;

    }
    case SYS_WAIT:
    {
      get_arg(f->esp, &arg, 1); 
      f->eax = sys_wait (arg[0]);
      break;
    }

    case SYS_CREATE:
    {
      get_arg(f->esp, &arg, 2);
      f->eax = sys_create(arg[0], arg[1]);
      break;
    }      
    case SYS_REMOVE:
    {
      get_arg(f->esp, &arg, 1);
      f->eax = sys_remove(arg[0]);
      break;
    }
    case SYS_OPEN:
    {
      get_arg(f->esp, &arg, 1);
      f->eax = sys_open(arg[0]);
      break;
    }                
    case SYS_FILESIZE:
    {
      get_arg(f->esp, &arg, 1);
      f->eax = sys_filesize(arg[0]);
      break;
    }
    case SYS_READ:
    {
      get_arg(f->esp, &arg, 3);
      check_valid_buffer((void *) arg[1], (unsigned) arg[2], true);
      f->eax = sys_read(arg[0], arg[1], arg[2]);
      break;
    }
    case SYS_WRITE:
    {
      get_arg(f->esp, &arg, 3);
      check_valid_buffer((void *) arg[1], (unsigned) arg[2], true);
      f->eax = sys_write(arg[0], arg[1], arg[2]);
      break;
    }
      
    case SYS_SEEK:
      {
        get_arg(f->esp, &arg, 2);
        sys_seek(arg[0], arg[1]);
        break;
      }
    case SYS_TELL:
      {
        get_arg(f->esp, &arg, 1);
        f->eax = sys_tell(arg[0]);
        break;
      }
    case SYS_CLOSE:
      {
        get_arg(f->esp, &arg, 1);
        sys_close(arg[0]);
        break;
      }
    case SYS_MMAP:
      {
        get_arg(f->esp, &arg, 2);
	      f->eax = sys_mmap(arg[0], arg[1]);
        break;
      }
    case SYS_MUNMAP:
      {
        get_arg(f->esp, &arg, 1);
        sys_munmap((int) arg[0]);
        break;
      }
  }
}

void
sys_halt(void) {
  shutdown_power_off();
}

void
sys_exit(int status) {
  printf("%s: exit(%d)\n", thread_current()->name, status);
  thread_current()->exit_status = status;
  thread_exit();
}

pid_t
sys_exec (const char *cmd_line) {
  int pid;
  struct thread *cur = thread_current ();
  // if(!list_empty(&cur->mmap_list)){
  //   printf("\n\nexec this list is not empty\n\n");
  // }
  // printf("exec %d\n", thread_tid());
  pid = process_execute(cmd_line);
  return pid;
}

int
sys_wait (pid_t pid) {
  int exit_status;
  exit_status = process_wait(pid);
  return exit_status;
}

bool
sys_create (const char *file, unsigned initial_size) {
  if (file == NULL) {
    sys_exit(-1);
  }
  return filesys_create(file, initial_size);
}

bool
sys_remove (const char *file) {
  return filesys_remove(file);
}

int
sys_open(const char *file) {
  if (file == NULL) {
    sys_exit(-1);
  }

  lock_acquire(&filesys_lock);
  struct file *f = filesys_open(file);
  if (f == NULL) {
    lock_release(&filesys_lock);
    return -1;
  }
  else {
    if (strcmp(thread_current()->name, file) == 0) {
      file_deny_write(f);
    }
    int fd = process_add_file(f);
    lock_release(&filesys_lock);
    return fd;
  }
}

int
sys_filesize (int fd) {
  struct file *f = process_get_file(fd);
  if (f == NULL) {
    return -1;
  }
  return file_length(f);
}

int
sys_read (int fd, void *buffer, unsigned size) {
  check_address((uint32_t *) buffer);
  // check_address((uint32_t *) buffer + size - 1);

  lock_acquire(&filesys_lock);

  int i, real_size;

  if (fd == 0) {
    real_size = input_getc();
    lock_release(&filesys_lock);
    return real_size;
  }
  else {
    struct file *f = process_get_file(fd);

    real_size = file_read(f, buffer, size);
 
    lock_release(&filesys_lock);
    return real_size;
  }
  lock_release(&filesys_lock);

  return -1; 
}

int
sys_write (int fd, const void *buffer, unsigned size) {

  lock_acquire(&filesys_lock);


  int i, real_size;

  if (fd == 1) {
    putbuf(buffer, size);
    lock_release(&filesys_lock);
    return size;
  }
  else {
    struct file *f = process_get_file(fd);

    real_size = file_write(f, buffer, size);
    lock_release(&filesys_lock);
    return real_size;
  }

  return -1;

}

void
sys_seek (int fd, unsigned position) {
  struct file *f = process_get_file(fd);
  if (f == NULL) {
    return -1;
  }
  file_seek(f, position);
}

unsigned
sys_tell (int fd) {
  struct file *f = process_get_file(fd);
  if (f == NULL) {
    return -1;
  }
  return file_tell(f);
}

void
sys_close (int fd) {
  process_close_file(fd);
}

int
sys_mmap (int *fd, void *addr) {
  // printf("%d", addr);
  if (addr == NULL || pg_ofs(addr) != 0)
    return -1;
  if (fd <= 1)
    return -1;

  struct mmap_file *mmap_f;
  struct file *f;
  struct file *f_copy;
  struct thread *cur = thread_current();
  mmap_f = malloc(sizeof(struct mmap_file));

  f = process_get_file(fd);

  lock_acquire(&filesys_lock);

  if ( f == NULL ) {
    goto mmap_fail;
  }
  f_copy = file_reopen(f);
  size_t f_length = file_length(f_copy);
  if ( f_length == 0 ) {
    goto mmap_fail;
  }

  size_t ofs;
  uint32_t page_read_bytes;
  uint32_t page_zero_bytes;
  list_init(&(mmap_f->vme_list));
// store file information to vme structure
// insert new vme to vme_list in mmap_f
  for (ofs = 0; ofs < f_length; ofs = ofs + PGSIZE) {
    void *upage = addr + ofs;
    
    if ( find_vme(&cur->page_entry_list, upage) != NULL ) {
      goto mmap_fail;
    }

    struct vm_entry *vme;
    vme = malloc(sizeof(struct vm_entry));

    if (f_length - ofs < PGSIZE) {
      page_read_bytes = f_length - ofs;
      page_zero_bytes = PGSIZE - page_read_bytes;
    }
    else {
      page_read_bytes = PGSIZE;
      page_zero_bytes = 0;
    }
    vme->vaddr = upage;
    vme->offset = ofs;
    vme->read_bytes = page_read_bytes;
    vme->zero_bytes = page_zero_bytes;
    vme->file = f_copy;
    vme->is_loaded = false;
    vme->writable = true;
    vme->type = VM_FILE;
    vme->mmap = true;
    // printf("%d\n", vme->vaddr);

    list_push_back(&(mmap_f->vme_list), &(vme->mmap_elem));
    list_push_back(&(cur->page_entry_list), &(vme->page_entry_elem));
  }
  cur->max_mid += 1;
  mmap_f->file = f_copy;
  mmap_f->mapid = cur->max_mid;
  list_push_back(&(cur->mmap_list), &(mmap_f->elem));
  lock_release(&filesys_lock);

  
  return cur->max_mid;

mmap_fail:
  lock_release(&filesys_lock);
  return -1;
}

void
sys_munmap(int *mapid) {
  
  struct thread *cur = thread_current();
  struct list_elem *e;

  while(!list_empty(&cur->mmap_list)) {
    e = list_begin (&cur->mmap_list);
    struct mmap_file *mmap_f = list_entry (e, struct mmap_file, elem);
    if (mmap_f->mapid == mapid) {
      do_munmap(mmap_f);
    }
    list_remove(&mmap_f->elem);
    file_close(mmap_f->file);
    free(mmap_f);
  }
}

void
do_munmap(struct mmap_file *mmap_f) {
  struct list_elem *mmap_e;
  struct thread *cur = thread_current();

  while(!list_empty(&mmap_f->vme_list)) {
    mmap_e = list_begin (&mmap_f->vme_list);
    struct vm_entry *vme = list_entry(mmap_e, struct vm_entry, mmap_elem);
    if (vme->is_loaded) {
      if (pagedir_is_dirty(cur->pagedir, vme->vaddr)){
        lock_acquire(&filesys_lock);
        file_write_at(vme->file, vme->vaddr, vme->read_bytes, vme->offset);
        lock_release(&filesys_lock);
      }
      pagedir_clear_page(cur->pagedir, vme->vaddr);
    }
    list_remove(&vme->mmap_elem);
    list_remove(&vme->page_entry_elem);
    free(vme);
  }

}

struct vm_entry*
check_address(void *address) {
  struct thread *cur = thread_current();
  if (!is_user_vaddr(address))
    {
      sys_exit(-1);
    }
  return find_vme(&cur->page_entry_list, address);
}

void
check_valid_buffer (void * buffer, unsigned size, bool to_write) {
  void *upage;
  struct vm_entry *vme;

  for (upage = pg_round_down(buffer);upage < buffer + size; upage += PGSIZE) {
    vme = check_address(upage);
    if(vme == NULL) {
      sys_exit(-1);
    }
    handle_mm_fault(vme);
  }
}

void get_arg (void *esp, int *arg, int n)
{
  int i;
  int *ptr;
  for (i = 0; i < n; i++)
    {
      ptr = (int *) esp + i + 1;
      check_address((const void *) ptr);
      arg[i] = *ptr;
    }
}