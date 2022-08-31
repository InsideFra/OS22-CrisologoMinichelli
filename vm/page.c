#include <types.h>
#include <vm.h>
#include <spinlock.h>
#include <page.h>
#include <machine/tlb.h>
#include <spl.h>
#include <lib.h>
#include <test.h>
#include <addrspace.h>

static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;

// We need to divide the physiical memory into fixed-sized blocks called FRAMES
// Divide logical memory into blocks of same size called PAGES
// Keep track of all free frames
// SET UP A PAGE TABLE to translate logical to physical addresses
struct RAM_PG_ (*main_PG) = NULL;

struct frame_list_struct (*frame_list) = NULL;

/**
* Allocate/free some kernel-space virtual pages
* @param npages How many pages to alloc
* @param as_vbase Starting virtual address
* @return 0 if error else the starting physical address
*/
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

/**
* free some kernel-space virtual pages
* @param addr Virtual address to free
* @return 1 if error else 0
*/
int
free_kpages(vaddr_t addr)
{
    addr &= PAGE_FRAME;
    uint32_t page_number = addr >> 12; 
    _Bool valid_pages = false;
    uint32_t page_index = 0;   
    for(unsigned int i = 0; i < PAGETABLE_ENTRY; i++) {
        if (main_PG[i].Valid == 1) {
            if (main_PG[i].page_number == page_number) {
                valid_pages = true;
                page_index = i;
            } else {
                continue;
            }            
        } else {
            continue;
        }
    }

    if (valid_pages == false) {
        return 1;
    } else {
            /* DEBUG */
            char toPass = (char)(page_index); 
            char* ptrPass;
            char** ptrPass2;

            ptrPass2 = &ptrPass;
            ptrPass = &toPass;
                
            view_pagetable(3, ptrPass2);
            /* DEBUG */
            main_PG[page_index].Valid = 0;
            main_PG[page_index].pid = 0;
            main_PG[page_index].page_number = 0;

            if (removeTLB(addr)) {
                return 1;
            }

            as_zero_region(PADDR_TO_KVADDR(page_index << 12), 1);

            struct frame_list_struct* currentFrame = NULL;
            if (frame_list == NULL) {
                frame_list = (struct frame_list_struct*)kmalloc(sizeof(struct frame_list_struct));
                frame_list->frame_number = page_index;
                frame_list->next_item = NULL;
            } else {
                currentFrame = frame_list;

                struct frame_list_struct* bufferPointer = currentFrame->next_item;
                currentFrame->next_item = (struct frame_list_struct*)kmalloc(sizeof(struct frame_list_struct));
                if (currentFrame->next_item != NULL) {
                    currentFrame = currentFrame->next_item;
                    currentFrame->frame_number = page_index;
                    currentFrame->next_item = bufferPointer;
                } else {
                    panic ("Error in free_kpages");
                }

            }

            return 0;         
    }
    return 1;
}

/**
* This method checks if the virtual address passed belong to a code segment.
* This methos uses the address space to achieve this.
* @author @InsideFra
* @param vaddr The virtual address (0x00..)
* @param as the address space
* @date 09/08/2022
* @return 1 if everything is okay else ..
*/
int
is_codeSegment(vaddr_t vaddr, struct addrspace* as) {
    vaddr_t offset = (vaddr_t)(as->as_npages_code << 12);
    if (vaddr <= as->processPageTable_INFO->code_vaddr + offset) {
        if (vaddr >= as->processPageTable_INFO->code_vaddr)
            return 1;
    }
    return 0;
}

/**
* This method checks if the virtual address passed belong to a data segment.
* This methos uses the address space to achieve this.
* @author @InsideFra
* @param vaddr The virtual address (0x00..)
* @param as the address space
* @date 09/08/2022
* @return 1 if everything is okay else ..
*/
int
is_dataSegment(vaddr_t vaddr, struct addrspace* as) {
    vaddr_t offset = (vaddr_t)(as->as_npages_data << 12);
    if (vaddr <= as->processPageTable_INFO->data_vaddr + offset) {
        if (vaddr >= as->processPageTable_INFO->data_vaddr) {
            return 1;
        }
    }
    return 0;
}

