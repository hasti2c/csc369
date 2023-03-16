#include <assert.h>

#include "pagetable_generic.h"
#include "sim.h"

size_t clock_hand = 0;

/* Page to evict is chosen using the CLOCK algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int
clock_evict(void)
{
  for (struct frame *frame = &coremap[clock_hand]; frame->pte != NULL && get_referenced(frame->pte); clock_hand = (clock_hand + 1) % memsize) {
    set_referenced(frame->pte, false);
  }
  int ret = (int) clock_hand;
  clock_hand = (clock_hand + 1) % memsize;
  return ret;
}

/* This function is called on each access to a page to update any information
 * needed by the CLOCK algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void
clock_ref(int frame)
{
  set_referenced(coremap[frame].pte, true);
}

/* Initialize any data structures needed for this replacement algorithm. */
void
clock_init(void)
{}

/* Cleanup any data structures created in clock_init(). */
void
clock_cleanup(void)
{}
