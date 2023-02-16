#include "check_thread_util.h"

#include <stdio.h>
#include <unistd.h>

#include "check.h"

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
// Other definitions
//****************************************************************************
long* array[CSC369_MAX_THREADS];

int
yield_till_main_thread(void)
{
  int num_yields = 0;

  // Yield until we are back at the main thread
  int result;
  do {
    result = CSC369_ThreadYield();
    ck_assert_int_ge(result, 0);
    ck_assert_int_lt(result, CSC369_MAX_THREADS);

    num_yields++;
  } while (result != 0);

  return num_yields;
}

int
yieldto_till_main_thread(int tid)
{
  int num_yields = 0;

  // Yield until we are back at the main thread
  int result;
  do {
    result = CSC369_ThreadYieldTo(tid);
    num_yields++;
  } while (result != CSC369_ERROR_THREAD_BAD);

  return num_yields;
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
f_0_has_exited(void)
{
  Tid const self = CSC369_ThreadId();

  int const yield1 = CSC369_ThreadYield();
  ck_assert_int_eq(yield1, self);

  int const yield2 = CSC369_ThreadYieldTo(0);
  ck_assert_int_eq(yield2, CSC369_ERROR_THREAD_BAD);

  // Make sure the whole process did not exit prematurely
  // (may result in leaks)
  _exit(CSC369_TESTS_EXIT_SUCCESS);
}
