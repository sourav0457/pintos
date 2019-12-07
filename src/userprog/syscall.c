#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "list.h"
#include "process.h"

static void syscall_handler (struct intr_frame *);
void remove_sys(struct intr_frame *,const char * );
// void is_valid_add(const void*);
struct file_entry* file_list_entry(int fd);
void is_valid_add_multiple(int *, unsigned count);
void write_sys(struct intr_frame *, int , int , int );

void
syscall_init (void)
{
    intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

// void is_valid_add(const void *vaddr){
//     if(!is_user_vaddr(vaddr) || !pagedir_get_page(thread_current()->pagedir, vaddr))
//         exit_proc(-1);
// }

void is_valid_add_multiple(int *vaddr, unsigned count){
    for(int i = 0; i < count; i++) {
        if(!is_user_vaddr((const void *) vaddr[i]) || !pagedir_get_page(thread_current()->pagedir, (const void *) vaddr[i]))
            exit_proc(-1);
    }
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
    int arg[3];
    for (int i = 0; i< 2; i++) {
        arg[i] = ((int *)f->esp)+i;
    }
    is_valid_add_multiple(&arg[0], 2);
    switch(*((int *)f->esp)){
        case SYS_HALT:
            shutdown_power_off();
            break;

        case SYS_EXIT:
            arg[0] = *((int *) f->esp+1);
            exit_proc(arg[0]);
            break;

        case SYS_EXEC:
            arg[0] = *((int *) f->esp+1);
            is_valid_add_multiple(&arg[0], 1);
            f->eax = process_execute(arg[0]);
            break;

        case SYS_WAIT:
            arg[0] = *((int *) f->esp+1);
            f->eax = process_wait(arg[0]);
            break;
            
        case SYS_REMOVE:
            arg[0] = *((int *) f->esp+1);
            is_valid_add_multiple(&arg[0], 1);
            // is_valid_add((const void *) arg[0]);
            remove_sys(f,(const char*)arg[0]);
            break;

        case SYS_CLOSE:
            arg[0] = *((int *) f->esp+1);
            struct list *files3 = &thread_current()->open_files;
            struct list_elem *e = list_begin(files3);

            acquire_filesys_lock();
            while (e!=list_end(files3))
            {
                struct file_entry *file3 = list_entry(e, struct file_entry, elem);
                if(file3->fd == arg[0])
                {
                    file_close(file3->ptr);
                    list_remove(e);
                    free(file3);
                    break;
                }
                e=list_next(e);
            }
            release_filesys_lock();
            break;

        case SYS_OPEN:
            arg[0] = *((int *) f->esp+1);
            is_valid_add_multiple(&arg[0], 1);
            acquire_filesys_lock();
            struct file* file_ptr = filesys_open(arg[0]);
            release_filesys_lock();
            int set_value;
            if(file_ptr == NULL){
                set_value = -1;
            }
            else{
                struct thread *curr_thread = thread_current();
                struct file_entry *file = malloc(sizeof(struct file_entry));
                file -> ptr = file_ptr;
                file -> fd = curr_thread -> count_file_descriptor++;
                list_push_back(&curr_thread->open_files, &file->elem);
                set_value = file -> fd;
            }
            f -> eax = set_value;
            break;

        case SYS_TELL:
            arg[0] = *((int *) f->esp+1);
            struct file_entry *file2 = file_list_entry(arg[0]);
            struct file *fpointer2 = file2->ptr;
            if (file2)
            {
                acquire_filesys_lock();
                f->eax = file_tell(fpointer2);
                release_filesys_lock();
            }
            break;

        case SYS_FILESIZE:
            arg[0] = *((int *) f->esp+1);
            acquire_filesys_lock();
            struct file_entry * file_open = file_list_entry(arg[0]);
            f -> eax = file_length(file_open->ptr);
            release_filesys_lock();
            break;

        case SYS_WRITE:
            for (int i = 0; i<= 2; i++) {
                arg[i] = *((int *) f->esp+5+i);
            }
            is_valid_add_multiple(&arg[1], 1);
            write_sys(f,arg[0],arg[1],arg[2]);
            break;

        case SYS_SEEK:
            for (int i = 0; i < 2; i++)
                arg[i] = *((int *) f->esp+4+i);

            struct file_entry *file1 = file_list_entry(arg[0]);
            struct file *fpointer1 = file1->ptr;
            if (file1)
            {
                acquire_filesys_lock();
                file_seek(fpointer1, arg[1]);
                release_filesys_lock();
            }
            break;

        case SYS_CREATE:
            for (int i = 0; i<= 1; i++) {
                arg[i] = *((int *) f->esp+4+i);
            }
            is_valid_add_multiple(&arg[0], 1);
            acquire_filesys_lock();
            f->eax = filesys_create(arg[0], arg[1]);
            release_filesys_lock();
            break;

        case SYS_READ:
            for (int i = 0; i<= 2; i++) {
                arg[i] = *((int *) f->esp+5+i);
            }
            is_valid_add_multiple(&arg[1], 1);
            if(arg[0] == 0){
                f->eax = input_getc();
            }
            else{
                struct file_entry* fptr = file_list_entry(arg[0]);
                if(fptr == NULL){
                    f->eax = -1;
                }
                else{
                    acquire_filesys_lock();
                    f -> eax = file_read(fptr -> ptr, arg[1], arg[2]);
                    release_filesys_lock();
                }
            }
            break;


        default:
            printf("Running default condition");
    }
}

struct file_entry* file_list_entry(int fd)
{
    struct list_elem *e;
    struct file_entry *file_entry = NULL;
    struct list *file_list = &thread_current()->open_files;
    for(e=list_begin(file_list); e!=list_end(file_list); e=list_next(e))
    {
        struct file_entry *temp = list_entry(e, struct file_entry, elem);
        if (temp->fd = fd)
            file_entry = temp;
        break;
    }
    return file_entry;
}

void exit_proc(int status)
{
    struct list_elem *e;
    struct thread * curr = thread_current();
    struct thread * parent = curr->parent;
    e = list_begin(&parent->process_child);
    while( e!= list_end(&parent->process_child)){
        struct child * c = list_entry(e, struct child, elem);
        if(c->tid == curr->tid)
        {
            c->is_done = true;
            c->code_exit = status;
        }
        e = list_next(e);
    }
    curr-> code_exit = status;
    if(parent->being_waiting_on == curr->tid)
        sema_up(&parent->wait_for_child);

    thread_exit();
}

void remove_sys(struct intr_frame *p UNUSED,const char * file){
    acquire_filesys_lock();
    p->eax = filesys_remove(file)==NULL ? false : true;
    release_filesys_lock();
}

void write_sys(struct intr_frame *p UNUSED, int descriptor, int buff, int size){
    if(size <=0){
        p->eax = size;
        return;
    }
    if(descriptor==1){
                putbuf(buff, size);
                p -> eax = size;
                return;
    }
    struct file_entry* pt = file_list_entry(descriptor);
    if(pt == NULL){
        p -> eax = -1;
        return;
    }
    acquire_filesys_lock();
    p -> eax = file_write(pt -> ptr, buff, size);
    release_filesys_lock();
}


