/* Page tables and page table entry manipulation go here */

#include <pt.h>
#include <types.h>
#include <vm.h>
#include <spl.h>
#include <lib.h>
#include <kern/errno.h>
#include <addrspace.h>
#include <proc.h>
#include <current.h>

// We need to divide the physiical memory into fixed-sized blocks called FRAMES
// Divide logical memory into blocks of same size called PAGES
// Keep track of all free frames
// SET UP A PAGE TABLE to translate logical to physical addresses

// This is the pointer to the inverter page table
struct invertedPT (*main_PG) = NULL;

/** 
 * @brief Number of entries of the inverted page table. This number is updated as soon as the vm is initialized. Defined in "pt.c"
 */
unsigned int PAGETABLE_ENTRY = 0;

/**
 * @brief Page Table Link Register. This number is updated as soon as the vm is initialized. Defined in "pt.c"
 */
unsigned int PTLR = sizeof(struct invertedPT);

/** This is the pointer to the available frame list.
 * 
 * It is written as a list and it is itended to be used as a FIFO queue
 * The last element is always the element that has next_frame as NULL
 * The FIFO queue is empty if frame_list == NULL
 */
struct frame_list_struct (*frame_list) = NULL;

uint32_t pt_counter = 0;

/**
* This method checks if the virtual address is present and valid inside the inverted PageTable.
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
        if (main_PG[i].Valid == 1) {
            if (main_PG[i].page_number == addr/PAGE_SIZE) {
                index = i;
                found = 1;
                break;
            }
        }
    }

    if (found == 0) {
        return noEntryFound;
    } else {
        return index;        
    }
    return noEntryFound;
}

/**
* This method adds an entry to the page table.
* @author @InsideFra
* @param frame_index The frame number
* @param vaddr The virtual address (0x00..)
* @date 09/08/2022
* @return 0 if everything ok, else panic;
*/
int addPT(uint32_t frame_index, vaddr_t vaddr, uint32_t pid) {
    vaddr &= PAGE_FRAME; // alignment
    vaddr = vaddr/PAGE_SIZE; // get page number

    //print_page_table();
    
    if (main_PG[frame_index].Valid == 1) {
        if (main_PG[frame_index].page_number == 0 && main_PG[frame_index].pid == 0) {
            //DEBUG(DB_VM, "addPT(): Page valid");
            ;
        } else {
            panic("addPT() error: You cannot add this page table");
        }
    }
    
    main_PG[frame_index].page_number = vaddr;
    main_PG[frame_index].Valid = 1;
    main_PG[frame_index].pid = pid;
    //main_PG[frame_index].victim_counter = pt_counter;

    // DEBUG
    // kprintf("(addPT    ): [%3d] PN: %x\tpAddr: 0x%x\t-%s-",
    //     frame_index, 
    //     main_PG[frame_index].page_number, 
    //     PADDR_TO_KVADDR(4096*(frame_index)), 
    //     main_PG[frame_index].Valid == 1 ? "V" : "N");
    // if (main_PG[frame_index].Valid == 1)
    //     kprintf("%s-\t", main_PG[frame_index].pid == 0 ? "KERNEL" : "USER");
    // else 
    //     kprintf("\t");
        
    // kprintf("\n");
    return 0;
}

/**
* This function allow to find the victim page inside RAM page table
* @author @Marco_Embed
* @param bool type: 0 code segment, 1 data segment
* @date 20/09/2022
* @return page number of victim page;
*/
int victim_pageSearch(bool type){
    uint32_t min_value;
    int victim_page = noEntryFound;
    bool first = true;
    
    for(unsigned int i = 0; i < PAGETABLE_ENTRY; i++) {
        if(type == 0){
            if(main_PG[i].pid == curproc->pid){
                if(is_codeSegment(main_PG[i].page_number*PAGE_SIZE, proc_getas())){
                    if(first){
                        //first code page 
                        min_value = main_PG[i].victim_counter;
                        victim_page = i;
                        first = false;
                    } else if(min_value > main_PG[i].victim_counter){
                        min_value = main_PG[i].victim_counter;
                        victim_page = i;
                    }
                } else{
                    continue;
                }
            }
        } else if(type == 1){
            if(main_PG[i].pid == curproc->pid){
                if(is_dataSegment(main_PG[i].page_number*PAGE_SIZE, proc_getas())){
                    if(first){
                        //first data page 
                        min_value = main_PG[i].victim_counter;
                        victim_page = i;
                        first = false;
                    } else if(min_value > main_PG[i].victim_counter){
                        min_value = main_PG[i].victim_counter;
                        victim_page = i;
                    }
                } else{
                    continue;
                }
            }
        }
    }
    return victim_page;
}

/* LRU algorithm */
int page_replacement(int page_num){
	free_kpages((paddr_t)(page_num*PAGE_SIZE + MIPS_KSEG0));
	return 0;
}

/* swap in func*/

/* swap out func*/
