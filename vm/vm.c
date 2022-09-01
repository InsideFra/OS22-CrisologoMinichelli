/* Code for handling the Virtual Memory */

#include <vm.h>
#include <types.h>

/**
 * Called during the startup of the virtual memory system.
 * @function
 */
void vm_bootstrap(void) {

}

/**
 * Usually called by mips_trap().
 * @param {int} faulttype - Fault code error.
 * @param {int} faultaddress - Virtual Address that causes the error.
 * @return {int} 0 if fault is addressed.
 */
int vm_fault(int faulttype, vaddr_t faultaddress) {
    (void)faulttype;
    (void)faultaddress;
    return 1;
}

/**
 * TLB shootdown handling called from interprocessor_interrupt
 */
void vm_tlbshootdown(const struct tlbshootdown *tlb) {
    (void)tlb;
}v