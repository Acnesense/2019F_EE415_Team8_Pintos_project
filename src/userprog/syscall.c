#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "userprog/process.h"
#include "threads/vaddr.h"

static void gdbstp(void){ printf("???\n");}
static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  lock_init(&filesys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

int exec(const char * cmd_line)
{
	int tid=process_execute(cmd_line);
	struct thread * child=get_child_process(tid);
	if(child==NULL) return -1;
	sema_down(&child->load_lock);
	if(child->process_loaded) return tid;
	return -1;
}

void exit(int exit_status)
{
	printf("%s: exit(%d)\n", thread_current()->name, exit_status);
	thread_current()->exit_status=exit_status;
	thread_exit();
}

void check_addr(void * addr)
{
	if(!is_user_vaddr(addr)) exit(-1);
}

static void
syscall_handler (struct intr_frame *f) 
{
  void *esp=f->esp;
  int number=*(int *) esp;
  struct file* dum_file;
  int fd;

  switch(number)
  {
	  case SYS_HALT:            /* Halt the operating system. */
	    shutdown_power_off();
		break;
	  case SYS_EXIT:                   /* Terminate this process. */
	    check_addr(esp+4);
		exit(*(int*)(esp+4));
		break;
	  case SYS_EXEC:                   /* Start another process. */
	    check_addr(esp+4);
		f->eax=exec(*(const char**)(esp+4));
		break;
	  case SYS_WAIT:                   /* Wait for a child process to die. */
	    check_addr(esp+4);
		f->eax=process_wait(*(int*)(esp+4));
		break;
	  case SYS_CREATE:                 /* Create a file. */
	    check_addr(esp+4);
		check_addr(esp+8);
		if(*(const char **)(esp+4)==NULL) exit(-1);
	    f->eax=filesys_create(*(const char **)(esp+4),*(unsigned *)(esp+8));
	    break;
	  case SYS_REMOVE:                 /* Delete a file. */
	    check_addr(esp+4);
		f->eax=filesys_remove(*(const char **)(esp+4));
		break;
      case SYS_OPEN:                  /* Open a file. */
	    check_addr(esp+4);
		char * file= *(char **)(esp+4);
		if(file==NULL) exit(-1);
		dum_file=filesys_open(file);
		if(dum_file) 
		{
			if (strcmp(thread_current()->name, file) == 0) file_deny_write(dum_file);
			f->eax=process_add_file(dum_file);
		}
		else f->eax=-1;
		break;
	  case SYS_FILESIZE:               /* Obtain a file's size. */
	    check_addr(esp+4);
		if(*(const char **)(esp+4)==NULL) exit(-1);
		dum_file=thread_current()->fdt[*(int*)(esp+4)];
		if(dum_file) f->eax=file_length(dum_file);
		else f->eax=-1;
		break;
	  case SYS_READ:                   /* Read from a file. */
		check_addr(esp+4);
		check_addr(esp+8);
		check_addr(esp+12);
	    lock_acquire(&filesys_lock);
		fd=*(int*)(esp+4);
		if(fd==0) f->eax=input_getc();
		else if(fd<64 && fd>=0) 
		{
			dum_file=thread_current()->fdt[fd];
			if(dum_file) f->eax=file_read(dum_file,*(void **)(esp+8),*(unsigned *)(esp+12));
			else f->eax=-1;
		}
		else f->eax=-1;
		
		lock_release(&filesys_lock);
		break;
      case SYS_WRITE:                  /* Write to a file. */
		check_addr(esp+4);
		check_addr(esp+8);
		check_addr(esp+12);
	    lock_acquire(&filesys_lock);
		fd=*(int*)(esp+4);
		if(fd==1)
		{
			putbuf(*(void **)(esp+8),*(unsigned *)(esp+12));
			f->eax=*(unsigned *)(esp+12);
		}
		else if(fd<64 && fd>=0)
		{
			dum_file=thread_current()->fdt[fd];
			if(dum_file) f->eax=file_write(dum_file,*(void **)(esp+8),*(unsigned *)(esp+12));
			else f->eax=-1;
		}
		else f->eax=-1;
		lock_release(&filesys_lock);
		break;
	  case SYS_SEEK:                   /* Change position in a file. */
		check_addr(esp+4);
		check_addr(esp+8);
		fd=*(int*)(esp+4);
		if(fd<64 && fd>=0)
		{
			dum_file=thread_current()->fdt[fd];
			file_seek(dum_file,*(void **)(esp+8));
		}
		else exit(-1);
		break;
      case SYS_TELL:                   /* Report current position in a file. */
		check_addr(esp+4);	    
		fd=*(int *)(esp+4);
		if(fd<64 && fd>=0)
		{
			dum_file=thread_current()->fdt[fd];
			file_tell(dum_file);
		}
		else exit(-1);
		break;
      case SYS_CLOSE:                  /* Close a file. */
		check_addr(esp+4);	   
		fd=*(int *)(esp+4);
		if(fd<64 && fd>=0)
		{
			dum_file=thread_current()->fdt[fd];
			file_close(dum_file);
		}
		else exit(-1);
		break;
  }
}
