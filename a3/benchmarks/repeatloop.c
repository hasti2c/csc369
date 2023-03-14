#include <stdio.h>
#include <stdlib.h>

#include "marker.h"

int
main(int argc, char* argv[])
{
  write_marker_file("repeatloop.marker");
  MARKER_START();

  const char* usage = "Usage: %s <length> <repetitions>\n";
  if (argc < 3) {
    fprintf(stderr, usage, argv[0]);
    return 1;
  }
  size_t length = strtoul(argv[1], NULL, 10);
  size_t repetitions = strtoul(argv[2], NULL, 10);
  if (!length || !repetitions) {
    fprintf(stderr, usage, argv[0]);
    return 1;
  }

  long* data = (long*)malloc(length * sizeof(long));
  if (!data) {
    perror("malloc");
    return 1;
  }

  // Use random and a side effect (printf) to avoid being optimized away
  long salt = random();
  for (size_t i = 0; i < length; ++i) {
    data[i] = i ^ salt;
  }

  long acc = 0;
  for (size_t r = 0; r < repetitions; ++r) {
    for (size_t i = 0; i < length; ++i) {
      acc += data[i];
    }
    acc += random();
  }
  printf("%ld\n", acc);

  MARKER_END();
  return 0;
}
