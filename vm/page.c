// Low-level implementation of a page-table
// The first implementation is made with a 1-level page table.

#include <types.h>
#include <vm.h>
#include <spinlock.h>
#include <page.h>
#include <machine/tlb.h>
#include <spl.h>
#include <lib.h>

// You can find information in the vm.h include file

static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;
// We need to divide the physiical memory into fixed-sized blocks called FRAMES
// Divide logical memory into blocks of same size called pages
// Keep track of all free frames
// To run a program of size N pages, need to find N free frames and load program
// SET UP A PAGE TABLE to translate logical to physical addresses

struct PG_ *main_PG = 0;

/* Allocate/free some kernel-space virtual pages */
vaddr_t
alloc_kpages(unsigned npages, vaddr_t as_vbase)
{
    paddr_t addr;

    if (main_PG == 0) {
        // VM still not initialized
        spinlock_acquire(&stealmem_lock);
	    addr = ram_stealmem(npages);
	    spinlock_release(&stealmem_lock);
	    if (addr==0) {
		    return 0;
	    }
        return PADDR_TO_KVADDR(addr);
    } else {
        // The VM has been initialized
        if (as_vbase == 0) {
            DEBUG(DB_VM, "\nPE_NOV\n");
            return alloc_pages(npages, 0);
        } else {
            return alloc_pages(npages, as_vbase);
        }
        
    }
    return 0;
}

void
free_kpages(vaddr_t addr)
{
	/* nothing - leak the memory. */

	(void)addr;
}

/* Allocate/free some user-space virtual pages */
vaddr_t
alloc_pages(unsigned npages, vaddr_t as_vbase)
{

    paddr_t addr = 0, return_addr = 0;

    //unsigned int first_valid_page_number = 0;
    //_Bool valid_pages_founded = 0;
    //unsigned buffer_npages = npages;

    /*for(unsigned int i = 0; i < PAGETABLE_ENTRY; i++) {
        if (buffer_npages != 0) {
            if (main_PG[i].Valid == 0) {
                first_valid_page_number = (buffer_npages == npages ? i : first_valid_page_number);
                buffer_npages--;
                if (buffer_npages == 0) {
                    valid_pages_founded = 1;
                    break;
                }   
            } else {
                if (buffer_npages != npages) {
                    first_valid_page_number = 0;
                    valid_pages_founded = 0;
                    buffer_npages = npages;
                }
            }
        }
    }

    if (valid_pages_founded == 0) {
        // TODO #6
    } else {
        // TODO #7
        for (unsigned int i = 0; i < npages; i++) {
            main_PG[first_valid_page_number+i].Valid = 1;
        }
        
    }*/


    //int spl = splhigh();
	//uint32_t ehi, elo;
    
    if (as_vbase == 0) {
        spinlock_acquire(&stealmem_lock);
        addr = ram_stealmem(1);
        spinlock_release(&stealmem_lock);

        if (addr==0) { 
            return 0; 
        }
        if (return_addr == 0) { 
            return_addr = addr; 
        }
        return PADDR_TO_KVADDR(return_addr);
    }

    // FIND THE PAGE NUMBER from virtual address
    unsigned int pn = as_vbase / PAGE_SIZE;
    pn -= 1024;

    if (pn > PAGETABLE_ENTRY) {
        panic("PAGING: OUT OF MEMORY ACCESS");
    }

	paddr_t pa;
    pn -= 1;
    for (unsigned int i = 0; i < npages; i++) {
        pn += 1;
        if (main_PG[pn].Valid == 0) {

            spinlock_acquire(&stealmem_lock);
            addr = ram_stealmem(1);
            spinlock_release(&stealmem_lock);

            if (addr==0) { 
                return 0; 
            }
            if (return_addr == 0) { 
                return_addr = addr; 
            }
            
            pa = PADDR_TO_KVADDR(addr) - MIPS_KSEG0;         // equivalent to pa = addr;

            main_PG[pn].Valid = 1;
            main_PG[pn].frame_number = ((pa) >> 12);
        } else {
            panic("PG: VALID = 1");
        }
    }
    
    DROP_PG();

	//splx(spl);
    return PADDR_TO_KVADDR(return_addr);
}

void DROP_PG(void) {
    DEBUG(DB_VM, "\n");
    for(unsigned int i = 0; i < PAGETABLE_ENTRY; i++) {
        DEBUG(DB_VM, "[0x%x]\tFN: %x\tpAddr: 0x%x\t-%s-\n", 
            (i+1024)*PAGE_SIZE, 
            main_PG[i].frame_number, 
            PADDR_TO_KVADDR(main_PG[i].frame_number << 12), 
            main_PG[i].Valid == 1 ? "V" : "N");
    }
}

void
free_pages(vaddr_t addr)
{
	/* nothing - leak the memory. */

	(void)addr;
}

