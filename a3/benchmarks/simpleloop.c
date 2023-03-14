#include <stdio.h>
#include <stdlib.h>

#include "marker.h"

#define RECORD_SIZE 128

struct krec
{
  double d[RECORD_SIZE];
};

void
heap_loop(int iters)
{
  int i;
  struct krec* ptr = malloc(iters * sizeof(struct krec));
  for (i = 0; i < iters; i++) {
    ptr[i].d[0] = (double)i;
  }
  free(ptr);
}

void
stack_loop(int iters)
{
  int i;
  struct krec a[iters];
  for (i = 0; i < iters; i++) {
    a[i].d[0] = (double)i;
  }
  (void)a; /* Use a to keep compiler happy */
}

int
main()
{
  write_marker_file("simpleloop.marker");
  MARKER_START();

  heap_loop(10000);
  // stack_loop(100);

  MARKER_END();
  return 0;
}
