#ifndef _VM_TLB_H_
#define _VM_TLB_H_

#include <types.h>

int addTLB(vaddr_t vaddr, pid_t pid, _Bool Dirty); 
int removeTLB(vaddr_t vaddr);
#endif /* _VM_TLB_H_ */