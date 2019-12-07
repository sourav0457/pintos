#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "list.h"
#include "process.h"

static void syscall_handler (struct intr_frame *);
void is_valid_add(const void*);
struct proc_file* list_search(struct list* files, int fd);
void is_valid_add_multiple(int *, unsigned count);

extern bool running;

void
syscall_init (void)
{
    intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void is_valid_add(const void *vaddr){
    if(!is_user_vaddr(vaddr)){
        exit_proc(-1);
    }
    void *p = pagedir_get_page(thread_current()->pagedir, vaddr);
    if(!p){
        exit_proc(-1);
    }
}

void is_valid_add_multiple(int *vaddr, unsigned count){
    for(int i = 0; i < count; i++) {
        if(!is_user_vaddr((const void *) vaddr[i]) || !pagedir_get_page(thread_current()->pagedir, (const void *) vaddr[i]))
            exit_proc(-1);
    }
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
    int *p = f -> esp;
    int arg[3];
    is_valid_add(p);
    is_valid_add(p+1);
    int system_call = *p;
    switch(system_call){
        case SYS_HALT:
            shutdown_power_off();
            break;

        case SYS_EXIT:
            arg[0] = *((int *) f->esp+1);
            exit_proc(arg[0]);
            break;

        case SYS_EXEC:
            arg[0] = *((int *) f->esp+1);
            is_valid_add((const void *) arg[0]);
            f->eax = exec_proc(arg[0]);
            break;

        case SYS_WAIT:
            arg[0] = *((int *) f->esp+1);
            f->eax = process_wait(arg[0]);
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
            
        case SYS_REMOVE:
            arg[0] = *((int *) f->esp+1);
            is_valid_add((const void *) arg[0]);
            acquire_filesys_lock();
            if(filesys_remove(arg[0])==NULL)
                f -> eax = false;
            else
                f -> eax = true;
            release_filesys_lock();
            break;

        case SYS_OPEN:
            arg[0] = *((int *) f->esp+1);
            is_valid_add((const void *) arg[0]);
            acquire_filesys_lock();

            struct file* fptr = filesys_open(arg[0]);
            release_filesys_lock();
            if(fptr == NULL){
                f -> eax = -1;
            }
            else{
                struct proc_file *pfile = malloc(sizeof(*pfile));
                pfile -> ptr = fptr;
                pfile -> fd = thread_current() -> count_file_descriptor;
                thread_current() -> count_file_descriptor++;
                list_push_back(&thread_current()->open_files, &pfile->elem);
                f -> eax = pfile -> fd;
            }
            break;

        case SYS_FILESIZE:
            arg[0] = *((int *) f->esp+1);
            acquire_filesys_lock();
            f -> eax = file_length(list_search(&thread_current()->open_files, arg[0])->ptr);
            release_filesys_lock();
            break;

        case SYS_READ:
            for (int i = 0; i<= 2; i++) {
                arg[i] = *((int *) f->esp+5+i);
            }
            is_valid_add_multiple(&arg[1], 1);
            if(arg[0]==0){
                int i;
                uint8_t* buffer = arg[1];
                for(i=0;i<arg[2];i++){
                    buffer[i] = input_getc();
                    f->eax = arg[2];
                }
            }
            else{
                struct proc_file* fptr = list_search(&thread_current()->open_files, arg[0]);
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

        case SYS_WRITE:
            for (int i = 0; i<= 2; i++) {
                arg[i] = *((int *) f->esp+5+i);
            }
            is_valid_add_multiple(&arg[1], 1);
            if(arg[0]==1){
                putbuf(arg[1], arg[2]);
                f -> eax = arg[2];
            }
            else {
                struct proc_file* fptr = list_search(&thread_current()->open_files, arg[0]);
                if(fptr == NULL){
                    f -> eax = -1;
                }
                else {
                    acquire_filesys_lock();
                    f -> eax = file_write(fptr -> ptr, arg[1], arg[2]);
                    release_filesys_lock();
                }
            }
            break;

        case SYS_SEEK:
            for (int i = 0; i<= 1; i++) {
                arg[i] = *((int *) f->esp+4+i);
            }
            acquire_filesys_lock();
            file_seek(list_search(&thread_current() -> open_files, arg[0]) -> ptr, arg[1]);
            release_filesys_lock();
            break;

        case SYS_TELL:
            arg[0] = *((int *) f->esp+1);
            acquire_filesys_lock();
            f -> eax = file_tell(list_search(&thread_current() -> open_files, arg[0]) -> ptr);
            release_filesys_lock();
            break;

        case SYS_CLOSE:
            arg[0] = *((int *) f->esp+1);
            acquire_filesys_lock();
            close_file(&thread_current()->open_files, arg[0]);
            release_filesys_lock();
            break;

        default:
            printf("Running default condition");
    }
}



struct proc_file* list_search(struct list* files, int fd){
    struct list_elem *e;
    for(e=list_begin(files); e!=list_end(files); e=list_next(e)){
        struct proc_file *f = list_entry(e, struct proc_file, elem);
        return f;
    }
    return NULL;
}

int exec_proc(char * file_name) {
    acquire_filesys_lock();
    char *fn_cp = malloc(strlen(file_name)+1);
    strlcpy(fn_cp, file_name, strlen(file_name)+1);
    char *save_ptr;
    fn_cp = strtok_r(fn_cp, " ", &save_ptr);
    struct file* f = filesys_open(fn_cp);
    if(f == NULL) {
        release_filesys_lock();
        return -1;
    }
    else {
        file_close(f);
        release_filesys_lock();
        return process_execute(file_name);
    }
}

void exit_proc(int status){
    struct list_elem *e;
    for(e = list_begin(&thread_current()->parent->process_child); e!= list_end(&thread_current()->parent->process_child); e=list_next(e)){
        struct child *f = list_entry(e, struct child, elem);
        if(f->tid == thread_current()->tid){
            f->is_done = true;
            f-> code_exit = status;
        }
    }
    thread_current()-> code_exit = status;
    if(thread_current()->parent->being_waiting_on == thread_current()->tid){
        sema_up(&thread_current()->parent->wait_for_child);
    }
    thread_exit();
}

void close_file(struct list *files, int fd){
    struct list_elem *e;
    struct proc_file *f;
    for(e = list_begin(files); e!=list_end(files); e=list_next(e)){
        f = list_entry(e, struct proc_file, elem);
        if(f->fd == fd){
            file_close(f->ptr);
            list_remove(e);
        }
    }
    free(f);
}

void close_all_files(struct list* files){
    struct list_elem *e;
    while(!list_empty(files)){
        e = list_pop_front(files);
        struct proc_file *f = list_entry(e, struct proc_file, elem);
        file_close(f -> ptr);
        list_remove(e);
        free(f);
    }
}