#include <stdlib.h>
#include "check.h"

#include "csc369_thread.h"

void
yield_once(int tid)
{
    CSC369_ThreadYieldTo(tid);
}

void
set_up(void)
{
    ck_assert_int_eq(CSC369_ThreadInit(), 0);
}

void
tear_down(void)
{
}

START_TEST(test_master_has_id_0)
{
    ck_assert_int_eq(CSC369_ThreadId(), 0);
}
END_TEST

START_TEST(test_master_yield_itself)
{
    ck_assert_int_eq(CSC369_ThreadYield(), 0);
}
END_TEST

START_TEST(test_master_yieldto_itself)
{
    ck_assert_int_eq(CSC369_ThreadYieldTo(CSC369_ThreadId()), 0);
}

int
main(void)
{
    TCase *one_thread_case = tcase_create("Student Test Case");
    tcase_add_checked_fixture(one_thread_case, set_up, tear_down);
    tcase_add_test(one_thread_case, test_master_has_id_0);
    tcase_add_test(one_thread_case, test_master_yield_itself);
    tcase_add_test(one_thread_case, test_master_yieldto_itself);

    Suite *suite = suite_create("Student Test Suite");
    suite_add_tcase(suite, one_thread_case);

    SRunner *suite_runner = srunner_create(suite);
    srunner_run_all(suite_runner, CK_VERBOSE);

    int const number_failed = srunner_ntests_failed(suite_runner);
    srunner_free(suite_runner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
