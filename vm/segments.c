/* Code for tracking and manipulating segments */

#include <addrspace.h>
#include <segments.h>

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
    if (vaddr <= as->as_vbase_code + offset) {
        if (vaddr >= as->as_vbase_code)
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
    if (vaddr <= as->as_vbase_data + offset) {
        if (vaddr >= as->as_vbase_data) {
            return 1;
        }
    }
    return 0;
}

/**
* This method checks if the virtual address passed belong to a bss segment.
* This methos uses the address space to achieve this.
* @author @InsideFra
* @param vaddr The virtual address (0x00..)
* @param as the address space
* @date 25/08/2022
* @return 1 if everything is okay else ..
*/
int
is_bssSegment(vaddr_t vaddr, struct addrspace* as) {
    vaddr_t offset = (vaddr_t)(as->as_npages_data << 12);
    if (vaddr <= as->as_vbase_bss + offset) {
        if (vaddr >= as->as_vbase_bss) {
            return 1;
        }
    }
    return 0;
}