/**
 * CSC369 Assignment 2
 *
 * @file Defines the public interface for the CSC369 Thread Library.
 */
#ifndef CSC369_THREAD_H
#define CSC369_THREAD_H

/**
 * Error codes for the CSC369 Thread Library
 */
typedef enum
{
  CSC369_ERROR_TID_INVALID = -1,
  CSC369_ERROR_THREAD_BAD = -2,
  CSC369_ERROR_SYS_THREAD = -3,
  CSC369_ERROR_SYS_MEM = -4,
  CSC369_ERROR_OTHER = -5
} CSC369_ThreadError;

/**
 * Error codes for the CSC369 Thread Library
 */
typedef enum
{
  CSC369_EXIT_CODE_NORMAL = 0,
  CSC369_EXIT_CODE_FATAL = -1,
  CSC369_EXIT_CODE_KILL = -999,
} CSC369_ExitCode;

/**
 * The maximum number of threads supported by the library.
 */
#define CSC369_MAX_THREADS 256

/**
 * The minimum stack size, in bytes, of a thread.
 */
#define CSC369_THREAD_STACK_SIZE 32768

/**
 * The identifier for a thread. Valid ids are non-negative and less than
 * CSC369_MAX_THREADS.
 */
typedef int Tid;

/**
 * Initialize the CSC369 user-level thread library.
 *
 * This must be called before using other functions in this library.
 *
 * @return 0 on success, CSC369_ERROR_OTHER otherwise.
 */
int
CSC369_ThreadInit(void);

/**
 * Get the identifier of the calling thread.
 *
 * @return The calling thread's identifier.
 */
Tid
CSC369_ThreadId(void);

/**
 * Create a new thread that runs the function f with the argument arg.
 *
 * This function may fail if:
 *  - no more threads can be created (CSC369_ERROR_SYS_THREAD), or
 *  - there is no more memory available (CSC369_ERROR_SYS_MEM), or
 *  - something unexpected failed (CSC369_ERROR_OTHER)
 *
 * @param f A pointer to the function that this thread will execute.
 * @param arg The argument passed to f.
 *
 * @return If successful, the new thread's identifier. Otherwise, the
 * appropriate error code.
 */
Tid
CSC369_ThreadCreate(void (*f)(void*), void* arg);

/**
 * Suspend the calling thread and run the next ready thread. The calling thread
 * will be scheduled again after all *currently* ready threads have run.
 *
 * When the calling thread is the only ready thread, it yields to itself.
 *
 * @return The thread identifier yielded to.
 */
Tid
CSC369_ThreadYield(void);

/**
 * Suspend the calling thread and run the thread with identifier tid. The
 * calling thread will be scheduled again after all *currently* ready threads
 * have run.
 *
 * This function may fail if:
 *  - the identifier is invalid (CSC369_ERROR_TID_INVALID), or
 *  - the identifier is valid but that thread cannot run
 * (CSC369_ERROR_THREAD_BAD)
 *
 * @param tid The identifier of the thread to run.
 *
 * @return If successful, the identifier yielded to. Otherwise, the appropriate
 * error code.
 */
int
CSC369_ThreadYieldTo(Tid tid);

/**
 * Exit the calling thread. If the caller is the last thread in the system, then
 * the program exits with the given exit code. If the caller is not the last
 * thread in the system, then the next ready thread will be run.
 *
 * This function may fail when switching to another ready thread. In this case,
 * the program exits with exit code -1.
 *
 * @param exit_code The code to exit the process with.
 *
 * @return This function does not return.
 */
void
CSC369_ThreadExit(int exit_code);

/**
 * Kill the thread whose identifier is tid.
 *
 * The thread that is killed should "exit" with code -999.
 *
 * The thread being killed may be invalid. An invalid thread (not to be
 * confused with an invalid identifier) is a thread that is inactive
 * (e.g., the thread was never created or the thread was cleaned up already).
 *
 * Note: For this function, a "zombie" thread is not "invalid"
 *
 * This function may fail if:
 *  - the identifier is invalid (CSC369_ERROR_TID_INVALID), or
 *  - the identifier is of the calling thread (CSC369_ERROR_THREAD_BAD), or
 *  - the thread is invalid (CSC369_ERROR_SYS_THREAD)
 *
 * @param tid The identifier of the thread to kill.
 *
 * @return If successful, the killed thread's identifier. Otherwise, the
 * appropriate error code.
 */
