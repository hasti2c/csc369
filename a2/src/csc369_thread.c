#include "csc369_thread.h"

#include <ucontext.h>

#include <assert.h>
#include <sys/time.h>
#include <stdlib.h>

//#define DEBUG_USE_VALGRIND // uncomment to debug with valgrind
#ifdef DEBUG_USE_VALGRIND
#include <valgrind/valgrind.h>
#endif

#include "csc369_interrupts.h"

//****************************************************************************
// Private Definitions
//****************************************************************************

typedef enum
{
  CSC369_THREAD_FREE = 0,
  CSC369_THREAD_READY = 1,
  CSC369_THREAD_RUNNING = 2,
  CSC369_THREAD_ZOMBIE = 3,
  CSC369_THREAD_BLOCKED = 4
} CSC369_ThreadState;

/**
 * A wait queue.
 */
typedef struct csc369_wait_queue_t
{ 
  Tid threads[CSC369_MAX_THREADS];
  int head;
  int tail;
} CSC369_WaitQueue;
// TODO implement as linked list.

/**
 * The Thread Control Block.
 */
typedef struct
{
  Tid tid;

  CSC369_ThreadState state;

  void* stack;

  /**
   * The thread context.
   */
  ucontext_t context;

  /**
   * What code the thread exited with.
   */
  int exit_code;

  /**
   * The queue of threads that are waiting on this thread to finish.
   */
  CSC369_WaitQueue join_threads;

  int join_threads_num;
} TCB;

//**************************************************************************************************
// Private Global Variables (Library State)
//**************************************************************************************************
/**
 * All possible threads have their control blocks stored contiguously in memory.
 */
TCB threads[CSC369_MAX_THREADS]; 

Tid running_thread;

/**
 * Threads that are ready to run in FIFO order.
 */
CSC369_WaitQueue ready_threads;

/**
 * Threads that need to be cleaned up.
 */
CSC369_WaitQueue zombie_threads;

//**************************************************************************************************
// Helper Functions
//**************************************************************************************************
void
Queue_Init(CSC369_WaitQueue* queue)
{
  queue->head = 0;
  queue->tail = 0;
}

int
Queue_IsEmpty(CSC369_WaitQueue* queue)
{
  return queue->head == queue->tail;
}

int
Queue_IsFull(CSC369_WaitQueue* queue)
{
  return (queue->head - queue->tail + CSC369_MAX_THREADS) % CSC369_MAX_THREADS == 1;
}

/**
 * @return 0 on success, -1 if queue full.
 */
int
Queue_Enqueue(CSC369_WaitQueue* queue, Tid tid)
{
  assert(!CSC369_InterruptsAreEnabled());
  assert(queue->head >= 0 && queue->head < CSC369_MAX_THREADS);
  assert(queue->tail >= 0 && queue->tail < CSC369_MAX_THREADS);
  if (Queue_IsFull(queue))
    return -1;

  queue->threads[queue->tail] = tid;
  queue->tail = (queue->tail + 1) % CSC369_MAX_THREADS;
  return 0;
}

/**
 * @return dequeued tid on success, -1 if queue empty.
 */
Tid
Queue_Dequeue(CSC369_WaitQueue* queue)
{
  assert(!CSC369_InterruptsAreEnabled());
  assert(queue->head >= 0 && queue->head < CSC369_MAX_THREADS);
  assert(queue->tail >= 0 && queue->tail < CSC369_MAX_THREADS);
  if (Queue_IsEmpty(queue))
    return -1;

  Tid tid = queue->threads[queue->head];
  queue->head = (queue->head + 1) % CSC369_MAX_THREADS;
  return tid;
}

/**
 * Remove tid from queue and move everything after it one space back.
 *
 * @return 0 on success, -1 if tid not in queue;
 */
int
Queue_Remove(CSC369_WaitQueue* queue, Tid tid)
{
  assert(!CSC369_InterruptsAreEnabled());
  assert(queue->head >= 0 && queue->head < CSC369_MAX_THREADS);
  assert(queue->tail >= 0 && queue->tail < CSC369_MAX_THREADS);
  if (Queue_IsEmpty(queue))
    return -1;

  int found = 0;
  for (int cur = queue->head; cur != queue->tail; cur = (cur + 1) % CSC369_MAX_THREADS) {
    if (!found && queue->threads[cur] == tid)
      found = 1;
    if (found) {
      int next = (cur + 1) % CSC369_MAX_THREADS;
      queue->threads[cur] = queue->threads[next];
    }
  }
  if (!found)
    return -1;
  queue->tail = (queue->tail - 1 + CSC369_MAX_THREADS) % CSC369_MAX_THREADS;
  return 0;   
}

