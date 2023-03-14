#include <assert.h>

#include "pagetable_generic.h"

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int
lru_evict(void)
{
  assert(false);
  return -1;
}

/* This function is called on each access to a page to update any information
 * needed by the LRU algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void
lru_ref(int frame)
{
  (void)frame;
}

/* Initialize any data structures needed for this replacement algorithm. */
void
lru_init(void)
{}

/* Cleanup any data structures created in lru_init(). */
void
lru_cleanup(void)
{}
