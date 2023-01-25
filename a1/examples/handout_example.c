#include <stdio.h>

#include "csc369_thread.h"

void
hello_world(void* arg)
{
  // Do some work
  char const* msg = arg;
  fprintf(stdout, "TID(%d): %s\n", CSC369_ThreadId(), msg);

  CSC369_ThreadYield(); // yield

  // Do some more work
  fprintf(stdout, "TID(%d): Hello, world!\n", CSC369_ThreadId());

  CSC369_ThreadYield(); // yield

  // implicit thread exit
}

int
main()
{
  // Initialize the user-level thread package
  CSC369_ThreadInit();

  fprintf(stdout, "TID(%d): Create a thread\n", CSC369_ThreadId());
  int const foo_tid = CSC369_ThreadCreate(hello_world, "Foo");

  // Wait for thread to finish
  int tid = foo_tid;
  while (tid == foo_tid) {
    fprintf(stdout, "TID(%d): yield\n", CSC369_ThreadId());
    tid = CSC369_ThreadYield();
  }

  return 0; // exit
}
