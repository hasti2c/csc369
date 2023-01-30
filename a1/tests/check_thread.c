#include "check.h"
#include <malloc.h>
#include <stdlib.h>

#include "csc369_thread.h"

//****************************************************************************
// Private definitions
//****************************************************************************
long* array[CSC369_MAX_THREADS];

static int
set_flag(int val)
{
  static int flag_value;
  return __sync_lock_test_and_set(&flag_value, val);
}

//****************************************************************************
// Functions to pass to CSC369_ThreadCreate
//****************************************************************************
void
f_do_nothing(void)
{}

void
f_yield_once(int tid)
{
  CSC369_ThreadYieldTo(tid);
}

void
f_yield_twice(int tid)
{
  CSC369_ThreadYieldTo(tid);
  CSC369_ThreadYieldTo(tid);
}

void
f_no_exit(void)
{
  while (1) {
    CSC369_ThreadYield();
  }
}

void
f_save_to_array(int x)
{
  array[CSC369_ThreadId()] = (long*)&x;
}

void
f_fp_alignment(void)
{
  Tid tid = CSC369_ThreadYieldTo(CSC369_ThreadId());
  ck_assert_int_gt(tid, 0);
  ck_assert_int_lt(tid, CSC369_MAX_THREADS);

  // We cast the return value to a float because that helps to check whether the
  // stack alignment of the frame pointer is correct
  char str[20];
  sprintf(str, "%3.0f\n", (float)tid);
  // A failure here would be something like a segmentation fault
}

int
f_factorial(int n)
{
  if (n == 1) {
    return 1;
  }

  CSC369_ThreadYield();
  return n * f_factorial(n - 1);
}

void
f_set_flag_and_exit(void)
{
  int const old_flag = set_flag(1);
  ck_assert_int_eq(old_flag, 0);

  CSC369_ThreadExit();
  ck_assert_msg(0, "This thread should have exited.");
}

void
f_0_has_exited(void)
{
  Tid const self = CSC369_ThreadId();

  int const yield1 = CSC369_ThreadYield();
  ck_assert_int_eq(yield1, self);

  int const yield2 = CSC369_ThreadYieldTo(0);
  ck_assert_int_eq(yield2, CSC369_ERROR_THREAD_BAD);
}

//****************************************************************************
// Functions to run before/after every test
//****************************************************************************
void
set_up(void)
{
  ck_assert_int_eq(CSC369_ThreadInit(), 0);
}

void
tear_down(void)
{}

//****************************************************************************
// Test for expected error return values
//****************************************************************************
START_TEST(test_error_0_yieldto_invalid)
{
  ck_assert_int_eq(CSC369_ThreadYieldTo(0xDEADBEEF), CSC369_ERROR_TID_INVALID);
}
END_TEST

START_TEST(test_error_0_kill_self)
{
  ck_assert_int_eq(CSC369_ThreadKill(0), CSC369_ERROR_THREAD_BAD);
}
END_TEST

START_TEST(test_error_0_kill_negative_tid)
{
  ck_assert_int_eq(CSC369_ThreadKill(-42), CSC369_ERROR_TID_INVALID);
}
END_TEST

START_TEST(test_error_0_kill_uncreated_tid)
{
  ck_assert_int_eq(CSC369_ThreadKill(42), CSC369_ERROR_SYS_THREAD);
}
END_TEST

START_TEST(test_error_0_kill_created_tid_twice)
{
  Tid tid = CSC369_ThreadCreate((void (*)(void*))f_no_exit, NULL);
  ck_assert_int_gt(tid, 0);
  ck_assert_int_lt(tid, CSC369_MAX_THREADS);

  ck_assert_int_eq(CSC369_ThreadKill(tid), tid);

  // Potentially necessary, depending on how kill is implemented
  CSC369_ThreadYield();

  ck_assert_int_eq(CSC369_ThreadKill(tid), CSC369_ERROR_SYS_THREAD);
}
END_TEST

START_TEST(test_error_create_more_than_max)
{
  for (int i = 0; i < CSC369_MAX_THREADS - 1; i++) {
    Tid tid = CSC369_ThreadCreate((void (*)(void*))f_yield_once, (void*)0);

    ck_assert_int_gt(tid, 0);
    ck_assert_int_lt(tid, CSC369_MAX_THREADS);
  }

  // Now we are out of threads. Next create should fail.
  Tid tid = CSC369_ThreadCreate((void (*)(void*))f_yield_once, (void*)0);
  ck_assert_int_eq(tid, CSC369_ERROR_SYS_THREAD);
}
END_TEST

