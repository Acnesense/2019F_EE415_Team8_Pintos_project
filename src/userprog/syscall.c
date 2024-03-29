#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "userprog/process.h"
#include "threads/vaddr.h"
#include "vm/page.h"

static void gdbstp(void){ printf("???\n");}
static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  lock_init(&filesys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

int syscall_exec(const char * cmd_line)
{
	int tid=process_execute(cmd_line);
	return tid;
}

void syscall_exit(int exit_status)
{
	printf("%s: exit(%d)\n", thread_current()->name, exit_status);
	thread_current()->exit_status=exit_status;
	thread_exit();
}


struct vm_entry *
check_addr(void * addr, void * esp)
{
	if(!is_user_vaddr(addr)) syscall_exit(-1);
	struct vm_entry *res=find_vme(addr);
	if(res==NULL) syscall_exit(-1);
	return res;	
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
	    check_addr(esp+4, esp);
		syscall_exit(*(int*)(esp+4));
		break;
	  case SYS_EXEC:                   /* Start another process. */
	    check_addr(esp+4, esp);
		f->eax=syscall_exec(*(const char**)(esp+4));
		break;
	  case SYS_WAIT:                   /* Wait for a child process to die. */
	    check_addr(esp+4, esp);
		f->eax=process_wait(*(int*)(esp+4));
		break;
	  case SYS_CREATE:                 /* Create a file. */
	    check_addr(esp+4, esp);
		check_addr(esp+8, esp);
		if(*(const char **)(esp+4)==NULL) syscall_exit(-1);
	    f->eax=filesys_create(*(const char **)(esp+4),*(unsigned *)(esp+8));
	    break;
	  case SYS_REMOVE:                 /* Delete a file. */
	    check_addr(esp+4, esp);
		f->eax=filesys_remove(*(const char **)(esp+4));
		break;
      case SYS_OPEN:                  /* Open a file. */
	    check_addr(esp+4, esp);
		char * file= *(char **)(esp+4);
		if(file==NULL) syscall_exit(-1);
		dum_file=filesys_open(file);
		if(dum_file) 
		{
			if (strcmp(thread_current()->name, file) == 0) file_deny_write(dum_file);
			f->eax=process_add_file(dum_file);
		}
		else f->eax=-1;
		break;
	  case SYS_FILESIZE:               /* Obtain a file's size. */
	    check_addr(esp+4, esp);
		if(*(const char **)(esp+4)==NULL) syscall_exit(-1);
		dum_file=thread_current()->fdt[*(int*)(esp+4)];
		if(dum_file) f->eax=file_length(dum_file);
		else f->eax=-1;
		break;
	  case SYS_READ:                   /* Read from a file. */
		check_addr(esp+4, esp);
		//check_addr(esp+8);
		check_addr(esp+12, esp);
		check_valid_buffer(esp+8,*(unsigned *)(esp+12),esp, true);
		check_addr(*(int*)(esp+8), esp);
	    lock_acquire(&filesys_lock);
		fd=*(int*)(esp+4);
		if(fd==0) f->eax=input_getc();
		else if(fd<64 && fd>0) 
		{
			dum_file=thread_current()->fdt[fd];
			if(dum_file) f->eax=file_read(dum_file,*(void **)(esp+8),*(unsigned *)(esp+12));
			else f->eax=-1;
		}
		else f->eax=-1;
		
		lock_release(&filesys_lock);
		break;
      case SYS_WRITE:                  /* Write to a file. */
		check_addr(esp+4, esp);
		check_valid_string(esp+8, esp);
		check_addr(esp+12, esp);
		check_addr(*(int*)(esp+8), esp);
	    lock_acquire(&filesys_lock);
		fd=*(int*)(esp+4);
		if(fd==1)
		{
			putbuf(*(void **)(esp+8),*(unsigned *)(esp+12));
			f->eax=*(unsigned *)(esp+12);
		}
		else if(fd<64 && fd>0)
		{
			dum_file=thread_current()->fdt[fd];
			if(dum_file) f->eax=file_write(dum_file,*(void **)(esp+8),*(unsigned *)(esp+12));
			else f->eax=-1;
		}
		else f->eax=-1;
		lock_release(&filesys_lock);
		break;
	  case SYS_SEEK:                   /* Change position in a file. */
		check_addr(esp+4, esp);
		check_addr(esp+8, esp);
		fd=*(int*)(esp+4);
		if(fd<64 && fd>0)
		{
			dum_file=thread_current()->fdt[fd];
			file_seek(dum_file,*(int *)(esp+8));
		}
		else syscall_exit(-1);
		break;
      case SYS_TELL:                   /* Report current position in a file. */
		check_addr(esp+4, esp);	    
		fd=*(int *)(esp+4);
		if(fd<64 && fd>0)
		{
			dum_file=thread_current()->fdt[fd];
			file_tell(dum_file);
		}
		else syscall_exit(-1);
		break;
      case SYS_CLOSE:                  /* Close a file. */
		check_addr(esp+4, esp);	   
		fd=*(int *)(esp+4);
		if(fd<64 && fd>0)
		{
			dum_file=thread_current()->fdt[fd];
			file_close(dum_file);
			thread_current()->fdt[fd]=NULL;
		}
		else syscall_exit(-1);
		break;
  }
}
