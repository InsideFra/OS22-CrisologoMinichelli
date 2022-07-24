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
#include <machine/tlb.h>
#include <spl.h>
#include <page.h>

extern struct PG_ *main_PG;

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof (struct addrspace));
	if (as == NULL) {
		return NULL;
	}
	as->as_pbase_code = 0;  // will be set later
	as->as_vbase_code = 0;  // will be set later
	as->as_npages_code = 4; // fixed for now

	as->as_pbase_data = 0;  // will be set later
	as->as_vbase_data = 0;  // will be set later
	as->as_npages_data = 1; // fixed for now

	as->as_pbase_stack = 0; // will be set later
	as->as_vbase_stack = 0;  // will be set later
	as->as_npages_stack = 1; // fixed for now
	
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

	/* Disable interrupts on this CPU while frobbing the TLB. */
	// TLB Not supported yet
	// int i, spl;
	// spl = splhigh();

	// for (i=0; i<NUM_TLB; i++) {
	//  	tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	// }

	// splx(spl);
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
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsize,
		 int readable, int writeable, int executable)
{
	/*
	 * Write this.
	 */

	(void)as;
	(void)vaddr;
	(void)memsize;
	(void)readable;
	(void)writeable;
	(void)executable;
	if (executable) {
		// We are now loading a CODE segment
		as->as_vbase_code = vaddr;

	} else {
		// We are now loading a DATA segment
		as->as_vbase_data = vaddr;
	}
	// TODO #9

	DEBUG(DB_VM, "\nPAGING:\n");
	return 0;
	//return ENOSYS;
}

int
as_prepare_load(struct addrspace *as)
{
	as->as_pbase_code = alloc_kpages(as->as_npages_code);
	if (as->as_pbase_code == 0) {
		return ENOMEM;
	}
	DEBUG(DB_VM, "\nCODE: vAddr = 0x%x\nCODE: pAddr = 0x%x",
		as->as_vbase_code, as->as_pbase_code);

	as->as_pbase_data = alloc_kpages(as->as_npages_data);
	if (as->as_pbase_data == 0) {
		return ENOMEM;
	}

	DEBUG(DB_VM, "\nDATA: vAddr = 0x%x\nDATA: pAddr = 0x%x",
		as->as_vbase_data, as->as_pbase_data);

	
	as->as_pbase_stack = alloc_kpages(as->as_npages_stack);
	if (as->as_pbase_stack == 0) {
		return ENOMEM;
	}

	as->as_vbase_stack = as->as_vbase_data + PAGE_SIZE*as->as_npages_data;

	DEBUG(DB_VM, "\nSTACK: vAddr = 0x%x\nvSTACK: pAddr = 0x%x\n",
		as->as_vbase_stack, as->as_pbase_stack);

	// EVERYTHING NEXT IS DEBUG AND SHOULD BE DELETED IN PRODUCTION
	// BUT NOW IT STAYS HERE TO RUN SHELL PROGRAM

	int spl = splhigh();
	uint32_t ehi, elo;
	paddr_t pa;
	for (unsigned int i = 0; i < as->as_npages_code; i++) {
		ehi = (as->as_vbase_code + i*PAGE_SIZE);
		pa = ((as->as_pbase_code - MIPS_KSEG0)) + i*PAGE_SIZE;
		elo = pa | TLBLO_VALID | TLBLO_DIRTY;
		tlb_write(ehi, elo, i) ;
	}

	ehi = as->as_vbase_data - (as->as_vbase_data%PAGE_SIZE); // PAGE ALIGN
	pa = ((as->as_pbase_data - MIPS_KSEG0));
	elo = pa | TLBLO_VALID | TLBLO_DIRTY;
	tlb_write(ehi, elo, 4);
	splx(spl);

	ehi = as->as_vbase_stack - (as->as_vbase_stack%PAGE_SIZE); // PAGE ALIGN
	pa = ((as->as_pbase_stack - MIPS_KSEG0));
	elo = pa | TLBLO_VALID | TLBLO_DIRTY;
	tlb_write(ehi, elo, 5);
	splx(spl);

	//as_zero_region(as->as_pbase_data, as->as_npages_data);
	//as_zero_region(as->as_pbase_code, as->as_npages_code);
	(void)as;
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
	// Avoid warnings
	(void)paddr;
	(void)npages;

	// TLB
	//bzero(&paddr, npages * PAGE_SIZE);
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	struct addrspace *as;
	
	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
		/* We always create pages read-write, so we can't get this */
		panic("dumbvm: got VM_FAULT_READONLY\n");
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
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

	as = proc_getas();
	if (as == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}
	return EFAULT;
	//return 0;
}

void
vm_bootstrap(void)
{
	vaddr_t location;

    location = ram_getsize() - (PAGETABLE_ENTRY * sizeof(struct PG_));

	DEBUG(DB_VM, "VM: PG vLocation: 0x%x\nVM: PG pLocation: 0x%x\nVM: Entries: %u\nVM: Sizeof(Entry): %u\n", 
			location, PADDR_TO_KVADDR(location), PAGETABLE_ENTRY, sizeof(struct PG_));
	DEBUG(DB_VM, "VM: %uk physical memory available after VM\n", 
			location - ram_getfirstaddr());

	main_PG = (struct PG_*) PADDR_TO_KVADDR(location);
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("dumbvm tried to do tlb shootdown?!\n");
}

