#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);
struct file_descriptor_mapper{
    struct list_elem elem_file;
    struct file * address;
    int file_descriptor;
};

struct lock file_lock;

void is_valid_add(const void * ptr){
    if(ptr == NULL || ptr<(void * )0x08048000 || !is_user_vaddr(ptr)){
        exit(-1);
    }
}

void
syscall_init (void) 
{
  lock_init(&file_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int * p = f->esp;
  is_valid_add(p);
  int t = *p++;
  switch(t){
      case SYS_HALT:
          break;
      case SYS_EXIT:
           break;
      case SYS_WAIT:
          break;
      default:
          break;
  }
  printf ("system call!\n");
  thread_exit ();
}

void exit (int exit_status)
{
    thread_current()->exit_status = exit_status;
    printf("%s: exit(%d)\n", thread_current()->name, exit_status);
    thread_exit ();
}