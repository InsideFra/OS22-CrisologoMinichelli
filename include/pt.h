#include <types.h>
#include <vm.h>

// Inverted page table struct
struct invertedPT {
    _Bool CachingDisabled;
    _Bool Referenced;
    _Bool Modified;
    _Bool Valid;
    pid_t pid;
    uint16_t page_number;
    uint16_t victim_counter;
};

/** Frame list struct
 * 
 */
struct frame_list_struct {
    uint32_t frame_number;
    struct frame_list_struct* next_frame;    
};

enum PT_Error{
    noEntryFound = -1
};
int pageSearch(vaddr_t addr);

int addPT(uint32_t frame_index, vaddr_t vaddr, uint32_t pid);

int victim_pageSearch(void);
int page_replacement(int page_num);