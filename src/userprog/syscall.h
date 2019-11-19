#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

struct lock filesys_lock;
int syscall_exec(const char *);
void syscall_exit(int);
struct vm_entry *check_addr(void *, void *);
void syscall_init (void);

#endif /* userprog/syscall.h */
