#ifndef _SEGMENTS_H_
#define _SEGMENTS_H_

#include <vm.h>
#include "opt-dumbvm.h"
#include <types.h>

int is_dataSegment(vaddr_t vaddr, struct addrspace* as);
int is_codeSegment(vaddr_t vaddr, struct addrspace* as);
int is_bssSegment(vaddr_t vaddr, struct addrspace* as);

#endif