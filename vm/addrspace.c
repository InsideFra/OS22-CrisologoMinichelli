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
#include <vm.h>
#include <proc.h>
#include <current.h>
#include <spl.h>
#include <page.h>
#include <machine/tlb.h>
#include <kern/fcntl.h>
#include <vfs.h>

extern struct RAM_PG_ *main_PG;
extern struct frame_list_struct (*frame_list);

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof (struct addrspace));
	//struct process_PG *p_PG = kmalloc(Process_PG_ENTRY*(sizeof(struct process_PG)));
	struct PG_Info *p_PG_Info = kmalloc(sizeof (struct PG_Info));  
	
	//DROP_PG(1);
	if (as == NULL) {
		return NULL;
	}
	as->as_pbase_code = 0;  // will be set later
	as->as_vbase_code = 0;  // will be set later
	as->as_npages_code = 0; // will be set later

	as->as_pbase_data = 0;  // will be set later
	as->as_vbase_data = 0;  // will be set later
	as->as_npages_data = 0; // will be set later

	as->as_pbase_stack = 0; // will be set later
	as->as_vbase_stack = 0;  // will be set later
	as->as_npages_stack = 1; // fixed for now

	//as->processPageTable = p_PG;
	as->processPageTable_INFO = p_PG_Info;

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
}

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
		as->as_npages_code = memsize/4096 + 1;	// We store how many pages would require the code segment
		
		// DEBUG
		DEBUG(DB_VM, "\nCODE SEGMENT: start vAddr: 0x%x, size: %u pages: %u\n", 
			as->as_vbase_code, memsize, as->as_npages_code);
		if (as->as_npages_code > MAX_CODE_SEGMENT_PAGES) 
			DEBUG(DB_VM, "CODE SEGMENT: Demand Paging Activate\n");
	    else 
			DEBUG(DB_VM, "CODE SEGMENT: Demand Paging not needed in this case\n");

		as->processPageTable_INFO->code_vaddr = as->as_vbase_code & PAGE_FRAME;	// We store in the pagetable INFO the first virtual address of the code segment
		as->processPageTable_INFO->code_entries = MAX_CODE_SEGMENT_PAGES;	// If we mantain this as a DEFINE, we can not use code_entries but use the MAX_CODE..

		for (unsigned int i = 0; i < MAX_CODE_SEGMENT_PAGES; i++) {
			
			// Virtualizzation
			DEBUG(DB_VM, "CODE SEGMENT: ");
			paddr_t addr = alloc_kpages(1, as->as_vbase_code+i*PAGE_SIZE);

			if (addr == 0) {
				panic("Error during memory allocation. See alloc_kpages called by as_define_region.\n");
			}

			if (as->as_pbase_code == 0) {
				// We store the first physical address, but i think this is useless?
				as->as_pbase_code = addr;	
			}
			/* Process Page */
			/* not fully implemented yet */
			// struct process_PG pPage = {0};
			// pPage.Protection = (1 << 2) | (0 << 1) | (1 << 0); // RWE
			// pPage.CachingDisabled = 1;
			// pPage.frame_number = addr/PAGE_SIZE;
			// pPage.Modified = 0;
			// pPage.pagenumber = as->as_vbase_code/PAGE_SIZE;
			// pPage.Referenced = 0;
			// pPage.Valid = 1;

			// if (update_process_PG(as->processPageTable, &pPage)) {
			// 	panic("Generic VM Error\n");
			// }
			/* End Process Page */

			// Update TLB??
			DEBUG(DB_VM, "CODE SEGMENT: ");
			addTLB(as->as_vbase_code+i*PAGE_SIZE, addr);
		}
		return 0;
	} else {
		// Not executable code
		if (writeable) {
			// Writable segment
		}

		if (readable) {
			// Readable segment
		}

		if (writeable && readable) {	// writable and readable
			// We are now loading a DATA segment 	
			as->as_vbase_data = vaddr;				// We store the first virtual address of the data segment 
			as->as_npages_data = memsize/4096 + 1;	// We store how many pages would require the data segment
			
			// DEBUG
			DEBUG(DB_VM, "\nDATA SEGMENT: start vAddr: 0x%x, size: %u pages: %u\n", 
				as->as_vbase_data, memsize, as->as_npages_data);
			if (as->as_npages_data > MAX_DATA_SEGMENT_PAGES) 
				DEBUG(DB_VM, "DATA SEGMENT: Demand Paging Activate\n");
			else 
				DEBUG(DB_VM, "DATA SEGMENT: Demand Paging not needed in this case\n");

			as->processPageTable_INFO->data_vaddr = as->as_vbase_data & PAGE_FRAME;	// We store in the pagetable INFO the first virtual address of the code segment
			as->processPageTable_INFO->data_entries = MAX_DATA_SEGMENT_PAGES;	// If we mantain this as a DEFINE, we can not use code_entries but use the MAX_CODE..

			for (unsigned int i = 0; i < MAX_DATA_SEGMENT_PAGES; i++) {
				
				// Virtualizzation
				DEBUG(DB_VM, "DATA SEGMENT: ");
				paddr_t addr = alloc_kpages(1, as->as_vbase_data+i*PAGE_SIZE);

				if (addr == 0) {
					panic("Error during memory allocation. See alloc_kpages called by as_define_region.\n");
				}

				if (as->as_pbase_data == 0) {
					// We store the first physical address, but i think this is useless?
					as->as_pbase_data = addr;
				}

				/* Process Page */
				/* not fully implemented yet */
				// struct process_PG pPage = {0};
				// pPage.Protection = (1 << 2) | (0 << 1) | (1 << 0); // RWE
				// pPage.CachingDisabled = 1;
				// pPage.frame_number = addr/PAGE_SIZE;
				// pPage.Modified = 0;
				// pPage.pagenumber = as->as_vbase_data/PAGE_SIZE;
				// pPage.Referenced = 0;
				// pPage.Valid = 1;

				// if (update_process_PG(as->processPageTable, &pPage)) {
				// 	panic("Generic VM Error\n");
				// }
				/* End Process Page */

				// Update TLB??
				DEBUG(DB_VM, "DATA SEGMENT: ");
					addTLB(as->as_vbase_data+i*PAGE_SIZE, addr);
			}
			return 0;
		}
		DEBUG(DB_VM, "PAGING: General Error");
		return ENOSYS;
	}
	DEBUG(DB_VM, "PAGING: General Error");
	return ENOSYS;
}

