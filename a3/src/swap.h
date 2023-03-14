#ifndef CSC369_SWAP_H
#define CSC369_SWAP_H

#include <sys/types.h>

// Swap functions for use in other files

void
swap_init(size_t size);
void
swap_destroy(void);

int
swap_pagein(unsigned int frame, off_t offset);
off_t
swap_pageout(unsigned int frame, off_t offset);

#endif /* CSC369_SWAP_H */