//****************************************************************************
// Test for basic functionality when there is only one thread (TID 0)
//****************************************************************************
START_TEST(test_main_thread_has_id_0)
{
  ck_assert_int_eq(CSC369_ThreadId(), 0);
}
END_TEST

START_TEST(test_main_thread_yield_itself)
{
  ck_assert_int_eq(CSC369_ThreadYield(), 0);
}
END_TEST

START_TEST(test_main_thread_yieldto_itself)
{
  ck_assert_int_eq(CSC369_ThreadYieldTo(CSC369_ThreadId()), 0);
}

//****************************************************************************
// Test functionality when there are two threads
//****************************************************************************
START_TEST(test_create_with_explicit_exit)
{
  set_flag(0);
  int const new_tid =
    CSC369_ThreadCreate((void (*)(void*))f_set_flag_and_exit, NULL);
  ck_assert_int_ge(new_tid, 1);

  int const yield_tid = CSC369_ThreadYieldTo(new_tid);
  ck_assert_int_eq(yield_tid, new_tid);
  ck_assert_int_eq(set_flag(0), 1);

  int const yield_tid2 = CSC369_ThreadYieldTo(new_tid);
  ck_assert_int_eq(yield_tid2, CSC369_ERROR_THREAD_BAD);
}

START_TEST(test_0_with_explicit_exit)
{
  set_flag(0);
  int const new_tid =
    CSC369_ThreadCreate((void (*)(void*))f_0_has_exited, NULL);
  ck_assert_int_ge(new_tid, 1);

  CSC369_ThreadExit();
  ck_assert_msg(0, "This thread should have exited.");
}

START_TEST(test_create_with_recursion)
{
  set_flag(0);
  int const new_tid =
    CSC369_ThreadCreate((void (*)(void*))f_factorial, (void*)10);
  ck_assert_int_ge(new_tid, 1);

  // Yield until we are back at the main thread
  int result;
  int num_yields = 0;
  do {
    result = CSC369_ThreadYieldTo(new_tid);
    num_yields++;
  } while (result != CSC369_ERROR_THREAD_BAD);

  ck_assert_int_eq(num_yields, 11);
}

//****************************************************************************
// Test behaviour when the maximum number of threads are created
//****************************************************************************
START_TEST(test_yield_and_recreate_all)
{
  for (int i = 0; i < CSC369_MAX_THREADS - 1; i++) {
    Tid tid = CSC369_ThreadCreate((void (*)(void*))f_yield_once, (void*)0);

    ck_assert_int_gt(tid, 0);
    ck_assert_int_lt(tid, CSC369_MAX_THREADS);
  }

  // Yield until we are back at the main thread
  int result;
  do {
    result = CSC369_ThreadYield();
  } while (result != 0);

  // Recreate the maximum number of threads, minus one
  for (int i = 0; i < CSC369_MAX_THREADS - 1; i++) {
    int new_tid = CSC369_ThreadCreate((void (*)(void*))f_yield_once, (void*)0);

    ck_assert_int_gt(new_tid, 0);
    ck_assert_int_lt(new_tid, CSC369_MAX_THREADS);
  }
}
END_TEST

START_TEST(test_yield_and_kill_all)
{
  Tid children[CSC369_MAX_THREADS - 1];

  for (int i = 0; i < CSC369_MAX_THREADS - 1; i++) {
    // Create a thread that yield back to the main thread (TID 0)
    children[i] = CSC369_ThreadCreate((void (*)(void*))f_yield_twice, (void*)0);

    ck_assert_int_gt(children[i], 0);
    ck_assert_int_lt(children[i], CSC369_MAX_THREADS);
  }

  // Let all threads yield back to the main thread
  for (int i = 0; i < CSC369_MAX_THREADS; i++) {
    int result = CSC369_ThreadYield();

    ck_assert_int_ge(result, 0);
    ck_assert_int_lt(result, CSC369_MAX_THREADS);
  }

  // Kill all non-main threads
  for (int i = 1; i < CSC369_MAX_THREADS - 1; i++) {
    int const tid = children[i];
    ck_assert_int_eq(CSC369_ThreadKill(tid), tid);
  }

  // Yield until we are back at the main  thread, in case killed threads need to
  // run and exit
  int result;
  int i = 0;
  do {
    result = CSC369_ThreadYield();
    i++;
  } while (result != 0);

  // Should only need to yield at most CSC369_MAX_THREADS - 1 + 2 times.
  ck_assert_int_le(i, CSC369_MAX_THREADS + 1);
}
END_TEST