int
as_prepare_load(struct addrspace *as)
{
	vaddr_t vbase_topass = 0;
	DEBUG(DB_VM, "\nCODE: vAddr = 0x%x\tCODE: pAddr = 0x%x\n",
		as->as_vbase_code, as->as_pbase_code);

	DEBUG(DB_VM, "\nDATA: vAddr = 0x%x\tDATA: pAddr = 0x%x\n",
		as->as_vbase_data, as->as_pbase_data);

	as->as_vbase_stack = as->as_vbase_data + PAGE_SIZE*as->as_npages_data;
	vbase_topass = as->as_vbase_stack - (as->as_vbase_stack%PAGE_SIZE); // PAGE ALIGN
	as->as_pbase_stack = alloc_kpages(as->as_npages_stack, vbase_topass);
	if (as->as_pbase_stack == 0) {
		return ENOMEM;
	}

	DEBUG(DB_VM, "\nSTACK: vAddr = 0x%x\tSTACK: pAddr = 0x%x\n",
		as->as_vbase_stack, as->as_pbase_stack);

	//as_zero_region
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
	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;
	(void)*as;
	return 0;
}

void
as_zero_region(paddr_t paddr, unsigned int npages) {
	bzero((uint32_t*)paddr, npages * PAGE_SIZE);
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	struct addrspace *as;
	
	as = proc_getas();
	if (as == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}

	faultaddress &= PAGE_FRAME;

	struct vnode *v;
	vaddr_t entrypoint;
	int result = 0;
	int ret = 0;
	uint32_t a, b, c, d;
	_Bool inMemory = false;
	switch (faulttype) {
	    case VM_FAULT_READONLY:
		/* We always create pages read-write, so we can't get this */
			panic("dumbvm: got VM_FAULT_READONLY\n");
			break;
	    case VM_FAULT_READ:
			ret = pageSearch(faultaddress);
			if (ret > 0) {
				panic("Function not developed yet");
			} else {
				if (is_codeSegment(faultaddress, as)) {
					// the TLB miss happened during a read to a code segment
					// we can load this code segment from the ELF program file
					b = as->processPageTable_INFO->code_vaddr;
					d = (faultaddress - b) >> 12;
					a = as->processPageTable_INFO->code_entries;
					c = (d%a);

					for (unsigned int i = 0; i < (as->as_npages_code/as->processPageTable_INFO->code_entries); i++) {
						ret = pageSearch(b + c*PAGE_SIZE + i*(a*PAGE_SIZE));
						if (ret != noEntryFound) {
							if(free_kpages((vaddr_t)(main_PG[ret].page_number)*PAGE_SIZE)) {
								return EINVAL;
							}
							break;
						} else {
							continue;
						}
					}

					result = alloc_kpages(1, faultaddress);

					if (result == 0) {
						return EINVAL;
					}

					if (addTLB(faultaddress, result)) {
						return EINVAL;
					}

					/* Open the file. */
					result = vfs_open(as->program_name, O_RDONLY, 0, &v);
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
					// Trying to read from a data segment which is not in RAM
					
					// we should check if it is in disk memory
					/* inMemory = is_inMemory(fauladdress, pid) */
					(void)(inMemory);

					b = as->processPageTable_INFO->data_vaddr & PAGE_FRAME ;
					d = (faultaddress - b) >> 12;
					a = as->processPageTable_INFO->data_entries;
					c = (d%a);

					for (unsigned int i = 0; i < (as->as_npages_data/as->processPageTable_INFO->data_entries); i++) {
						ret = pageSearch(b + c*PAGE_SIZE + i*(a*PAGE_SIZE));
						if (ret != noEntryFound) {
							// before freeing the page from the memory, we should save it?
							if(free_kpages((vaddr_t)(main_PG[ret].page_number)*PAGE_SIZE)) {
								return EINVAL;
							}
							break;
						} else {
							continue;
						}
					}

					result = alloc_kpages(1, faultaddress);

					if (result == 0) {
						return EINVAL;
					}

					if (addTLB(faultaddress, result)) {
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
					panic ("You cannot write 0x%x, is a code segment.\n", faultaddress);
				}
				if (is_dataSegment(faultaddress, as)) {
					// Trying to write in a data segment which is not in RAM
					
					// we should check if it is in disk memory
					/* inMemory = is_inMemory(fauladdress, pid) */
					(void)(inMemory);

					// TODO #12
					b = as->processPageTable_INFO->data_vaddr & PAGE_FRAME ;
					d = (faultaddress - b) >> 12;
					a = as->processPageTable_INFO->data_entries;
					c = (d%a);

					for (unsigned int i = 0; i < (as->as_npages_data/as->processPageTable_INFO->data_entries); i++) {
						ret = pageSearch(b + c*PAGE_SIZE + i*(a*PAGE_SIZE));
						if (ret != noEntryFound) {
							// before freeing the page from the memory, we should save it?
							if(free_kpages((vaddr_t)(main_PG[ret].page_number)*PAGE_SIZE)) {
								return EINVAL;
							}
							break;
						} else {
							continue;
						}
					}

					result = alloc_kpages(1, faultaddress);

					if (result == 0) {
						return EINVAL;
					}

					if (addTLB(faultaddress, result)) {
						return EINVAL;
					}

					// we should load from disk?	
					return 0;
				}
				return 1;
			}
			
			if(addTLB(faultaddress, (ret*PAGE_SIZE + MIPS_KSEG0)))
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

void
vm_bootstrap(void)
{
	vaddr_t location;

	unsigned int free_pages_number = 0;

	PAGETABLE_ENTRY = ram_getsize()/PAGE_SIZE;

    location = ram_getsize() - (PAGETABLE_ENTRY * PTLR);

	// Location page align
	if (location%PAGE_SIZE != 0) {
		location -= location%PAGE_SIZE;
	}

	DEBUG(DB_VM, "VM: PG vLocation: 0x%x\nVM: PG pLocation: 0x%x\nVM: Entries: %u\nVM: Sizeof(Entry): %u\n", 
			location, PADDR_TO_KVADDR(location), PAGETABLE_ENTRY, sizeof(struct RAM_PG_));
	
	DEBUG(DB_VM, "VM: %uk physical memory available after VM boot\n", 
			location - ram_getfirstaddr());
	
	free_pages_number = (location - ram_getfirstaddr())/PAGE_SIZE;
	
	DEBUG(DB_VM, "VM: %u free pages\n", 
		free_pages_number);

	DEBUG(DB_VM, "VM: %u pages used by the kernel\n", 
		(ram_getfirstaddr())/PAGE_SIZE);

	DEBUG(DB_VM, "VM: %u pages used by the VM\n", 
		(ram_getsize() - location)/PAGE_SIZE);
	
	DEBUG(DB_VM, "\n");

	main_PG = (struct RAM_PG_*)PADDR_TO_KVADDR(location);

	// Maybe virtualize kernel space is useless?
	for(unsigned int i = 0; i < ram_getfirstaddr()/PAGE_SIZE; i++) {
		main_PG[i].page_number = 0;
		main_PG[i].pid = 0;
		main_PG[i].Valid = 1;
	}

	// VM Memory
	for(unsigned int i = 0; i < (ram_getsize()-location)/PAGE_SIZE; i++) {
		main_PG[127-i].page_number = 0;
		main_PG[127-i].pid = 1;
		main_PG[127-i].Valid = 1;
	}

	// User Memory
	for(unsigned int i = ram_getfirstaddr()/PAGE_SIZE; i < free_pages_number; i++) {
		main_PG[i].page_number = 0;
		main_PG[i].pid = 0;
		main_PG[i].Valid = 0;
		struct frame_list_struct* currentFrame = NULL;
		for (unsigned int p = 0; p < free_pages_number; p++) {
			if (frame_list == NULL) {
				// Add first element to the frame list
				frame_list = (struct frame_list_struct*)kmalloc(sizeof(struct frame_list_struct));
				if (frame_list == NULL) {
					// Error while allocating frame list
					panic("Error (1), see debugger");
				}
				frame_list->frame_number = i;
				break;
 			} else {
				if (currentFrame == NULL) {
					currentFrame = frame_list;
				}
				if (currentFrame->next_item != NULL) {
					currentFrame = currentFrame->next_item;
					continue;
				}
				
				currentFrame->next_item = (struct frame_list_struct*)kmalloc(sizeof(struct frame_list_struct));
				if (currentFrame->next_item == NULL) {
					// Error while allocating frame list
					panic("Error (2), see debugger");
				}

				currentFrame = currentFrame->next_item;
				currentFrame->frame_number = i;
				break;
			}
		}
	}

	// TLB

	uint32_t ehi, elo;
    int32_t spl = splhigh();

    for (unsigned int i = 0; i < NUM_TLB; i++) {
		ehi = TLBHI_INVALID(i);
    	elo = TLBLO_INVALID();
		tlb_write(ehi, elo, i);
	}

	splx(spl);

	//DROP_PG(1);
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("dumbvm tried to do tlb shootdown?!\n");
}

