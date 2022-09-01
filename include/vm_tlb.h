#ifndef _VM_TLB_H_
#define _VM_TLB_H_

int addTLB(vaddr_t vaddr, paddr_t paddr);
int removeTLB(vaddr_t vaddr);
#endif /* _VM_TLB_H_ */