/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm_tlb.h>
#include <proc.h>
#include <pt.h>
#include <vm.h>
#include <current.h>
#include <vm_tlb.h>

extern struct frame_list_struct *frame_list;

extern struct invertedPT (*main_PG);

extern _Bool VM_Started;

extern unsigned int PAGETABLE_ENTRY;
/**
* @brief This method is called during the creation of a process's address space.
* @author @InsideFra
* @return a struct address space pointer
*	At this point, we don't know much about the process, so we just allocate a small
*	part of the memory to store the variable that will contain the information abount the address space
*/
struct addrspace *
as_create(void)
{
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		panic("Error while kmalloc in as_create()");
		return NULL;
	}

	as->as_vbase_code = 0;
	as->as_npages_code = 0;

	as->as_vbase_data = 0;
	as->as_npages_data = 0;
	as->as_vbase_bss = 0;

	as->as_vbase_stack = 0;

	return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}

	/*
	 * Write this.
	 */

	(void)old;

	*ret = newas;
	return 0;
}

void
as_destroy(struct addrspace *as)
{
	/*
	 * Clean up as needed.
	 */

	kfree(as);
	for (unsigned int i = 0; i < PAGETABLE_ENTRY; i++) {
		if (main_PG[i].pid == curproc->pid && main_PG[i].Valid == 1) {
			free_kpages(i*4096 + MIPS_KSEG0);
		}
	}

	invalidate_TLB();
}

static _Bool TLB_PID = 0;

void
as_activate(void)
{
	struct addrspace *as;

	as = proc_getas();
	if (as == NULL) {
		/*
		 * Kernel thread without an address space; leave the
		 * prior address space in place.
		 */
		return;
	} else {
		if (curproc->pid != TLB_PID) {
			invalidate_TLB();
			TLB_PID = curproc->pid;
		}
	}
}

void
as_deactivate(void)
{
	/*
	 * Write this. For many designs it won't need to actually do
	 * anything. See proc.c for an explanation of why it (might)
	 * be needed.
	 */
}

