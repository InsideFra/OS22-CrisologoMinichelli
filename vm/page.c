// Low-level implementation of a page-table
// The first implementation is made with a 1-level page table.

#include <types.h>
#include <vm.h>
#include <spinlock.h>
#include <page.h>

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
alloc_kpages(unsigned npages)
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
        return alloc_pages(npages);
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
alloc_pages(unsigned npages)
{

    paddr_t addr = 0;

    unsigned int first_valid_page_number = 0;
    _Bool valid_pages_founded = 0;
    unsigned buffer_npages = npages;

    for(unsigned int i = 0; i < PAGETABLE_ENTRY; i++) {
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
        spinlock_acquire(&stealmem_lock);
        addr = ram_stealmem(npages);
        spinlock_release(&stealmem_lock);
        if (addr==0) {
            return 0;
        }
        return PADDR_TO_KVADDR(addr);
    }
    return 0;
}

void
free_pages(vaddr_t addr)
{
	/* nothing - leak the memory. */

	(void)addr;
}

