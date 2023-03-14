#include "sim.h"
#include "pagetable_generic.h"
#include "swap.h"
#include "timer.h"
#include <assert.h>
#include <err.h>
#include <getopt.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Define global variables declared in sim.h
size_t memsize = 0;
bool debug = false;
unsigned char* physmem = NULL;
struct frame* coremap = NULL;
char* tracefile = NULL;

/*
 * Add up all memory in simulator process's maps. Subtract baseline usage for
 *
 */

unsigned long
get_bytes_used(struct mallinfo* start_mallinfo)
{
  char filename[128];
  char line[256];
  char mapsource[80];
  FILE* f;
  char ex = 0;
  int matches = 0;
  unsigned long start;
  unsigned long end;
  unsigned long all_bytes = 0;

  sprintf(filename, "/proc/%d/maps", getpid());
  f = fopen(filename, "r");
  assert(f);
  do {
    /* read line */
    if (fgets(line, sizeof(line), f) != NULL) {
      matches = sscanf(line,
                       "%lx-%lx %*c%*c%c%*c %*s %*s %*s %s",
                       &start,
                       &end,
                       &ex,
                       mapsource);
    } else {
      matches = EOF;
    }
    if (ex != 'x' && (strstr(mapsource, "/sim") != NULL)) {
      unsigned long bytes = end - start;
      all_bytes += bytes;
    }
  } while (matches > 0);

  struct mallinfo end_mallinfo = mallinfo();
  unsigned long start_malbytes =
    start_mallinfo->hblkhd + start_mallinfo->uordblks;
  unsigned long end_malbytes = end_mallinfo.hblkhd + end_mallinfo.uordblks;
  unsigned long malloc_bytes = end_malbytes - start_malbytes;

  all_bytes += malloc_bytes;

  return all_bytes;
}

/* Each eviction algorithm is represented by a structure with its name
 * and three functions.
 */
struct functions
{
  const char* name;      // String name of eviction algorithm
  void (*init)(void);    // Initialize any data needed by alg
  void (*cleanup)(void); // Cleanup any data initialized in init()
  void (*ref)(int);      // Called on each reference
  int (*evict)(void);    // Called to choose victim for eviction
};

/* The algs array gives us a mapping between the name of an eviction
 * algorithm as given in a command line argument, and the function to
 * call to select the victim page.
 */
static struct functions algs[] = {
  { "rand", rand_init, rand_cleanup, rand_ref, rand_evict },
  { "rr", rr_init, rr_cleanup, rr_ref, rr_evict },
  { "clock", clock_init, clock_cleanup, clock_ref, clock_evict },
  { "lru", lru_init, lru_cleanup, lru_ref, lru_evict },
};
static size_t num_algs = sizeof(algs) / sizeof(algs[0]);

static void (*init_func)() = NULL;
static void (*cleanup_func)() = NULL;

void (*ref_func)(int) = NULL;
int (*evict_func)() = NULL;

/* An actual memory access based on the vaddr from the trace file.
 *
 * The find_physpage() function is called to translate the virtual address
 * to a (simulated) physical address -- that is, a pointer to the right
 * location in physmem array. The find_physpage() function is responsible for
 * everything to do with memory management - including translation using the
 * pagetable, allocating a frame of (simulated) physical memory (if needed),
 * evicting an existing page from the frame (if needed) and reading the page
 * in from swap (if needed).
 *
 * We then check that the memory has the expected content (just a copy of the
 * virtual address) and, in case of a write reference, increment the version
 * counter.
 */
static void
access_mem(char type, vaddr_t vaddr, unsigned char val, size_t linenum)
{
  unsigned char* pgptr;
  unsigned char* memptr;
  unsigned offset = vaddr % PAGE_SIZE;

  pgptr = find_physpage(vaddr, type);
  memptr = pgptr + offset;

  if ((type == 'S') || (type == 'M')) {
    // write access to page, update value in simulated memory
    *memptr = val;
  } else if ((type == 'L' || type == 'I')) {
    if (*memptr != val) {
      printf("ERROR at trace line %zu: vaddr has %hhu but should have %hhu\n",
             linenum,
             *memptr,
             val);
    }
  }
}

