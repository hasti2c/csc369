#ifndef CSC369_TIMER_H
#define CSC369_TIMER_H

#include <sys/time.h>

// Returns current time in seconds since the Epoch as a floating point number
static inline double
get_time()
{
  struct timeval t;
  gettimeofday(&t, NULL);
  return t.tv_sec + t.tv_usec / 1000000.0;
}

#endif /* CSC369_TIMER_H */
