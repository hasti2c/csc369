/**
 * @file An application that relies on preemption to pass a hot potato around.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "csc369_interrupts.h"
#include "csc369_thread.h"

// Number of threads to create
#define THREAD_COUNT 64
// Approximately how long the main thread will spin
#define RUNTIME_DURATION 10000000

// Each thread tracks whether it has the hot potato
int thread_storage[THREAD_COUNT];
// One lock for the thread_storage array
int lock = 0;
// The original start time
struct timeval start_time;

void
report_pass(int from, int to)
{
  struct timeval pend, pdiff;
  gettimeofday(&pend, NULL);
  timersub(&pend, &start_time, &pdiff);

  CSC369_InterruptsPrintf("%9.6f: hot potato passed from %d to %d.\n",
                          (float)pdiff.tv_sec + (float)pdiff.tv_usec / 1000000,
                          from,
                          to);
}

int
pass_hot_potato_randomly(int this_index)
{
  assert(CSC369_InterruptsAreEnabled());

  double const rand = ((double)random()) / RAND_MAX;
  int const next_index = (int)(rand * THREAD_COUNT - 1);

  int err = __sync_bool_compare_and_swap(&lock, 0, 1);
  if (err == 0) {
    return 0; // Could not acquire the lock, try again later
  }

  if (thread_storage[this_index] == 1) {
    // We have the hot potato, pass it!
    thread_storage[this_index] = 0;
    thread_storage[next_index] = 1;

    report_pass(this_index, next_index);
  }

  err = __sync_bool_compare_and_swap(&lock, 1, 0);
  assert(err); // We should definitely be able to release the lock

  return 1;
}

int
pass_hot_potato(int this_index)
{
  assert(CSC369_InterruptsAreEnabled());

  int const next_index = (this_index + 1) % THREAD_COUNT;

  int err = __sync_bool_compare_and_swap(&lock, 0, 1);
  if (!err) {
    return 0; // Could not acquire the lock, try again later
  }

  if (thread_storage[this_index] == 1) {
    thread_storage[this_index] = 0;
    thread_storage[next_index] = 1;
    report_pass(this_index, next_index);
  }

  err = __sync_bool_compare_and_swap(&lock, 1, 0);
  assert(err); // We should definitely be able to release the lock

  return 1;
}

void
f_potato(int this_index)
{
  assert(CSC369_InterruptsAreEnabled());
  int pass = 1;

  while (1) {
    // TODO: Once you have pass_hot_potato working, consider trying
    //  pass_hot_potato_randomly
    int const was_passed = pass_hot_potato(this_index);
    // int const was_passed = pass_hot_potato_randomly(this_index);
    pass += was_passed;

    CSC369_ThreadSpin(CSC369_INTERRUPTS_SIGNAL_INTERVAL * 2);
  }

  // This thread function must be killed to exit
}

void
run_hot_potato()
{
  srandom(369);

  CSC369_InterruptsPrintf("Starting hot potato.\n");
  gettimeofday(&start_time, NULL);

  thread_storage[0] = 1; // Give the hot potato to index 0
  for (int i = 1; i < THREAD_COUNT; i++) {
    thread_storage[i] = 0; // All other indexes do not have the potato
  }

  Tid potato_tids[THREAD_COUNT];
  for (long i = 0; i < THREAD_COUNT; i++) {
    potato_tids[i] = CSC369_ThreadCreate((void (*)(void*))f_potato, (void*)i);
  }

  CSC369_ThreadSpin(RUNTIME_DURATION);

  CSC369_InterruptsPrintf("Killing all created threads.\n");
  for (int i = 0; i < THREAD_COUNT; i++) {
    assert(CSC369_InterruptsAreEnabled());
    CSC369_ThreadKill(potato_tids[i]);
  }

  CSC369_InterruptsPrintf("Hot potato is done.\n");
}

int
main()
{
  // Initialize the user-level thread package
  CSC369_ThreadInit();
  // Initialize and enable interrupts
  CSC369_InterruptsInit();
  // Uninterrupted prints are expensive, keep the interrupts logging quiet
  CSC369_InterruptsSetLogLevel(CSC369_INTERRUPTS_QUIET);

  run_hot_potato();

  return 0;
}