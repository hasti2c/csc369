/*
 * This code is provided solely for the personal and private use of students
 * taking the CSC369H course at the University of Toronto. Copying for purposes
 * other than this use is expressly prohibited. All forms of distribution of
 * this code, including but not limited to public repositories on GitHub,
 * GitLab, Bitbucket, or any other online platform, whether as given or with
 * any changes, are expressly prohibited.
 *
 * Authors: Andrew Peterson, Karen Reid, Alexey Khrabrov
 *
 * All of the files in this directory and all subdirectories are:
 * Copyright (c) 2019, 2021 Karen Reid
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pagetable.h"
#include "pagetable_generic.h"
#include "sim.h"
#include "swap.h"

// Counters for various events.
// Your code must increment these when the related events occur.
size_t hit_count = 0;
size_t miss_count = 0;
size_t ref_count = 0;
size_t evict_clean_count = 0;
size_t evict_dirty_count = 0;

bool
get_flag(uint8_t flags, int flag_id)
{
  return !!(flags & flag_id);
}

void
set_flag(uint8_t *flags, int flag_id, int val) {
  if(val)
    *flags = *flags | flag_id;
  else
    *flags = *flags &(PAGE_MAX - flag_id);
}

bool
is_valid(pt_entry_t* pte)
{
  return get_flag(pte->flags, PAGE_VALID);
}

void
set_valid(pt_entry_t* pte, int val)
{
  set_flag(&pte->flags, PAGE_VALID, val);
}

bool
is_dirty(pt_entry_t* pte)
{
  return get_flag(pte->flags, PAGE_DIRTY); 
}

pd_t*
get_pd(vaddr_t vaddr)
{
  unsigned int index = vaddr >> PDPT_SHIFT;
  pdpt_entry_t* pdp = &pdpt.pds[index];
  if (!pdp->pdp) {
    pd_t *pd = malloc(sizeof(pd_t));
    pdp->pdp = (uintptr_t) pd;
    //printf("Malloc: %p\n", pd);
    memset(pd, 0, sizeof(pd_t));
    pdpt.in_use_cnt++;
  }
  return (pd_t*) pdp->pdp;
}

pt_t*
get_pt(vaddr_t vaddr)
{
  unsigned int index = (vaddr >> PD_SHIFT) & PD_MASK;
  pd_t* pd = get_pd(vaddr);
  pd_entry_t* ptp = (pd_entry_t*) &pd->pts[index];
  if (!ptp->pde) {
    pt_t *pt = malloc(sizeof(pt_t));
    ptp->pde = (uintptr_t) pt;
    //printf("Malloc: %p\n", pt);
    memset(pt, 0, sizeof(pt_t));
    pt->in_use_cnt++;
  }
  return (pt_t*) ptp->pde;
}

pt_entry_t*
get_page(vaddr_t vaddr)
{
  unsigned int index = (vaddr >> PT_SHIFT) & PT_MASK;
  pt_t* pt = get_pt(vaddr);
  return &pt->pages[index];
}

/*
 * Allocates a frame to be used for the virtual page represented by p.
 * If all frames are in use, calls the replacement algorithm's evict_func to
 * select a victim frame. Writes victim to swap if needed, and updates
 * page table entry for victim to indicate that virtual page is no longer in
 * (simulated) physical memory.
 *
 * Counters for evictions should be updated appropriately in this function.
 */
static int
allocate_frame(pt_entry_t* pte)
{
  int frame = -1;
  for (size_t i = 0; i < memsize; ++i) {
    if (!coremap[i].in_use) {
      frame = i;
      break;
    }
  }

  if (frame == -1) { // Didn't find a free page.
    // Call replacement algorithm's evict function to select victim
    frame = evict_func();
    assert(frame != -1);
    pt_entry_t* victim = coremap[frame].pte;

    // All frames were in use, so victim frame must hold some page
    // Write victim page to swap, if needed, and update page table

    // IMPLEMENTATION NEEDED
    if (get_flag(victim->flags, PAGE_DIRTY)) {
      evict_dirty_count++;
      if (get_flag(victim->flags, PAGE_ONSWAP))
        victim->swap_offset = swap_pageout(frame, victim->swap_offset); // TODO make sure to initialize to INVALID_SWAP
      else
        victim->swap_offset = swap_pageout(frame, INVALID_SWAP);      
  
      if (victim->swap_offset == INVALID_SWAP)
        exit(-1);
    } else {
      evict_clean_count++;
    }

    set_flag(&victim->flags, PAGE_VALID, 0);
    set_flag(&victim->flags, PAGE_ONSWAP, 1);
    set_flag(&victim->flags, PAGE_DIRTY, 0);
    set_flag(&victim->flags, PAGE_REF, 0);
  }

  // Record information for virtual page that will now be stored in frame
  coremap[frame].in_use = true;
  coremap[frame].pte = pte;

  return frame;
}

