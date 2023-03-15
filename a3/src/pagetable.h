#ifndef CSC369_PAGETABLE_H
#define CSC369_PAGETABLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

// User-level virtual addresses on a 64-bit Linux system are 48 bits in our
// traces, and the page size is 4096 (12 bits). We split the remaining 36 bits
// evenly into 3 parts, using 12 bits for each:
// - top-level (page directory pointer table - PDPT) index,
// - 2nd-level (page directory - PD) index,
// - 3rd-level (page table - PT) index.
//
// 47           35           23           11          0
// |------------|------------|------------|------------|
//   PDPT index    PD index     PT index    page offset

#define PAGE_VALID 0x1  // Valid bit in pdp or pde or pte, set if in memory
#define PAGE_DIRTY 0x2  // Dirty bit in pte, set if page has been modified
#define PAGE_REF 0x4    // Reference bit in pte, set if page has been referenced
#define PAGE_ONSWAP 0x8 // Set if page has been evicted to swap
#define PAGE_MAX 0xe    // 1111 in binary. Used for bitwise operations.

#define PT_SHIFT 12   // Leaves top 36 bits of vaddr
#define PD_SHIFT 24   // Leaves top 24 bits of vaddr
#define PDPT_SHIFT 36 // Leaves top 12 bits of vaddr

#define PTRS_PER_PT 4096
#define PTRS_PER_PD 4096
#define PTRS_PER_PDPT 4096

#define PT_MASK (PTRS_PER_PT - 1)
#define PD_MASK (PTRS_PER_PD - 1)

#define PT_INDEX(x) (((x) >> PT_SHIFT) & PT_MASK)
#define PD_INDEX(x) (((x) >> PD_SHIFT) & PD_MASK)
#define PDPT_INDEX(x) ((x) >> PDPT_SHIFT)

// These defines allow us to take advantage of the compiler's typechecking

// Page directory pointer table (top-level)
typedef struct
{
  pdpt_entry_t pdps[PTRS_PER_PDPT];
  int16_t valid_cnt;
} pdpt_t;

// Page directory pointer table entry (top-level)
typedef struct
{
  uintptr_t pdp;
  uint8_t flag; // valid bit
} pdpt_entry_t;

// Page directory table (2nd-level)
typedef struct
{
  pd_entry_t pds[PTRS_PER_PD];
  int16_t valid_cnt;
} pd_t;

// Page directory entry (2nd-level)
typedef struct
{
  uintptr_t pde;
  uint8_t flag; // valid bit
} pd_entry_t;

// Page table (3rd-level)
typedef struct
{
  pt_entry_t pts[PTRS_PER_PT];
  int16_t valid_cnt;
} pt_t;

// Page table entry (3rd-level)
typedef struct pt_entry_s
{
  unsigned int frame; // if valid bit == 1, physical frame holding vpage
  off_t swap_offset;  // offset in swap file of vpage, if any
  uint8_t flags;
} pt_entry_t;

pdpt_t pdpt; // main page table (pdpt)

#endif /* CSC369_PAGETABLE_H */
