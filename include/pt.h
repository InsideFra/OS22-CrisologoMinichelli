#include <types.h>
#include <vm.h>

// Inverted page table struct
struct invertedPT {
    _Bool CachingDisabled;
    _Bool Referenced;
    _Bool Modified;
    _Bool Valid;
    uint32_t clock;
    uint32_t pid;
    uint32_t page_number;
};

/** Frame list struct
 * 
 */
struct frame_list_struct {
    uint32_t frame_number;
    struct frame_list_struct* next_frame;    
};

enum PT_Error{
    noEntryFound
};

int pageSearch(vaddr_t addr);

int addPT(uint32_t frame_index, vaddr_t vaddr);