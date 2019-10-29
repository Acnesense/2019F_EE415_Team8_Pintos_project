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

void check_address(void *address);
static void syscall_handler (struct intr_frame *);


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  // lock_init(&filesys_lock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int *syscall_number;
  syscall_number = f->esp;


  switch (*syscall_number){
    case SYS_HALT:
      sys_halt();
    case SYS_EXIT:
    {
      uint32_t *exit_status = f->esp + 4;
      check_address(exit_status);
      sys_exit((int) *exit_status);
    }

    case SYS_EXEC:
    {
      uint32_t *file = f->esp + 4;
      check_address(file);
      f->eax = sys_exec((const char *) *file);
      break;
    }
    case SYS_WAIT:
    {
      uint32_t *pid = f->esp + 4;
      check_address(pid);
      f->eax = sys_wait ((pid_t) *pid);
      break;
    }

    case SYS_CREATE:
    {
      uint32_t *file = f->esp + 4;
      uint32_t *initial_size = f->esp + 8;
      check_address(file);
      check_address(initial_size);
      f->eax = sys_create((const char *) *file, (unsigned) *initial_size);
      break;
    }      
    case SYS_REMOVE:
    {
      uint32_t *file = f->esp + 4;
      check_address(file);
      f->eax = sys_remove((const char *) *file);
      break;
    }
    case SYS_OPEN:
    {
      uint32_t *file = f->esp + 4;
      check_address(file);
      f->eax = sys_open((const char *) *file);
      break;
    }                
    case SYS_FILESIZE:
    {
      uint32_t *fd = f->esp + 4;
      check_address(fd);
      f->eax = sys_filesize((int) *fd);
      break;
    }
    case SYS_READ:
    {
      uint32_t *fd = f->esp + 4;
      uint32_t *buffer = f->esp + 8;
      uint32_t *size = f->esp + 12;
      check_address(fd);
      check_address(buffer);
      check_address(size);
      f->eax = sys_read((int)*fd, (void *)*buffer, (unsigned)*size);
      break;
    }
    case SYS_WRITE:
    {
      uint32_t *fd = f->esp + 4;
      uint32_t *buffer = f->esp + 8;
      uint32_t *size = f->esp + 12;
      check_address(fd);
      check_address(buffer);
      check_address(size);
      f->eax = sys_write((int)*fd, (void *) *buffer, (unsigned)*size);
      break;
    }
      
    case SYS_SEEK:
      break;
    case SYS_TELL:
      {
        uint32_t *fd = f->esp + 4;
        check_address(fd);
        f->eax = sys_tell((int) *fd);
        break;
      }
    case SYS_CLOSE:
      break;
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
  struct file *f = filesys_open(file);
  if (f == NULL) {
    return -1;
  }
  else {
    return process_add_file(f);
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
  int i;
  if (fd == 0) {
    for (i = 0; i < size; i ++) {
      if (((char *)buffer)[i] == '\0') {
        break;
      }
    }
  }
  return i;

  // // lock_acquire(&filesys_lock);

  // int i, real_size;

  // if (fd == 0) {
  //   return input_getc();
  // }
  // else {
  //   struct file *f = process_get_file(fd);
  //   return file_read(f, buffer, size);
  // }
  // return -1; 
}

int
sys_write (int fd, const void *buffer, unsigned size) {

  if (fd == 1) {
    putbuf(buffer, size);
    return size;
  }
  return -1;

  // // lock_acquire(&filesys_lock);

  // int i, real_size;

  // if (fd == 0) {
  //   putbuf(buffer, size);
  //   return size;
  // }
  // else {
  //   struct file *f = process_get_file(fd);
  //   return file_write(f, buffer, size);
  // }
  // return -1;

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
check_address(void *address) {
  if (!is_user_vaddr(address)) {
    sys_exit(-1);
  }
}


