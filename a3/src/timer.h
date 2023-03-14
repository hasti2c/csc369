#ifndef CSC369_TIMER_H
#define CSC369_TIMER_H

#include <time.h>

// Returns current time in seconds since the Epoch as a floating point number
static inline double
get_time()
{
  struct timespec t;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
  return t.tv_sec + t.tv_nsec / 1000000000.0;
}

#endif /* CSC369_TIMER_H */
