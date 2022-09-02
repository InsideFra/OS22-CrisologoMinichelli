/* Code for handling the Virtual Memory */

#include <vm.h>
#include <types.h>
#include <pt.h>
#include <machine/tlb.h>
#include <spl.h>
#include <lib.h>

// This variables indicates if the vm has been initialized
_Bool VM_Started = false;

extern struct invertedPT (*main_PG);
extern unsigned int PAGETABLE_ENTRY;
extern unsigned int PTLR;

extern struct frame_list_struct (*frame_list);

/**
 * Called during the startup of the virtual memory system.
 * @function
 */
void vm_bootstrap(void) {
    vaddr_t location;
    uint32_t PT_Size = 0;
    uint32_t RAM_Size = ram_getsize();
    uint32_t RAM_FirstFree = ram_getfirstfree();
    struct frame_list_struct* currentFrame = NULL;

	PAGETABLE_ENTRY = RAM_Size/PAGE_SIZE;
	
	PT_Size = PAGETABLE_ENTRY * PTLR;

    location = RAM_Size - (PT_Size);

	// Location page align
	location &= PAGE_FRAME;

	main_PG = (struct invertedPT*)PADDR_TO_KVADDR(location);
    memset(main_PG, 0, PT_Size);

	// Maybe virtualize kernel svpace is useless?
	for(uint32_t i = 0; i < (unsigned int)RAM_FirstFree/PAGE_SIZE; i++) {
		main_PG[i].page_number = 0;
		main_PG[i].pid = 0;
		main_PG[i].Valid = 1;
	}

	// Memory used by the VM
	for(uint32_t i = 0; i < (RAM_Size-location)/PAGE_SIZE; i++) {
		main_PG[127-i].page_number = 0;
		main_PG[127-i].pid = 1;
		main_PG[127-i].Valid = 1;
	}

    // TODO
    // Fill the free frame list
    // This for goes from the last frame used by the kernel to the first frame used by the VM.
    (void)currentFrame;

	// TLB invalid fill
	uint32_t ehi, elo;
    int32_t spl = splhigh();
    for (uint32_t i = 0; i < NUM_TLB; i++) {
		ehi = TLBHI_INVALID(i);
    	elo = TLBLO_INVALID();
		tlb_write(ehi, elo, i);
	}
	splx(spl);
    // end TLB invalid fill

    VM_Started = true;
}

/**
 * Usually called by mips_trap().
 * @param {int} faulttype - Fault code error.
 * @param {vaddr_t} faultaddress - Virtual Address that causes the error.
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
};

/**
 * Used to allocate physical frame to user space.
 * @param {uint} npages - Number of frame to allocated.
 * @param {vaddr} The virtual address that has to be mapped with the physical frame
 * @return 0 if everything ok.
 */
paddr_t alloc_pages(uint8_t npages, vaddr_t vaddr) {
	KASSERT(VM_Started);
	(void)npages;
	(void)vaddr;
	return 0;
	return 1;
}