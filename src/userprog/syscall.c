#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"

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

  // printf("syscall num : %d\n", *syscall_number);

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
      break;                   
    case SYS_WAIT:
      break;                   
    case SYS_CREATE:
    {
      uint32_t *file = f->esp + 4;
      uint32_t *initial_size = f->esp + 4;
      check_address(file);
      check_address(initial_size);
      f->eax = sys_create((const char *) *file, (unsigned) *initial_size);
      break;
    }      
    case SYS_REMOVE:                 
      break;
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
      f->eax = sys_write((int)*fd, (void *)*buffer, (unsigned)*size);
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
  thread_exit();
}

// pid_t
// exec (const char *cmd_line) {
//   return 
// }

bool
sys_create (const char *file, unsigned initial_size) {
  return filesys_create(file, initial_size);
}


int
sys_read (int fd, const void *buffer, unsigned size) {

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