void 
TCB_Init(TCB* tcb, Tid tid)
{
  tcb->tid = tid;
  tcb->state = CSC369_THREAD_FREE;
  Queue_Init(&tcb->join_threads);
  tcb->join_threads_num = 0;
}

/*
 * Assumes TCB_Init has already been called.
 *
 * @return 0 on success, -1 on failure.
 */
int
TCB_MainInit() {
  TCB* tcb = &threads[0];
  assert(tcb->tid == 0);
  running_thread = 0;
  tcb->state = CSC369_THREAD_RUNNING;
  return getcontext(&tcb->context);
}

void
TCB_Zombify(Tid tid, int exit_code) {
  assert(!CSC369_InterruptsAreEnabled());
  TCB* tcb = &threads[tid];
  tcb->exit_code = exit_code;
  tcb->state = CSC369_THREAD_ZOMBIE;
  int err = Queue_Enqueue(&zombie_threads, tcb->tid);
  assert(!err);
  CSC369_ThreadWakeAll(&tcb->join_threads);
}

int 
TCB_CanFree(Tid tid) {
  assert(!CSC369_InterruptsAreEnabled());
  TCB* tcb = &threads[tid];
  if (tcb->join_threads_num > 0)
    return 1;
  else
    return 0;
}

/**
 * Assumes no threads are waiting to read exit code.
 *
 * Process should NOT be the running thread, NOR be in the ready queue, NOR be in the zombie queue. 
 */
void
TCB_Free(Tid tid)
{
  // FIXME
#ifdef DEBUG_USE_VALGRIND
  // VALGRIND_STACK_DEREGISTER(...);
#endif
  assert(!CSC369_InterruptsAreEnabled());
  assert(TCB_CanFree(tid));
  TCB* tcb = &threads[tid]; 
  tcb->state = CSC369_THREAD_FREE;
  tcb->context = (ucontext_t) {0}; // TODO correct?
  tcb->exit_code = 0;
  Queue_Init(&tcb->join_threads);
  free(tcb->stack);
}

void
Queue_FreeAll(CSC369_WaitQueue* queue) {
  int prev_state = CSC369_InterruptsDisable();
  for (int cur = queue->head; cur != queue->tail; cur = (cur + 1) % CSC369_MAX_THREADS) {
    if (TCB_CanFree(queue->threads[cur]))
      TCB_Free(queue->threads[cur]);
  }
  CSC369_InterruptsSet(prev_state);
}

void
ThreadList_Init() {
  for (int i = 0; i < CSC369_MAX_THREADS; i++)
    TCB_Init(&threads[i], i);
}

/**
 * @return unique tid not used by any threads in threads if threads not full, -1 if threads full.
 */
int 
ThreadList_Avail() {
  assert(!CSC369_InterruptsAreEnabled());
  for (int i = 0; i < CSC369_MAX_THREADS; i++)
     if (threads[i].state == CSC369_THREAD_FREE)
       return i;
  return -1;
}

/**
 * Call f then exit properly by calling CSC369_ThreadExit(). 
 */
void ThreadStub(void (*f)(void *), void *arg) {
  Queue_FreeAll(&zombie_threads);
  CSC369_InterruptsEnable();
  f(arg);
  CSC369_ThreadExit(CSC369_EXIT_CODE_NORMAL);
}

/**
 * @return 0 on success, -1 on failure.
 */
int
Context_Create(ucontext_t* context, void (*f)(void*), void* arg, void* stack) {
  assert(!CSC369_InterruptsAreEnabled());
  int err = getcontext(context);
  if (err)
      return -1;
  context->uc_mcontext.gregs[REG_RIP] = (greg_t) &ThreadStub;
  context->uc_mcontext.gregs[REG_RDI] = (greg_t) f;
  context->uc_mcontext.gregs[REG_RSI] = (greg_t) arg;

  context->uc_mcontext.gregs[REG_RSP] = (greg_t) stack + CSC369_THREAD_STACK_SIZE + 15;
  context->uc_mcontext.gregs[REG_RSP] -= (context->uc_mcontext.gregs[REG_RSP] - 8) % 16;
  return 0;
}

/*
 * @return tid if successful, CSC369_ERROR_SYS_MEM if no memory, CSC369_ERROR_OTHER if other error.
 */
