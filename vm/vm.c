/* Code for handling the Virtual Memory */

#include <vm.h>
#include <types.h>
#include <pt.h>
#include <machine/tlb.h>
#include <spl.h>
#include <lib.h>
#include <list.h>
#include <kern/errno.h>
#include <current.h>
#include <addrspace.h>
#include <proc.h>
#include <vm_tlb.h>
#include <current.h>
#include <swapfile.h>

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

	KASSERT(sizeof(struct frame_list_struct) < LARGEST_SUBPAGE_SIZE);

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
		main_PG[PAGETABLE_ENTRY-1-i].page_number = 0;
		main_PG[PAGETABLE_ENTRY-1-i].pid = 0;
		main_PG[PAGETABLE_ENTRY-1-i].Valid = 1;
	}

    for (uint32_t i = (unsigned int)RAM_FirstFree/PAGE_SIZE; i < PAGETABLE_ENTRY - (RAM_Size-location)/PAGE_SIZE; i++) {
		if(addToFrameList(i, addBOTTOM)) {
			panic("Error in addToList()");
		}
	}

	//print_frame_list();
	//print_page_table();

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

	swapfile_init();

    VM_Started = true;
}

#include <kern/fcntl.h>
#include <vfs.h>

/**
 * Usually called by mips_trap().
 * @param {int} faulttype - Fault code error.
 * @param {vaddr_t} faultaddress - Virtual Address that causes the error.
 * @return {int} 0 if fault is addressed.
 */
int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	struct addrspace *as;
	//pid_t pid = curproc->pid;
	
	as = proc_getas();
	if (as == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}

	struct vnode *v;
	vaddr_t entrypoint;
	int result = 0;
	int ret = 0;
	uint32_t a, b, c, d;
	_Bool inMemory = false;
	switch (faulttype) {
	    case VM_FAULT_READONLY:
			// This fault happen when a program tries to write to a only-read segment.
			// If such exception occurs, the kernel must terminate the process.
			// The kernel should not crash!
			// TODO #14:
			panic("dumbvm: got VM_FAULT_READONLY\n");
			break;
	    case VM_FAULT_READ:
			ret = pageSearch(faultaddress);
			if (ret > 0) {
				faultaddress &= PAGE_FRAME;
				panic("Function not developed yet");
			} else {
				if (is_codeSegment(faultaddress, as)) {
					faultaddress &= PAGE_FRAME;

					// the TLB miss happened during a read to a code segment
					// we can load this code segment from the ELF program file
					b = as->as_vbase_data & PAGE_FRAME;
					d = (faultaddress - b) >> 12;
					a = MAX_CODE_SEGMENT_PAGES;
					c = (d%a);

					for (unsigned int i = 0; i < (as->as_npages_code/MAX_CODE_SEGMENT_PAGES); i++) {
						ret = pageSearch(b + c*PAGE_SIZE + i*(a*PAGE_SIZE));
						if (ret != noEntryFound) {
							free_kpages((paddr_t)(ret*PAGE_SIZE + MIPS_KSEG0));
							break;
						} else {
							continue;
						}
					}

					result = alloc_kpages(1);
					
					if (result == 0) {
						return EINVAL;
					}

					addPT(( (result-MIPS_KSEG0)/PAGE_SIZE), faultaddress & PAGE_FRAME, curproc->pid );

					if (addTLB(faultaddress, curproc->pid, 0)) {
						return EINVAL;
					}

					/* Open the file. */
					result = vfs_open(curproc->p_name, O_RDONLY, 0, &v);
					if (result) {
						return result;
					}

					/* Load 1 pages from the faultaddress. */
					result = load_elf(v, &entrypoint, faultaddress, 1);
					if (result) {
						/* p_addrspace will go away when curproc is destroyed */
						vfs_close(v);
						return result;
					}

					return 0;
				} else if (is_dataSegment(faultaddress, as)) {
					faultaddress &= PAGE_FRAME;
					// Trying to read from a data segment which is not in RAM
					
					// we should check if it is in disk memory
					/* inMemory = is_inMemory(fauladdress, pid) */
					(void)(inMemory);

					b = as->as_vbase_data & PAGE_FRAME ;
					d = (faultaddress - b) >> 12;
					a = MAX_DATA_SEGMENT_PAGES;
					c = (d%a);

					for (unsigned int i = 0; i < (as->as_npages_data/MAX_DATA_SEGMENT_PAGES); i++) {
						ret = pageSearch(b + c*PAGE_SIZE + i*(a*PAGE_SIZE));
						if (ret != noEntryFound) {
							// before freeing the page from the memory, we should save it?
							free_kpages((paddr_t)(ret*PAGE_SIZE + MIPS_KSEG0));
							break;
						} else {
							continue;
						}
					}

					result = alloc_kpages(1);
					
					if (result == 0) {
						return EINVAL;
					}

					addPT(( (result-MIPS_KSEG0)/PAGE_SIZE), faultaddress & PAGE_FRAME, curproc->pid );

					if (addTLB(faultaddress, curproc->pid, 0)) {
						return EINVAL;
					}

					// we should load from disk?	
					return 0;
					
					return EINVAL;
				}
				return EINVAL;
			}
			break;
	    case VM_FAULT_WRITE:
			ret = pageSearch(faultaddress);
			if (ret < 0) {
				// vAddress not loaded in the RAM Page Table
				if (is_codeSegment(faultaddress, as)) {
					faultaddress &= PAGE_FRAME;
					panic ("You cannot write 0x%x, is a code segment.\n", faultaddress);
				}
				if (is_dataSegment(faultaddress, as)) {
					faultaddress &= PAGE_FRAME;
					// Trying to write in a data segment which is not in RAM
					
					// we should check if it is in disk memory
					/* inMemory = is_inMemory(fauladdress, pid) */
					(void)(inMemory);

					b = as->as_vbase_data & PAGE_FRAME ;
					d = (faultaddress - b) >> 12;
					a = MAX_DATA_SEGMENT_PAGES;
					c = (d%a);
					
					/*--------------------------------------------------*/	
					//LRU algorithm
					//victim_page = victim_pageSearch();
					//if(page_swapOut(victim_page)){
					//	page_replacement(victim_page);
					//}

					/*--------------------------------------------------*/	

					for (unsigned int i = 0; i < (as->as_npages_data/MAX_DATA_SEGMENT_PAGES); i++) {
						ret = pageSearch(b + c*PAGE_SIZE + i*(a*PAGE_SIZE));
						if (ret != noEntryFound) {
							/* Saving the frame in the disk memory
							 * before freeing the frame
							 */
							swapOut(ret*PAGE_SIZE + MIPS_KSEG0);
							break;
						} else {
							continue;
						}
					}

					result = alloc_kpages(1);
					
					if (result == 0) {
						return EINVAL;
					}

					addPT(( (result-MIPS_KSEG0)/PAGE_SIZE), faultaddress & PAGE_FRAME, curproc->pid );

					if (addTLB(faultaddress, curproc->pid, 0)) {
						return EINVAL;
					}

					// we should load from disk?	
					return 0;
				}
				return 1;
			}
			
			if(addTLB(faultaddress, curproc->pid, 1))
				return 1;

			return 0;
			break;
	    default:
			return EINVAL;
	}

	if (curproc == NULL) {
		/*
		 * No process. This is probably a kernel fault early
		 * in boot. Return EFAULT so as to panic instead of
		 * getting into an infinite faulting loop.
		 */
		return EFAULT;
	}
	return EFAULT;
}