/**
* This method checks if the virtual address is present and valid inside the RAM Page Table.
* @author @InsideFra
* @param vaddr The virtual address (0x00..)
* @date 09/08/2022
* @return index of the page table else noEntryFound;
*/
int
pageSearch(vaddr_t addr) {
    unsigned int    index = 0;
    _Bool           found = 0;

    for(unsigned int i = 0; i < PAGETABLE_ENTRY; i++) {
        if (main_PG[i].Valid == 1 && main_PG[i].page_number == (addr/PAGE_SIZE)) {
            found = 1;
            index = i;
            break;
        }
    }

    if (found == 0) {
        //DEBUG(DB_VM, "(pageSearc): No Entry Found - 0x%x\n", addr);
        return noEntryFound;
    } else {
        /* DEBUG */
        char toPass = (char)(index); 
        char* ptrPass;
        char** ptrPass2;
        ptrPass2 = &ptrPass;
        ptrPass = &toPass;
        view_pagetable(2, ptrPass2);
        /* END DEBUG */

        return index;        
    }
    //DEBUG(DB_VM, "(pageSearc): No Entry Found - 0x%x\n", addr);
    return noEntryFound;
}

/**
* This method will be used to add an entry to the hardware TLB.
* @author @InsideFra
* @param vaddr The virtual address (0x00..) (even not aligned)
* @param paddr The physicial address (0x80..)
* @date 02/08/2022
* @return 0 if everything is okay else panic
*/
int
addTLB(vaddr_t vaddr, paddr_t paddr) {
    uint32_t ehi, elo;
    paddr_t pa;
    int32_t tlb_index_probe;

    ehi = vaddr & PAGE_FRAME; // PAGE ALIGN
    pa = paddr - MIPS_KSEG0;
    elo = pa | TLBLO_VALID | TLBLO_DIRTY;

    splhigh();
    tlb_index_probe = tlb_probe(ehi, elo); 
    if (tlb_index_probe > 0) {
        DEBUG(DB_VM, "TLB Error: duplicate TLB entries\n");
        return 1;
    }

    for (unsigned int i = 0; i < NUM_TLB; i++) {
        tlb_read(&ehi, &elo, i);

        if(! ((elo & 0x00000fff) & TLBLO_VALID)) { // invalid
            ehi = vaddr & PAGE_FRAME; // PAGE ALIGN
            pa = paddr - MIPS_KSEG0;
            elo = pa | TLBLO_VALID | TLBLO_DIRTY;
            tlb_write(ehi, elo, i);

            tlb_index_probe = tlb_probe(ehi, elo); 
            if (tlb_index_probe < 0) {
                panic("Generic TLB Error\n");
            }

            DEBUG(DB_VM, "(TLBwrite ): [%3d] PN: %x\tpAddr: 0x%x\n", tlb_index_probe, ehi >> 12, paddr);

            spl0();
            return 0;
        }
    }
    return 1;
}

/**
* This method will be used to remove an entry to the hardware TLB.
* @author @InsideFra
* @date 02/08/2022
* @param {vaddr_t} vaddr The virtual address (0x00..) (even not aligned)
* @return 0 if everything is entry is missing/the entry has been removed, 1 if error
*/
int
removeTLB(vaddr_t vaddr) {
    uint32_t ehi, elo;
    uint32_t paddr;
    vaddr &= PAGE_FRAME; // PAGE ALIGN

    splhigh();
    for (unsigned int i = 0; i < NUM_TLB; i++) {
        tlb_read(&ehi, &elo, i);

        if(ehi == vaddr) {
            paddr = elo >> 12;
            ehi = TLBHI_INVALID(i);
            elo = TLBLO_INVALID();
            tlb_write(ehi, elo, i);

            DEBUG(DB_VM, "(TLBremove): [%3d] PN: %x\tpAddr: 0x%x\t", i, vaddr >> 12, PADDR_TO_KVADDR(paddr << 12));
            
            if( (elo & 0x00000fff) & TLBLO_DIRTY) {
                DEBUG(DB_VM, "Dirty "); }
            else {
                DEBUG(DB_VM, "No Dirty "); }
            
            if( (elo & 0x00000fff) & TLBLO_VALID) {
                DEBUG(DB_VM, "Invalid "); }
            else {
                DEBUG(DB_VM, "Valid "); }

            DEBUG(DB_VM, "\n");
            spl0();
            return 0;
        }

    }
    DEBUG(DB_VM, "removeTLB Error: noEntryFound\n");
    spl0();
    return 1;
}

