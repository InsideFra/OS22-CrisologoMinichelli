#include <pt.h>
#include <list.h>
#include <vm.h>
#include <lib.h>

extern struct frame_list_struct (*frame_list);

extern unsigned int PAGETABLE_ENTRY;

/**
* This method adds a value in the user-frame list.
* @author @InsideFra
* @param {uint32_t} value - The corresponding frame number
* @param {(addTOP|addBOTTOM)} position - Add the number at the TOP or at the bottom of the frame list
* @date 06/09/2022
* @return 0 if no error else 1 or panic;
*/
int addToFrameList(uint32_t value, uint8_t position) {
    struct frame_list_struct* currentFrame = NULL;
    struct frame_list_struct* bufferFrame = NULL;
    
    if(( currentFrame = (struct frame_list_struct*)kmalloc(sizeof(struct frame_list_struct))) == 0) {
            panic("Error kmalloc in addToList()\n");
    }

    if (currentFrame == (void*)0xdeadeef) {
        panic("1");
    }

    currentFrame->frame_number = value;
    KASSERT(value <= PAGETABLE_ENTRY);

    switch (position) {
        case addTOP:
            currentFrame->next_frame = frame_list;
            frame_list = currentFrame;
            return 0; 
            break;
        case addBOTTOM:
            if (frame_list == NULL) {
                return addToFrameList(value, addTOP);
            }
            bufferFrame = frame_list;
            while(1) {
                if (bufferFrame->next_frame != NULL) {
                    if (bufferFrame->next_frame != (void*)0xdeadbeef) {
                        bufferFrame = bufferFrame->next_frame;
                        continue;
                    }
                }
                bufferFrame->next_frame = currentFrame;
                //DEBUG(DB_VM, "Added frame %d to the frame list (addr: 0x%x) at 0x%x\n", bufferFrame->frame_number, (uint32_t)frame_list, (uint32_t)bufferFrame);
                return 0;
                break;
            } 
            break;
    }
    return 1;
}

int removeFromList(void) {
    return 1;
}