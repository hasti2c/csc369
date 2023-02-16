#include "check.h"

#include <stdlib.h>
#include <unistd.h>

#include "csc369_interrupts.h"
#include "csc369_thread.h"

#include "check_thread_util.h"

#define THREAD_COUNT 128
#define EXIT_CODE_1 42

int shared_integer = 0;

//****************************************************************************
// Functions to pass to CSC369_ThreadCreate
//****************************************************************************
void
f_yield_explicit_exit(int exit_status)
{
  ck_assert(CSC369_InterruptsAreEnabled());

  // Yield until we are the only running thread
  while(CSC369_ThreadYield() != CSC369_ThreadId());

  // Now exit
  CSC369_ThreadExit(exit_status);
}

void
f_join(int tid) {
  int exit_code;
  int const join_ret = CSC369_ThreadJoin(tid, &exit_code);
  f_factorial(3);

  if(join_ret == tid) {
    // this thread joined with parent before parent exited
    ck_assert_int_eq(exit_code, EXIT_CODE_1);
  } else {
    ck_assert_int_eq(join_ret, CSC369_ERROR_SYS_THREAD);
  }

  CSC369_ThreadExit(CSC369_TESTS_EXIT_SUCCESS);
}

void
f_join_max(int tid) {
  int exit_code;
  int const join_ret = CSC369_ThreadJoin(tid, &exit_code);
  f_factorial(3);

  if (join_ret == tid) {
    // this thread joined with parent before parent exited
    ck_assert_int_eq(exit_code, EXIT_CODE_1);
  } else {
    ck_assert_int_eq(join_ret, CSC369_ERROR_SYS_THREAD);
  }

  if (__sync_fetch_and_add(&shared_integer, 1) == THREAD_COUNT - 1) {
    _exit(CSC369_TESTS_EXIT_SUCCESS);
  }

  // otherwise, normal exit from stub
}

void
f_sleep(void *arg)
{
  CSC369_WaitQueue* queue = (CSC369_WaitQueue*) arg;
  CSC369_ThreadSleep(queue);
}

void
f_kill(int tid)
{
  // Yield until we are the only running thread
  while(CSC369_ThreadYield() != CSC369_ThreadId());

  ck_assert_int_eq(CSC369_ThreadKill(tid), tid);

  int exit_code;
  ck_assert_int_eq(CSC369_ThreadJoin(tid, &exit_code), CSC369_ERROR_SYS_THREAD);

  _exit(CSC369_TESTS_EXIT_SUCCESS);
}

//****************************************************************************
// Functions to run before/after every test
//****************************************************************************
void
set_up_with_interrupts(void)
{
  ck_assert_int_eq(CSC369_ThreadInit(), 0);
  CSC369_InterruptsInit();
}

//****************************************************************************
// Testing sleep and wakeup behaviour
//****************************************************************************
START_TEST(test_sleep_no_ready_threads)
{
  CSC369_WaitQueue *queue = CSC369_WaitQueueCreate();
  // if we ran out of memory here, something is very wrong
  ck_assert(queue != NULL);

  ck_assert_int_eq(CSC369_ThreadSleep(queue), CSC369_ERROR_SYS_THREAD);
  ck_assert_int_eq(CSC369_WaitQueueDestroy(queue), 0);

  _exit(CSC369_TESTS_EXIT_SUCCESS);
}
END_TEST

START_TEST(test_sleep_f_sleep)
{
  CSC369_WaitQueue *queue = CSC369_WaitQueueCreate();
  // if we ran out of memory here, something is very wrong
  ck_assert(queue != NULL);

  Tid const tid = CSC369_ThreadCreate((void (*)(void*)) f_sleep, (void*) queue);
  ck_assert_int_gt(tid, 0);
  ck_assert_int_lt(tid, CSC369_MAX_THREADS);

  // Let the created thread sleep. Note, from CSC369_ThreadYield:
  // "When the calling thread is the only ready thread, it yields to itself."
  yield_till_main_thread();

  // Created thread should be on the queue since it hasn't been woken up
  ck_assert_int_eq(CSC369_WaitQueueDestroy(queue), CSC369_ERROR_OTHER);

  _exit(CSC369_TESTS_EXIT_SUCCESS);
}
END_TEST

