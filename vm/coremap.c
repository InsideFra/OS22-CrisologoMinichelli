/* Keep track of free physical frames */
#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <vm.h>
#include <pt.h>
#include <vm_tlb.h>
#include <list.h>
#include <current.h>
#include <coremap.h>
extern struct frame_list_struct (*frame_list);

uint32_t check_free_frame(uint32_t npages) {
	// Find space in physical memory ram through the use of the Frame List FIFO
    struct frame_list_struct* currentFrame;
	currentFrame = frame_list;
	if (currentFrame == NULL) {
		return 0;
	}
	
    uint32_t FIFOSize = 1;
	for (unsigned int p = 1; p < npages; p++) {
		if (currentFrame->next_frame == NULL) {
			break;
		}
		FIFOSize++;
	}
    return FIFOSize;
}