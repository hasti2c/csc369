#ifndef CSC369_SIM_H
#define CSC369_SIM_H

#include <stdbool.h>
#include <stddef.h>

/* Simulated physical memory page frame size */
#define SIMPAGESIZE 16

extern size_t memsize;
extern bool debug;

extern size_t hit_count;
extern size_t miss_count;
extern size_t ref_count;
extern size_t evict_clean_count;
extern size_t evict_dirty_count;

/* We simulate physical memory with a large array of bytes */
extern unsigned char* physmem;

extern void (*ref_func)(int frame);
extern int (*evict_func)(void);

extern char* tracefile; // for opt

#endif /* CSC369_SIM_H */