int
TCB_Create(Tid tid, void (*f)(void*), void* arg) {
  assert(!CSC369_InterruptsAreEnabled());
  assert(tid >= 0 && tid < CSC369_MAX_THREADS);
  TCB* tcb = &threads[tid];
  assert(tcb->state == CSC369_THREAD_FREE && tcb->tid == tid);
  tcb->state = CSC369_THREAD_READY;
  tcb->stack = malloc(CSC369_THREAD_STACK_SIZE + 16);
  if (tcb->stack == NULL)
    return CSC369_ERROR_SYS_MEM;

  int err = Context_Create(&tcb->context, f, arg, tcb->stack);
  if (err)
    return CSC369_ERROR_OTHER;

  err = Queue_Enqueue(&ready_threads, tid);
  if (err)
    return CSC369_ERROR_OTHER;
  return tid;
}

/**
 * Switch to thread with tid.
 *
 * Assumes tid has already been removed from ready queue.
 *
 * @return Doesn't return if successful, returns -1 on failure.
 */
int Switch(Tid tid) {
  assert(!CSC369_InterruptsAreEnabled());
  assert(tid >= 0 && tid < CSC369_MAX_THREADS);
  TCB *tcb = &threads[tid];
  assert(tcb->state == CSC369_THREAD_READY && tcb->tid == tid);
  tcb->state = CSC369_THREAD_RUNNING;

  if (threads[running_thread].state == CSC369_THREAD_RUNNING) {
    int err = Queue_Enqueue(&ready_threads, running_thread);
    assert(!err);
    threads[running_thread].state = CSC369_THREAD_READY;
  }

  running_thread = tid;
  setcontext(&tcb->context);
  return -1; // shouldn't get here.
}

//**************************************************************************************************
// thread.h Functions
//**************************************************************************************************
int
CSC369_ThreadInit(void)
{
  Queue_Init(&ready_threads);
  Queue_Init(&zombie_threads);
  ThreadList_Init();
  int err = TCB_MainInit();
  if (err)
    return CSC369_ERROR_OTHER;
  return 0;
}

Tid
CSC369_ThreadId(void)
{
  return running_thread;
}

Tid
CSC369_ThreadCreate(void (*f)(void*), void* arg)
{
  Queue_FreeAll(&zombie_threads);
  
  int prev_state = CSC369_InterruptsDisable();
  Tid tid = ThreadList_Avail();
  if (tid == -1)
    return CSC369_ERROR_SYS_THREAD;
  int ret = TCB_Create(tid, f, arg);
  CSC369_InterruptsSet(prev_state);

  return ret;
}

void
CSC369_ThreadExit(int exit_code)
{
  // TODO what if blocked threads?
  // TODO exit code fatal
  // TODO tid 0
  int prev_state = CSC369_InterruptsDisable();
  if (Queue_IsEmpty(&ready_threads)) // empty
     exit(exit_code);
  TCB_Zombify(running_thread, exit_code);
  CSC369_ThreadYield();
  CSC369_InterruptsSet(prev_state);

  exit(-1); // should not get here.
}

Tid
CSC369_ThreadKill(Tid tid)
{
  if (tid == running_thread)
    return CSC369_ERROR_THREAD_BAD;
  else if (tid < 0 || tid >= CSC369_MAX_THREADS)
    return CSC369_ERROR_TID_INVALID;

  int prev_state = CSC369_InterruptsDisable();
  TCB *tcb = &threads[tid]; 
  if (tcb->state == CSC369_THREAD_FREE) // TODO other invalid states?
    return CSC369_ERROR_SYS_THREAD;
  Queue_Remove(&ready_threads, tid); // it might or might not be in ready queue
 
  TCB_Zombify(tid, CSC369_EXIT_CODE_KILL); 
  if (TCB_CanFree(tid))
    TCB_Free(tid);
  CSC369_InterruptsSet(prev_state);
  return tid;
}

int
CSC369_ThreadYield()
{
  int prev_state = CSC369_InterruptsDisable();
  volatile int called = 0;
  int err = getcontext(&threads[running_thread].context); 
  assert(!err); 
  int tid;
  if (!called) {
    tid = Queue_Dequeue(&ready_threads);
    if (tid == -1) // empty ready queue
      return running_thread;

    called = 1;
    Switch(tid);
    return CSC369_ERROR_OTHER; // should not get here.
  }
  CSC369_InterruptsSet(prev_state);
  Queue_FreeAll(&zombie_threads);
  return tid;
}

