#ifndef CSC369_PAGETABLE_GENERIC_H
#define CSC369_PAGETABLE_GENERIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

// Everything in this file should be independent of the actual page table
// format. All definitions that are specific to a particular page table format
// or implementation should go in pagetable.h

// User-level virtual addresses on a 64-bit Linux system are 48 bits in our
// traces, and the page size is 4096 (12 bits).
#define NUM_VPN_BITS 36
#define PAGE_SHIFT 12
#define PAGE_SIZE 4096
#define PAGE_MASK (~(PAGE_SIZE - 1))

typedef unsigned long vaddr_t;

// Page table entry - actual definition will go in pagetable.h or pagetable.c
struct pt_entry_s;

#define INVALID_SWAP (off_t) - 1

/* The coremap holds information about physical memory.
 * The index into coremap is the physical page frame number stored
 * in the page table entry (pt_entry_t).
 */
struct frame
{
  bool in_use;            // true if frame is allocated, false if frame is free
  struct pt_entry_s* pte; // Pointer back to pagetable entry (pte) for page
                          // stored in this frame
  struct frame* next;
  struct frame* prev;
};

extern struct frame* coremap;

static inline void
frame_list_init_head(struct frame* head)
{
  head->next = head;
  head->prev = head;
}

static inline void
frame_list_insert(struct frame* entry, struct frame* prev, struct frame* next)
{
  next->prev = entry;
  entry->next = next;
  entry->prev = prev;
  prev->next = entry;
}

static inline void
frame_list_delete(struct frame* entry)
{
  entry->next->prev = entry->prev;
  entry->prev->next = entry->next;
  entry->next = NULL;
  entry->prev = NULL;
}

void
init_pagetable(void);
void
print_pagetable(void);
void
free_pagetable(void);
unsigned char*
find_physpage(vaddr_t vaddr, char type);
bool
is_valid(struct pt_entry_s* pte);
bool
is_dirty(struct pt_entry_s* pte);
bool
get_referenced(struct pt_entry_s* pte);
void
set_referenced(struct pt_entry_s* pte, bool val);

// Replacement algorithm functions
// These may not need to do anything for some algorithms
void
rand_init(void);
void
rr_init(void);
void
clock_init(void);
void
lru_init(void);
void
mru_init(void);
void
opt_init(void);

// These may not need to do anything for some algorithms
void
rand_cleanup(void);
void
rr_cleanup(void);
void
clock_cleanup(void);
void
lru_cleanup(void);
void
mru_cleanup(void);
void
opt_cleanup(void);

// These may not need to do anything for some algorithms
void
rand_ref(int frame);
void
rr_ref(int frame);
void
clock_ref(int frame);
void
lru_ref(int frame);
void
mru_ref(int frame);
void
opt_ref(int frame);

int
rand_evict(void);
int
rr_evict(void);
int
clock_evict(void);
int
lru_evict(void);
int
mru_evict(void);
int
opt_evict(void);

#endif /* CSC369_PAGETABLE_GENERIC_H */
