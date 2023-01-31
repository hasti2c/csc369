#define _GNU_SOURCE

#include "csc369_thread.h"

#include <ucontext.h>
#include <stdlib.h>
#include <assert.h>


//****************************************************************************
// Private Definitions
//****************************************************************************
enum ThreadState {RUNNING, READY, ZOMBIE};

/**
 * The Thread Control Block.
 */
typedef struct
{
    Tid tid;
    enum ThreadState state;
    ucontext_t context;
    void *stack;
} TCB;
//**************************************************************************************************
// Private Global Variables (Library State)
//**************************************************************************************************
TCB *running_thread;
TCB *zombie_thread; // can keep only one zombie since next running thread frees it before doing anything else.
TCB *all_threads[CSC369_MAX_THREADS];
Tid ready_queue[CSC369_MAX_THREADS];
int rq_head, rq_tail;
//**************************************************************************************************
// Helper Functions
//**************************************************************************************************
/**
 * @return unique tid not used by any threads in all_threads if all_threads not full, -1 if all_threads full.
 */
int get_available_tid() {
    for (int i = 0; i < CSC369_MAX_THREADS; i++)
        if (all_threads[i] == NULL)
            return i;
    return -1;
}

/**
 * Add tcb to thread list all_threads at index tid.
 *
 * @return 0 on success, 1 on failure.
 */
int tl_add(TCB* tcb) {
    Tid tid = tcb->tid;
    if (tid < 0 || tid >= CSC369_MAX_THREADS || all_threads[tid] != NULL)
        return 1;
    all_threads[tid] = tcb;
    return 0;
}

/**
 * Remove tcb with tid from thread list all_threads.
 *
 * @return 0 on success, 1 on failure (if no such tid).
 */
int tl_remove(Tid tid) {
    if (tid < 0 || tid >= CSC369_MAX_THREADS || all_threads[tid] == NULL)
        return 1;
    all_threads[tid] = NULL; 
    return 0;
}

/**
 * Enqueue tid in ready_queue.
 * 
 * @return 0 on success, -1 if ready_queue full, -2 if rq_head, rq_tail invalid.
 */
int rq_enqueue(Tid tid) {
    if (rq_head < 0 || rq_head >= CSC369_MAX_THREADS || rq_tail < 0 || rq_tail >= CSC369_MAX_THREADS)
        return -2;
    if ((rq_head - rq_tail + CSC369_MAX_THREADS) % CSC369_MAX_THREADS == 1)
        return -1;
    
    ready_queue[rq_tail] = tid;
    rq_tail = (rq_tail + 1) % CSC369_MAX_THREADS;
    return 0;
}

/**
 * Dequeue first element in ready_queue.
 * 
 * @return dequeued tid on success, -1 if ready_queue empty, -2 if rq_head,rq_tail invalid.
 */
Tid rq_dequeue() {
    if (rq_head < 0 || rq_head >= CSC369_MAX_THREADS || rq_tail < 0 || rq_tail >= CSC369_MAX_THREADS)
        return -2;
    if (rq_head == rq_tail)
        return -1;

    Tid tid = ready_queue[rq_head];
    rq_head = (rq_head + 1) % CSC369_MAX_THREADS;
    return tid;
}

/** Remove tid from ready_queue and move everything else forward.
 * 
 * @return 0 on succes, 1 on failure (tid not in ready_queue).
 */
int rq_remove(Tid tid) {
    if (rq_head == rq_tail)
        return 1;

    int found = 0;
    for (int cur = rq_head; cur != rq_tail; cur = (cur + 1) % CSC369_MAX_THREADS) {
        if (ready_queue[cur] == tid)
            found = 1;
        if (found) {
            int next = (cur + 1) % CSC369_MAX_THREADS;
            ready_queue[cur] = ready_queue[next];
        }
    }
    if (!found)
        return 1;
    rq_tail = (rq_tail - 1 + CSC369_MAX_THREADS) % CSC369_MAX_THREADS;
    return 0;
}

/**
 * Switch to thread with tid.
 *
 * Assumes tid has already been removed from ready queue.
 *
 * Returns tid on success, CSC369_ERROR_TID_INVALID if tid invalid, CSC369_ERROR_THREAD_BAD if fails otherwise.
 */
int switch_thread(Tid tid) {
    if (tid < 0 || tid >= CSC369_MAX_THREADS)
        return CSC369_ERROR_TID_INVALID;
    TCB *tcb = all_threads[tid];
    if (tcb == NULL)
        return CSC369_ERROR_THREAD_BAD;
    tcb->state = RUNNING;

    if (running_thread->state != ZOMBIE) {
        int err = rq_enqueue(running_thread->tid);
        assert(!err);
        running_thread->state = READY;
    }

    running_thread = tcb;
    setcontext(&tcb->context);
    return CSC369_ERROR_THREAD_BAD; // shouldn't get here.
}
 