int
CSC369_ThreadYieldTo(Tid tid)
{
  int prev_state = CSC369_InterruptsDisable();
  if (tid == running_thread)
    return tid;
  if (tid < 0 || tid >= CSC369_MAX_THREADS)
    return CSC369_ERROR_TID_INVALID;
  if (threads[tid].state != CSC369_THREAD_READY)
    return CSC369_ERROR_THREAD_BAD;
  int err = Queue_Remove(&ready_threads, tid);
  assert(!err);   

  volatile int called = 0;
  err = getcontext(&threads[running_thread].context); 
  assert(!err); 
  if (!called) {
    called = 1;
    Switch(tid);
    return CSC369_ERROR_OTHER; // should not get here.
  }
  CSC369_InterruptsSet(prev_state);
  Queue_FreeAll(&zombie_threads);
  return tid;
}

//****************************************************************************
// New Assignment 2 Definitions - Task 2
//****************************************************************************
CSC369_WaitQueue*
CSC369_WaitQueueCreate(void)
{
  int prev_state = CSC369_InterruptsDisable();
  CSC369_WaitQueue* queue = malloc(sizeof(CSC369_WaitQueue));
  Queue_Init(queue);
  CSC369_InterruptsSet(prev_state);
  return queue;
}

int
CSC369_WaitQueueDestroy(CSC369_WaitQueue* queue)
{
  int ret = 0;
  int prev_state = CSC369_InterruptsDisable();
  if (!Queue_IsEmpty(queue))
    ret = CSC369_ERROR_OTHER;
  else
    free(queue);
  CSC369_InterruptsSet(prev_state);
  return ret;
}

void
CSC369_ThreadSpin(int duration)
{
  struct timeval start, end, diff;

  int ret = gettimeofday(&start, NULL);
  assert(!ret);

  while (1) {
    ret = gettimeofday(&end, NULL);
    assert(!ret);
    timersub(&end, &start, &diff);

    if ((diff.tv_sec * 1000000 + diff.tv_usec) >= duration) {
      return;
    }
  }
}

int
CSC369_ThreadSleep(CSC369_WaitQueue* queue)
{
  assert(queue != NULL);
  assert(!Queue_IsFull(queue)); 
  
  int prev_state = CSC369_InterruptsDisable();  
  if (Queue_IsEmpty(&ready_threads))
    return CSC369_ERROR_SYS_THREAD;

  TCB* tcb = &threads[running_thread];
  tcb->state = CSC369_THREAD_BLOCKED; 
  
  CSC369_InterruptsSet(prev_state);
  int ret = CSC369_ThreadYield();
  CSC369_InterruptsSet(prev_state);
  return ret;
}

int
CSC369_ThreadWakeNext(CSC369_WaitQueue* queue)
{
  assert(queue != NULL);
  int ret = 1;
  int prev_state = CSC369_InterruptsDisable();
  Tid tid = Queue_Dequeue(queue);
  if (tid == -1) {
    ret = 0;
  } else {
    TCB* tcb = &threads[tid];
    tcb->state = CSC369_THREAD_READY;
    int err = Queue_Enqueue(&ready_threads, tid);
    assert(!err);
  }
  CSC369_InterruptsSet(prev_state);
  return ret;
}

int
CSC369_ThreadWakeAll(CSC369_WaitQueue* queue)
{
  int prev_state = CSC369_InterruptsDisable();
  int ret = 0;
  while (!Queue_IsEmpty(queue))
    ret += CSC369_ThreadWakeNext(queue);
  CSC369_InterruptsSet(prev_state);
  return ret;
}

//****************************************************************************
// New Assignment 2 Definitions - Task 3
//****************************************************************************
int
CSC369_ThreadJoin(Tid tid, int* exit_code)
{
  if (tid == running_thread)
    return CSC369_ERROR_THREAD_BAD;
  else if (tid < 0 || tid >= CSC369_MAX_THREADS)
    return CSC369_ERROR_TID_INVALID;

  int prev_state = CSC369_InterruptsDisable();
  TCB* tcb = &threads[tid];
  if (tcb->state == CSC369_THREAD_FREE || tcb->state == CSC369_THREAD_ZOMBIE)
    return CSC369_ERROR_SYS_THREAD;
  
  tcb->join_threads_num++;
  int ret = CSC369_ThreadSleep(&tcb->join_threads);
  assert(ret >= 0);
  *exit_code = tcb->exit_code;
  tcb->join_threads_num--;
  if (TCB_CanFree(tcb->tid))
    TCB_Free(tcb->tid);
  CSC369_InterruptsSet(prev_state);
  return tid;
}
