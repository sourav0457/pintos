#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

struct proc_file {
    struct file* ptr;
    int fd;
    struct list_elem elem;
};

#endif /* userprog/syscall.h */