/**
 * Free memory allocated to tcb and remove tcb from all_threads, which "frees" its TID.
 *
 * Process should NOT be the running thread, nor be in the ready queue. 
 *
 * Return 0 on success, 1 on failure.
 */
int free_thread(TCB* tcb) {
    int err = tl_remove(tcb->tid);
    assert(!err);
    if (err)
        return 1;
    free(tcb->stack); 
    free(tcb);
    zombie_thread = NULL;
    return 0;
}

/**
 * Call f then exit properly by calling CSC369_ThreadExit(). 
 */
void thread_stub(void (*f)(void *), void *arg) {
    if (zombie_thread != NULL) {
        int err = free_thread(zombie_thread);
        assert(!err);
    }
    f(arg);
    CSC369_ThreadExit();
}
//**************************************************************************************************
// thread.h Functions
//**************************************************************************************************
int
CSC369_ThreadInit(void)
{
    rq_head = 0, rq_tail = 0;
    running_thread = malloc(sizeof(TCB));
    if (running_thread == NULL)
        return CSC369_ERROR_OTHER;

    running_thread->tid = 0;
    running_thread->state = RUNNING;
    int err = getcontext(&running_thread->context);
    err &= tl_add(running_thread);
    if (err)
        return CSC369_ERROR_OTHER;
    return 0;
}

Tid
CSC369_ThreadId(void)
{
    return running_thread->tid;
}

Tid
CSC369_ThreadCreate(void (*f)(void*), void* arg)
{
    assert(zombie_thread == NULL);   
    Tid new_tid = get_available_tid();
    if (new_tid == -1)
         return CSC369_ERROR_SYS_THREAD;

    TCB* tcb = malloc(sizeof(TCB));
    if (tcb == NULL)
        return CSC369_ERROR_SYS_MEM;
    tcb->tid = new_tid;
    tcb->state = READY;
    tcb->stack = malloc(CSC369_THREAD_STACK_SIZE + 16);
    if (tcb->stack == NULL)
        return CSC369_ERROR_SYS_MEM;

    int err = getcontext(&tcb->context);
    if (err)
        return CSC369_ERROR_OTHER;
    tcb->context.uc_mcontext.gregs[REG_RIP] = (greg_t) &thread_stub;
    tcb->context.uc_mcontext.gregs[REG_RDI] = (greg_t) f;
    tcb->context.uc_mcontext.gregs[REG_RSI] = (greg_t) arg;

    tcb->context.uc_mcontext.gregs[REG_RSP] = (greg_t) tcb->stack + CSC369_THREAD_STACK_SIZE + 15;
    tcb->context.uc_mcontext.gregs[REG_RSP] -= (tcb->context.uc_mcontext.gregs[REG_RSP] - 8) % 16;

    err = tl_add(tcb);
    err &= rq_enqueue(tcb->tid);
    if (err)
        return CSC369_ERROR_OTHER;
    return tcb->tid;
}

void
CSC369_ThreadExit()
{
    if (rq_head == rq_tail) // empty
        exit(0);
    
    running_thread->state = ZOMBIE;
    zombie_thread = running_thread; 
    CSC369_ThreadYield();
    exit(-1); // should not get here.
}

Tid
CSC369_ThreadKill(Tid tid)
{
    if (tid == running_thread->tid)
        return CSC369_ERROR_THREAD_BAD;
    else if (tid < 0 || tid >= CSC369_MAX_THREADS)
        return CSC369_ERROR_TID_INVALID;
    TCB *tcb = all_threads[tid];
    if (tcb == NULL)
        return CSC369_ERROR_SYS_THREAD;
        
    int err = rq_remove(tid); 
    if (err) // thread not in ready queue
        return CSC369_ERROR_SYS_THREAD;
    err = free_thread(tcb);    
    assert(!err);
    return tid;
}

int
CSC369_ThreadYield()
{
    volatile int called = 0;
    int err = getcontext(&running_thread->context); 
    assert(!err); 
    int tid;
    if (!called) {
        tid = rq_dequeue();
        if (tid == -1) // empty ready queue
            return running_thread->tid;
        assert(tid != -2);

        called = 1;
        err = switch_thread(tid);
        return err; // should not get here.
    }
    // only gets here when exectues again.
    if (zombie_thread != NULL) {
        err = free_thread(zombie_thread);
        assert(!err);
    }
    return tid;
}

int
CSC369_ThreadYieldTo(Tid tid)
{
    volatile int called = 0;
    int err = getcontext(&running_thread->context);
    assert(!err);
    if (!called) {
        if (tid == running_thread->tid)
            return tid;

        if (tid < 0 || tid >= CSC369_MAX_THREADS)
            return CSC369_ERROR_TID_INVALID;
        int err = rq_remove(tid);
        if (err)
            return CSC369_ERROR_THREAD_BAD;
        
        called = 1;
        err = switch_thread(tid);
        return err; // should not get here.
    }
    // only gets here when exectues again.
    if (zombie_thread != NULL) {
        int err = free_thread(zombie_thread);
        assert(!err);
    }
    return tid;
}
