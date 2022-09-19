#ifndef _VM_TLB_H_
#define _VM_TLB_H_

#include <types.h>

int addTLB(vaddr_t vaddr, paddr_t paddr, _Bool Dirty);
int removeTLB(vaddr_t vaddr);
#endif /* _VM_TLB_H_ */