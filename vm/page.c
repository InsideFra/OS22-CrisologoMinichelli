// Low-level implementation of a page-table
// The first implementation is made with a 1-level page table.

#include <types.h>
#include <vm.h>
#include <spinlock.h>
#include <page.h>
#include <machine/tlb.h>
#include <spl.h>
#include <lib.h>
#include <test.h>

// You can find information in the vm.h include file

static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;
// We need to divide the physiical memory into fixed-sized blocks called FRAMES
// Divide logical memory into blocks of same size called pages
// Keep track of all free frames
// To run a program of size N pages, need to find N free frames and load program
// SET UP A PAGE TABLE to translate logical to physical addresses

struct RAM_PG_ (*main_PG) = NULL;

/* Allocate/free some kernel-space virtual pages */
vaddr_t
alloc_kpages(unsigned npages, vaddr_t as_vbase)
{
    paddr_t addr;

    if (main_PG == NULL) {
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

int
pageSearch(vaddr_t addr) {
    unsigned int first_valid = 0;
    _Bool valid_pages = 0;
    for(unsigned int i = 0; i < PAGETABLE_ENTRY; i++) {
        if (main_PG[i].Valid == 1 && main_PG[i].page_number == addr/PAGE_SIZE) {
            first_valid = i;
            valid_pages = 1;
            break;
        } else {
            continue;
        }
    }

    if (valid_pages == 0) {
        return noEntryFound;
    } else {
        // DEBUG
        char toPass = (char)(first_valid); 
        char* ptrPass;
        char** ptrPass2;

        ptrPass2 = &ptrPass;
        ptrPass = &toPass;
            
        view_pagetable(2, ptrPass2);

        return first_valid;        
    }
    return noEntryFound;
}

/**
* This method will be used to add an entry to the hardware TLB.
* @author @InsideFra
* @param vaddr The virtual address (0x00..)
* @param paddr The physicial address (0x80..)
* @date 02/08/2022
* @return 0 if everything is okay else panic
*/
int
addTLB(vaddr_t vaddr, paddr_t paddr) {
    uint32_t ehi, elo;
    paddr_t pa;
    int spl = splhigh();

    ehi = vaddr - ((vaddr)%PAGE_SIZE); // PAGE ALIGN
    pa = paddr - MIPS_KSEG0;
    elo = pa | TLBLO_VALID | TLBLO_DIRTY;

    tlb_random(ehi, elo);
    int tlb_index_probe = tlb_probe(ehi, elo); 
    if (tlb_index_probe < 0) {
        panic("Generic TLB Error\n");
    }

    DEBUG(DB_VM, "Written vAddr: 0x%x -> pAddr: 0x%x in to the TLB Index [%u]\n", 
    ehi, paddr, tlb_index_probe);

    splx(spl);

    return 0;
}

/* Allocate/free some user-space virtual pages */
vaddr_t
alloc_pages(unsigned npages, vaddr_t as_vbase)
{
    paddr_t addr = 0, return_addr = 0;    
        for (unsigned int i = 0; i < npages; i++) {
            // Find space in physical memory ram through the use of the Page Table
            unsigned int first_valid = 0;
            unsigned int buffer_npages = npages;
            _Bool valid_pages = 0;

            for(unsigned int i = 0; i < PAGETABLE_ENTRY; i++) {
                if (buffer_npages != 0) {
                    if (main_PG[i].Valid == 0) {
                        first_valid = (buffer_npages == npages ? i : first_valid);
                        buffer_npages--;
                        if (buffer_npages == 0) {
                            valid_pages = 1;
                            break;
                        }   
                    } else {
                        if (buffer_npages != npages) {
                            first_valid = 0;
                            valid_pages = 0;
                            buffer_npages = npages;
                        }
                    }
                }
            }

            if (valid_pages == 0) {
                // TODO #6
            } else {
                // TODO #7
                for (unsigned int i = 0; i < npages; i++) {
                    main_PG[first_valid+i].Valid = 1;
                    if (return_addr == 0) {
                        addr = (first_valid+i)*4096;
                        if (addr >= (paddr_t)main_PG - MIPS_KSEG0) {
                            panic("You're trying to write over the PG");
                        }
                        if (addr==0) { 
                            return 0; 
                        }
                        return_addr = addr; 
                    }
                    main_PG[first_valid+i].page_number = as_vbase/PAGE_SIZE;

                    // DEBUG
                    char toPass = (char)(first_valid+i); 
                    char* ptrPass;
                    char** ptrPass2;

                    ptrPass2 = &ptrPass;
                    ptrPass = &toPass;
                     
                    view_pagetable(1, ptrPass2);
                }
                        
            }
            return PADDR_TO_KVADDR(return_addr);
        }
        return 0;
}

int view_pagetable(int nargs, char **args) {
    (void)nargs;
    unsigned int number;
    if (nargs != 0) {
        switch (nargs) {
            case 1:
                number = (unsigned int)(**args);
                DEBUG(DB_VM, "\n");
                DEBUG(DB_VM, "(1): [%d] PN: %x\tpAddr: 0x%x\t-%s-\t",
                    number, 
                    main_PG[number].page_number, 
                    PADDR_TO_KVADDR(4096*(number)), 
                    main_PG[number].Valid == 1 ? "V" : "N");
                    DEBUG(DB_VM, "\n");
                break;
            case 2:
                number = (unsigned int)(**args);
                DEBUG(DB_VM, "\n");
                DEBUG(DB_VM, "(2): [%d] PN: %x\tpAddr: 0x%x\t-%s-\t",
                    number, 
                    main_PG[number].page_number, 
                    PADDR_TO_KVADDR(4096*(number)), 
                    main_PG[number].Valid == 1 ? "V" : "N");
                    DEBUG(DB_VM, "\n");
                break;
            
            default:
                break;
        }
    } else {
        DROP_PG(1);
    }
    
    return 0;
}

void DROP_PG(unsigned int interval) {
    DEBUG(DB_VM, "\n");
    for(unsigned int i = 0; (i+3) < PAGETABLE_ENTRY; i += 4*interval) {
        for (unsigned int j = 0; j < 4; j++) {
            DEBUG(DB_VM, "[%d] PN: %x\tpAddr: 0x%x\t-%s-\t",
            i+j, 
            main_PG[i+j].page_number, 
            PADDR_TO_KVADDR(4096*(i+j)), 
            main_PG[i+j].Valid == 1 ? "V" : "N");
        }
        DEBUG(DB_VM, "\n");
    }
}

void
free_pages(vaddr_t addr)
{
	/* nothing - leak the memory. */

	(void)addr;
}

int update_process_PG(struct process_PG* pPage, struct process_PG* data) {
    for (unsigned int i = 0; i < Process_PG_ENTRY; i++) {
        if (pPage[i].Valid == 0) {
            data->Valid = 1;
            memcpy((pPage + i*sizeof(struct process_PG)), data, sizeof(struct process_PG));
            return 0;
        } else {
            continue;
        }
    }
    return 1;
}

