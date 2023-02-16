#ifndef CHECK_THREAD_UTIL_H
#define CHECK_THREAD_UTIL_H

#include "csc369_thread.h"

#define CSC369_TESTS_EXIT_SUCCESS 20231369

extern long* array[CSC369_MAX_THREADS];

void
set_up(void);

void
tear_down(void);

int
yield_till_main_thread(void);

int
yieldto_till_main_thread(int tid);

void
f_do_nothing(void);

void
f_yield_once(int tid);

void
f_yield_twice(int tid);

void
f_no_exit(void);

void
f_save_to_array(int x);

void
f_fp_alignment(void);

int
f_factorial(int n);

void
f_0_has_exited(void);

#endif // CHECK_THREAD_UTIL_H
