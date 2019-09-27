# pintos

Project:
-       Priority donation is the most difficult:
-   For alarm clock:
    -   zero - use timer_sleep which is part of timer.c
             - tick - unit of time for the operating system
    -   negative - 
             if(ticks<=0){
                 return;
             }
    -   single
    -   multiple
    -   Priority
    -   simultaneous

For the above:
call_thread_sleep()
current_ticks+ticks
t = current_thread();
t => wake_up_ticks = ticks
read semaphores from sync.h and read about synch_up and synch_down (changes value of flag to 1). each thread has its own semaphore. sema down releases the resources.
            We then save the thread which gave up its resources in a new list which is ordered by wake up ticks. Let's assume we insert it into list_insert_ordered
            list_insert_ordered defined in list.c has an ordered function which can be used to arrange the threads based on the wake_up_ticks, so that the next thread to be woken up is always at the head of the list

            You need to define an instance of list_insert_ordered as it will be used in multiple places

            THREAD.H
            in order to send a thread to sleep, look for thread_int. In the file called thread.h, call the sleep_list_ordered(i.e. define it there) (use some other name for the list)
            Also, define a variable wake_up_ticks.

            Waking up a thread. 
            Write this code in timer_interrupt function. THis is a function that is called on every tick. Get current_ticks = timer_ticks.
            Get the first element from the sleep_list_ordered. Since it is a link list, you need to pop the value.

            if(wake_up_ticks <= current_ticks){wake_up}

            DO PRIORITY DONATION AT THE END. Do MLFQ first.

Priority : Test cases - Alarm priority, Priority Sema, Priority Condvar, fifo, change, preempt, Donation
    Scheduling: Create a thread - use thread.c. Call the thread_create function to create a new thread.
    Use if(thread_get_priority(current_thread) < new_thread_priority) { thread.yield() } 
    Priority of the new thread will be provided by the test cases. Current thread is the one that has currently occupied all the system resources.
    Pintos creates idle thread with tID of 2. so thread with t_ID 2 is NEVER inserted into the ordered_list(sleep_list_ordered). So make sure you have the following check in place:
    if(tID!=2){do_something i.e. insert into the ordered list}

    thread_set_priority() -> replace the thread in the current position of list(readylist)
    if(ready_list is not null i.e. it is not empty)
    if(current_thread.priority < priority of the thread at the head of the list)
    NOTE: ONLY IF THE ABOVE TWO CONDITIONS PASS, THEN CALL thread.yield()

    Sema and Condvar:
    - Insert into ready list in correct position
    - while waking up the thread which is ready -> get the thread with max_priority (Use any sorting algorithm to find the max priority)

Priority Donation:
K>L>M: We cannot directly donate priority from K to M. We need to follow the chain i.e. K to L and then to M.
Needs to be constantly checked and so we need to work with the timer_interrupt function. This is because if A (Max priority) donates priority to C and at that time D comes into the
ready queue with a higher priority than A but somehow A has the CPU, then we need to perform another priority donation so that D can utilise the CPU
Must find out a way of saving the original priority values of the threads so that they can be reverted back once they have finished execution. 

thread.c -> init_priority
         -> pointer waiting for lock
lock    -> max_priority
        -> list_element

        1) If lock has no holder, then
            current_thread() -----(hold)----> lock
            max_priority = current_thread_priority;
        2) if lock is held:
            read(thread_get_priority(current_thread))
            read(thread_get_priority(thread_req_for_lock))
            if(priority_thread_request_for_lock > priority_curr_thread)
                then donate;
                priority_current_thread <= init_priority
                current_priority = priority_req_thread
                max_priority = current_thread_priority

        Reset Lock:
        -> Reset priority of curr_thread to init_priority
