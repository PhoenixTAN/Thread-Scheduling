/**
 * Author: Ziqi Tan, Jiaqian Sun
 * 
 * Reference: https://www.gnu.org/software/grub/manual/multiboot/multiboot.html
*/

#include "multiboot.h"
#include "thread.h"
#include "print.h"

/* Macros. */
/* The number of threads */ 
#define N                       3
#define stack_size              1024
#define ready_queue_size        256

/* Variables */
static TCB tcb[N];
static int uid = 0;     // counter
static TCB* lastThread = (void*)0;

/* data structure and operationg for ready queue */
static TCB* ready_queue[ready_queue_size];
static int head;
static int tail;
void init_ready_queue();
int en_queue(TCB* tcb);
TCB* de_queue();
int isEmpty();
int isFull();

/* Private stack for threads */
multiboot_uint32_t stack1[stack_size];
multiboot_uint32_t stack2[stack_size];
multiboot_uint32_t stack3[stack_size];

/* Forward declarations. */
void init(multiboot_info_t* pmb);
int create_thread(void* stack, void* run);
void thread1_run();
void thread2_run();
void thread3_run();
void delay();

int fifo_scheduler();
void yield();
void thread_finish();
void get_threads_ready();
TCB* get_next_thread();
void switch_thread();

/* entrance of the C code */
void init(multiboot_info_t* pmb) {
    cls();
    println("FIFOS-1: Ziqi Tan, Jiaqian Sun");
    println("First come first serve thread scheduler:");

    init_ready_queue();

    /* create threads */
    create_thread(&stack1[stack_size-1], thread1_run);
    create_thread(&stack2[stack_size-1], thread2_run);
    create_thread(&stack3[stack_size-1], thread3_run);

    /* schedule */
    fifo_scheduler();
}

/* Create thread */
int create_thread(void* stack, void* run) {
    // index out of bound exception
    if( uid > N ) {
        return -1;
    }
    tcb[uid].sp = (multiboot_uint32_t*)stack;
    tcb[uid].tid = uid + 1;
    tcb[uid].run = run;
    tcb[uid].status = NEW;
    tcb[uid].priority = 0;
    
    // initial context
    *((multiboot_uint32_t *)stack) = (multiboot_uint32_t)run;   // EIP
    
    // FLAG[1] 1，always 1 in EFLAGS
    // FLAG[9] 0，disable interrupt
    *((multiboot_uint32_t *)stack -  1) = 2;          // FLAG
    *((multiboot_uint32_t *)stack -  2) = 0;          // EAX   
    *((multiboot_uint32_t *)stack -  3) = 0;          // EBX 
    *((multiboot_uint32_t *)stack -  4) = 0;          // ECX 
    *((multiboot_uint32_t *)stack -  5) = 0;          // EDX
    *((multiboot_uint32_t *)stack -  6) = 0;          // ESP
    *((multiboot_uint32_t *)stack -  7) = 0;          // EBP 
    *((multiboot_uint32_t *)stack -  8) = 0;          // ESI 
    *((multiboot_uint32_t *)stack -  9) = 0;          // EDI

    // update the stack pointer at one time
    tcb[uid].sp = (multiboot_uint32_t*)stack - 9;
    uid++;

    return tcb[uid].tid;
}

void yield() {
    en_queue(lastThread);
    fifo_scheduler();  
}

void thread_finish() {
    lastThread->status = TERMINATED;
    fifo_scheduler(); 
}

void thread1_run() {
    int jobs = 2;
    while( jobs ) {
        print("Thread<0001> is running...  ");
        delay();
        jobs--;
        yield();
    }
    println("Thread<0001> finished.");
    thread_finish();
}

void thread2_run() {
    int jobs = 4;
    while( jobs ) {
        print("Thread<0002> is running...  ");
        delay();
        jobs--;
        yield(); 
    }
    println("Thread<0002> finished.");
    thread_finish();
}

void thread3_run() {
    int jobs = 5;
    while( jobs ) {
        print("Thread<0003> is running...  ");
        // delay
        delay();
        jobs--;
        yield();  
    }
    println("Thread<0003> finished.");
    thread_finish();
}

/* First come first serve scheduler */
int fifo_scheduler() {

    // (1) add all NEW threads into ready queue.
    get_threads_ready();

    // (2) ready queue is empty
    if( isEmpty() == 1 ) {
        println("Ready queue is empty.");
        println("Schedule ends.");
        __asm__ volatile("jmp schedule_finish");
        return 1;
    }

    // (3) Find the next thread
    TCB* nextThread = get_next_thread();
    
    // (4) use asm to switch thread
    switch_thread(nextThread);
    
    return 0;
}

/* add new threads into ready queue */
void get_threads_ready() {
    int i;
    for( i = 0; i < N; i++ ) {
        if( tcb[i].status == NEW ) {
            en_queue(&tcb[i]);
            tcb[i].status = READY;
        }
    }
}

/* get next thread from the ready queue */
TCB* get_next_thread() {
    return de_queue();
}

/* switch from last*/
void switch_thread(TCB* nextThread) {
    if( lastThread == (void*)0 || lastThread->status == TERMINATED ) {
        lastThread = nextThread;
        nextThread->status = RUNNING;
        __asm__ volatile("call context_retrieve"::"D"(nextThread));
    }
    else {
        TCB* temp = lastThread;     // record last thread
        lastThread = nextThread;    // update last thread
        nextThread->status = RUNNING;
        println("Go back to scheduler...");
        __asm__ volatile("call context_protection"::"S"(temp), "D"(nextThread));
        // "S": ESI, "D": EDI
    }
}

/* Opeartions for ready queue */
void init_ready_queue() {
    head = 0;
    tail = 0;
}

/* add */
int en_queue(TCB* tcb) {
    if( isFull() == 1 ) {
        return -1;
    }
    ready_queue[tail] = tcb;
    tail = (tail + 1) % ready_queue_size;
    return 0;
}

/* poll */
TCB* de_queue() {
    if( isEmpty() == 1 ) {
        return (void*)0;
    }
    TCB* tcb = ready_queue[head];
    head = (head + 1) % ready_queue_size;
    return tcb;
}

/* isEmpty */
int isEmpty() {
    if(head == tail) {
        return 1;
    }
    return -1;
}

/* isFull */
int isFull() {
     if ((tail + 1) % ready_queue_size == head ) {
         return 1;
     }
     return -1;
}

void delay() {
    int i, j;
    for( i = 0; i < 10000; i++ ) {
        for( j = 0; j < 30000; j++ ) {
        }
    }
}