/*
 * Initializes your page table.
 * This function is called once at the start of the simulation.
 * For the simulation, there is a single "process" whose reference trace is
 * being simulated, so there is just one overall page table.
 *
 * In a real OS, each process would have its own page table, which would
 * need to be allocated and initialized as part of process creation.
 *
 * The format of the page table, and thus what you need to do to get ready
 * to start translating virtual addresses, is up to you.
 */
void
init_pagetable(void)
{
  // TODO
  memset(&pdpt, 0, sizeof(pdpt_t));
}

/*
 * Initializes the content of a (simulated) physical memory frame when it
 * is first allocated for some virtual address. Just like in a real OS, we
 * fill the frame with zeros to prevent leaking information across pages.
 */
static void
init_frame(int frame)
{
  // Calculate pointer to start of frame in (simulated) physical memory
  unsigned char* mem_ptr = &physmem[frame * SIMPAGESIZE];
  memset(mem_ptr, 0, SIMPAGESIZE); // zero-fill the frame
}

/*
 * Locate the physical frame number for the given vaddr using the page table.
 *
 * If the page table entry is invalid and not on swap, then this is the first
 * reference to the page and a (simulated) physical frame should be allocated
 * and initialized to all zeros (using init_frame).
 *
 * If the page table entry is invalid and on swap, then a (simulated) physical
 * frame should be allocated and filled by reading the page data from swap.
 *
 * When you have a valid page table entry, return the start of the page frame
 * that holds the requested virtual page.
 *
 * Counters for hit, miss and reference events should be incremented in
 * this function.
 */
unsigned char*
find_physpage(vaddr_t vaddr, char type)
{
  int frame = -1; // Frame used to hold vaddr

  // To keep compiler happy - remove when you have a real use
  (void)vaddr;
  (void)type;
  (void)allocate_frame;
  (void)init_frame;

  // IMPLEMENTATION NEEDED

  // Use your page table to find the page table entry (pte) for the
  // requested vaddr.
  pt_entry_t* pte = get_page(vaddr);

  // Check if pte is valid or not, on swap or not, and handle appropriately.
  // You can use the allocate_frame() and init_frame() functions here,
  // as needed.

  if (!get_flag(pte->flags, PAGE_VALID)) {
    miss_count++;
    pte->frame = allocate_frame(pte);
    if (!get_flag(pte->flags, PAGE_ONSWAP)) { // uninitialized
      init_frame(pte->frame);
      set_flag(&pte->flags, PAGE_DIRTY, 1);
    } else { // on swap
      int err = swap_pagein(pte->frame, pte->swap_offset);
      if (err)
        exit(-1);
    }
  } else {
    hit_count++;
  }
  frame = pte->frame;

  // Make sure that pte is marked valid and referenced. Also mark it
  // dirty if the access type indicates that the page will be written to.
  // (Note that a page should be marked DIRTY when it is first accessed,
  // even if the type of first access is a read (Load or Instruction type).
  set_flag(&pte->flags, PAGE_VALID, 1);
  set_flag(&pte->flags, PAGE_REF, 1);
  if (type == 'S' || type == 'M')
    set_flag(&pte->flags, PAGE_DIRTY, 1); 
  ref_count++;

  // Call replacement algorithm's ref_func for this page.
  assert(frame != -1);
  ref_func(frame);

  // Return pointer into (simulated) physical memory at start of frame
  return &physmem[frame * SIMPAGESIZE];
}

void
print_pagetable(void)
{
  for (int i = 0; i < PTRS_PER_PDPT; i++) {
    pd_t* pd = (pd_t*) pdpt.pds[i].pdp;
    if (pd == NULL) {
      // printf("(%x) Not In Use\n", i);
      continue;
    }
    for (int j = 0; j < PTRS_PER_PD; j++) {
      pt_t* pt = (pt_t*) pd->pts[j].pde;
      if (pt == NULL) {
        // printf("(%x-%x Not In Use\n", i, j);
        continue;
      }
      for (int k = 0; k < PTRS_PER_PT; k++) {
        pt_entry_t* pte = &pt->pages[k];
        if (get_flag(pte->flags, PAGE_VALID))
          printf("(%x-%x-%x) Valid [Frame: %d]\n", i, j, k, pte->frame);
        else if (get_flag(pte->flags, PAGE_ONSWAP))
          printf("(%x-%x-%x) On Swap [Offset: %ld]\n", i, j, k, pte->swap_offset);
        // else
        // printf("(%d-%d-%d) Not In Use\n", i, j, k);
      }
      printf("\n");
    }
    printf("\n");
  }
}

void
free_pagetable(void)
{
  for (int i = 0; i < PTRS_PER_PDPT; i++) {
    pd_t* pd = (pd_t*) pdpt.pds[i].pdp;
    if (pd != NULL) {
      for (int j = 0; j < PTRS_PER_PD; j++) {
        pt_t* pt = (pt_t*) pd->pts[j].pde;
        if (pt != NULL) {
          //printf("Free: %d %p\n", j, pt);
          free(pt);
        }
      }
      //printf("Free: %d %p\n", i, pd);
      free(pd);
    }
  }
}
