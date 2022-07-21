// Low-level implementation of a page-table
// The first implementation is made with a 1-level page table.

#include <types.h>
#include <vm.h>

// You can find information in the vm.h include file

// Page-table lenght register
//The size in kB of a page table entry
#define PTLR_ 512

// Page-table base register
// The start address in memory where to store page table (stack like)
#define PTBR_ 1

// The size of the allocaed space to store page table
#define PAGETABLE_SPACE 8192

#define PAGETABLE_ENTRY PAGETABLE_SPACE/PTLR_

// We need to divide the physiical memory into fixed-sized blocks called FRAMES
// Divide logical memory into blocks of same size called pages
// Keep track of all free frames
// To run a program of size N pages, need to find N free frames and load program
// SET UP A PAGE TABLE to translate logical to physical addresses


static struct PG_ {
    _Bool Valid;
    int frame_number;
} main_PG[PAGETABLE_ENTRY] = {{0}};

/* Allocate/free some kernel-space virtual pages */
vaddr_t
alloc_kpages(unsigned npages)
{

    int first_valid_page_number = 0;
    _Bool valid_pages_founded = 0;
    unsigned buffer_npages = npages;

    for(int i = 0; i < PAGETABLE_ENTRY; i++) {
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
            // DEBUG
            main_PG[first_valid_page_number+i].frame_number = PADDR_TO_VADDR(first_valid_page_number);
        }

        return PADDR_TO_VADDR(first_valid_page_number);
    }
    return NULL;
}

void
free_kpages(vaddr_t addr)
{
	/* nothing - leak the memory. */

	(void)addr;
}