static void
replay_trace(FILE* f)
{
  char line[256];
  size_t linenum = 0;
  while (fgets(line, sizeof(line), f)) {
    ++linenum;
    if (line[0] == '=') {
      continue;
    }

    vaddr_t vaddr;
    char type;
    unsigned char val;
    if (sscanf(line, "%c %zx %hhu", &type, &vaddr, &val) != 3) {
      fprintf(stderr, "Invalid trace line %zu: %s\n", linenum, line);
      exit(1);
    }
    if (type != 'I' && type != 'L' && type != 'S' && type != 'M') {
      fprintf(stderr, "Invalid reftype, line %zu: %s\n", linenum, line);
      exit(1);
    }
    if ((vaddr % PAGE_SIZE) > SIMPAGESIZE) {
      fprintf(stderr,
              "Invalid vaddr, offset must be in range of simulated page frame "
              "size, line %zu: %s\n",
              linenum,
              line);
      exit(1);
    }
    if (debug) {
      printf("%c %lx %hhu\n", type, vaddr, val);
    }

    access_mem(type, vaddr, val, linenum);
  }
}

int
main(int argc, char* argv[])
{
  size_t swapsize = 0;
  char* replacement_alg = NULL;
  double starttime;
  double endtime;
  struct mallinfo start_mallinfo;
  unsigned long bytes_used;
  const char* usage =
    "USAGE: sim -f tracefile -m memorysize -s swapsize -a algorithm\n";

  int opt;
  while ((opt = getopt(argc, argv, "f:m:a:s:")) != -1) {
    switch (opt) {
      case 'f':
        tracefile = optarg;
        break;
      case 'm':
        memsize = strtoul(optarg, NULL, 10);
        break;
      case 'a':
        replacement_alg = optarg;
        break;
      case 's':
        swapsize = strtoul(optarg, NULL, 10);
        break;
      default:
        fprintf(stderr, "%s", usage);
        return 1;
    }
  }
  if (!tracefile || !memsize || !swapsize || !replacement_alg) {
    fprintf(stderr, "%s", usage);
    return 1;
  }

  FILE* tfp = fopen(tracefile, "r");
  if (!tfp) {
    perror(tracefile);
    return 1;
  }

  // Initialize main data structures for simulation.
  // This happens before calling the replacement algorithm init function
  // so that the init_func can refer to the coremap if needed.
  // coremap = calloc(memsize, sizeof(struct frame));
  // physmem = calloc(memsize, SIMPAGESIZE);
  coremap = malloc(memsize * sizeof(struct frame));
  physmem = malloc(memsize * SIMPAGESIZE);
  swap_init(swapsize);

  for (size_t i = 0; i < num_algs; ++i) {
    if (strcmp(algs[i].name, replacement_alg) == 0) {
      init_func = algs[i].init;
      cleanup_func = algs[i].cleanup;
      ref_func = algs[i].ref;
      evict_func = algs[i].evict;
      break;
    }
  }
  if (!evict_func) {
    fprintf(
      stderr, "Error: invalid replacement algorithm - %s\n", replacement_alg);
    return 1;
  }

  start_mallinfo = mallinfo();
  starttime = get_time();
  // Call pagetable and replacement algorithm's init_func before
  // replaying trace.
  init_pagetable();
  init_func();
  // replay_trace(tfp);
  (void)replay_trace;
  endtime = get_time();
  bytes_used = get_bytes_used(&start_mallinfo);

  if (debug) {
    print_pagetable();
  }
  cleanup_func();

  // Cleanup data structures and remove temporary swapfile
  free(coremap);
  free(physmem);
  swap_destroy();
  free_pagetable();

  printf("\n");
  printf("Hit count: %zu\n", hit_count);
  printf("Miss count: %zu\n", miss_count);
  printf("Clean evictions: %zu\n", evict_clean_count);
  printf("Dirty evictions: %zu\n", evict_dirty_count);
  printf("Total references: %zu\n", ref_count);
  printf("Hit rate: %.4f\n", ((double)hit_count / ref_count) * 100.0);
  printf("Miss rate: %.4f\n", ((double)miss_count / ref_count) * 100.0);

  printf("Time to run simulation: %f\n", endtime - starttime);
  printf("Memory used by simulation: %lu bytes\n", bytes_used);

  return 0;
}