/**
 * TLB shootdown handling called from interprocessor_interrupt
 */
void vm_tlbshootdown(const struct tlbshootdown *tlb) {
    (void)tlb;
	panic("Someone tried to shutdown the TLB");
};

/**
 * Used to allocate physical frame to user space.
 * @param {uint} npages - Number of frame to allocated.
 * @param {vaddr} The virtual address that has to be mapped with the physical frame
 * @return 0 if everything ok.
 */
paddr_t alloc_pages(uint8_t npages, vaddr_t vaddr) {
	paddr_t paddr = 0;
	uint32_t frame_index = 0;
	uint32_t pid = curproc->pid;
	KASSERT(VM_Started);
	for (unsigned int i = 0; i < npages; i++) {
		paddr = alloc_kpages(1);
		if (paddr == 0) {
			panic("Error in alloc_pages() during kmalloc()");
		}

		frame_index = (paddr - MIPS_KSEG0)/PAGE_SIZE;

		addPT(frame_index, vaddr, pid);
	}
	return 0;
}

/*
* This method is used to print to the console the current content of the frame list.
* @author @InsideFra
* @date 10/09/2022
*/
void print_frame_list(void) {
	struct frame_list_struct* currentFrame = NULL;
	currentFrame = frame_list;
	unsigned int counter = 0;
	
	while(1) {
		if (frame_list == NULL) {
			panic("Errore");
		}

		kprintf("\nframe_list[%3d]: 0x%x\tframe_list[%3d]->frame_number: %d\tframe_list[%3d]->next_frame: 0x%x", 
			counter, (uint32_t)currentFrame, counter, currentFrame->frame_number, counter, (uint32_t)currentFrame->next_frame);

		counter++;
		
		if (currentFrame->next_frame == NULL) {
			break;
		} else {
			currentFrame = currentFrame->next_frame;
		}
	}
}

/*
* This method is used to print to the console the current content of the inverted page table.
* @author @InsideFra
* @date 10/09/2022
*/
void print_page_table(void) {
	kprintf("main_PG: 0x%x\tPageTable_Entries: %d\n", (uint32_t)main_PG, PAGETABLE_ENTRY);
	for (unsigned int number = 0; number < PAGETABLE_ENTRY; number++) {
		kprintf("main_PG[%d]\tPN: %x\tpAddr: 0x%x\t-%s-",
				number, 
				main_PG[number].page_number, 
				PADDR_TO_KVADDR(4096*(number)), 
				main_PG[number].Valid == 1 ? "V" : "N");
		if (main_PG[number].Valid == 1)
			kprintf("%s-\t", main_PG[number].pid == 0 ? "KERNEL" : "USER");
		else 
			kprintf("\t");
			
		kprintf("\n");
	}
}