/* Allocate/free some user-space virtual pages */
vaddr_t
alloc_pages(unsigned npages, vaddr_t as_vbase)
{
    paddr_t addr = 0, return_addr = 0;
    struct frame_list_struct* currentFrame;
    uint8_t FIFOSize = 0;
    
    // Find space in physical memory ram through the use of the Frame List FIFO
    currentFrame = frame_list;
    if (currentFrame == NULL) {
        panic ("Error in alloc_pages");
        return 0;
    }
    FIFOSize = 1;
    for (unsigned int p = 1; p < npages; p++) {
        if (currentFrame->next_item == NULL) {
            break;
        }
        FIFOSize++;
    }

    uint8_t index = 0;
    if (FIFOSize == npages) {
        for (unsigned int p = 0; p < npages; p++) {
            currentFrame = frame_list;
            index = currentFrame->frame_number;
            main_PG[index].Valid = 1;
            main_PG[index].page_number = (as_vbase + p) >> 12;
            if (return_addr == 0) {
                addr = (index)*4096;
                if (addr >= (paddr_t)main_PG - MIPS_KSEG0) {
                    panic("You're trying to write over the PG");
                }
                if (addr==0) {
                    panic("Error in alloc_pages"); 
                    return 0; 
                }
                return_addr = addr; 
            }
            struct frame_list_struct* deleteFrame;
            deleteFrame = currentFrame;
            frame_list = currentFrame->next_item;
            
            /* DEBUG */
            char toPass = (char)(currentFrame->frame_number); 
            char* ptrPass;
            char** ptrPass2;

            ptrPass2 = &ptrPass;
            ptrPass = &toPass;
    
            view_pagetable(1, ptrPass2);
            /* DEBUG */

            kfree(deleteFrame);
        }
    } else {
        // TODO #6
        panic ("Error in alloc_pages");
        return 0;
    }
    return PADDR_TO_KVADDR(return_addr);
}

int view_pagetable(int nargs, char **args) {
    (void)nargs;
    unsigned int number;
    if (nargs != 0) {
        switch (nargs) {
            case 1:
                number = (unsigned int)(**args);
                DEBUG(DB_VM, "(allocPage): [%3d] PN: %x\tpAddr: 0x%x\t-%s-\t",
                    number, 
                    main_PG[number].page_number, 
                    PADDR_TO_KVADDR(4096*(number)), 
                    main_PG[number].Valid == 1 ? "V" : "N");
                    DEBUG(DB_VM, "\n");
                break;
            case 2:
                number = (unsigned int)(**args);
                DEBUG(DB_VM, "(pageSearc): [%3d] PN: %x\tpAddr: 0x%x\t-%s-\t",
                    number, 
                    main_PG[number].page_number, 
                    PADDR_TO_KVADDR(4096*(number)), 
                    main_PG[number].Valid == 1 ? "V" : "N");
                    DEBUG(DB_VM, "\n");
                break;
            case 3:
                number = (unsigned int)(**args);
                DEBUG(DB_VM, "(freePage ): [%3d] PN: %x\tpAddr: 0x%x\t-%s-\t",
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

// int update_process_PG(struct process_PG* pPage, struct process_PG* data) {
//     for (unsigned int i = 0; i < Process_PG_ENTRY; i++) {
//         if (pPage[i].Valid == 0) {
//             data->Valid = 1;
//             memcpy((pPage + i*sizeof(struct process_PG)), data, sizeof(struct process_PG));
//             return 0;
//         } else {
//             continue;
//         }
//     }
//     return 1;
// }