int
CSC369_ThreadKill(Tid tid);

//****************************************************************************
// New Assignment 2 Definitions - Task 2
//****************************************************************************
/**
 * A (FIFO) queue of waiting threads.
 *
 * Representation Invariants:
 *  - None of the threads in the wait queue are currently running
 *  - A thread cannot be in more than one wait queue at a time
 */
typedef struct csc369_wait_queue_t CSC369_WaitQueue;

/**
 * Create an empty queue for created threads.
 *
 * The queue created by this function must be freed using
 * CSC369_WaitQueueDestroy.
 *
 * @return If successful, a pointer to newly allocated queue. Otherwise, NULL.
 */
CSC369_WaitQueue*
CSC369_WaitQueueCreate(void);

/**
 * Destroy the queue, freeing up allocated memory.
 *
 *  This function may fail if:
 *  - the queue is not empty (CSC369_ERROR_OTHER)
 *
 *  @return If successful, 0. Otherwise, the appropriate error code.
 *
 *  @pre queue is not NULL
 */
int
CSC369_WaitQueueDestroy(CSC369_WaitQueue* queue);

/**
 * Spin for some amount of time.
 *
 * @param duration The time to spin in microseconds.
 */
void
CSC369_ThreadSpin(int duration);

/**
 * Suspend the calling thread, enqueueing it on queue, and run the next ready
 * thread.
 *
 *  This function may fail if:
 *  - there are no other threads that can run (CSC369_ERROR_SYS_THREAD)
 *
 * @param queue The wait queue that the calling thread should be added to.
 *
 * @return If successful, the identifier of the thread that ran. Otherwise, the
 * appropriate error code.
 *
 * @pre queue is not NULL
 */
int
CSC369_ThreadSleep(CSC369_WaitQueue* queue);

/**
 * Wake up the first thread in queue (and move it to the ready queue).
 *
 * The calling thread continues to execute (i.e., it is not suspended).
 *
 * @param queue The wait queue to dequeue.
 *
 * @return The number of threads woken up, which can be 0.
 *
 * @pre queue is not NULL
 */
int
CSC369_ThreadWakeNext(CSC369_WaitQueue* queue);

/**
 * Wake up all threads in queue in FIFO order (and move them to the ready
 * queue).
 *
 * The calling thread continues to execute (i.e., it is not suspended).
 *
 * @param queue The wait queue to dequeue.
 *
 * @return The number of threads woken up, which can be 0.
 *
 * @pre queue is not NULL
 */
int
CSC369_ThreadWakeAll(CSC369_WaitQueue* queue);

//****************************************************************************
// New Assignment 2 Definitions - Task 3
//****************************************************************************
/**
 * Suspend the calling thread until the thread with identifier tid exits. If
 * the thread has already exited, this function returns immediately.
 *
 * The thread being waited for may be invalid. An invalid thread (not to be
 * confused with an invalid identifier) is a thread that is currently being
 * cleaned up (e.g., a zombie) or a thread that is inactive (e.g., the thread
 * was never created or the thread was cleaned up already).
 *
 * This function also copies the exit status of the target thread (tid) to
 * exit_code.
 *
 * This function may fail if:
 *  - the identifier is invalid (CSC369_ERROR_TID_INVALID), or
 *  - the identifier is of the calling thread (CSC369_ERROR_THREAD_BAD), or
 *  - the thread is not valid (CSC369_ERROR_SYS_THREAD)
 *
 * @param tid The identifier of the thread to wait for.
 * @param exit_code The code the thread that finished exited with.
 *
 * @return If successful, the identifier of the thread that exited. Otherwise,
 * the appropriate error code.
 *
 * @pre exit_code is not NULL
 */
int
CSC369_ThreadJoin(Tid tid, int* exit_code);

#endif /* CSC369_THREAD_H */
