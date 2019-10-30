#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

struct lock filesys_lock;
int exec(const char *);
void exit(int);
void check_addr(void *);
void syscall_init (void);

#endif /* userprog/syscall.h */
