#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include <syscall-nr.h>

typedef int pid_t;

void sys_halt (void);
void sys_exit (int status);
pid_t sys_exec (const char *file);
int sys_wait (tid_t);
bool sys_create (const char *file, unsigned initial_size);
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
      break;
    case SYS_FILESIZE:
      break;
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
      break;
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
sys_read (int fd, void *buffer, unsigned size) {

  int i, real_size;
  if (fd == 0) {
    for (i = 0, real_size = 0; i < size; i++){
      if (((char *)buffer)[i] == '\0'){
        break;
      }
      real_size++;
    }
    return real_size;
  }
  return -1; 

}

int
sys_write (int fd, const void *buffer, unsigned size) {
  if (fd == 1) {
    putbuf(buffer, size);
    return size;
  }
  return -1; 
}

void
check_address(void *address) {
  if (!is_user_vaddr(address)) {
    sys_exit(-1);
  }
}