START_TEST(test_wakenext_f_sleep)
{
  CSC369_WaitQueue *queue = CSC369_WaitQueueCreate();
  // if we ran out of memory here, something is very wrong
  ck_assert(queue != NULL);

  Tid const tid = CSC369_ThreadCreate((void (*)(void*)) f_sleep, (void*) queue);
  ck_assert_int_gt(tid, 0);
  ck_assert_int_lt(tid, CSC369_MAX_THREADS);

  // Let the created thread sleep. Note, from CSC369_ThreadYield:
  // "When the calling thread is the only ready thread, it yields to itself."
  yield_till_main_thread();

  // Created thread should be on the queue since it hasn't been woken up
  ck_assert_int_eq(CSC369_WaitQueueDestroy(queue), CSC369_ERROR_OTHER);
  ck_assert_int_eq(CSC369_ThreadWakeNext(queue), 1);

  _exit(CSC369_TESTS_EXIT_SUCCESS);
}
END_TEST

START_TEST(test_wakeall_f_sleep)
{
  CSC369_WaitQueue *queue = CSC369_WaitQueueCreate();
  // if we ran out of memory here, something is very wrong
  ck_assert(queue != NULL);

  Tid const tid = CSC369_ThreadCreate((void (*)(void*)) f_sleep, (void*) queue);
  ck_assert_int_gt(tid, 0);
  ck_assert_int_lt(tid, CSC369_MAX_THREADS);

  // Let the created thread sleep. Note, from CSC369_ThreadYield:
  // "When the calling thread is the only ready thread, it yields to itself."
  yield_till_main_thread();

  // Created thread should be on the queue since it hasn't been woken up
  ck_assert_int_eq(CSC369_WaitQueueDestroy(queue), CSC369_ERROR_OTHER);
  ck_assert_int_eq(CSC369_ThreadWakeAll(queue), 1);

  _exit(CSC369_TESTS_EXIT_SUCCESS);
}
END_TEST

START_TEST(test_wakeall_f_sleep_max)
{
  CSC369_WaitQueue *queue = CSC369_WaitQueueCreate();
  // if we ran out of memory here, something is very wrong
  ck_assert(queue != NULL);

  Tid children[CSC369_MAX_THREADS - 1];
  for (int i = 0; i < CSC369_MAX_THREADS - 1; i++) {
    // Create a thread that sleeps
    children[i] = CSC369_ThreadCreate((void (*)(void*))f_sleep, (void*)queue);
    ck_assert_int_gt(children[i], 0);
    ck_assert_int_lt(children[i], CSC369_MAX_THREADS);
  }

  // Let the created thread sleep. Note, from CSC369_ThreadYield:
  // "When the calling thread is the only ready thread, it yields to itself."
  yield_till_main_thread();

  // Created threads should be on the queue since they have not been woken up
  ck_assert_int_eq(CSC369_WaitQueueDestroy(queue), CSC369_ERROR_OTHER);
  ck_assert_int_eq(CSC369_ThreadWakeAll(queue), CSC369_MAX_THREADS - 1);

  _exit(CSC369_TESTS_EXIT_SUCCESS);
}
END_TEST

//****************************************************************************
// Testing join behaviour
//****************************************************************************
START_TEST(test_join_self)
{
  ck_assert_int_eq(CSC369_ThreadId(), 0);

  int exit_value;
  ck_assert_int_eq(CSC369_ThreadJoin(0, &exit_value), CSC369_ERROR_THREAD_BAD);

  _exit(CSC369_TESTS_EXIT_SUCCESS);
}
END_TEST

START_TEST(test_join_uncreated_tid)
{
  int exit_value;
  ck_assert_int_eq(CSC369_ThreadJoin(5, &exit_value), CSC369_ERROR_SYS_THREAD);

  _exit(CSC369_TESTS_EXIT_SUCCESS);
}
END_TEST

START_TEST(test_join_created_thread)
{
  int desired_exit = 42;

  Tid const tid = CSC369_ThreadCreate((void (*)(void*)) f_yield_explicit_exit, (void*) (size_t) desired_exit);
  ck_assert_int_gt(tid, 0);
  ck_assert_int_lt(tid, CSC369_MAX_THREADS);

  int exit_value;
  ck_assert_int_eq(CSC369_ThreadJoin(tid, &exit_value), tid);
  ck_assert_int_eq(exit_value, desired_exit);

  _exit(CSC369_TESTS_EXIT_SUCCESS);
}
END_TEST