extern uint32_t freeTLBEntries;

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(	struct addrspace *as, vaddr_t vaddr, size_t memsize, 
					int readable, int writeable, int executable) 
{
	if (executable) {
		// We are now loading a CODE segment
		as->as_vbase_code = vaddr; 				// We store the first virtual address of the code segment 
		as->as_npages_code = memsize/PAGE_SIZE + 1;	// We store how many pages would require the code segment
		
		// START DEBUG SECTION
		DEBUG(DB_VM, "\nCODE SEGMENT: start vAddr: 0x%x, size: %u pages: %u\n",  as->as_vbase_code, memsize, as->as_npages_code);
		if (as->as_npages_code > MAX_CODE_SEGMENT_PAGES) {
			DEBUG(DB_VM, "CODE SEGMENT: Demand Paging Activate\n");
			as->as_npages_code_loaded = MAX_CODE_SEGMENT_PAGES;
		}
	    else {
			DEBUG(DB_VM, "CODE SEGMENT: Demand Paging not needed in this case\n");
			as->as_npages_code_loaded = as->as_npages_code;
		}
		DEBUG(DB_VM, "DATA SEGMENT: Loading %d pages\n", as->as_npages_code_loaded);
		// END DEBUG SECTION
		
		for (unsigned int i = 0; i < as->as_npages_code_loaded; i++) {
			// Virtualizzation
			// For each code page, allocate a physical frame and associate it to a page.
			//DEBUG(DB_VM, "CODE SEGMENT: ");

			// We are defining a region for the code segment, which will be uploaded for the first time
			// In this case, we are loading a code segment from the ELF file
			// This code segment is not in the page table neither in the swapFile
			// When we will load the segment, we will write to the code frames, so we set for now the Dirty bit to 1
			// Then after the upload, we will set it back to zero
			if (alloc_pages(1, as->as_vbase_code+i*PAGE_SIZE, 1)) {
				panic("Error during memory allocation. See alloc_kpages called by as_define_region.\n");
			}
			
			// Update TLB??
			//DEBUG(DB_VM, "CODE SEGMENT: ");
			if (freeTLBEntries)
				addTLB(as->as_vbase_code+i*PAGE_SIZE, curproc->pid);
		}
		return 0;
	} else {
		// Not executable code
		if (!writeable) {
			// Not Writable segment
			panic ("Function not developed yet");
		}

		if (!readable) {
			// Not Readable segment
			panic ("Function not developed yet");
		}

		if (writeable && readable) {	// writable and readable
			// We are now loading a DATA segment 	
			as->as_vbase_data = vaddr;				// We store the first virtual address of the data segment 
			as->as_npages_data = memsize/PAGE_SIZE + 1;	// We store how many pages would require the data segment
			
			// START DEBUG SECTION
			DEBUG(DB_VM, "\nDATA SEGMENT: start vAddr: 0x%x, size: %u pages: %u\n", as->as_vbase_data, memsize, as->as_npages_data);
			if (as->as_npages_data > MAX_DATA_SEGMENT_PAGES) { 
				DEBUG(DB_VM, "DATA SEGMENT: Demand Paging Activate\n");
				as->as_npages_data_loaded = MAX_PAGES_PER_PROCESS - (as->as_npages_code_loaded - MAX_STACK_SEGMENT_PAGES); 
			}
			else {
				DEBUG(DB_VM, "DATA SEGMENT: Demand Paging not needed in this case\n");
				as->as_npages_data_loaded = as->as_npages_data;
			}
			DEBUG(DB_VM, "DATA SEGMENT: Loading %d pages\n", as->as_npages_data_loaded);
			// END DEBUG SECTION
			
			for (unsigned int i = 0; i < as->as_npages_data_loaded; i++) {
				
				// Virtualizzation
				//DEBUG(DB_VM, "DATA SEGMENT: ");
				
				// We are defining a region for the data segment
				// We set the dirty bit to 1
				if (alloc_pages(1, as->as_vbase_data+i*PAGE_SIZE, 1)) {
					panic("Error during memory allocation. See alloc_kpages called by as_define_region.\n");
				}

				// // Update TLB??
				//DEBUG(DB_VM, "DATA SEGMENT: ");
				if (freeTLBEntries)
					addTLB(as->as_vbase_data+i*PAGE_SIZE, curproc->pid);
			}
			return 0;
		}
		DEBUG(DB_VM, "PAGING: General Error");
		return ENOSYS;
	}
	DEBUG(DB_VM, "PAGING: General Error");
	return ENOSYS;
}

/**
* @brief This method is called during the loading of the program.
* @author @InsideFra
* @return a struct address space pointer
*	This function is called after as_define_region();
* 	In this case we just define the virtual address of the stack segment
*/
int
as_prepare_load(struct addrspace *as)
{
	paddr_t addr = 0;
	DEBUG(DB_VM, "\nCODE: vAddr = 0x%x\t Pages Allocated: %d\n", as->as_vbase_code, 
		//((as->as_npages_code > MAX_CODE_SEGMENT_PAGES) ? MAX_CODE_SEGMENT_PAGES : as->as_npages_code));
		as->as_npages_code_loaded);

	DEBUG(DB_VM, "\nDATA: vAddr = 0x%x\t Pages Allocated: %d\n", as->as_vbase_data,
		as->as_npages_data_loaded);

	as->as_vbase_stack = as->as_vbase_data + PAGE_SIZE*as->as_npages_data;
	
	// We are defining a region for the stack segment
	// We set the dirty bit to 1
	// By now we just allocate one page for the stack. Later if needed, the kernel will use more pages
	addr = alloc_pages(1, as->as_vbase_stack, 1); 
	
	if (addr) {
		panic ("Error while as_prepare_load()");
		return ENOMEM;
	}
	
	DEBUG(DB_VM, "\nSTACK: vAddr = 0x%x\t\n",
		as->as_vbase_stack);

	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
 	*stackptr = (uint32_t)as->as_vbase_stack;
	KASSERT(as->as_vbase_stack != 0);
	DEBUG(DB_VM, "\nDefining stack pointer at address: 0x%x\n", (uint32_t)*stackptr);
	return 0;
}

