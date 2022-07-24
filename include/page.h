#include <types.h>
#include <vm.h>

// Page-table lenght register
//The size in kB of a page table entry
#define PTLR_ sizeof(struct PG_)

// Page-table base register
// The start address in memory where to store page table (stack like)
#define PTBR_ 1

// The size of the allocaed space to store page table
#define PAGETABLE_SPACE 8192

#define PAGETABLE_ENTRY PAGETABLE_SPACE/PTLR_

struct PG_ {
    uint8_t Valid;
    int frame_number;
};
