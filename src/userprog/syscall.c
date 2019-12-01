#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "process.h"
#include "userprog/pagedir.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

static void syscall_handler (struct intr_frame *);
static void exit(int exit_status);
static int read(int fd, void *buffer, unsigned length);
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
  int *addr = f->esp;
  int args[3];
  void * phys_page_ptr;
  is_valid_add((const void*) addr);
  switch(*addr){
    case SYS_HALT:
      shutdown_power_off();
    case SYS_WAIT:
      is_valid_add((const void *) ++addr);
      f->eax = process_wait(*addr);
      break;
    case SYS_CREATE:
      // int *temp = addr;
      // for(int i=0; i< 2; i++, temp++){
      //   is_valid_add((const void*) temp);
      // }
      // is_valid_add((const void *) *addr);
      get_stack_arguments(f, &args[0], 2);
      check_buffer((void *)args[0], args[1]);
      phys_page_ptr=pagedir_get_page(thread_current()->pagedir, (const void *) args[0]);
      if(phys_page_ptr == NULL){
        exit(-1);
      }
      args[0] = (int) phys_page_ptr;
      lock_acquire(&file_lock);
      f->eax = filesys_create((const char *) args[0], (unsigned) args[1]);
      lock_release(&file_lock);
      break;
    case SYS_READ:
      get_stack_arguments(f, &args[0], 3);
      check_buffer((void *)args[1], args[2]);
      phys_page_ptr=pagedir_get_page(thread_current()->pagedir, (const void *) args[1]);
      if(phys_page_ptr == NULL)
        exit(-1);
      args[1] = (int) phys_page_ptr;
      f->eax = read(args[0], (void *) args[1], (unsigned) args[2]);
      
      break;
  }
  thread_exit ();
}

int read(int fd, void *buffer, unsigned length) {
  struct list_elem *temp;
  lock_acquire(&file_lock);
  if(fd == 0){
    lock_release(&file_lock);
    return (int) input_getc();
  }
  if(fd==1 || list_empty(&thread_current()->file_descriptors)){
    lock_release(&file_lock);
    return 0;
  }
  for(temp=list_front(&thread_current()->file_descriptors); temp!= NULL; temp = temp-> next){
    struct file_descriptor_mapper *t = list_entry (temp, struct file_descriptor_mapper, elem_file);
    if(t->file_descriptor == fd){
      lock_release(&file_lock);
      int bytes = (int) file_read(t->address, buffer, length);
      return bytes;
    }
  }
  lock_release(&file_lock);
  return -1;
}

void exit (int exit_status)
{
    thread_current()->exit_status = exit_status;
    thread_exit ();
}

void check_buffer (void *buff_to_check, unsigned size)
{
  unsigned i;
  char *ptr  = (char * )buff_to_check;
  for (i = 0; i < size; i++)
    {
      check_valid_addr((const void *) ptr);
      ptr++;
    }
}

void get_stack_arguments (struct intr_frame *f, int *args, int num_of_args)
{
  int i;
  int *ptr;
  for (i = 0; i < num_of_args; i++)
    {
      ptr = (int *) f->esp + i + 1;
      check_valid_addr((const void *) ptr);
      args[i] = *ptr;
    }
}