#define _GNU_SOURCE

#include "csc369_thread.h"

#include <ucontext.h>

#include <stdlib.h>

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
int thread_count = 0;
TCB *all_threads[CSC369_MAX_THREADS];
Tid ready_queue[CSC369_MAX_THREADS];
int rq_head, rq_tail;
//**************************************************************************************************
// Helper Functions
//**************************************************************************************************
/**
 * @return unique tid not used by any threads in tl if tl not full, -1 if tl full.
 */
int get_available_tid(TCB** tl) {
    for (int i = 0; i < CSC369_MAX_THREADS; i++)
        if (tl[i] == NULL)
            return i;
    return -1;
}

/**
 * Add tcb to thread list tl at index tid.
 *
 * @return 0 on success, 1 on failure.
 */
int tl_add(TCB** tl, TCB* tcb) {
    Tid tid = tcb->tid;
    if (tid < 0 || tid >= CSC369_MAX_THREADS || tl[tid] != NULL)
        return 1;
    tl[tid] = tcb;
    thread_count++;
    return 0;
}

/**
 * Remove tcb with tid from tl.
 *
 * @return 0 on success, 1 on failure (if no such tid).
 */
int tl_remove(TCB** tl, Tid tid) {
    if (tl[tid] == NULL)
        return 1;
    tl[tid] = NULL;
    thread_count--;
    return 0;
}

/**
 * Enqueue tid in ready queue rq.
 * 
 * @return 0 on success, -1 if rq full, -2 if rq_head, rq_tail invalid.
 */
int rq_enqueue(Tid* rq, Tid tid) {
    if (rq_head < 0 || rq_head >= CSC369_MAX_THREADS || rq_tail < 0 || rq_tail >= CSC369_MAX_THREADS)
        return -2;
    if (rq_tail == rq_head - 1 || (rq_head == CSC369_MAX_THREADS - 1 && rq_tail == 0))
        return -1;
    
    rq[rq_tail] = tid;
    rq_tail++;
    if (rq_tail == CSC369_MAX_THREADS)
        rq_tail = 0;
    return 0;
}

/**
 * Dequeue first element in rq.
 * 
 * @return dequeued tid on success, -1 if rq empty, -2 if rq_head,rq_tail invalid.
 */
Tid rq_dequeue(Tid* rq){
    if (rq_head < 0 || rq_head >= CSC369_MAX_THREADS || rq_tail < 0 || rq_tail >= CSC369_MAX_THREADS)
        return -2;
    if (rq_head == rq_tail)
        return -1;

    Tid tid = rq[rq_head];
    rq_head--;
    if (rq_head == -1)
        rq_head = CSC369_MAX_THREADS - 1;
    return tid;
}

void thread_stub(void (*f)(void *), void *arg) {
    f(arg);
    CSC369_ThreadExit();
}
//**************************************************************************************************
// thread.h Functions
//**************************************************************************************************
int
CSC369_ThreadInit(void)
{
    thread_count = 1, rq_head = 0, rq_tail = 0;
    running_thread = malloc(sizeof(TCB));
    if (running_thread == NULL)
        return CSC369_ERROR_OTHER;

    running_thread->tid = 0;
    running_thread->state = RUNNING;
    int err = getcontext(&running_thread->context);
    err &= tl_add(all_threads, running_thread);
    if (err)
        return CSC369_ERROR_OTHER;
}

Tid
CSC369_ThreadId(void)
{
    return running_thread->tid;
}

Tid
CSC369_ThreadCreate(void (*f)(void*), void* arg)
{
    Tid new_tid = get_available_tid(all_threads);
    if (new_tid == -1)
        return CSC369_ERROR_SYS_THREAD;

    TCB* tcb = malloc(sizeof(TCB));
    if (tcb == NULL)
        return CSC369_ERROR_SYS_MEM;
    tcb->tid = new_tid;
    tcb->state = READY;
    tcb->stack = malloc(CSC369_THREAD_STACK_SIZE);
    if (tcb->stack == NULL)
        return CSC369_ERROR_SYS_MEM;

    int err = getcontext(&tcb->context);
    if (err)
        return CSC369_ERROR_OTHER;
    tcb->context.uc_mcontext.gregs[REG_RIP] = (greg_t) &thread_stub;
    tcb->context.uc_mcontext.gregs[REG_RDI] = (greg_t) f;
    tcb->context.uc_mcontext.gregs[REG_RSI] = (greg_t) arg;
    tcb->context.uc_mcontext.gregs[REG_RSP] = (greg_t) tcb->stack + CSC369_THREAD_STACK_SIZE - 1; // TODO check
    // TODO handle byte alignment, @149 on piazza

    err = tl_add(all_threads, tcb);
    err &= rq_enqueue(ready_queue, tcb->tid);
    if (err)
        return CSC369_ERROR_OTHER;
    return tcb->tid;
}

void
CSC369_ThreadExit()
{}

Tid
CSC369_ThreadKill(Tid tid)
{
  return -1;
}

int
CSC369_ThreadYield()
{
  return -1;
}

int
CSC369_ThreadYieldTo(Tid tid)
{
  return -1;
}
