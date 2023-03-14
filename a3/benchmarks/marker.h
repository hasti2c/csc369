#ifndef CSC369_MARKER_H
#define CSC369_MARKER_H

#include <stdio.h>
#include <stdlib.h>

static volatile char __start_marker;
static volatile char __end_marker;

static inline void
MARKER_START(void)
{
  __start_marker = 'S';
}

static inline void
MARKER_END(void)
{
  __end_marker = 'E';
}

static inline void
write_marker_file(const char* path)
{
  FILE* f = fopen(path, "w");
  if (!f) {
    perror(path);
    exit(1);
  }
  fprintf(f, "%p %p\n", &__start_marker, &__end_marker);
  fclose(f);
}

#endif /* CSC369_MARKER_H */
