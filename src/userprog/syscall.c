#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "list.h"
#include "process.h"
// #include "devices/shutdown.h"
// #include "filesys/file.h"
// #include "filesys/filesys.h"

static void syscall_handler (struct intr_frame *);
void* check_addr(const void*);
struct proc_file* list_search(struct list* files, int fd);
// static void exit_proc(int status);
// static void close_file(struct list *files, int fd);
// static void close_all_files(struct list* files);

extern bool running;

/*struct proc_file {
    struct file* ptr;
    int fd;
    struct list_elem elem;
};*/

void
syscall_init (void)
{
    intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
    int *p = f -> esp;
    check_addr(p);
    int system_call = *p;
    switch(system_call){
        case SYS_HALT:
            shutdown_power_off();
            break;

        case SYS_EXIT:
            check_addr(p+1);
            exit_proc(*(p+1));
            break;

        case SYS_EXEC:
            check_addr(p+1);
            check_addr(*(p+1));
            f->eax = exec_proc(*(p+1));
            break;

        case SYS_WAIT:
            check_addr(p+1);
            f->eax = process_wait(*(p+1));
            break;

        case SYS_CREATE:
            check_addr(p+5);
            check_addr(*(p+4));
            // Declaration
            acquire_filesys_lock();
            f->eax = filesys_create(*(p+4), *(p+5));
            // Declaration: extern bool running??
            release_filesys_lock();
            break;
        case SYS_REMOVE:
            check_addr(p+1);
            check_addr(*(p+1));
            //Declaration
            acquire_filesys_lock();
            if(filesys_remove(*(p+1))==NULL)
                f -> eax = false;
            else
                f -> eax = true;
            // Declaration
            release_filesys_lock();
            break;

        case SYS_OPEN:
            check_addr(p+1);
            check_addr(*(p+1));
            // Declaration:
            acquire_filesys_lock();

            struct file* fptr = filesys_open(*(p+1));
            // Declaration
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
            check_addr(p+1);
            // Declaration
            acquire_filesys_lock();
            f -> eax = file_length(list_search(&thread_current()->open_files, *(p+1))->ptr);
            // Declaration
            release_filesys_lock();
            break;

        case SYS_READ:
            check_addr(p+7);
            check_addr(*(p+6));
            if(*(p+5)==0){
                int i;
                uint8_t* buffer = *(p+6);
                for(i=0;i<*(p+7);i++){
                    buffer[i] = input_getc();
                    f->eax = *(p+7);
                }
            }
            else{
                struct proc_file* fptr = list_search(&thread_current()->open_files, *(p+5));
                if(fptr == NULL){
                    f->eax = -1;
                }
                else{
                    //Declaration
                    acquire_filesys_lock();
                    f -> eax = file_read(fptr -> ptr, *(p+6), *(p+7));
                    // Declaration
                    release_filesys_lock();
                }
            }
            break;

        case SYS_WRITE:
            check_addr(p+7);
            check_addr(*(p+6));
            if(*(p+5)==1){
                putbuf(*(p+6), *(p+7));
                f -> eax = *(p+7);
            }
            else {
                struct proc_file* fptr = list_search(&thread_current()->open_files, *(p+5));
                if(fptr == NULL){
                    f -> eax = -1;
                }
                else {
                    // Declaration
                    acquire_filesys_lock();
                    f -> eax = file_write(fptr -> ptr, *(p+6), *(p+7));
                    // Declaration
                    release_filesys_lock();
                }
            }
            break;

        case SYS_SEEK:
            check_addr(p+5);
            //Declaration
            acquire_filesys_lock();
            file_seek(list_search(&thread_current() -> open_files, *(p+4)) -> ptr, *(p+5));
            // Declaration
            release_filesys_lock();
            break;

        case SYS_TELL:
            check_addr(p+5);
            //Declaration
            acquire_filesys_lock();
            f -> eax = file_tell(list_search(&thread_current() -> open_files, *(p+1)) -> ptr);
            // Declaration
            release_filesys_lock();
            break;

        case SYS_CLOSE:
            check_addr(p+1);
            //Declaration
            acquire_filesys_lock();
            close_file(&thread_current()->open_files, *(p+1));
            // Declaration
            release_filesys_lock();
            break;

        default:
            printf("Running default condition");
    }
}

void* check_addr(const void *vaddr){
    if(!is_user_vaddr(vaddr)){
        exit_proc(-1);
        return 0;
    }
    void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
    if(!ptr){
        exit_proc(-1);
        return 0;
    }
    return ptr;
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