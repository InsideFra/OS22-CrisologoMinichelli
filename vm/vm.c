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
#include <kern/time.h>
#include <clock.h>
#include <syscall.h>
#include <segments.h>

// This variables indicates if the vm has been initialized
_Bool VM_Started = false;

extern struct invertedPT (*main_PG);
extern unsigned int PAGETABLE_ENTRY;
extern unsigned int PTLR;

extern struct frame_list_struct (*frame_list);

extern uint32_t freeTLBEntries;

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

	DEBUG(DB_VMINIT, "VM: Memory starts from: 0x%x until 0x%x\n", MIPS_KSEG0, PADDR_TO_KVADDR(RAM_Size));
	DEBUG(DB_VMINIT, "VM: Memory available starts from frame: %d until frame %d\n", RAM_FirstFree/PAGE_SIZE, PAGETABLE_ENTRY - (RAM_Size-location)/PAGE_SIZE - 1);
	DEBUG(DB_VMINIT, "VM: sizeof(invertedPT): %d bytes, entries: %d, total = %d bytes\n", sizeof(struct invertedPT), PAGETABLE_ENTRY, sizeof(struct invertedPT)*PAGETABLE_ENTRY);
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
	freeTLBEntries = NUM_TLB ;
    // end TLB invalid fill

	swapfile_init();

	VM_Started = true;

	for (uint32_t i = (unsigned int)RAM_FirstFree/PAGE_SIZE; i < PAGETABLE_ENTRY - (RAM_Size-location)/PAGE_SIZE; i++) {
		if(addToFrameList(i, addBOTTOM)) {
			panic("Error in addToList()");
		}
	}

	// DEBUG SECTION
	DEBUG(DB_VMINIT, "VM: PG vLocation: 0x%x\tVM: PG pLocation: 0x%x\tVM: Entries: %u\tVM: Sizeof(Entry): %u\n", 
			location, PADDR_TO_KVADDR(location), PAGETABLE_ENTRY, sizeof(struct invertedPT));
	
	DEBUG(DB_VM, "VM: %3uk physical memory available\n",
		(RAM_Size)/1024);

	DEBUG(DB_VM, "VM: %3uk physical memory used \tVM: %u used pages\n",
		(RAM_FirstFree)/1024, (RAM_FirstFree)/PAGE_SIZE);

	DEBUG(DB_VM, "VM: %3uk physical memory free\tVM: %u free pages\n", 
			(location - RAM_FirstFree)/1024, (location - RAM_FirstFree)/PAGE_SIZE);

	DEBUG(DB_VM, "VM: %3u pages used by the VM\n", 
		(RAM_Size-location)/PAGE_SIZE);
	
	DEBUG(DB_VM, "\n");
}

#include <kern/fcntl.h>
#include <vfs.h>
extern unsigned int TLB_Faults; 
extern unsigned int TLB_Reloads;
extern unsigned int PF_Zeroed;
extern unsigned int PF_ELF;
extern unsigned int SF_Writes;
extern unsigned int PF_Disk;
extern unsigned int PF_Swapfile;
extern unsigned int TLB_Faults_wFree;
extern unsigned int TLB_Faults_wReplace;

struct timespec duration_VMFAULTREAD1, duration_VMFAULTREAD2;

/**
 * Usually called by mips_trap().
 * @param {int} faulttype - Fault code error.
 * @param {vaddr_t} faultaddress - Virtual Address that causes the error.
 * @return {int} 0 if fault is addressed.
 */