//****************************************************************************
// Test for how the library manages (allocates, frees) memory
//****************************************************************************
START_TEST(test_dynamically_allocates_stack)
{
  struct mallinfo info = mallinfo();
  int const allocated_space = info.uordblks;

  int new_tid = CSC369_ThreadCreate((void (*)(void*))f_do_nothing, NULL);
  ck_assert_int_gt(new_tid, 0);
  ck_assert_int_lt(new_tid, CSC369_MAX_THREADS);

  info = mallinfo();
  ck_assert_int_gt(info.uordblks, allocated_space);
}
END_TEST

START_TEST(test_stacks_sufficiently_apart)
{
  // Create a variable on the stack
  int x = 5;
  // Save the address of that variable
  array[CSC369_ThreadId()] = (long*)&x;

  for (int i = 0; i < CSC369_MAX_THREADS - 1; i++) {
    Tid tid = CSC369_ThreadCreate((void (*)(void*))f_save_to_array, (void*)4);

    ck_assert_int_gt(tid, 0);
    ck_assert_int_lt(tid, CSC369_MAX_THREADS);
  }

  // Yield until we are back at the main thread
  int result;
  do {
    result = CSC369_ThreadYield();
  } while (result != 0);

  // Pairwise comparison of stack addresses to make sure they are sufficiently
  // far apart
  for (int tid_a = 0; tid_a < CSC369_MAX_THREADS; tid_a++) {
    for (int tid_b = 0; tid_b < CSC369_MAX_THREADS; tid_b++) {
      if (tid_a != tid_b) {
        long const stack_sep = (long)(array[tid_a]) - (long)(array[tid_b]);
        ck_assert_int_ge(labs(stack_sep), CSC369_THREAD_STACK_SIZE);
      }
    }
  }
}
END_TEST

START_TEST(test_fp_alignment)
{
  int new_tid = CSC369_ThreadCreate((void (*)(void*))f_fp_alignment, NULL);
  ck_assert_int_gt(new_tid, 0);
  ck_assert_int_lt(new_tid, CSC369_MAX_THREADS);

  int yield_tid = CSC369_ThreadYieldTo(new_tid);
  ck_assert_int_eq(new_tid, yield_tid);
}
END_TEST

//****************************************************************************
// libcheck boilerplate
//****************************************************************************
int
main(void)
{
  TCase* errors_case = tcase_create("Errors Test Case");
  tcase_add_checked_fixture(errors_case, set_up, tear_down);
  tcase_add_test(errors_case, test_error_0_yieldto_invalid);
  tcase_add_test(errors_case, test_error_0_kill_self);
  tcase_add_test(errors_case, test_error_0_kill_negative_tid);
  tcase_add_test(errors_case, test_error_0_kill_uncreated_tid);
  tcase_add_test(errors_case, test_error_0_kill_created_tid_twice);
  tcase_add_test(errors_case, test_error_create_more_than_max);

  TCase* one_thread_case = tcase_create("One Thread Case");
  tcase_add_checked_fixture(one_thread_case, set_up, tear_down);
  tcase_add_test(one_thread_case, test_main_thread_has_id_0);
  tcase_add_test(one_thread_case, test_main_thread_yield_itself);
  tcase_add_test(one_thread_case, test_main_thread_yieldto_itself);

  TCase* two_threads_case = tcase_create("Two Threads Case");
  tcase_add_checked_fixture(two_threads_case, set_up, tear_down);
  tcase_add_test(two_threads_case, test_create_with_explicit_exit);
  tcase_add_test(two_threads_case, test_create_with_recursion);
  tcase_add_test(two_threads_case, test_0_with_explicit_exit);

  TCase* max_threads_case = tcase_create("Max Threads Case");
  tcase_add_checked_fixture(max_threads_case, set_up, tear_down);
  tcase_add_test(max_threads_case, test_yield_and_recreate_all);
  tcase_add_test(max_threads_case, test_yield_and_kill_all);

  TCase* memory_case = tcase_create("Memory Case");
  tcase_add_checked_fixture(memory_case, set_up, tear_down);
  tcase_add_test(memory_case, test_dynamically_allocates_stack);
  tcase_add_test(memory_case, test_stacks_sufficiently_apart);
  tcase_add_test(memory_case, test_fp_alignment);

  Suite* suite = suite_create("Student Test Suite");
  suite_add_tcase(suite, errors_case);
  suite_add_tcase(suite, one_thread_case);
  suite_add_tcase(suite, two_threads_case);
  suite_add_tcase(suite, max_threads_case);
  suite_add_tcase(suite, memory_case);

  SRunner* suite_runner = srunner_create(suite);
  srunner_run_all(suite_runner, CK_VERBOSE);

  int const number_failed = srunner_ntests_failed(suite_runner);
  srunner_free(suite_runner);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
