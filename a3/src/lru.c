#include <assert.h>

#include "pagetable_generic.h"
#include "sim.h"

struct frame *frame_head; // Head = Most Recently Ref'ed

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int
lru_evict(void)
{
  return frame_head - coremap;
}

/* This function is called on each access to a page to update any information
 * needed by the LRU algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void
lru_ref(int frame)
{
  struct frame *frame_node = &coremap[frame];
  if (frame_head == NULL) { // First to be ref'ed.
    frame_list_init_head(frame_node);
  } else if (frame_node != frame_head) {
    if (frame_node->next != NULL && frame_node->prev != NULL)
      frame_list_delete(frame_node);
    frame_list_insert(frame_node, frame_head->prev, frame_head);
  }
  frame_head = frame_node;
}

/* Initialize any data structures needed for this replacement algorithm. */
void
lru_init(void)
{
}

/* Cleanup any data structures created in lru_init(). */
void
lru_cleanup(void)
{}
