#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "vm/page.h"

struct thread * get_child_process(int);
void remove_child_process(struct thread*);
tid_t process_execute (const char *);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
int process_add_file(struct file*);
bool handle_mm_fault(struct vm_entry *);


#endif /* userprog/process.h */