int
vm_fault(int faulttype, struct trapframe *tf)
{
	// Time stats
	struct timespec before, after, duration;
	gettime(&before);

	struct addrspace *as;
	vaddr_t faultaddress = tf->tf_vaddr;
	
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

	TLB_Faults++; // If the program crashes, we should decrease this variable
	if (freeTLBEntries)
		TLB_Faults_wFree++;
	else
		TLB_Faults_wReplace++;

	int index = 0;
	bool code_seg = 0;
	bool data_seg = 1;
	int victim_page;
	uint32_t paddress;
	int p_num = faultaddress/PAGE_SIZE;

	switch (faulttype) {
	    case VM_FAULT_READONLY:
			// This fault happen when a program tries to write to a only-read segment.
			// If such exception occurs, the kernel must terminate the process.
			// The kernel should not crash!
			kprintf("Closing the program as it is trying to write to a CODE segment address.\n");
			kprintf("The fault address is 0x%x and the page number %d\n", faultaddress, p_num);
			
			TLB_Faults--; // If the program crashes, we should decrease this variable
			if (freeTLBEntries)
				TLB_Faults_wFree--;
			else
				TLB_Faults_wReplace--;
			sys__exit(0);
			break;
	    case VM_FAULT_READ:
			ret = pageSearch(faultaddress);
			if (ret > 0) {
				faultaddress &= PAGE_FRAME;
				if (addTLB(faultaddress, curproc->pid)) {
					return EINVAL;
				}
				TLB_Reloads++;
				// gettime(&after);
				// timespec_sub(&after, &before, &duration);
				// timespec_add(&duration, &duration_VMFAULTREAD, &duration_VMFAULTREAD);
				
				DEBUG(DB_TIME, "VM_FAULT_READ with Page present took %llu.%09lu seconds\n", 
					(unsigned long long) duration.tv_sec,
					(unsigned long) duration.tv_nsec);
				return 0;
			} else { 
				if (is_codeSegment(faultaddress, as)) {
					faultaddress &= PAGE_FRAME;

						/*---------------------------- LOAD FOR THE FIRST TIME -------------------*/
						/*allocate page in PT*/
						if(!(result = alloc_kpages(1))){
						  //page table full, we have to free a page
							victim_page = victim_pageSearch(code_seg);
							if (victim_page == noEntryFound) {
								panic("No victim found.");
							}
							paddress = victim_page*PAGE_SIZE + MIPS_KSEG0;
              				free_kpages(paddress);
							result = alloc_kpages(1);
						}

						// In this case, we are loading a code segment from the ELF file
						// This code segment is not in the page table neither in the swapFile (usually code segment are not in the swap File)
						// When we will load the segment, we will write to the code frames, so we set for now the Dirty bit to 1
						// Then after the upload, we will set it back to zero
						addPT(( (result-MIPS_KSEG0)/PAGE_SIZE), faultaddress & PAGE_FRAME, curproc->pid, 1);
						
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
						vfs_close(v);

						// The code segment has been load and now is write protected
						//main_PG[((result-MIPS_KSEG0)/PAGE_SIZE)].Dirty = 0;
						// No need to do this, this function has been moved to load_elf() function.

            			PF_Disk++;
					  	PF_ELF++;

					// gettime(&after);
					// timespec_sub(&after, &before, &duration);
					// timespec_add(&duration, &duration_VMFAULTREAD, &duration_VMFAULTREAD);

					return 0;
				} else if (is_dataSegment(faultaddress, as)) {
					faultaddress &= PAGE_FRAME;
					// Trying to write in a data segment which is not in RAM
					index = swapfile_checkv1(p_num, curproc->pid);
					if(index == noEntryFound){
						/*---------------------------- LOAD FOR THE FIRST TIME -------------------*/
						/*allocate page in PT*/
						if(!(result = alloc_kpages(1))){
							//page table full, we have to free a page
							victim_page = victim_pageSearch(data_seg);
							paddress = victim_page*PAGE_SIZE + MIPS_KSEG0;	
							
							swapOut((uint32_t*)paddress);
              				SF_Writes++;
							
							result = alloc_kpages(1);
						}
            
						// Dirty bit is set to 1, because this is a data Segment
						addPT(( (result-MIPS_KSEG0)/PAGE_SIZE), faultaddress & PAGE_FRAME, curproc->pid, 1);
						
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
						vfs_close(v);
            			PF_ELF++;
						PF_Disk++;

						// gettime(&after);
						// timespec_sub(&after, &before, &duration);
						// timespec_add(&duration, &duration_VMFAULTREAD1, &duration_VMFAULTREAD1);

						return 0;
					} else {
						/*---------------------------- SWAP IN NEEDED ---------------------------*/
						victim_page = victim_pageSearch(data_seg);
						paddress = victim_page*PAGE_SIZE + MIPS_KSEG0;
						gettime(&before);
						if(swapOut((uint32_t*)paddress)){
							return 1;
						}
						SF_Writes++;
						gettime(&after);
						
						swapIn(index);
						PF_Swapfile++;
						PF_Disk++;

						
						//timespec_sub(&after, &before, &duration);
						//timespec_add(&duration, &duration_VMFAULTREAD2, &duration_VMFAULTREAD2);
						
						return 0;
          			}
				}
				return EINVAL;
			}
			return EINVAL;
			break;
	    case VM_FAULT_WRITE:
			// search TLB missing page in Page Table
			// ret < 0 => Not found in PT => (page loaded for the first time) || (check swapfile)
			// ret >= 0 => Found in PT
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
          
					/*---------------------------- CHECK SWAPFILE --------------------------------*/

					index = swapfile_checkv1(p_num, curproc->pid);
					if(index == noEntryFound){
						/*---------------------------- LOAD FOR THE FIRST TIME -------------------*/
						/*allocate page in PT*/
						if(!(result = alloc_kpages(1))){
							//page table full, we have to free a page
							
							victim_page = victim_pageSearch(data_seg);
							if (victim_page != noEntryFound) {
								paddress = victim_page*PAGE_SIZE + MIPS_KSEG0;	
							
								swapOut((uint32_t*)paddress);
              					SF_Writes++;
								
							} else {
								panic(" ");
							}

							result = alloc_kpages(1);
						}

						// Dirty bit is set to 1, because this is a data Segment
						addPT(( (result-MIPS_KSEG0)/PAGE_SIZE), faultaddress & PAGE_FRAME, curproc->pid, 1);

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
						vfs_close(v);
						PF_ELF++;
            			PF_Disk++;
					} else {
						/*---------------------------- SWAP IN NEEDED ---------------------------*/
						victim_page = victim_pageSearch(data_seg);
						paddress = victim_page*PAGE_SIZE + MIPS_KSEG0;
						if(swapOut((uint32_t*)paddress) == 0){
							SF_Writes++;
							
							swapIn(index);
							PF_Disk++;
							PF_Swapfile++;
						}
					}

					gettime(&after);
					timespec_sub(&after, &before, &duration);
					DEBUG(DB_TIME, "VM_FAULT_WRITE with Page not present took %llu.%09lu seconds\n", 
						(unsigned long long) duration.tv_sec,
						(unsigned long) duration.tv_nsec);
					return 0;
				}
				return 1;
			}
			
			// Complete the operation, the add to the TLB
			// load s4 in tf->vaddr
			//kprintf("Loading 0x%x to 0x%x\n", tf->tf_s4, tf->tf_vaddr);

			if(addTLB(faultaddress, curproc->pid))
				return 1;
			
			TLB_Reloads++;
			
			gettime(&after);
			timespec_sub(&after, &before, &duration);
			DEBUG(DB_TIME, "VM_FAULT_WRITE with Page present took %llu.%09lu seconds\n", 
					(unsigned long long) duration.tv_sec,
					(unsigned long) duration.tv_nsec);
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
 * Also add the information in the page table
 * @param {uint} npages - Number of frame to allocated.
 * @param {vaddr} The virtual address that has to be mapped with the physical frame
 * @return 0 if everything ok.
 */
paddr_t alloc_pages(uint8_t npages, vaddr_t vaddr, bool Dirty) {
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

		addPT(frame_index, vaddr, pid, Dirty);
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