START_TEST(test_join_main_exits)
{
  Tid const tid = CSC369_ThreadCreate((void (*)(void*)) f_join, (void*) 0);
  ck_assert_int_gt(tid, 0);
  ck_assert_int_lt(tid, CSC369_MAX_THREADS);

  // Give the created thread a chance to start
  int const yield_ret = CSC369_ThreadYieldTo(tid);
  ck_assert(yield_ret == tid || yield_ret == CSC369_ERROR_THREAD_BAD);

  CSC369_ThreadExit(EXIT_CODE_1);
}
END_TEST

START_TEST(test_join_main_exits_many)
{
  shared_integer = 0;

  Tid children[THREAD_COUNT];
  for (int i = 0; i < THREAD_COUNT; i++) {
    children[i] = CSC369_ThreadCreate((void (*)(void*))f_join_max, (void*) 0);
    ck_assert_int_gt(children[i], 0);
    ck_assert_int_lt(children[i], CSC369_MAX_THREADS);

    // Give the created thread a chance to start
    int const yield_ret = CSC369_ThreadYieldTo(children[i]);
    ck_assert(yield_ret == children[i] || yield_ret == CSC369_ERROR_THREAD_BAD);
  }

  CSC369_ThreadExit(EXIT_CODE_1);
}
END_TEST

START_TEST(test_join_main_is_killed)
{
  Tid const tid = CSC369_ThreadCreate((void (*)(void*)) f_kill, (void*) 0);
  ck_assert_int_gt(tid, 0);
  ck_assert_int_lt(tid, CSC369_MAX_THREADS);

  // Wait on our killer
  int exit_code;
  CSC369_ThreadJoin(tid, &exit_code);

  ck_abort_msg("The main thread should have been killed.");
}
END_TEST

//****************************************************************************
// libcheck boilerplate
//****************************************************************************
int
main(void)
{
  TCase* sleep_case = tcase_create("Sleep and Wake Test Case");
  tcase_add_checked_fixture(sleep_case, set_up_with_interrupts, NULL);
  tcase_add_exit_test(sleep_case, test_sleep_no_ready_threads, CSC369_TESTS_EXIT_SUCCESS);
  tcase_add_exit_test(sleep_case, test_sleep_f_sleep, CSC369_TESTS_EXIT_SUCCESS);
  tcase_add_exit_test(sleep_case, test_wakenext_f_sleep, CSC369_TESTS_EXIT_SUCCESS);
  tcase_add_exit_test(sleep_case, test_wakeall_f_sleep, CSC369_TESTS_EXIT_SUCCESS);
  tcase_add_exit_test(sleep_case, test_wakeall_f_sleep_max, CSC369_TESTS_EXIT_SUCCESS);

  TCase* join_case = tcase_create("Join Test Case");
  tcase_add_checked_fixture(join_case, set_up_with_interrupts, NULL);
  tcase_add_exit_test(join_case, test_join_self, CSC369_TESTS_EXIT_SUCCESS);
  tcase_add_exit_test(join_case, test_join_uncreated_tid, CSC369_TESTS_EXIT_SUCCESS);
  tcase_add_exit_test(join_case, test_join_created_thread, CSC369_TESTS_EXIT_SUCCESS);
  tcase_add_exit_test(join_case, test_join_main_exits, CSC369_TESTS_EXIT_SUCCESS);
  tcase_add_exit_test(join_case, test_join_main_exits_many, CSC369_TESTS_EXIT_SUCCESS);
  tcase_add_exit_test(join_case, test_join_main_is_killed, CSC369_TESTS_EXIT_SUCCESS);

  Suite* suite = suite_create("Student Test Suite");
  suite_add_tcase(suite, sleep_case);
  suite_add_tcase(suite, join_case);

  SRunner* suite_runner = srunner_create(suite);
  srunner_run_all(suite_runner, CK_VERBOSE);

  srunner_ntests_failed(suite_runner);
  srunner_free(suite_runner);

  return EXIT_SUCCESS;
}
