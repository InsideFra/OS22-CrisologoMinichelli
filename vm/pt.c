/* Page tables and page table entry manipulation go here */

#include <pt.h>
#include <types.h>
#include <vm.h>
#include <spl.h>

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
