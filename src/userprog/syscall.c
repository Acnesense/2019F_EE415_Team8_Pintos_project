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

struct vm_entry*
check_address(void *address) {
  struct thread *cur = thread_current();
  if (address < (void *)0x08048000 || address >= (void *)0xc0000000)
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