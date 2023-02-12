#include "csc369_thread.h"

#include <ucontext.h>

#include <assert.h>
#include <sys/time.h>

// TODO: You may find this useful, otherwise remove it
//#define DEBUG_USE_VALGRIND // uncomment to debug with valgrind
#ifdef DEBUG_USE_VALGRIND
#include <valgrind/valgrind.h>
#endif

#include "csc369_interrupts.h"

//****************************************************************************
// Private Definitions
//****************************************************************************
// TODO: You may find this useful, otherwise remove it
typedef enum
{
  CSC369_THREAD_FREE = 0,
  CSC369_THREAD_READY = 1,
  CSC369_THREAD_RUNNING = 2,
  CSC369_THREAD_ZOMBIE = 3,
  CSC369_THREAD_BLOCKED = 4
} CSC369_ThreadState;

/**
 * The Thread Control Block.
 */
typedef struct
{
  // TODO: Populate this struct with other things you need

  /**
   * The thread context.
   */
  ucontext_t context; // TODO: You may find this useful, otherwise remove it

  /**
   * What code the thread exited with.
   */
  int exit_code; // TODO: You may find this useful, otherwise remove it

  /**
   * The queue of threads that are waiting on this thread to finish.
   */
  CSC369_WaitQueue* join_threads; // TODO: You may find this useful, otherwise remove it
} TCB;

/**
 * A wait queue.
 */
typedef struct csc369_wait_queue_t
{
  TCB* head;
} CSC369_WaitQueue;

//**************************************************************************************************
// Private Global Variables (Library State)
//**************************************************************************************************
/**
 * All possible threads have their control blocks stored contiguously in memory.
 */
TCB threads[CSC369_MAX_THREADS]; // TODO: you may find this useful, otherwise remove it

/**
 * Threads that are ready to run in FIFO order.
 */
CSC369_WaitQueue ready_threads; // TODO: you may find this useful, otherwise remove it

/**
 * Threads that need to be cleaned up.
 */
CSC369_WaitQueue zombie_threads; // TODO: you may find this useful, otherwise remove it

//**************************************************************************************************
// Helper Functions
//**************************************************************************************************
void // TODO: You may find this useful, otherwise remove it
Queue_Init(CSC369_WaitQueue* queue)
{
  // FIXME
}

int // TODO: You may find this useful, otherwise remove it
Queue_IsEmpty(CSC369_WaitQueue* queue)
{
  return 0; // FIXME
}

void // TODO: You may find this useful, otherwise remove it
Queue_Enqueue(CSC369_WaitQueue* queue, TCB* tcb)
{
  // FIXME
}

TCB* // TODO: You may find this useful, otherwise remove it
Queue_Dequeue(CSC369_WaitQueue* queue)
{
  return NULL; // FIXME
}

void // TODO: You may find this useful, otherwise remove it
Queue_Remove(CSC369_WaitQueue* queue, TCB* tcb)
{
  // FIXME
}

void // TODO: You may find this useful, otherwise remove it
TCB_Init(TCB* tcb, Tid thread_id)
{
  // FIXME
}

void // TODO: You may find this useful, otherwise remove it
TCB_Free(TCB* tcb)
{
  // FIXME
#ifdef DEBUG_USE_VALGRIND
  // VALGRIND_STACK_DEREGISTER(...);
#endif
}

// TODO: You may find it useful to create a helper function to create a context
// TODO: You may find it useful to create a helper function to switch contexts

//**************************************************************************************************
// thread.h Functions
//**************************************************************************************************
int
CSC369_ThreadInit(void)
{
  return -1;
}

Tid
CSC369_ThreadId(void)
{
  return -1;
}

Tid
CSC369_ThreadCreate(void (*f)(void*), void* arg)
{
  return -1;
}

void
CSC369_ThreadExit(int exit_code)
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

//****************************************************************************
// New Assignment 2 Definitions - Task 2
//****************************************************************************
CSC369_WaitQueue*
CSC369_WaitQueueCreate(void)
{
  return NULL;
}

int
CSC369_WaitQueueDestroy(CSC369_WaitQueue* queue)
{
  return -1;
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
  return -1;
}

int
CSC369_ThreadWakeNext(CSC369_WaitQueue* queue)
{
  assert(queue != NULL);
  return -1;
}

int
CSC369_ThreadWakeAll(CSC369_WaitQueue* queue)
{
  assert(queue != NULL);
  return -1;
}

//****************************************************************************
// New Assignment 2 Definitions - Task 3
//****************************************************************************
int
CSC369_ThreadJoin(Tid tid, int* exit_code)
{
  return -1;
}
